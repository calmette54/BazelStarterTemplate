"""tools/remote/server/gpu-scaler against fake vastai, ssh, scp and curl: the scaler without a box.

Each fake logs its calls as JSON lines and keeps the state in a directory:
`vastai show instances` lists state/instances.json, `search offers` prints
state/offers.json, `create instance` adds an instance (its status from
state/statuses, one word per creation; "running" by default) and `destroy
instance` removes it. ssh answers the readiness check, keeps each tunnel's
control master as state/tunnel.<name>, the extension of its socket (`-M` opens
it, `-O check` tests it, `-O exit` closes it) and accepts the worker's start.
curl serves the scheduler's admin listings from state/workers.json and
state/demand.json (empty unless written; state/scheduler_down fails them) and
accepts the drain.
"""

import json
import os
import pathlib
import re
import subprocess
import sys
import time

import pytest
from python.runfiles import Runfiles

LABEL = "rbe-gpu"
GPU_ACTION = {
    "operation_id": "op-1",
    "queued_since_ms": 0,
    "priority": 0,
    "platform_properties": {"OSFamily": "linux", "gpu": "cuda"},
}
CPU_ACTION = {
    "operation_id": "op-2",
    "queued_since_ms": 0,
    "priority": 0,
    "platform_properties": {"OSFamily": "linux"},
}
OFFERS = [
    {
        "id": 101,
        "machine_id": 11,
        "dph_total": 0.18,
        "gpu_name": "RTX 3090",
        "geolocation": "Czechia, CZ",
    },
    {
        "id": 102,
        "machine_id": 12,
        "dph_total": 0.21,
        "gpu_name": "RTX A5000",
        "geolocation": "Texas, US",
    },
    {
        "id": 103,
        "machine_id": 13,
        "dph_total": 0.25,
        "gpu_name": "RTX 3090",
        "geolocation": "Utah, US",
    },
]
PUBLIC_KEY = "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIFake gpu-scaler@test"

FAKE_COMMON = """\
import json, os, pathlib, sys
state = pathlib.Path(os.environ["FAKE_STATE"])
with open(state / "calls.jsonl", "a") as log:
    log.write(json.dumps({"tool": TOOL, "argv": sys.argv[1:]}) + "\\n")
"""

FAKE_VASTAI = """\
TOOL = "vastai"
{common}
args = sys.argv[1:]
instances_file = state / "instances.json"
instances = json.loads(instances_file.read_text()) if instances_file.exists() else []
if args[:2] == ["show", "instances"]:
    if (state / "api_fails").exists():
        sys.exit(1)
    print(json.dumps(instances))
elif args[:2] == ["search", "offers"]:
    print((state / "offers.json").read_text())
elif args[:2] == ["create", "instance"]:
    offers = json.loads((state / "offers.json").read_text())
    offer = next(o for o in offers if str(o["id"]) == args[2])
    created = state / "created"
    count = len(created.read_text()) if created.exists() else 0
    created.write_text("x" * (count + 1))
    statuses_file = state / "statuses"
    statuses = statuses_file.read_text().split() if statuses_file.exists() else ["running"]
    instance_id = 5000 + count
    instances.append({{
        "id": instance_id,
        "label": args[args.index("--label") + 1],
        "machine_id": offer["machine_id"],
        "actual_status": statuses[min(count, len(statuses) - 1)],
        "public_ipaddr": "203.0.113.7",
        "ports": {{"22/tcp": [{{"HostIp": "0.0.0.0", "HostPort": "40022"}}]}},
    }})
    instances_file.write_text(json.dumps(instances))
    print(json.dumps({{"success": True, "new_contract": instance_id}}))
elif args[:2] == ["destroy", "instance"]:
    instances_file.write_text(json.dumps([i for i in instances if str(i["id"]) != args[2]]))
"""

