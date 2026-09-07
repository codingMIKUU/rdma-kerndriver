#!/usr/bin/env python3
"""Compile actual group-creation and hot-ring code against mocked RDMA calls.

No RDMA devices, sudo, installed-library changes or module reloads are needed.
"""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
MLX5 = ROOT / "drivers/infiniband/hw/mlx5"


def function(source, signature):
    start = source.index(signature)
    brace = source.index("\n{", start) + 1
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end] + "\n"


def main():
    source = (MLX5 / "scheduler.c").read_text()
    with tempfile.TemporaryDirectory(prefix="srm-multipeer-test-") as tmp:
        tmp = Path(tmp)
        (tmp / "srm_setup_under_test.inc").write_text(
            function(source, "int is_xrc_exists("))
        (tmp / "srm_hot_ring_under_test.inc").write_text(
            function(source, "static __always_inline void mlx5_srm_poll_ring_move_front(")
            .replace("__always_inline", "inline"))
        (tmp / "srm_select_under_test.inc").write_text(function(
            (MLX5 / "qp.c").read_text(),
            "static struct mlx5_ib_srmc *\nmlx5_ib_find_balanced_srmc_by_gid("))
        command = [os.environ.get("CC", "cc"), "-std=gnu11", "-O2", "-Wall", "-Wextra",
                   "-Werror", "-Wno-unused-parameter", "-pthread", "-I", str(MLX5),
                   "-I", str(tmp), str(ROOT / "tests/srm_multipeer_test.c"),
                   "-o", str(tmp / "test")]
        subprocess.run(command, check=True)
        subprocess.run([str(tmp / "test")], check=True)


if __name__ == "__main__":
    main()
