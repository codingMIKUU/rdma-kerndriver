#!/usr/bin/env python3
"""Run the actual kernel CQ dispatcher against mocked hardware and user rings."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
MLX5 = ROOT / "drivers/infiniband/hw/mlx5"


def block(source, signature):
    start = source.index(signature)
    opening = source.index("{", start)
    end, depth = opening + 1, 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def main():
    scheduler = (MLX5 / "scheduler.c").read_text()
    cq = (MLX5 / "cq.c").read_text()
    uapi = (ROOT / "include/uapi/rdma/mlx5-abi.h").read_text()
    actual = [block(uapi, "struct mlx5_srm_sw_cqe {") + ";",
              block(uapi, "struct mlx5_srm_sw_cq {") + ";",
              block(uapi, "struct mlx5_srm_direct_cqe_meta {") + ";",
              block((MLX5 / "scheduler.h").read_text(),
                    "struct mlx5_srm_direct_batch {") + ";",
              block(scheduler, "int mlx5_ib_activate_srm_cq_route("),
              block(scheduler, "int mlx5_ib_srm_dispatch_completion("),
              block(scheduler, "void mlx5_ib_srm_direct_flush("),
              block(scheduler, "int mlx5_ib_srm_direct_completion("),
              block(cq, "static inline u64 mlx5_ib_srmc_complete_post("),
              block(cq, "static int mlx5_poll_one_srm_dispatch("),
              block(cq, "int mlx5_ib_poll_srm_dispatch("),
              block(cq, "int mlx5_ib_poll_srm_direct(")]
    with tempfile.TemporaryDirectory(prefix="srm-cq-dispatch-") as directory:
        out = Path(directory)
        (out / "actual_dispatch.h").write_text("\n".join(actual))
        binary = out / "test"
        subprocess.run(["cc", "-std=gnu11", "-O2", "-Wall", "-Wextra",
                        "-Werror", "-Wno-unused-parameter", "-I", str(out),
                        str(ROOT / "tests/srm_cq_dispatch_test.c"),
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
