# Remote execution and GPU workers

`--config=remote` runs every action on a server you host, which speaks the
Remote Execution API. Builds and CPU tests run on the server. GPU tests run on
a [vast.ai](https://vast.ai) box that the server rents for them and destroys
once it is idle. The laptop uploads only the inputs the server is missing. It
downloads only what tests produce: logs, `test.xml` and `test.outputs/`, cached
results included (`test:remote --remote_download_regex`). Every other output
stays in the server's store (`--remote_download_minimal`).

```bash
ssh -fN rbe-server                                  # once per session
bazelisk test --config=remote //...                 # builds and CPU tests on the server
bazelisk test --config=remote --config=gpu //...    # GPU tests on a rented box
```

```text
laptop:  bazelisk test --config=remote //...      (repository rules still run here)
  │  ssh -fN rbe-server   (LocalForward 50051 → 127.0.0.1:50051)
  ▼
server (Ubuntu 24.04, always on)
  ├─ nativelink  127.0.0.1:50051 cas, ac, execution   127.0.0.1:50061 worker_api, admin, health
  │     CAS and action cache on disk, the scheduler, a CPU worker {gpu: none}
  └─ gpu-scaler (systemd): polls /admin/.../demand and /workers every 10 s
        queued action with gpu=cuda and no GPU worker → rent;  idle IDLE_MINUTES → drain, destroy
        │  ssh -N -R 50051 -R 50061 root@box   (the box opens no port)
        ▼
vast.ai box (plain ubuntu:24.04, one sm_86 GPU, billed by the minute)
  └─ nativelink worker {gpu: cuda}, one action at a time
```

| File | What it is |
| --- | --- |
| `server/setup.sh` | Root installer for the server: NativeLink (pinned, sha256-checked), the `vastai` CLI, the API key, the ssh key, systemd units, BBR |
| `server/nativelink.json5` | Server config: CAS, action cache, scheduler, CPU worker, loopback listeners |
| `server/gpu-scaler`, `server/gpu-scaler.service` | The autoscaler: rent, tunnel, start the worker, drain, destroy, book the cost |
| `server/gpu.env` | The offer query (GPUs, price cap), idle, orphan and lifetime limits, monthly spend cap |
| `worker/nativelink.json5` | Config of the GPU box's worker. It reaches the server's CAS and scheduler through the tunnels |
| `BUILD.bazel` | The execution platform `:platform` (`gpu: none`) for `--config=remote` |
| `../../bzl/gpu.bzl` | `GPU` / `LARGE_GPU`: the tags and `exec_properties` of a GPU test |
| `../../test/remote/` | `gpu_scaler_test`: the scaler run against fake `vastai`, `ssh`, `scp` and `curl` |

## Marking a GPU test

```python
load("//:bzl/gpu.bzl", "GPU")

cc_test(
    name = "saxpy_test",
    ...
    exec_properties = GPU.exec_properties,  # {"test.gpu": "cuda"}
    tags = GPU.tags,                        # ["requires-gpu"]
)
```

- **Running GPU tests.** The tag keeps the test out of `bazelisk test //...`
  and CI (it is only built there). `--config=gpu` selects it, on this machine's
  GPU or, with `--config=remote`, on a rented one. `LARGE_GPU` (tag
  `requires-large-gpu`, `--config=large-gpu`) is for tests that need 12-24 GB.
- **Routing.** The NativeLink scheduler matches the `gpu` platform property
  exactly. The execution platform `//tools/remote:platform` makes every action
  ask for `gpu=none`, which only the server's own worker offers. A GPU test
  overrides that for its test action with `test.gpu=cuda`, which only the GPU
  boxes offer.
- **Why the `test.` prefix.** It limits the override to the test action, so
  the test is still built on the server.
- **Why the explicit `none`.** A property that an action does not request
  never restricts the match. Without `none`, a GPU box would also take build
  actions.
- **Timeouts.** Bazel sends a test's timeout as the action's timeout, and
  NativeLink counts a fresh box's input download in it. So `test:remote`
  raises the four size timeouts (`--test_timeout`). Both workers allow 7200 s
  at most (`max_action_timeout_s`); a longer request is rejected.
- **CUDA on the box.** The box runs plain Ubuntu. vast.ai's runtime provides
  the driver, and the test's runfiles provide CUDA: the hermetic `rules_cuda`
  toolkit (`MODULE.bazel`, C++) or the nvidia wheels of a PyTorch dependency
  (Python). The build targets `sm_86` only
  (`--@rules_cuda//cuda:archs` in `.bazelrc`), so `QUERY` in `gpu.env` rents
  Ampere GPUs with 24 GB. Change both together.

## The GPU boxes

`gpu-scaler` runs as a systemd service.

- **Watching the queue.** Every `POLL_SECONDS` it reads the scheduler's admin
  listings: `GET /admin/scheduler/MAIN_SCHEDULER/demand` (queued actions) and
  `.../workers`.
- **Renting.** When a `gpu=cuda` action is queued and no GPU worker is
  connected, it rents the cheapest vast.ai offer that matches `QUERY` and
  costs at most `MAX_DPH`.
- **Connecting.** It opens two reverse ssh tunnels, one per port: the CAS on
  50051 and the scheduler on 50061. In a single connection, a large input
  download starved the scheduler's keepalives. It then copies the NativeLink
  binary and `worker/nativelink.json5` to the box and starts the worker.
- **Idle shutdown.** A worker that has been idle for `IDLE_MINUTES` is drained
  and its box destroyed. Idle means no GPU action running or queued, and no
  bytes moving on a client's connection to port 50051 since the last poll. An
  invocation that is still uploading its next GPU test's inputs thus keeps
  the box.
- **Interrupted tests.** NativeLink does not cancel an action whose client
  went away (Ctrl-C): the box would run it to its end. When a GPU action runs
  and no client byte has moved for `ORPHAN_MINUTES`, the scaler kills it.
- **Failures.** A worker that does not register within `LAUNCH_TIMEOUT` costs
  its box. A lost tunnel is reopened. A box that never comes up is replaced by
  one on another machine.
- **Spending.** Rentals are booked in `/var/lib/rbe-remote/ledger`. Once
  `MONTHLY_CAP` dollars have been spent in a month, nothing more is rented.
- **Safety net.** A watchdog on the box destroys it when no ssh session (the
  tunnels) has been left for `IDLE_MINUTES`, or after `MAX_HOURS`. A dead
  scaler therefore leaves nothing billing. The watchdog also kills processes
  that a finished test left running in the background, which would hold the
  GPU.

```bash
/opt/rbe-remote/gpu-scaler status   # the box, its worker, the queued GPU actions, this month's spend
/opt/rbe-remote/gpu-scaler up       # rent ahead of a batch of GPU tests
/opt/rbe-remote/gpu-scaler down     # drain and destroy now
journalctl -u gpu-scaler -f
```

## Security

NativeLink's worker API and admin endpoints have no authentication. Anything
that reaches port 50061 can register as a worker and run, or forge the results
of, every action. For that reason:

- Both listeners bind to the server's loopback only.
- The laptop reaches port 50051 through its ssh tunnel.
- The GPU boxes reach both ports through the server's reverse tunnels.

The secrets stay on the server:

- the vast.ai API key, in `/root/.config/vastai/vast_api_key`;
- the ssh key the boxes accept, in `/var/lib/rbe-remote/id_ed25519`, which
  `setup.sh` registers on the vast.ai account.

A box gets no secret beyond vast.ai's own per-instance key, which its
watchdog uses. Every input of a remote action ends up in the server's CAS and
on the rented boxes. Keep this in mind for private data.

## Setup

Run this once, on a fresh Ubuntu 24.04 server with a public ssh port:

```bash
git clone <this repository> && cd <it>
read -rs VAST_API_KEY && export VAST_API_KEY && sudo -E tools/remote/server/setup.sh
```

`setup.sh` installs:

- the pinned NativeLink release and the configs, under `/opt/rbe-remote`;
- the `vastai` CLI, in a venv;
- the API key and the ssh key;
- the two systemd units.

It also switches TCP to BBR congestion control. It is idempotent: run it again
after any change to `tools/remote/`.

On the laptop, add the server to `~/.ssh/config`:

```text
Host rbe-server
  HostName <the server>
  User <you>
  LocalForward 127.0.0.1:50051 127.0.0.1:50051
  ExitOnForwardFailure yes
  ServerAliveInterval 30
```

Start every session with `ssh -fN rbe-server`. If a previous tunnel still holds
the port, `ExitOnForwardFailure` makes this command fail instead of lingering
without the forward.

The first `bazelisk build --config=remote //...` uploads every toolchain input
over the laptop's uplink. Running that first build once on the server itself
(a clone there, the same command) fills the CAS at datacenter speed.

## Costs

- **The server**, which is always on.
- **A GPU box**, while it exists: typically $0.2-0.35 an hour on vast.ai, for
  its start-up, the input download, the tests and then `IDLE_MINUTES`. Boxes
  are destroyed, not stopped, so every fresh box downloads its inputs from the
  server again.
- **Waiting.** A GPU test with no box up takes a few minutes before it starts:
  renting, booting and the input download. With a box up, it starts at once.

## What does not use it

- CI (`.github/workflows/ci.yml`) builds on GitHub's runners and only builds
  the GPU tests.
- `--config=gpu` without `--config=remote` runs the GPU tests on the local GPU.
- Tests tagged `local`, such as the Docker integration test, always run on
  the laptop.
- Switching between `--config=remote` and local builds changes the execution
  platform, so Bazel re-analyzes and rebuilds.