FAKE_SSH = """\
TOOL = "ssh"
{common}
args = sys.argv[1:]
flags = []
while args and args[0].startswith("-"):
    if args[0] in ("-o", "-F", "-S", "-O", "-R", "-i"):
        flags.append((args[0], args[1]))
        args = args[2:]
    else:
        flags.append((args[0], None))
        args = args[1:]
socket = dict(flags).get("-S")
tunnel = state / ("tunnel" + pathlib.Path(socket).suffix) if socket else None
if ("-O", "check") in flags:
    sys.exit(0 if tunnel.exists() else 255)
if ("-O", "exit") in flags:
    tunnel.unlink(missing_ok=True)
    sys.exit(0)
if ("-M", None) in flags:
    if (state / "tunnel_fails").exists():
        sys.exit(255)
    tunnel.touch()
    sys.exit(0)
command = " ".join(args[1:])
if "test -e /work/.ready" in command and (state / "not_ready").exists():
    sys.exit(255)
"""

FAKE_SCP = """\
TOOL = "scp"
{common}
"""

FAKE_CURL = """\
TOOL = "curl"
{common}
args = sys.argv[1:]
if (state / "scheduler_down").exists():
    sys.exit(22)
if "-X" in args and args[args.index("-X") + 1] == "POST":
    sys.exit(0)
listing = state / (args[-1].rsplit("/", 1)[1] + ".json")
print(listing.read_text() if listing.exists() else "[]")
"""


def source(name):
    return pathlib.Path(Runfiles.Create().Rlocation(f"_main/{name}")).read_text()


def settings():
    """QUERY, MAX_DPH, DISK and MONTHLY_CAP of tools/remote/server/gpu.env."""
    path = Runfiles.Create().Rlocation("_main/tools/remote/server/gpu.env")
    out = subprocess.run(
        [
            "bash",
            "-c",
            'source "$0" && printf "%s\\n" "$QUERY" "$MAX_DPH" "$DISK" "$MONTHLY_CAP"',
            path,
        ],
        capture_output=True,
        text=True,
        check=True,
    ).stdout.splitlines()
    return dict(zip(["QUERY", "MAX_DPH", "DISK", "MONTHLY_CAP"], out, strict=True))


@pytest.fixture
def scaler(tmp_path):
    """A SCALER_DIR (the script, gpu.env with short timings, the worker config, a
    binary), a STATE_DIR, the fakes on PATH and their state."""
    scaler_dir = tmp_path / "rbe-remote"
    scaler_dir.mkdir()
    (scaler_dir / "gpu-scaler").write_text(source("tools/remote/server/gpu-scaler"))
    (scaler_dir / "gpu.env").write_text(
        source("tools/remote/server/gpu.env")
        + "LAUNCH_TIMEOUT=2\nPOLL_SECONDS=0.2\nDRAIN_SECONDS=1\n"
    )
    (scaler_dir / "worker.json5").write_text(source("tools/remote/worker/nativelink.json5"))
    (scaler_dir / "nativelink").write_text("#!/bin/sh\n")
    state_dir = tmp_path / "state"
    state_dir.mkdir()
    (state_dir / "id_ed25519.pub").write_text(PUBLIC_KEY + "\n")

    fakes = tmp_path / "fakes"
    fakes.mkdir()
    for tool, text in [
        ("vastai", FAKE_VASTAI),
        ("ssh", FAKE_SSH),
        ("scp", FAKE_SCP),
        ("curl", FAKE_CURL),
    ]:
        path = fakes / tool
        # Not `#!{sys.executable}`: the runfiles path of the interpreter can
        # exceed the kernel's shebang limit (255 bytes), and exec then fails.
        # PATH (below) puts this interpreter's directory first.
        path.write_text("#!/usr/bin/env python3\n" + text.format(common=FAKE_COMMON))
        path.chmod(0o755)
    fake_state = tmp_path / "fake_state"
    fake_state.mkdir()
    (fake_state / "offers.json").write_text(json.dumps(OFFERS))
    home = tmp_path / "home"
    home.mkdir()
    return {
        "dir": scaler_dir,
        "state": state_dir,
        "fake": fake_state,
        "env": {
            # The fakes, this interpreter (python3 for the JSON parsing), coreutils, flock.
            "PATH": os.pathsep.join(
                [str(fakes), str(pathlib.Path(sys.executable).parent), "/usr/bin:/bin"]
            ),
            "HOME": str(home),
            "SCALER_DIR": str(scaler_dir),
            "STATE_DIR": str(state_dir),
            "FAKE_STATE": str(fake_state),
            "ADMIN": "http://admin.test/scheduler/MAIN_SCHEDULER",
        },
    }


