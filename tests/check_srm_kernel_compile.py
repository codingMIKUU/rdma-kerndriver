#!/usr/bin/env python3
"""Compile both historical CQ modes using local Kbuild flags, without installing.

Requires an already configured OFED tree and its recorded .*.o.cmd files.
All objects go to a TemporaryDirectory, never to the live module build.
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
    parser.add_argument("--units", nargs="+", default=["scheduler", "cq", "qp"])
    args = parser.parse_args()
    config = dict(line.split("=", 1) for line in
                  (ROOT / "configure.mk.kernel").read_text().splitlines()
                  if "=" in line and not line.startswith("#"))
    with tempfile.TemporaryDirectory(prefix="srm-legacy-cq-compile-") as tmp:
        for unit in args.units:
            if unit not in {"scheduler", "cq", "qp", "ah", "main"}:
                raise SystemExit("Unsupported unit: " + unit)
            record = (MLX5 / ("." + unit + ".o.cmd")).read_text().splitlines()[0]
            command = shlex.split(record.split(":=", 1)[1])
            if any(x in command for x in (";", "&&", "||")):
                raise SystemExit("Compound Kbuild command is not supported")
            if str(MLX5 / (unit + ".c")) not in command:
                raise SystemExit("Kbuild record belongs to a different workspace")
            command = [x for x in command
                       if not x.startswith(("-Wp,-MD,", "-Wp,-MMD,",
                                            "-DMLX5_SRM_ENABLE_CQE_SIMPLIFY="))]
            output = command.index("-o") + 1
            for mode in (0, 1):
                command[output] = str(Path(tmp) / (unit + "-cq" + str(mode) + ".o"))
                print("Compile", unit, "CQE_SIMPLIFY=" + str(mode), flush=True)
                subprocess.run(command + ["-DMLX5_SRM_ENABLE_CQE_SIMPLIFY=" + str(mode)],
                               cwd=config["KSRC_OBJ"], check=True)
        print("PASS: both historical CQ modes compile; no module installed")


if __name__ == "__main__":
    main()
