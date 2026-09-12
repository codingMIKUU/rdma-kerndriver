#!/usr/bin/env python3
"""Compile affected mlx5 units with existing Kbuild flags, into temporary files.

This is an object compilation check, NOT a module link/install/load.  Reuses
the last local build's .*.o.cmd and kernel headers; never writes the original
root-owned objects, dependencies, configuration or installed modules.
"""
import argparse
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
MLX5 = ROOT / "drivers/infiniband/hw/mlx5"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--units", nargs="+", default=["scheduler", "qp", "cq", "ah", "main"])
    parser.add_argument("--cq-modes", nargs="+", type=int, choices=(0, 1, 2),
                        default=[0, 1, 2],
                        help="CQ delivery modes to compile (default: all three)")
    opts = parser.parse_args()
    config = dict(line.split("=", 1) for line in
                  (ROOT / "configure.mk.kernel").read_text().splitlines()
                  if "=" in line and not line.startswith("#"))
    kernel_build = Path(config["KSRC_OBJ"]).resolve()
    with tempfile.TemporaryDirectory(prefix="srm-kernel-compile-") as tmp:
        for unit in opts.units:
            if unit not in {"scheduler", "qp", "cq", "ah", "main"}:
                raise SystemExit("Unexpected unit: " + unit)
            line = (MLX5 / ("." + unit + ".o.cmd")).read_text().splitlines()[0]
            args = shlex.split(line.split(":=", 1)[1])
            if any(x in args for x in (";", "&&", "||")):
                raise SystemExit("Unsupported compound Kbuild command; use the normal build")
            source = str(MLX5 / (unit + ".c"))
            if source not in args:
                raise SystemExit("Kbuild record is not from this workspace: " + unit)
            # Dependency writes are not part of this isolated compile check.
            args = [x for x in args if not x.startswith(("-Wp,-MD,", "-Wp,-MMD,"))]
            output = args.index("-o") + 1
            for levels in (0, 1):
                for mode in opts.cq_modes:
                    args[output] = str(Path(tmp) /
                        (unit + "-large" + str(levels) + "-cq" + str(mode) + ".o"))
                    command = args + [
                        "-DMLX5_SRM_ENABLE_LARGE_KERNEL_QP=" + str(levels),
                        "-DMLX5_SRM_ENABLE_CQE_SIMPLIFY=" + str(mode),
                    ]
                    print("Compile", unit, "size_split=" + str(levels),
                          "cq_simplify=" + str(mode), flush=True)
                    subprocess.run(command, cwd=kernel_build, check=True)
        print("PASS: affected kernel units compile across requested CQ modes and size split OFF/ON")


if __name__ == "__main__":
    main()