def run(scaler, command, **env):
    process = subprocess.run(
        ["bash", str(scaler["dir"] / "gpu-scaler"), command],
        env={**scaler["env"], **env},
        capture_output=True,
        text=True,
        timeout=60,
    )
    return process.returncode, process.stdout, process.stderr


def running_box(scaler, status="running"):
    """The box of the account that the scaler should find (or replace)."""
    instance = {
        "id": 4000,
        "label": LABEL,
        "machine_id": 9,
        "actual_status": status,
        "public_ipaddr": "198.51.100.4",
        "ports": {"22/tcp": [{"HostIp": "0.0.0.0", "HostPort": "41022"}]},
    }
    (scaler["fake"] / "instances.json").write_text(json.dumps([instance]))


def gpu_worker(scaler, running=0):
    """A registered GPU worker (and the server's CPU worker) in the admin listing."""
    workers = [
        {
            "id": "cpu-worker-1",
            "running_actions": 3,
            "is_draining": False,
            "platform_properties": {"OSFamily": "linux", "gpu": "none"},
        },
        {
            "id": "gpu-worker-1",
            "running_actions": running,
            "is_draining": False,
            "platform_properties": {"OSFamily": "linux", "gpu": "cuda"},
        },
    ]
    (scaler["fake"] / "workers.json").write_text(json.dumps(workers))


def demand(scaler, *actions):
    (scaler["fake"] / "demand.json").write_text(json.dumps(list(actions)))


def rented(scaler, seconds_ago, price="0.180", instance=4000):
    (scaler["state"] / "rented").write_text(
        f"{int(time.time()) - seconds_ago} {price} {instance}\n"
    )


def last_busy(scaler, seconds_ago):
    (scaler["state"] / "last_busy").write_text(f"{int(time.time()) - seconds_ago}\n")


def instances(scaler):
    path = scaler["fake"] / "instances.json"
    return json.loads(path.read_text()) if path.exists() else []


def calls(scaler, tool=None):
    log = scaler["fake"] / "calls.jsonl"
    entries = [json.loads(line) for line in log.read_text().splitlines()] if log.exists() else []
    return [c["argv"] for c in entries if tool is None or c["tool"] == tool]


def vastai(scaler, verb):
    return [c for c in calls(scaler, "vastai") if c[0] == verb]


def ssh_commands(scaler):
    """(flags, host, remote command) of each ssh call."""
    commands = []
    for args in calls(scaler, "ssh"):
        flags = []
        while args and args[0].startswith("-"):
            if args[0] in ("-o", "-F", "-S", "-O", "-R", "-i"):
                flags.append((args[0], args[1]))
                args = args[2:]
            else:
                flags.append((args[0], None))
                args = args[1:]
        commands.append((flags, args[0], " ".join(args[1:])))
    return commands


TUNNEL_FILES = {"tunnel.cas", "tunnel.scheduler"}


def open_tunnels(scaler):
    for name in TUNNEL_FILES:
        (scaler["fake"] / name).touch()


def open_tunnel_files(scaler):
    return {p.name for p in scaler["fake"].glob("tunnel.*")}


def tunnels(scaler):
    """The reverse forwards of each tunnel that was opened."""
    return [
        [value for flag, value in flags if flag == "-R"]
        for flags, _, _ in ssh_commands(scaler)
        if ("-M", None) in flags
    ]


