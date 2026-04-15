// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
//
// SPDX-FileCopyrightText: Copyright © DuMux Project contributors, see AUTHORS.md in root folder
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include <config.h>

#include <iostream>
#include <random>
#include <fstream>
#include <cstdlib>
#include <string>

#include <dune/common/parallel/mpihelper.hh>
#include <dune/common/timer.hh>
#include <dune/common/version.hh>

#include <dumux/common/initialize.hh>
#include <dumux/common/dumuxmessage.hh>
#include <dumux/common/parameters.hh>
#include <dumux/common/properties.hh>

#include <dumux/io/vtkoutputmodule.hh>
#include <dumux/io/grid/cakegridmanager.hh>

#include <dumux/linear/istlsolvers.hh>
#include <dumux/linear/stokes_solver.hh>

#include <dumux/multidomain/fvassembler.hh>
#include <dumux/multidomain/traits.hh>
#include <dumux/multidomain/newtonsolver.hh>

#include <dumux/freeflow/navierstokes/velocityoutput.hh>

#include <dumux/discretization/elementsolution.hh>
#include <dumux/discretization/evalsolution.hh>

#include "properties.hh"

template<class Vector, class MomGG, class MassGG, class MomP, class MomIdx, class MassIdx>
auto dirichletDofs(std::shared_ptr<MomGG> momentumGridGeometry,
                   std::shared_ptr<MassGG> massGridGeometry,
                   std::shared_ptr<MomP> momentumProblem,
                   MomIdx momentumIdx, MassIdx massIdx)
{
    Vector dirichletDofs;
    dirichletDofs[momentumIdx].resize(momentumGridGeometry->numDofs());
    dirichletDofs[massIdx].resize(massGridGeometry->numDofs());
    dirichletDofs = 0.0;

    auto fvGeometry = localView(*momentumGridGeometry);
    for (const auto& element : elements(momentumGridGeometry->gridView()))
    {
        fvGeometry.bind(element);
        for (const auto& scv : scvs(fvGeometry))
        {
            if (momentumGridGeometry->dofOnBoundary(scv.dofIndex()))
            {
                const auto bcTypes = momentumProblem->boundaryTypes(element, scv);
                for (int i = 0; i < bcTypes.size(); ++i)
                    if (bcTypes.isDirichlet(i))
                        dirichletDofs[momentumIdx][scv.dofIndex()][i] = 1.0;
            }
        }
    }

    return dirichletDofs;
}

