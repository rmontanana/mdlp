# ***************************************************************
# SPDX-FileCopyrightText: Copyright 2024 Ricardo Montañana Gómez
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT
# ***************************************************************

import subprocess
import sys
from pathlib import Path

# Resolved from this file's location rather than the working directory, so the
# script works from anywhere now that it no longer sits beside README.md.
REPO_ROOT = Path(__file__).resolve().parent.parent
readme_file = REPO_ROOT / "README.md"
print("Updating coverage...")
if len(sys.argv) < 2:
    print("Usage: update_coverage.py <directory containing coverage.info>")
    sys.exit(1)
# The directory comes from the command line, so constrain it before it reaches
# a subprocess: it must resolve to a real coverage.info inside this repository.
# Resolving first means "../.." and symlinks cannot escape the containment check.
coverage_dir = Path(sys.argv[1]).resolve()
if REPO_ROOT not in coverage_dir.parents and coverage_dir != REPO_ROOT:
    print(f"⛔Refusing to read outside the repository: {coverage_dir}")
    sys.exit(1)
coverage_info = coverage_dir / "coverage.info"
if not coverage_info.is_file():
    print(f"⛔No coverage.info found in {coverage_dir}")
    sys.exit(1)
# Argument list rather than a shell string: under shell=True a value like
# "x; rm -rf ~" would run as a command.
output = subprocess.check_output(
    ["lcov", "--summary", str(coverage_info)],
)
value = output.decode("utf-8").strip()
percentage = 0
for line in value.splitlines():
    if "lines" in line:
        percentage = float(line.split(":")[1].split("%")[0])
        break
print(f"Coverage: {percentage}%")
if percentage < 90:
    print("⛔Coverage is less than 90%. I won't update the badge.")
    sys.exit(1)
percentage_label = str(percentage).replace(".", ",")
coverage_line = f"[![Coverage Badge](https://img.shields.io/badge/Coverage-{percentage_label}%25-green)](html/index.html)"
# Update README.md
with open(readme_file, "r") as f:
    lines = f.readlines()
with open(readme_file, "w") as f:
    for line in lines:
        if "img.shields.io/badge/Coverage" in line:
            f.write(coverage_line + "\n")
        else:
            f.write(line)
print(f"✅Coverage updated with value: {percentage}")
