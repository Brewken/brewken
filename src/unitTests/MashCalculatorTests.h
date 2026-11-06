/*======================================================================================================================
 * unitTests/MashCalculatorTests.h is part of Brewken, and is copyright the following authors 2026:
 *   • Matt Young <mfsy@yahoo.com>
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
#ifndef UNITTESTS_MASHCALCULATORTESTS_H
#define UNITTESTS_MASHCALCULATORTESTS_H
#pragma once

#include "unitTests/TestClassBase.h"

/**
 * \brief Unit tests for \c MashCalculator.  Apart from \c testWithRecipe, these use \c MashCalculator::Inputs set
 *        directly, so they don't need any model objects and we can check the results against figures worked out by
 *        hand.
 *
 *        See comment in \c unitTests/TestRegistry.h for how to add new tests.
 */
class MashCalculatorTests : public UnitTests::TestClassBase {
   Q_OBJECT

public:
   MashCalculatorTests();
   virtual ~MashCalculatorTests();

private slots:
   //! \brief Strike water amount from mash thickness, and its temperature
   void testStrikeWater();

   //! \brief Fixed amounts and/or temperatures are respected, and we warn if both are fixed and don't work together
   void testFixedValues();

   //! \brief Infusions after the first one, including heating with boiling water and cooling
   void testStepInfusions();

   //! \brief Decoction amounts
   void testDecoction();

   //! \brief Batch and fly sparge amounts and temperatures, and \c MashCalculator::setSparge
   void testSparge();

   //! \brief With no sparge, all the water goes in as infusions
   void testNoSparge();

   //! \brief Things that can't be done, or that the user should check
   void testErrorsAndWarnings();

   //! \brief We don't calculate water temperatures that don't make sense (eg colder than the coldest water available)
   void testSensibleTemperatures();

   //! \brief \c MashCalculator::Inputs::fromRecipe and \c MashCalculator::apply, including applying to a copy of a mash
   void testWithRecipe();

   /**
    * \brief The example mash profiles in \c data/DefaultContent006-MashProfiles.beer import as locked, and can be
    *        calculated for a typical recipe
    */
   void testExampleProfiles();
};

#endif