int main(int argc, char** argv)
{
    using namespace Dumux;

    // define the type tag for this problem
    using MomentumTypeTag = Properties::TTag::TYPETAG_MOMENTUM;
    using MassTypeTag = Properties::TTag::TYPETAG_MASS;

    // initialize MPI, finalize is done automatically on exit
    initialize(argc, argv);
    const auto& mpiHelper = Dune::MPIHelper::instance();

    Dune::Timer timer;

    // print dumux start message
    if (mpiHelper.rank() == 0)
        DumuxMessage::print(/*firstCall=*/true);

    // parse command line arguments and input file
    Parameters::init(argc, argv);

    // create a grid
    using Grid = GetPropType<MassTypeTag, Properties::Grid>;
    using GridManager = typename Dumux::CakeGridManager<Grid>;
    GridManager gridManager;
    gridManager.init();

    ////////////////////////////////////////////////////////////
    // run instationary non-linear problem on this grid
    ////////////////////////////////////////////////////////////

    // we compute on the leaf grid view
    const auto& leafGridView = gridManager.grid().leafGridView();
    std::cout << "Grid overlap size: " << leafGridView.overlapSize(0) << std::endl;
    std::cout << "Grid ghost size: " << leafGridView.ghostSize(0) << std::endl;

    // create the finite volume grid geometry
    using MomentumGridGeometry = GetPropType<MomentumTypeTag, Properties::GridGeometry>;
    auto momentumGridGeometry = std::make_shared<MomentumGridGeometry>(leafGridView);
    using MassGridGeometry = GetPropType<MassTypeTag, Properties::GridGeometry>;
    auto massGridGeometry = std::make_shared<MassGridGeometry>(leafGridView);

    // the coupling manager
    using CouplingManager = GetPropType<MomentumTypeTag, Properties::CouplingManager>;
    auto couplingManager = std::make_shared<CouplingManager>();

    // the problems (boundary conditions)
    using MomentumProblem = GetPropType<MomentumTypeTag, Properties::Problem>;
    auto momentumProblem = std::make_shared<MomentumProblem>(momentumGridGeometry, couplingManager);
    using MassProblem = GetPropType<MassTypeTag, Properties::Problem>;
    auto massProblem = std::make_shared<MassProblem>(massGridGeometry, couplingManager);

    // Extract indices for L2 norm calculation
    using MassIndices = typename GetPropType<MassTypeTag, Properties::ModelTraits>::Indices;
    using MomentumIndices = typename GetPropType<MomentumTypeTag, Properties::ModelTraits>::Indices;

    // the solution vector
    constexpr auto momentumIdx = CouplingManager::freeFlowMomentumIndex;
    constexpr auto massIdx = CouplingManager::freeFlowMassIndex;
    using Traits = MultiDomainTraits<MomentumTypeTag, MassTypeTag>;
    using SolutionVector = typename Traits::SolutionVector;
    SolutionVector x;
    x[momentumIdx].resize(momentumGridGeometry->numDofs());
    x[massIdx].resize(massGridGeometry->numDofs());
    std::cout << "Total number of dofs on rank " << mpiHelper.rank() << ": "
        << massGridGeometry->numDofs() + momentumGridGeometry->numDofs()*MomentumGridGeometry::GridView::dimension
        << std::endl;

    // the grid variables
    using MomentumGridVariables = GetPropType<MomentumTypeTag, Properties::GridVariables>;
    auto momentumGridVariables = std::make_shared<MomentumGridVariables>(momentumProblem, momentumGridGeometry);
    using MassGridVariables = GetPropType<MassTypeTag, Properties::GridVariables>;
    auto massGridVariables = std::make_shared<MassGridVariables>(massProblem, massGridGeometry);

    // compute coupling stencil and afterwards initialize grid variables (need coupling information)
    couplingManager->init(momentumProblem, massProblem, std::make_tuple(momentumGridVariables, massGridVariables), x);
    massGridVariables->init(x[massIdx]);
    momentumGridVariables->init(x[momentumIdx]);

    using Assembler = MultiDomainFVAssembler<Traits, CouplingManager, DiffMethod::numeric>;
    auto assembler = std::make_shared<Assembler>(std::make_tuple(momentumProblem, massProblem),
                                                 std::make_tuple(momentumGridGeometry, massGridGeometry),
                                                 std::make_tuple(momentumGridVariables, massGridVariables),
                                                 couplingManager);

    // initialize the vtk output module
    constexpr bool isDiamond = MassGridGeometry::discMethod == DiscretizationMethods::fcdiamond;
    const auto mode = isDiamond ? Dune::VTK::nonconforming : Dune::VTK::conforming;
    using IOFields = GetPropType<MassTypeTag, Properties::IOFields>;
    VtkOutputModule vtkWriter(*massGridVariables, x[massIdx], massProblem->name(), "", mode);
    IOFields::initOutputModule(vtkWriter); // Add model specific output fields
    vtkWriter.addVelocityOutput(std::make_shared<NavierStokesVelocityOutput<MassGridVariables>>());
    vtkWriter.write(0.0);

    // the linearize and solve
    if (getParam<bool>("LinearSolver.UseIterativeSolver", false))
    {
        using Matrix = typename Assembler::JacobianMatrix;
        using Vector = typename Assembler::ResidualType;
        using LinearSolver = StokesSolver<Matrix, Vector, MomentumGridGeometry, MassGridGeometry>;
        auto dDofs = dirichletDofs<Vector>(momentumGridGeometry, massGridGeometry, momentumProblem, momentumIdx, massIdx);
        auto linearSolver = std::make_shared<LinearSolver>(momentumGridGeometry, massGridGeometry, dDofs);
        using NewtonSolver = MultiDomainNewtonSolver<Assembler, LinearSolver, CouplingManager>;
        NewtonSolver nonLinearSolver(assembler, linearSolver, couplingManager);
        nonLinearSolver.solve(x);
    }
    else
    {
        using LinearSolver = UMFPackIstlSolver<SeqLinearSolverTraits, LinearAlgebraTraitsFromAssembler<Assembler>>;
        auto linearSolver = std::make_shared<LinearSolver>();
        using NewtonSolver = MultiDomainNewtonSolver<Assembler, LinearSolver, CouplingManager>;
        NewtonSolver nonLinearSolver(assembler, linearSolver, couplingManager);
        nonLinearSolver.solve(x);
    }

    // Initialize error variables
    using Scalar = GetPropType<MassTypeTag, Properties::Scalar>;
    Scalar l2ErrorMassSquared = 0.0;
    Scalar l2ErrorMomentumSquared = 0.0;

    // Loop over elements to integrate error
    for (const auto& element : elements(leafGridView))
    {
        auto geometry = element.geometry();
        const auto& quad = Dune::QuadratureRules<Scalar, Grid::dimension>::rule(element.type(), 3);

        for (const auto& qp : quad)
        {
            const auto& pos = geometry.global(qp.position());
            const auto weight = qp.weight() * geometry.integrationElement(qp.position());

            // --- Mass (Pressure) Error ---
            // Fix: Pass 'element' as first argument
            auto massElemSol = elementSolution(element, x[massIdx], *massGridGeometry);
            auto massNum = evalSolution(element, geometry, *massGridGeometry, massElemSol, pos);
            auto massExact = massProblem->analyticalSolution(pos);
            
            Scalar pDiff = massNum[MassIndices::pressureIdx] - massExact[MassIndices::pressureIdx];
            l2ErrorMassSquared += pDiff * pDiff * weight;

            // --- Momentum (Velocity) Error ---
            auto momElemSol = elementSolution(element, x[momentumIdx], *momentumGridGeometry);
            auto momNum = evalSolution(element, geometry, *momentumGridGeometry, momElemSol, pos);
            auto momExact = momentumProblem->analyticalSolution(pos);

            // Use the alias to access the specific velocity components
            Scalar vDiffX = momNum[MomentumIndices::velocityXIdx] - momExact[MomentumIndices::velocityXIdx];
            Scalar vDiffY = momNum[MomentumIndices::velocityYIdx] - momExact[MomentumIndices::velocityYIdx];

            l2ErrorMomentumSquared += (vDiffX*vDiffX + vDiffY*vDiffY) * weight;
        }
    }

    // Collect from all MPI ranks
    Scalar totalL2Mass = std::sqrt(leafGridView.comm().sum(l2ErrorMassSquared));
    Scalar totalL2Momentum = std::sqrt(leafGridView.comm().sum(l2ErrorMomentumSquared));

    if (mpiHelper.rank() == 0)
    {
        std::cout << "\n--- L2 Error Metrics ---" << std::endl;
        std::cout << "L2 Error (Pressure): " << totalL2Mass << std::endl;
        std::cout << "L2 Error (Velocity): " << totalL2Momentum << std::endl;
        std::cout << "------------------------\n" << std::endl;
    }

    // write vtk output
    vtkWriter.write(1.0);

    // update coupling manager for output
    couplingManager->updateSolution(x);

    timer.stop();
    const auto& comm = leafGridView.comm();
    std::cout << "Simulation took " << timer.elapsed() << " seconds on "
              << comm.size() << " processes.\n"
              << "The cumulative CPU time was " << timer.elapsed()*comm.size() << " seconds.\n";

    ////////////////////////////////////////////////////////////
    // finalize, print dumux message to say goodbye
    ////////////////////////////////////////////////////////////

    if (mpiHelper.rank() == 0)
    {
        std::cout << "\n--- Saving Metrics and Creating Zip ---" << std::endl;
        
        // Write the Metrics JSON
        std::ofstream out("solution_metrics.json");
        out << "{\n";
        out << "  \"max_l2_error_pressure\": " << totalL2Mass << ",\n";
        out << "  \"max_l2_error_velocity\": " << totalL2Momentum << "\n";
        // out << "  \"simulation_time\": " << timer.elapsed() << "\n";
        out << "}\n";
        out.close();

        // Identify files based on Problem.Name
        std::string configName = getParam<std::string>("Problem.Name");
        
        // Command to zip metrics and VTK files
        // Added -j to junk paths (keeps zip flat) and redirected stderr to null
        std::string zipCommand = "zip -j " + configName + ".zip solution_metrics.json " + 
                                 configName + "*.vtu " + configName + "*.pvtu " + 
                                 configName + "*.vtp > /dev/null 2>&1";

        std::cout << "Executing: " << zipCommand << std::endl;
        int result = std::system(zipCommand.c_str());
        
        if (result == 0)
            std::cout << "Successfully created " << configName << ".zip" << std::endl;
        else
            std::cerr << "Warning: Zip command failed. Ensure 'zip' is installed in the container." << std::endl;

        // Print final summary
        std::cout << "Simulation took " << timer.elapsed() << " seconds on "
                  << comm.size() << " processes.\n";
        Parameters::print();
        DumuxMessage::print(/*firstCall=*/false);
    }

    return 0;
}