def drains(scaler):
    return [c[-1] for c in calls(scaler, "curl") if "-X" in c and "set_drain_worker" in c[-1]]


def onstart(scaler):
    (create,) = vastai(scaler, "create")
    return create[create.index("--onstart-cmd") + 1]


def test_no_demand_rents_nothing(scaler):
    demand(scaler, CPU_ACTION)
    rc, _, stderr = run(scaler, "tick")
    assert rc == 0, stderr
    assert not vastai(scaler, "search") and not vastai(scaler, "create")
    assert not calls(scaler, "ssh")


def test_gpu_demand_rents_the_cheapest_offer(scaler):
    demand(scaler, CPU_ACTION, GPU_ACTION)
    rc, _, stderr = run(scaler, "tick")
    assert rc == 0, stderr
    assert "1 GPU action(s) queued" in stderr
    (search,) = vastai(scaler, "search")
    env = settings()
    assert search == [
        "search",
        "offers",
        f"{env['QUERY']} dph_total<={env['MAX_DPH']}",
        "-o",
        "dph_total",
        "--raw",
    ]
    (create,) = vastai(scaler, "create")
    assert create[:3] == ["create", "instance", "101"]
    options = create[3:]
    for flag, value in [("--disk", env["DISK"]), ("--label", LABEL)]:
        assert options[options.index(flag) + 1] == value
    assert options[options.index("--image") + 1].startswith("ubuntu:24.04@sha256:")
    assert {"--ssh", "--direct", "--cancel-unavail", "--raw"} <= set(options)
    assert "$0.180/h" in stderr and "RTX_3090" in stderr
    assert [i["label"] for i in instances(scaler)] == [LABEL]
    # Ready, then the tunnel, the binary and config, the worker.
    commands = ssh_commands(scaler)
    assert commands[0][2] == "test -e /work/.ready"
    assert {host for _, host, _ in commands} == {LABEL}
    # One ssh connection per port: the scheduler's stream never waits behind
    # the CAS's bulk transfers.
    assert tunnels(scaler) == [
        ["127.0.0.1:50051:127.0.0.1:50051"],
        ["127.0.0.1:50061:127.0.0.1:50061"],
    ]
    (copy,) = calls(scaler, "scp")
    assert copy[-3:] == [
        f"{scaler['dir']}/nativelink",
        f"{scaler['dir']}/worker.json5",
        f"{LABEL}:/work/",
    ]
    start = commands[-1][2]
    assert "kill -0" in start and "nohup ./nativelink worker.json5" in start
    since, price, instance = (scaler["state"] / "rented").read_text().split()
    assert abs(int(since) - time.time()) < 30 and price == "0.180" and instance == "5000"
    assert (scaler["state"] / "last_busy").exists()
    config = (scaler["state"] / f"{LABEL}.ssh_config").read_text().split()
    for option in [
        ["HostName", "203.0.113.7"],
        ["Port", "40022"],
        ["User", "root"],
        ["IdentityFile", f"{scaler['state']}/id_ed25519"],
        ["UserKnownHostsFile", f"{scaler['state']}/known_hosts.5000"],
        ["StrictHostKeyChecking", "accept-new"],
    ]:
        assert option in [config[i : i + 2] for i in range(len(config) - 1)]


def test_a_registered_worker_rents_nothing(scaler):
    running_box(scaler)
    gpu_worker(scaler)
    open_tunnels(scaler)
    demand(scaler, GPU_ACTION)
    rc, _, stderr = run(scaler, "tick")
    assert rc == 0, stderr
    assert not vastai(scaler, "search") and not vastai(scaler, "create")
    assert not vastai(scaler, "destroy")
    assert abs(int((scaler["state"] / "last_busy").read_text()) - time.time()) < 30


