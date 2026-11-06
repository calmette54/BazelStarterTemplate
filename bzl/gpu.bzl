"""The GPU tests' requirements.

A GPU test is tagged for the config that runs it (`--config=gpu`: it fits a
6 GB GPU; `--config=large-gpu`: it needs 12-24 GB) and, under remote execution
(`--config=remote`), requests the `gpu` platform property value that routes
its test action to a vast.ai worker (tools/remote/worker/nativelink.json5),
overriding the execution platform's "none" (tools/remote:platform). The `test.`
prefix limits the override to the test action: the build stays on the
server's worker.

    load("//:bzl/gpu.bzl", "GPU")

    py_test(
        ...
        exec_properties = GPU.exec_properties,
        tags = GPU.tags,
    )
"""

# What the GPU workers advertise (the value is also what gpu-scaler looks for).
_GPU_WORKER = {"test.gpu": "cuda"}

GPU = struct(
    tags = ["requires-gpu"],
    exec_properties = _GPU_WORKER,
)

LARGE_GPU = struct(
    tags = ["requires-large-gpu"],
    exec_properties = _GPU_WORKER,
)
