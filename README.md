# Rotating Cylinders Benchmark

This benchmark solves the classical Taylor–Couette flow problem (viscous flow between two concentric, independently rotating cylinders) with the
Navier–Stokes free-flow model in [DuMux](https://dumux.org). It compares the numerical solution against the known analytical solution for velocity and pressure.

The content of this DUNE module was extracted from the module `dumux`.
In particular, the following subfolder of `dumux` has been extracted:
*   `test/freeflow/navierstokes/rotatingcylinders`

Additionally, all headers in `dumux` that are required to build the
executable from the sources
*   `test/freeflow/navierstokes/rotatingcylinders/main.cc`

have been extracted. You can configure the module just like any other DUNE
module by using `dunecontrol`. For building and running the executable,
please go to the build folder corresponding to the sources listed above.


## License

This project is licensed under the terms and conditions of the GNU General Public
License (GPL) version 3 or - at your option - any later version.
The GPL can be found under [GPL-3.0-or-later.txt](LICENSES/GPL-3.0-or-later.txt)
provided in the `LICENSES` directory located at the topmost of the source code tree.

## Version Information

|      module name      |      branch name       |                 commit sha                 |         commit date         |
|-----------------------|------------------------|--------------------------------------------|-----------------------------|
|       dune-istl       |  origin/releases/2.10  |  21c67275b17e93918365177f93f42e4aaa9afd23  |  2025-02-03 09:13:05 +0000  |
|       dune-grid       |  origin/releases/2.10  |  954436b88247e904628ec4d7c8bb7b2eaac08900  |  2024-10-22 15:31:40 +0000  |
|  dune-localfunctions  |  origin/releases/2.10  |  ddbf693b5f9c867b2d58d418fe130bbe92c06a99  |  2025-06-27 05:40:52 +0000  |
|     dune-geometry     |  origin/releases/2.10  |  5673e95ac364ad3498aed9eaf65e0d224384d15a  |  2024-09-04 16:39:39 +0200  |
|      dune-uggrid      |  origin/releases/2.10  |  cf2513efb6497dc95744649e0658cedef2980bff  |  2024-09-05 08:40:59 +0200  |
|      dune-common      |  origin/releases/2.10  |  fa09b5bd31efa38cc051e22717b0d259de5ae8a1  |  2025-06-03 21:04:17 +0000  |

## Installation

### Option 1: Manual installation

Create a root folder, e.g. `DUMUX`, enter the previously created folder,
clone this repository and use the install script `install_rotating-cylinders.sh`
provided in this repository to install all dependent modules.

```sh
mkdir DUMUX
cd DUMUX
git clone https://git.iws.uni-stuttgart.de/benchmarks/rotating-cylinders.git rotating-cylinders
./rotating-cylinders/docker/install_rotating-cylinders.sh
```

This will clone all modules into the directory `DUMUX`,
configure your module with `dunecontrol` and build tests.


### Option 2: Docker

A `Dockerfile` is provided that builds all dependencies and the benchmark
executable into a single image.

Build the image:
```sh
docker build -t git.iws.uni-stuttgart.de:4567/benchmarks/rotating-cylinders .
```

Start a container and drop into a shell, with the current directory mounted
as the shared workspace at `/dumux/shared`:
```sh
./docker_rotatingCylinders.sh open
```
You can pass a different image name as an optional argument:
```sh
./docker_rotatingCylinders.sh open <image-name>
```

## Running the simulation

The executable `test_ff_navierstokes_rotatingcylinders` reads its
configuration from `params.input`, which controls:
*   `[Grid]` — mesh resolution and grading, radial and angular extent of the annulus between the two cylinders
*   `[Problem]` — inner/outer cylinder angular velocities (`Omega1`, `Omega2`) and whether inertia terms/gravity are enabled
*   `[Component]` — fluid density and dynamic viscosity
*   `[LinearSolver]` / `[Newton]` — solver and convergence settings

```sh
./test_ff_navierstokes_rotatingcylinders params.input
```

On completion, the run produces:
*   `solution_metrics.json` — relative L2 errors of pressure and velocity against the analytical Taylor–Couette solution
*   `<Problem.Name>.zip` — the `.pvd`/`.vtu` VTK output files (the raw files are removed after zipping)

## Visualization

`pvViewer.py` is a ParaView Python script that loads the `.pvd` output,
colors the domain by pressure, and saves a high-resolution screenshot. Run it
with `pvpython` (ParaView's Python interpreter) from the directory containing
the simulation output:

```sh
pvpython pvViewer.py
```

By default it expects a file named `test_rotatingcylinders.pvd` (matching
`Problem.Name` in `params.input`) and writes
`pressure_field_presentation_fixed.png`.
