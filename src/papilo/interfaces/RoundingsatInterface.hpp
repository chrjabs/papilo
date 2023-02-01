/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*               This file is part of the program and library                */
/*    PaPILO --- Parallel Presolve for Integer and Linear Optimization       */
/*                                                                           */
/* Copyright (C) 2020-2023 Konrad-Zuse-Zentrum                               */
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

#ifndef _PAPILO_INTERFACES_ROUNDINGSAT_INTERFACE_HPP_
#define _PAPILO_INTERFACES_ROUNDINGSAT_INTERFACE_HPP_

#include "Options.hpp"
#include "Stats.hpp"
#include "Solver.hpp"
#include "run.hpp"
#include "typedefs.hpp"
#include <csignal>
#include <fstream>
#include <memory>

namespace papilo
{

template <typename REAL>
class RoundingsatInterface : public SolverInterface<REAL>
{

   //   Num<REAL>& num;
   rs::CeArb objective;

   int
   doSetUp( const Problem<REAL>& problem, const Vec<int>& origRowMap,
            const Vec<int>& origColMap )
   {
      Num<REAL> num{};
      assert( objective->isReset() );
      rs::CeArb input = rs::run::solver.cePools.takeArb();

      const VariableDomains<REAL>& domains = problem.getVariableDomains();
      const Vec<REAL>& obj = problem.getObjective().coefficients;
      const Vec<REAL>& rhs = problem.getConstraintMatrix().getRightHandSides();
      const Vec<REAL>& lhs = problem.getConstraintMatrix().getLeftHandSides();
      const auto consMatrix = problem.getConstraintMatrix();

      input->reset();
      for( int col = 0; col < problem.getNCols(); ++col )
      {
         input->reset();
         assert( num.isIntegral( obj[col] ) );
         if( obj[col] == 0 )
            continue;
         // TODO round to integer and scale
         int var_index = col;
         int coeff = (int)obj[col];
         if( obj[col] < 0 )
            var_index = -col;
         rs::run::solver.setNbVars( abs( var_index ), true );
         input->addLhs( coeff, var_index );
      }
      input->copyTo( objective );

      for( int row = 0; row < problem.getNRows(); ++row )
      {
         input->reset();
         auto row_coeff =
             problem.getConstraintMatrix().getRowCoefficients( row );
         if( !consMatrix.getRowFlags()[row].test( RowFlag::kEquation ) )
         {
            map_cons_to_lhs( input, row_coeff );
            input->addRhs( (int) lhs[row] );
            if( rs::run::solver.addConstraint( input, rs::Origin::FORMULA )
                    .second == rs::ID_Unsat )
            {
               fmt::print( "An error occurred\n" );
               return -1;
            }
            input->invert();
            if( rs::run::solver.addConstraint( input, rs::Origin::FORMULA )
                    .second == rs::ID_Unsat )
            {
               fmt::print( "An error occurred\n" );
               return -1;
            }
            continue;
         }

         if( !consMatrix.getRowFlags()[row].test( RowFlag::kLhsInf ) )
         {
            map_cons_to_lhs( input, row_coeff );
            input->addRhs( (int) lhs[row] );
            if( rs::run::solver.addConstraint( input, rs::Origin::FORMULA )
                    .second == rs::ID_Unsat )
            {
               fmt::print( "An error occurred\n" );
               return -1;
            }
         }
         if( !consMatrix.getRowFlags()[row].test( RowFlag::kRhsInf ) )
         {
            map_cons_to_lhs( input, row_coeff );

            //TODO: fix rounding
            input->addRhs( (int) rhs[row] );
            input->invert();
            if( rs::run::solver.addConstraint( input, rs::Origin::FORMULA )
                    .second == rs::ID_Unsat )
            {
               fmt::print( "An error occurred\n" );
               return -1;
            }
         }
      }
      return 0;
   }

   void
   map_cons_to_lhs( rs::CeArb& input,
                    const SparseVectorView<REAL>& row_coeff ) const
   {
      Num<REAL> num{};

      for( int j = 0; j < row_coeff.getLength(); ++j )
      {
         int var_index = row_coeff.getIndices()[j];
         assert( num.isIntegral( row_coeff.getValues()[j] ) );
         // TODO round to integer and scale
         int coeff = (int)row_coeff.getValues()[j];
         if( coeff < 0 )
            var_index = -var_index;
         rs::run::solver.setNbVars( abs( var_index ), true );
         input->addLhs( coeff, var_index );
      }
   }

   int
   doSetUp( const Problem<REAL>& problem, const Vec<int>& origRowMap,
            const Vec<int>& origColMap, const Components& components,
            const ComponentInfo& component )
   {
      // TODO: implement
      return -1;
   }

 public:
   RoundingsatInterface()
   {
      // TODO:
      //      num = {};
      rs::run::solver.init();
      objective = rs::run::solver.cePools.takeArb();
   }

   void
   setUp( const Problem<REAL>& prob, const Vec<int>& row_maps,
          const Vec<int>& col_maps, const Components& components,
          const ComponentInfo& component ) override
   {
      if( doSetUp( prob, row_maps, col_maps, components, component ) != 0 )
         this->status = SolverStatus::kError;
   }

   void
   setNodeLimit( int num ) override
   {
   }

   void
   setGapLimit( const REAL& gaplim ) override
   {
   }

   void
   setSoftTimeLimit( double tlim ) override
   {
   }

   void
   setTimeLimit( double tlim ) override
   {
   }

   void
   setVerbosity( VerbosityLevel verbosity ) override
   {
   }

   void
   setUp( const Problem<REAL>& prob, const Vec<int>& row_maps,
          const Vec<int>& col_maps ) override
   {
      if( doSetUp( prob, row_maps, col_maps ) != 0 )
         this->status = SolverStatus::kError;
   }

   void
   solve() override
   {

      rs::run::solver.initLP( objective );

      rs::run::run( objective );
   }

   REAL
   getDualBound() override
   {
      return 0;
   }

   bool
   getSolution( Solution<REAL>& solbuffer ) override
   {
      return false;
   }

   bool
   getSolution( const Components& components, int component,
                Solution<REAL>& solbuffer ) override
   {

      return false;
   }

   SolverType
   getType() override
   {
      return SolverType::PseudoBoolean;
   }

   String
   getName() override
   {
      return "RoundingSat";
   }

   void
   printDetails() override
   {
   }

   bool
   is_dual_solution_available() override
   {
      return false;
   }

   void
   addParameters( ParameterSet& paramSet ) override
   {
   }

   ~RoundingsatInterface() = default;
};

template <typename REAL>
class RoundingsatFactory : public SolverFactory<REAL>
{
   RoundingsatFactory() = default;

 public:
   std::unique_ptr<SolverInterface<REAL>>
   newSolver( VerbosityLevel verbosity ) const override
   {
      auto satsolver = std::unique_ptr<SolverInterface<REAL>>(
          new RoundingsatInterface<REAL>() );

      return std::move( satsolver );
   }

   virtual void
   add_parameters( ParameterSet& parameter ) const
   {
   }

   static std::unique_ptr<SolverFactory<REAL>>
   create()
   {
      return std::unique_ptr<SolverFactory<REAL>>(
          new RoundingsatFactory<REAL>() );
   }
};

} // namespace papilo

#endif