def test_idle_worker_is_drained_and_destroyed(scaler):
    running_box(scaler)
    gpu_worker(scaler, running=0)
    open_tunnels(scaler)
    rented(scaler, seconds_ago=1000)
    last_busy(scaler, seconds_ago=700)
    rc, _, stderr = run(scaler, "tick")
    assert rc == 0, stderr
    assert "idle for 10 min" in stderr
    assert drains(scaler) == [
        "http://admin.test/scheduler/MAIN_SCHEDULER/set_drain_worker/gpu-worker-1/1"
    ]
    assert any(("-O", "exit") in flags for flags, _, _ in ssh_commands(scaler))
    assert not open_tunnel_files(scaler)
    assert [c[2] for c in vastai(scaler, "destroy")] == ["4000"] and not instances(scaler)
    # 1000 s at $0.18/h, booked for the month.
    (line,) = (scaler["state"] / "ledger").read_text().splitlines()
    assert line == f"{time.strftime('%Y-%m')} 0.050"
    assert not (scaler["state"] / "rented").exists()


@pytest.mark.parametrize("busy", ["running", "queued"])
def test_a_busy_worker_is_kept(scaler, busy):
    running_box(scaler)
    gpu_worker(scaler, running=1 if busy == "running" else 0)
    if busy == "queued":
        demand(scaler, GPU_ACTION)
    open_tunnels(scaler)
    rented(scaler, seconds_ago=1000)
    last_busy(scaler, seconds_ago=700)
    rc, _, stderr = run(scaler, "tick")
    assert rc == 0, stderr
    assert not vastai(scaler, "destroy") and not drains(scaler)
    assert abs(int((scaler["state"] / "last_busy").read_text()) - time.time()) < 30


def test_a_worker_that_never_registers_is_destroyed(scaler):
    running_box(scaler)
    open_tunnels(scaler)
    rented(scaler, seconds_ago=100)  # LAUNCH_TIMEOUT is 2 s here
    last_busy(scaler, seconds_ago=0)
    rc, _, stderr = run(scaler, "tick")
    assert rc == 0, stderr
    assert "no GPU worker registered" in stderr
    assert [c[2] for c in vastai(scaler, "destroy")] == ["4000"] and not drains(scaler)


def test_a_worker_that_is_not_registered_yet_is_started(scaler):
    """Before LAUNCH_TIMEOUT a missing worker is started (again): one that
    died at start (a config error) says why in the journal."""
    running_box(scaler)
    open_tunnels(scaler)
    rented(scaler, seconds_ago=0)
    last_busy(scaler, seconds_ago=0)
    rc, _, stderr = run(scaler, "tick")
    assert rc == 0, stderr
    (command,) = [c[2] for c in ssh_commands(scaler) if c[2]]  # not the tunnel's check
    assert "kill -0" in command and "nohup ./nativelink worker.json5" in command
    assert "tail -n 2 worker.log" in command
    assert not calls(scaler, "scp") and not vastai(scaler, "destroy")


def test_a_worker_that_re_registers_is_given_the_launch_time_again(scaler):
    """A worker seen after LAUNCH_TIMEOUT that drops off the scheduler (a lost
    stream) is not a worker that never registered."""
    running_box(scaler)
    open_tunnels(scaler)
    rented(scaler, seconds_ago=100)
    last_busy(scaler, seconds_ago=0)
    (scaler["state"] / "worker_seen").write_text(f"{int(time.time()) - 1}\n")
    rc, _, stderr = run(scaler, "tick")
    assert rc == 0, stderr
    assert "no GPU worker registered" not in stderr and not vastai(scaler, "destroy")
    (command,) = [c[2] for c in ssh_commands(scaler) if c[2]]
    assert "nohup ./nativelink worker.json5" in command


def test_a_lost_tunnel_is_reopened(scaler):
    running_box(scaler)
    gpu_worker(scaler)
    rented(scaler, seconds_ago=0)
    rc, _, stderr = run(scaler, "tick")
    assert rc == 0, stderr
    assert "reconnecting" in stderr
    assert len(tunnels(scaler)) == 2 and len(calls(scaler, "scp")) == 1
    assert open_tunnel_files(scaler) == TUNNEL_FILES
    assert not vastai(scaler, "destroy")


