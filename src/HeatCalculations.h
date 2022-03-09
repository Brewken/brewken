/*======================================================================================================================
 * HeatCalculations.h is part of Brewken, and is copyright the following authors 2009-2022:
 *   • Matt Young <mfsy@yahoo.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *
 * Brewken is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Brewken is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 =====================================================================================================================*/
#ifndef HEATCALCULATIONS_H
#define HEATCALCULATIONS_H
#pragma once

/*!
 * \brief Algorithms and constants related to the thermodynamics of beer.
 */
namespace HeatCalculations {

   double equivalentMCProduct(double m1, double c1, double m2, double c2);
   // Water temp when mass 1 is initially at T1 and is to be brought to Tf by
   // water. MCw = (mass of water)*(water sp. heat). MC1 = (mass 1)*(sp. heat 1).
   double requiredWaterTemp( double MCw, double MC1, double Tf, double T1 );

   /***Specific heats***/
   // Water's specific heat.
   extern double const Cw_JKgK;
   extern double const Cw_calGC;
   extern double const Cgrain_calGC;
}

#endif
