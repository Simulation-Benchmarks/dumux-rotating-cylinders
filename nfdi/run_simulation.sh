#!/usr/bin/env bash
set -euo pipefail

echo "=== [INFO] Starting rotating cylinders simulation script ==="
date
echo "User: $(whoami)"
echo "Hostname: $(hostname)"
echo "PWD at start: $(pwd)"

SIM_DIR="/dumux/rotating-cylinders/build-cmake/test/freeflow/navierstokes/rotatingcylinders"
EXEC="test_ff_navierstokes_rotatingcylinders"
SHARED="/dumux/shared"

echo "SIM_DIR  = ${SIM_DIR}"
echo "EXEC     = ${EXEC}"
echo "SHARED   = ${SHARED}"

echo "Changing directory to SIM_DIR"
cd "${SIM_DIR}"
echo "PWD after cd: $(pwd)"

echo "Listing contents of SIM_DIR:"
ls -lah

echo "=== [INFO] Building executable ==="
make "${EXEC}"
echo "=== [INFO] Build finished ==="

echo "=== [INFO] Running executable ==="
./"${EXEC}"
echo "=== [INFO] Executable finished ==="

echo "Listing files after execution:"
ls -lah

echo "Checking for VTU files"
shopt -s nullglob
vtus=(*.vtu)

if [ ${#vtus[@]} -eq 0 ]; then
    echo "ERROR: No .vtu files produced in $(pwd)" >&2
else
    echo "Found ${#vtus[@]} VTU file(s):"
    printf '  %s\n' "${vtus[@]}"
    echo "Copying VTU files to ${SHARED}"
    cp "${vtus[@]}" "${SHARED}/"
fi

echo "Listing files in SHARED directory:"
ls -lah "${SHARED}"

echo "=== [INFO] Script completed ==="
date

