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
# Generate badge line. Argument list rather than a shell string: the path comes
# from the command line, and under shell=True a value like "x; rm -rf ~" would
# run as a command.
coverage_info = Path(sys.argv[1]) / "coverage.info"
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
