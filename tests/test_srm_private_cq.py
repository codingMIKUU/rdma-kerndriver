#!/usr/bin/env python3
"""Exercise the actual private-CQ poller offline, without module installation."""
from pathlib import Path
import subprocess
import tempfile

from test_srm_cq_dispatch import block

ROOT = Path(__file__).resolve().parents[1]
MLX5 = ROOT / "drivers/infiniband/hw/mlx5"


def main():
    cq = (MLX5 / "cq.c").read_text()
    scheduler = (MLX5 / "scheduler.c").read_text()
    actual = (ROOT / "tests/srm_private_cq_test.c").read_text() + "\n"
    actual += block(cq, "static inline u64 mlx5_ib_srmc_complete_post(") + "\n"
    actual += block(cq, "int mlx5_ib_poll_srm_private_progress(") + "\n"
    actual += block(scheduler, "static __always_inline int\nmlx5_srm_refresh_poll_budget(")
    with tempfile.TemporaryDirectory(prefix="srm-private-cq-") as tmp:
        binary = str(Path(tmp) / "test")
        for budget in (1, 3, 256, 65536):
            subprocess.run([
                "cc", "-x", "c", "-std=gnu11", "-O1", "-g", "-Wall", "-Wextra",
                "-Werror", "-fsanitize=address,undefined", "-fno-pie", "-no-pie",
                "-DMLX5_SRM_ENABLE_PRIVATE_CQ=1",
                "-DMLX5_SRM_ENABLE_CQE_SIMPLIFY=1",
                "-DMLX5_SRM_PRIVATE_CQ_POLL_BUDGET=" + str(budget),
                "-o", binary, "-"], input=actual, text=True, check=True)
            subprocess.run([binary], check=True)
        ring = (ROOT / "tests/srm_private_cq_ring_test.c").read_text() + "\n"
        ring += block(scheduler, "static __always_inline int poll_srmc_inline(")
        subprocess.run([
            "cc", "-x", "c", "-std=gnu11", "-O1", "-g", "-Wall", "-Wextra",
            "-Werror", "-Wno-unused-parameter", "-fsanitize=address,undefined",
            "-fno-pie", "-no-pie", "-DMLX5_SRM_ENABLE_PRIVATE_CQ=1",
            "-o", binary, "-"], input=ring, text=True, check=True)
        subprocess.run([binary], check=True)


if __name__ == "__main__":
    main()
