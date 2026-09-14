#!/usr/bin/env python3
"""Compile both historical CQ modes using local Kbuild flags, without installing.

Requires an already configured OFED tree and its recorded .*.o.cmd files.
All objects go to a TemporaryDirectory, never to the live module build.
"""
import argparse
import itertools
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
MLX5 = ROOT / "drivers/infiniband/hw/mlx5"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--units", nargs="+", default=["scheduler", "cq", "qp"])
    parser.add_argument("--diagnostics", action="store_true",
                        help="compile all combinations of the two statistics switches too")
    parser.add_argument("--wqe-timing", action="store_true",
                        help="also compile with WQE timing off and on")
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
            command = [x for x in command if not x.startswith(
                ("-DMLX5_SRM_ENABLE_DB_SHARE_STATS=",
                 "-DMLX5_SRM_ENABLE_CQE_CYCLE_STATS="))]
            command = [x for x in command if not x.startswith(
                "-DMLX5_SRM_ENABLE_WQE_TIMING=")]
            output = command.index("-o") + 1
            combinations = (itertools.product((0, 1), repeat=3) if args.diagnostics
                            else ((0, 0, 0), (1, 0, 0)))
            variants = itertools.product(combinations, (0, 1) if args.wqe_timing else (0,))
            for (mode, db_stats, cq_stats), timing in variants:
                variant = "cq%d-db%d-cycles%d" % (mode, db_stats, cq_stats)
                variant += "-timing%d" % timing
                command[output] = str(Path(tmp) / (unit + "-" + variant + ".o"))
                print("Compile", unit, variant, flush=True)
                subprocess.run(command + ["-DMLX5_SRM_ENABLE_CQE_SIMPLIFY=" + str(mode),
                               "-DMLX5_SRM_ENABLE_DB_SHARE_STATS=" + str(db_stats),
                               "-DMLX5_SRM_ENABLE_CQE_CYCLE_STATS=" + str(cq_stats),
                               "-DMLX5_SRM_ENABLE_WQE_TIMING=" + str(timing)],
                               cwd=config["KSRC_OBJ"], check=True)
        print("PASS: both historical CQ modes compile; no module installed")


if __name__ == "__main__":
    main()