def test_a_box_that_is_not_running_is_destroyed(scaler):
    running_box(scaler, status="exited")
    rc, _, stderr = run(scaler, "tick")
    assert rc == 0, stderr
    assert "is exited" in stderr
    assert [c[2] for c in vastai(scaler, "destroy")] == ["4000"]


def test_the_monthly_cap_stops_renting(scaler):
    cap = settings()["MONTHLY_CAP"]
    (scaler["state"] / "ledger").write_text(f"2000-01 1.000\n{time.strftime('%Y-%m')} {cap}\n")
    demand(scaler, GPU_ACTION)
    rc, _, stderr = run(scaler, "tick")
    assert rc == 1
    assert f"MONTHLY_CAP is ${cap}" in stderr
    assert vastai(scaler, "search") == [] and not vastai(scaler, "create")


def test_no_offer_under_the_cap(scaler):
    (scaler["fake"] / "offers.json").write_text("[]")
    demand(scaler, GPU_ACTION)
    rc, _, stderr = run(scaler, "tick")
    assert rc == 1
    assert "no offer" in stderr
    assert not vastai(scaler, "create") and not calls(scaler, "ssh")


def test_an_api_failure_rents_nothing(scaler):
    (scaler["fake"] / "api_fails").touch()
    demand(scaler, GPU_ACTION)
    rc, _, stderr = run(scaler, "tick")
    assert rc == 1
    assert "instance list failed" in stderr
    assert not vastai(scaler, "search") and not vastai(scaler, "create")


def test_a_silent_scheduler_changes_nothing(scaler):
    running_box(scaler)
    (scaler["fake"] / "scheduler_down").touch()
    last_busy(scaler, seconds_ago=700)
    rc, _, stderr = run(scaler, "tick")
    assert rc == 1
    assert "did not answer" in stderr
    assert not vastai(scaler, "destroy")


def test_a_box_that_never_comes_up_is_replaced(scaler):
    (scaler["fake"] / "statuses").write_text("exited running\n")
    demand(scaler, GPU_ACTION)
    rc, _, stderr = run(scaler, "tick")
    assert rc == 0, stderr
    # The first box (offer 101 on machine 11) is destroyed; the next machine's offer is rented.
    assert [c[2] for c in vastai(scaler, "create")] == ["101", "102"]
    assert [c[2] for c in vastai(scaler, "destroy")] == ["5000"]
    assert [i["id"] for i in instances(scaler)] == [5001]
    # The failed attempt is booked; the running one is not yet.
    assert len((scaler["state"] / "ledger").read_text().splitlines()) == 1
    assert (scaler["state"] / "rented").read_text().split()[2] == "5001"


def test_only_the_lost_tunnel_is_reopened(scaler):
    running_box(scaler)
    gpu_worker(scaler)
    rented(scaler, seconds_ago=0)
    open_tunnels(scaler)
    (scaler["fake"] / "tunnel.scheduler").unlink()
    rc, _, stderr = run(scaler, "tick")
    assert rc == 0, stderr
    assert "reconnecting" in stderr
    # The CAS's tunnel, still open, keeps its port: only the scheduler's reopens.
    assert tunnels(scaler) == [["127.0.0.1:50061:127.0.0.1:50061"]]
    assert open_tunnel_files(scaler) == TUNNEL_FILES


def test_a_box_that_cannot_be_connected_is_destroyed(scaler):
    (scaler["fake"] / "tunnel_fails").touch()
    demand(scaler, GPU_ACTION)
    rc, _, stderr = run(scaler, "tick")
    assert rc == 1
    assert "could not connect" in stderr
    # The rented box by its instance id (not the rent time), and booked.
    assert [c[2] for c in vastai(scaler, "destroy")] == ["5000"]
    assert instances(scaler) == []
    assert not (scaler["state"] / "rented").exists()
    assert len((scaler["state"] / "ledger").read_text().splitlines()) == 1


