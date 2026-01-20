#!/usr/bin/env bash
set -euo pipefail

# ---------------------------------------------
# Rotating Cylinders Benchmark Installation & Simulation Script
# ---------------------------------------------

echo "=== [INFO] Installing rotating-cylinders benchmark ==="
installModule rotating-cylinders https://git.iws.uni-stuttgart.de/benchmarks/rotating-cylinders.git origin/main ""
echo "=== [INFO] Installation finished ==="

echo "=== [INFO] Configuring project ==="
if ! ./dune-common/bin/dunecontrol --opts=rotating-cylinders/cmake.opts all; then
    echo "ERROR: Configuration of the project failed" >&2
    exit 1
fi
echo "=== [INFO] Configuration finished ==="

# ---------------------------------------------
# Simulation Information
# ---------------------------------------------
echo "=== [INFO] Starting rotating cylinders simulation ==="
date
echo "User: $(whoami)"
echo "Hostname: $(hostname)"
echo "Working directory at start: $(pwd)"

# Directories and executable
SIM_DIR="/dumux/rotating-cylinders/build-cmake/test/freeflow/navierstokes/rotatingcylinders"
EXEC="test_ff_navierstokes_rotatingcylinders"
SHARED="/dumux/shared"

echo "SIM_DIR  = ${SIM_DIR}"
echo "EXEC     = ${EXEC}"
echo "SHARED   = ${SHARED}"

# ---------------------------------------------
# Change to simulation directory
# ---------------------------------------------
echo "=== [INFO] Changing directory to SIM_DIR ==="
cd "${SIM_DIR}"
echo "PWD after cd: $(pwd)"

echo "Listing contents of SIM_DIR:"
ls -lah

# ---------------------------------------------
# Build the executable
# ---------------------------------------------
echo "=== [INFO] Building executable ==="
make "${EXEC}"
echo "=== [INFO] Build finished ==="

# ---------------------------------------------
# Run the simulation
# ---------------------------------------------
echo "=== [INFO] Running executable ==="
./"${EXEC}"
echo "=== [INFO] Executable finished ==="

# ---------------------------------------------
# Post-processing: check for output files
# ---------------------------------------------
echo "Listing files after execution:"
ls -lah

echo "=== [INFO] Checking for VTU files ==="
shopt -s nullglob
vtus=(*.vtu)

if [ ${#vtus[@]} -eq 0 ]; then
    echo "ERROR: No .vtu files produced in $(pwd)" >&2
else
    echo "Found ${#vtus[@]} VTU file(s):"
    printf '  %s\n' "${vtus[@]}"
    
    echo "Copying VTU files to ${SHARED}"
    cp -v "${vtus[@]}" "${SHARED}/"
fi

echo "Listing files in SHARED directory:"
ls -lah "${SHARED}"

# ---------------------------------------------
# Completion message
# ---------------------------------------------
echo "=== [INFO] Script completed ==="
date
