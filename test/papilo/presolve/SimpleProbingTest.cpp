/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*               This file is part of the program and library                */
/*    PaPILO --- Parallel Presolve for Integer and Linear Optimization       */
/*                                                                           */
/* Copyright (C) 2020  Konrad-Zuse-Zentrum                                   */
/*                     fuer Informationstechnik Berlin                       */
/*                                                                           */
/* This program is free software: you can redistribute it and/or modify      */
/* it under the terms of the GNU Lesser General Public License as published  */
/* by the Free Software Foundation, either version 3 of the License, or      */
/* (at your option) any later version.                                       */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/* GNU Lesser General Public License for more details.                       */
/*                                                                           */
/* You should have received a copy of the GNU Lesser General Public License  */
/* along with this program.  If not, see <https://www.gnu.org/licenses/>.    */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "papilo/presolvers/SimpleProbing.hpp"
#include "catch/catch.hpp"
#include "papilo/core/PresolveMethod.hpp"
#include "papilo/core/Problem.hpp"
#include "papilo/core/ProblemBuilder.hpp"

using namespace papilo;

Problem<double>
setupProblemWithSimpleProbing();

TEST_CASE( "happy path - simple probing", "[presolve]" )
{
   Num<double> num{};
   Problem<double> problem = setupProblemWithSimpleProbing();
   Statistics statistics{};
   PresolveOptions presolveOptions{};
   presolveOptions.dualreds = 0;
   Postsolve<double> postsolve = Postsolve<double>( problem, num );
   ProblemUpdate<double> problemUpdate( problem, postsolve, statistics,
                                        presolveOptions, num );
   SimpleProbing<double> presolvingMethod{};
   Reductions<double> reductions{};
   problem.recomputeAllActivities();

   PresolveStatus presolveStatus =
       presolvingMethod.execute( problem, problemUpdate, num, reductions );

   BOOST_ASSERT( presolveStatus == PresolveStatus::kReduced );
   BOOST_ASSERT( reductions.size() == 4 );

   //  lb + x (ub -lb) = y => x = 1 -y
   BOOST_ASSERT( reductions.getReduction( 0 ).col == 1 );
   BOOST_ASSERT( reductions.getReduction( 0 ).row == ColReduction::REPLACE );
   BOOST_ASSERT( reductions.getReduction( 0 ).newval == -1 );

   BOOST_ASSERT( reductions.getReduction( 1 ).col == 0 );
   BOOST_ASSERT( reductions.getReduction( 1 ).row ==
                 papilo::ColReduction::NONE );
   BOOST_ASSERT( reductions.getReduction( 1 ).newval == 1 );

   //  lb + x (ub -lb) = z => x = 1 -z
   BOOST_ASSERT( reductions.getReduction( 2 ).col == 2 );
   BOOST_ASSERT( reductions.getReduction( 2 ).row ==
                 papilo::ColReduction::REPLACE );
   BOOST_ASSERT( reductions.getReduction( 2 ).newval == -1 );

   BOOST_ASSERT( reductions.getReduction( 3 ).col == 0 );
   BOOST_ASSERT( reductions.getReduction( 3 ).row ==
                 papilo::ColReduction::NONE );
   BOOST_ASSERT( reductions.getReduction( 3 ).newval == 1 );

}

Problem<double>
setupProblemWithSimpleProbing()
{
   // simple probing requires
   // - rhs = (sup -inf) / 2
   // futhermore for one column
   // - integral variables
   // - coeff = supp - rhs
   // i.e. 2x + y + z = 2 with (sup = 4 & x = binary)
   // -> lb + x (ub -lb) = y/z
   Vec<double> coefficients{ 3.0, 1.0, 1.0, 1.0 };
   Vec<double> upperBounds{ 1.0, 1.0, 1.0, 1.0 };
   Vec<double> lowerBounds{ 0.0, 0.0, 0.0, 0.0 };
   Vec<uint8_t> isIntegral{ 1, 1, 1, 1 };

   Vec<double> rhs{ 2.0, 2.0 };
   Vec<double> lhs{ rhs[0], 3.0 };
   Vec<std::string> rowNames{ "A1", "A2" };
   Vec<std::string> columnNames{ "c1", "c2", "c3", "c4" };
   Vec<std::tuple<int, int, double>> entries{
       std::tuple<int, int, double>{ 0, 0, 2.0 },
       std::tuple<int, int, double>{ 0, 1, 1.0 },
       std::tuple<int, int, double>{ 0, 2, 1.0 },
       std::tuple<int, int, double>{ 1, 1, 2.0 } };

   ProblemBuilder<double> pb;
   pb.reserve( entries.size(), rowNames.size(), columnNames.size() );
   pb.setNumRows( rowNames.size() );
   pb.setNumCols( columnNames.size() );
   pb.setColUbAll( upperBounds );
   pb.setColLbAll( lowerBounds );
   pb.setObjAll( coefficients );
   pb.setObjOffset( 0.0 );
   pb.setColIntegralAll( isIntegral );
   pb.setRowRhsAll( rhs );
   pb.setRowLhsAll( lhs );
   pb.addEntryAll( entries );
   pb.setColNameAll( columnNames );
   pb.setProblemName( "matrix for testing simple probing" );
   Problem<double> problem = pb.build();
   problem.getConstraintMatrix().modifyLeftHandSide( 0, lhs[0] );
   return problem;
}