def test_a_box_without_a_rental_record_is_kept_quietly(scaler):
    running_box(scaler)
    gpu_worker(scaler)
    open_tunnels(scaler)
    rc, _, stderr = run(scaler, "tick")
    assert rc == 0, stderr
    assert "No such file" not in stderr
    assert not vastai(scaler, "destroy")


def test_onstart_installs_the_watchdog(scaler):
    demand(scaler, GPU_ACTION)
    run(scaler, "tick")
    script = onstart(scaler)
    assert len(script) < 4048  # vast.ai's limit for the onstart field
    assert PUBLIC_KEY not in script  # an account key (setup.sh): vast.ai rewrites authorized_keys
    # Idle 10 minutes, 8 hours at most (gpu.env).
    assert "nohup bash /work/.watchdog 600 28800" in script
    assert script.rstrip().endswith("touch /work/.ready")
    watchdog = script.split("<<'W'\n", 1)[1].split("\nW\n", 1)[0]
    assert "/proc/net/tcp" in watchdog and ":0016" in watchdog
    assert "api/v0/instances/$id/" in watchdog and "-X DELETE" in watchdog
    for text in [script, watchdog]:
        check = subprocess.run(["bash", "-n"], input=text, capture_output=True, text=True)
        assert check.returncode == 0, check.stderr


def test_up_and_down(scaler):
    rc, _, stderr = run(scaler, "up")
    assert rc == 0, stderr
    assert len(vastai(scaler, "create")) == 1 and len(tunnels(scaler)) == 2
    gpu_worker(scaler)
    rc, _, stderr = run(scaler, "down")
    assert rc == 0, stderr
    assert drains(scaler) and [c[2] for c in vastai(scaler, "destroy")] == ["5000"]
    assert not instances(scaler) and not open_tunnel_files(scaler)
    rc, _, stderr = run(scaler, "down")
    assert rc == 0 and f"no {LABEL}" in stderr


def test_status(scaler):
    rc, out, stderr = run(scaler, "status")
    assert rc == 0, stderr
    assert out.splitlines() == [
        "box: none",
        "worker: none",
        "queued GPU actions: 0",
        "tunnel: down",
        f"booked this month: $0.00 of ${settings()['MONTHLY_CAP']}",
    ]
    running_box(scaler)
    gpu_worker(scaler, running=1)
    demand(scaler, GPU_ACTION)
    open_tunnels(scaler)
    _, out, _ = run(scaler, "status")
    assert out.splitlines()[:4] == [
        "box: 4000 running 198.51.100.4 41022",
        "worker: gpu-worker-1 1 0 (id, running actions, draining)",
        "queued GPU actions: 1",
        "tunnel: up",
    ]


def test_the_gpu_property_agrees_everywhere():
    """The GPU tests request what the GPU worker advertises, every other action
    what the server's worker advertises, the scheduler matches on it and the
    scaler looks for the GPU value."""
    assert '{"test.gpu": "cuda"}' in source("bzl/gpu.bzl")
    worker = source("tools/remote/worker/nativelink.json5")
    assert re.search(r'gpu:\s*\{\s*values:\s*\["cuda"\]', worker)
    assert re.search(r'OSFamily:\s*\{\s*values:\s*\["linux"\]', worker)
    server = source("tools/remote/server/nativelink.json5")
    assert re.search(r'gpu:\s*"exact"', server) and re.search(r'OSFamily:\s*"priority"', server)
    assert re.search(r'gpu:\s*\{\s*values:\s*\["none"\]', server)
    assert re.search(r'OSFamily:\s*\{\s*values:\s*\["linux"\]', server)
    assert "\nGPU=cuda\n" in source("tools/remote/server/gpu-scaler")


if __name__ == "__main__":
    raise SystemExit(pytest.main([__file__, "-q"]))
