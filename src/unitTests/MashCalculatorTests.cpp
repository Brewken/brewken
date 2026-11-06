/*======================================================================================================================
 * unitTests/MashCalculatorTests.cpp is part of Brewken, and is copyright the following authors 2026:
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
#include "unitTests/MashCalculatorTests.h"

#include <QDebug>
#include <QDir>
#include <QTextStream>
#include <QtTest/QtTest>

#include "MashCalculator.h"
#include "config.h"
#include "database/ObjectStoreWrapper.h"
#include "measurement/PhysicalConstants.h"
#include "model/Boil.h"
#include "model/Equipment.h"
#include "model/Fermentable.h"
#include "model/Mash.h"
#include "model/MashStep.h"
#include "model/Recipe.h"
#include "model/RecipeAdditionFermentable.h"
#include "model/RecipeUtils.h"
#include "serialization/json/dotBeer/DotBeer.h"
#include "unitTests/TestData.h"
#include "unitTests/TestRegistry.h"

REGISTER_TEST_CLASS(MashCalculatorTests)

namespace {
   using UnitTests::fuzzyComp;
   using Severity = MashCalculator::Problem::Severity;

   double constexpr tolerance = 0.001;

   //
   // Figures used in most of the tests.  They are chosen to make the hand calculations in the comments easy:
   //    - Grain heat capacity = 5 kg × 0.4 = 2 kcal/°C, same as the tun
   //    - With a thickness of 3 L/kg, strike water = 3 × 5 + 1 = 16 L
   //    - Drained grain bed heat capacity = 2 (grain) + 5 × 1 (absorbed water) + 2 (tun) = 9 kcal/°C
   //
   MashCalculator::Inputs makeInputs() {
      return MashCalculator::Inputs{
         .grain_kg               =  5.0,
         .grainTemp_c            = 20.0,
         .tunHeatCapacity_kcalC  =  2.0,
         .tunTemp_c              = 20.0,
         .boilingPoint_c         = 100.0,
         .grainAbsorption_LKg    =  1.0,
         .strikeWaterAllowance_l =  1.0,
         .targetTotalWater_l     = 30.0,
         .mashVesselVolume_l     = 40.0,
      };
   }

   MashCalculator::Step makeStep(QString const & name, MashStep::Type const type, double const startTemp_c) {
      return MashCalculator::Step{
         .name          = name,
         .type          = type,
         .startTemp_c   = startTemp_c,
         .stepTime_mins = 30.0,
      };
   }

   MashCalculator::Options thickness(double const thickness_LKg) {
      return MashCalculator::Options{.thickness_LKg = thickness_LKg};
   }

   //! \return \c true if one of the messages of the given severity contains \c text
   bool hasMessage(MashCalculator::Result const & result, Severity const severity, QString const & text) {
      QStringList const messages = result.messages(severity);
      bool const found = std::any_of(messages.cbegin(), messages.cend(),
                                     [&text](QString const & message) { return message.contains(text); });
      if (!found) {
         qDebug() << Q_FUNC_INFO << "Did not find" << text << "in" << messages;
      }
      return found;
   }

   int numMessages(MashCalculator::Result const & result, Severity const severity) {
      return result.messages(severity).size();
   }

   /**
    * \brief Independent check of an infusion: mixing \c amount_l of water at \c waterTemp_c with things of the given
    *        heat capacities at the given temperatures should give \c expectedTemp_c.
    */
   bool infusionBalances(double const amount_l,
                         double const waterTemp_c,
                         QList<std::pair<double, double>> const & heatCapacitiesAndTemps,
                         double const expectedTemp_c) {
      double heatCapacity = amount_l * PhysicalConstants::waterSpecificHeat_calGC;
      double energy       = heatCapacity * waterTemp_c;
      for (auto const & [hc, temp] : heatCapacitiesAndTemps) {
         heatCapacity += hc;
         energy       += hc * temp;
      }
      return fuzzyComp(energy / heatCapacity, expectedTemp_c, tolerance);
   }
}

MashCalculatorTests::MashCalculatorTests() = default;

MashCalculatorTests::~MashCalculatorTests() = default;

void MashCalculatorTests::testStrikeWater() {
   QList<MashCalculator::Step> steps{makeStep("Saccharification", MashStep::Type::Infusion, 66.0)};
   MashCalculator::setSparge(steps, MashCalculator::SpargeMethod::BatchSparge, 1);
   QCOMPARE(steps.size(), 2);

   auto const result = MashCalculator::calculate(makeInputs(), steps, thickness(3.0));
   QVERIFY(result.ok());
   QCOMPARE(numMessages(result, Severity::Warning), 0);

   //
   // Strike water = 3 L/kg × 5 kg + 1 L allowance = 16 L
   // Heat needed = 2 × (66 - 20) [grain] + 2 × (66 - 20) [tun] = 184 kcal
   // Water temp = 66 + 184 / 16 = 77.5 °C
   //
   auto const & strike = result.steps.at(0);
   QVERIFY(fuzzyComp(strike.amount_l, 16.0, tolerance));
   QVERIFY(strike.infuseTemp_c);
   QVERIFY(fuzzyComp(*strike.infuseTemp_c, 77.5, tolerance));
   QVERIFY(infusionBalances(16.0, *strike.infuseTemp_c, {{2.0, 20.0}, {2.0, 20.0}}, 66.0));
   // The thickness is recorded on the step
   QVERIFY(strike.liquorToGristRatio_lKg);
   QVERIFY(fuzzyComp(*strike.liquorToGristRatio_lKg, 3.0, tolerance));

   //
   // Without a thickness option, we use the step's liquor-to-grist ratio if it has one...
   //
   steps[0].liquorToGristRatio_lKg = 2.5;
   auto const result1 = MashCalculator::calculate(makeInputs(), steps);
   QVERIFY(result1.ok());
   QVERIFY(fuzzyComp(result1.steps.at(0).amount_l, 13.5, tolerance));
   steps[0].liquorToGristRatio_lKg = std::nullopt;

   //
   // ...otherwise the thickness of the existing strike water if there is any...
   //
   steps[0].amount_l = 11.0; // = 2 L/kg × 5 kg + 1 L
   auto const result2 = MashCalculator::calculate(makeInputs(), steps);
   QVERIFY(result2.ok());
   QVERIFY(fuzzyComp(result2.steps.at(0).amount_l, 11.0, tolerance));

   // ...otherwise the default
   steps[0].amount_l = 0.0;
   auto const result3 = MashCalculator::calculate(makeInputs(), steps);
   QVERIFY(result3.ok());
   QVERIFY(fuzzyComp(result3.steps.at(0).amount_l, MashCalculator::defaultThickness_LKg * 5.0 + 1.0, tolerance));

   //
   // If the tun starts at a different temperature from the grain, that's taken into account.  Eg with the tun
   // pre-heated to 66 °C, it doesn't need any heat, so water temp = 66 + 92 / 16 = 71.75 °C
   //
   auto inputs = makeInputs();
   inputs.tunTemp_c = 66.0;
   auto const result4 = MashCalculator::calculate(inputs, steps, thickness(3.0));
   QVERIFY(result4.ok());
   QVERIFY(fuzzyComp(*result4.steps.at(0).infuseTemp_c, 71.75, tolerance));
   return;
}

void MashCalculatorTests::testFixedValues() {
   auto makeSteps = [](std::optional<double> const amount_l, std::optional<double> const waterTemp_c) {
      QList<MashCalculator::Step> steps{makeStep("Strike", MashStep::Type::Infusion, 66.0)};
      if (amount_l) {
         steps[0].amount_l    = *amount_l;
         steps[0].amountFixed = true;
      }
      if (waterTemp_c) {
         steps[0].infuseTemp_c    = *waterTemp_c;
         steps[0].infuseTempFixed = true;
      }
      MashCalculator::setSparge(steps, MashCalculator::SpargeMethod::FlySparge, 1);
      return steps;
   };

   // Amount fixed at 20 L (so thickness ignored): water temp = 66 + 184 / 20 = 75.2 °C
   {
      auto const result = MashCalculator::calculate(makeInputs(), makeSteps(20.0, std::nullopt), thickness(3.0));
      QVERIFY(result.ok());
      QCOMPARE(result.steps.at(0).amount_l, 20.0);
      QVERIFY(fuzzyComp(*result.steps.at(0).infuseTemp_c, 75.2, tolerance));
      // Sparge gets the rest of the water
      QVERIFY(fuzzyComp(result.steps.at(1).amount_l, 10.0, tolerance));
   }

   // Temp fixed at 74 °C: amount = 184 / (74 - 66) = 23 L
   {
      auto const result = MashCalculator::calculate(makeInputs(), makeSteps(std::nullopt, 74.0), thickness(3.0));
      QVERIFY(result.ok());
      QVERIFY(fuzzyComp(result.steps.at(0).amount_l, 23.0, tolerance));
      QCOMPARE(*result.steps.at(0).infuseTemp_c, 74.0);
   }

   // Both fixed and consistent: no warning
   {
      auto const result = MashCalculator::calculate(makeInputs(), makeSteps(23.0, 74.0));
      QVERIFY(result.ok());
      QCOMPARE(numMessages(result, Severity::Warning), 0);
   }

   //
   // Both fixed and inconsistent: warning, and values unchanged.  16 L at 70 °C gives
   // (16 × 70 + 2 × 20 + 2 × 20) / (16 + 2 + 2) = 60 °C
   //
   {
      auto const result = MashCalculator::calculate(makeInputs(), makeSteps(16.0, 70.0));
      QVERIFY(result.ok());
      QCOMPARE(numMessages(result, Severity::Warning), 1);
      QVERIFY(hasMessage(result, Severity::Warning, "Strike"));
      QCOMPARE(result.steps.at(0).amount_l, 16.0);
      QCOMPARE(*result.steps.at(0).infuseTemp_c, 70.0);
   }

   // Temp marked as fixed but not set
   {
      auto steps = makeSteps(std::nullopt, 74.0);
      steps[0].infuseTemp_c = std::nullopt;
      auto const result = MashCalculator::calculate(makeInputs(), steps);
      QVERIFY(!result.ok());
   }

   // Water temp fixed below the step temp can't heat the mash
   {
      auto const result = MashCalculator::calculate(makeInputs(), makeSteps(std::nullopt, 60.0));
      QVERIFY(!result.ok());
   }
   return;
}

void MashCalculatorTests::testStepInfusions() {
   QList<MashCalculator::Step> steps{
      makeStep("Protein rest"    , MashStep::Type::Infusion, 50.0),
      makeStep("Saccharification", MashStep::Type::Infusion, 66.0),
      makeStep("Mash out"        , MashStep::Type::Infusion, 72.0),
   };
   // Mash out with water at 90 °C
   steps[2].infuseTemp_c    = 90.0;
   steps[2].infuseTempFixed = true;
   MashCalculator::setSparge(steps, MashCalculator::SpargeMethod::FlySparge, 1);

   //
   // With three infusions, we need more water in total than in the other tests, otherwise there's none left for the
   // sparge, and a bigger mash tun (otherwise we get a warning that the mash won't fit).
   //
   auto inputs = makeInputs();
   inputs.targetTotalWater_l = 40.0;
   inputs.mashVesselVolume_l = 50.0;
   auto const result = MashCalculator::calculate(inputs, steps, thickness(3.0));
   QVERIFY(result.ok());
   QCOMPARE(numMessages(result, Severity::Warning), 0);

   //
   // Strike: 16 L.  Heat needed = 4 × (50 - 20) = 120 kcal, so water temp = 50 + 120 / 16 = 57.5 °C
   //
   QVERIFY(fuzzyComp(result.steps.at(0).amount_l, 16.0, tolerance));
   QVERIFY(fuzzyComp(*result.steps.at(0).infuseTemp_c, 57.5, tolerance));

   //
   // Second infusion uses boiling water.  Mash heat capacity = 2 + 16 + 2 = 20 kcal/°C, so heat needed = 20 × 16 = 320
   // kcal, so amount = 320 / (100 - 66) = 9.41176 L
   //
   double const secondAmount_l = 320.0 / 34.0;
   QVERIFY(fuzzyComp(result.steps.at(1).amount_l, secondAmount_l, tolerance));
   QCOMPARE(*result.steps.at(1).infuseTemp_c, 100.0);
   QVERIFY(infusionBalances(result.steps.at(1).amount_l, 100.0, {{20.0, 50.0}}, 66.0));

   //
   // Third infusion has fixed water temp of 90 °C.  Mash heat capacity = 2 + 2 + 16 + 9.41176 = 29.41176 kcal/°C
   //
   double const mashHeatCapacity = 4.0 + 16.0 + secondAmount_l;
   double const thirdAmount_l = mashHeatCapacity * (72.0 - 66.0) / (90.0 - 72.0);
   QVERIFY(fuzzyComp(result.steps.at(2).amount_l, thirdAmount_l, tolerance));
   QVERIFY(infusionBalances(thirdAmount_l, 90.0, {{mashHeatCapacity, 66.0}}, 72.0));

   // Fly sparge gets the rest
   QVERIFY(fuzzyComp(result.steps.at(3).amount_l, 40.0 - 16.0 - secondAmount_l - thirdAmount_l, tolerance));

   //
   // If a step has an end temperature, the next step starts from that.  Here, the protein rest drops to 48 °C, so the
   // second infusion has to provide 20 × 18 = 360 kcal.
   //
   steps[0].endTemp_c = 48.0;
   auto const result2 = MashCalculator::calculate(makeInputs(), steps, thickness(3.0));
   QVERIFY(result2.ok());
   QVERIFY(fuzzyComp(result2.steps.at(1).amount_l, 360.0 / 34.0, tolerance));

   //
   // An infusion can also cool the mash, in which case we use water at the grain temperature (20 °C).  Going from
   // 50 °C to 45 °C needs 20 × 5 = 100 kcal taken out, so amount = 100 / (45 - 20) = 4 L
   //
   QList<MashCalculator::Step> coolingSteps{
      makeStep("Strike" , MashStep::Type::Infusion, 50.0),
      makeStep("Cooling", MashStep::Type::Infusion, 45.0),
   };
   MashCalculator::setSparge(coolingSteps, MashCalculator::SpargeMethod::FlySparge, 1);
   auto const result3 = MashCalculator::calculate(makeInputs(), coolingSteps, thickness(3.0));
   QVERIFY(result3.ok());
   QVERIFY(fuzzyComp(result3.steps.at(1).amount_l, 4.0, tolerance));
   QCOMPARE(*result3.steps.at(1).infuseTemp_c, 20.0);

   //
   // A temperature step (ie direct heat) doesn't need any calculation, but the following infusion starts from its
   // temperature
   //
   QList<MashCalculator::Step> directHeatSteps{
      makeStep("Strike"     , MashStep::Type::Infusion   , 50.0),
      makeStep("Direct heat", MashStep::Type::Temperature, 60.0),
      makeStep("Infusion"   , MashStep::Type::Infusion   , 66.0),
   };
   MashCalculator::setSparge(directHeatSteps, MashCalculator::SpargeMethod::FlySparge, 1);
   auto const result4 = MashCalculator::calculate(makeInputs(), directHeatSteps, thickness(3.0));
   QVERIFY(result4.ok());
   QCOMPARE(result4.steps.at(1).amount_l, 0.0);
   QVERIFY(fuzzyComp(result4.steps.at(2).amount_l, 20.0 * 6.0 / 34.0, tolerance));
   return;
}

void MashCalculatorTests::testDecoction() {
   QList<MashCalculator::Step> steps{
      makeStep("Protein rest"    , MashStep::Type::Infusion , 50.0),
      makeStep("Saccharification", MashStep::Type::Decoction, 66.0),
   };
   MashCalculator::setSparge(steps, MashCalculator::SpargeMethod::FlySparge, 1);

   auto const result = MashCalculator::calculate(makeInputs(), steps, thickness(3.0));
   QVERIFY(result.ok());

   //
   // After the strike, water and grain heat capacity = 16 + 2 = 18 kcal/°C, total (with tun) = 20 kcal/°C.
   // Proportion of mash to decoct = 20 × (66 - 50) / (18 × (100 - 50)) = 0.35556
   // Mash volume = 16 L water + 5 kg / 0.963 kg/L grain = 21.19211 L
   //
   double const ratio = 20.0 * 16.0 / (18.0 * 50.0);
   double const mashVolume_l = 16.0 + 5.0 / PhysicalConstants::grainDensity_kgL;
   QVERIFY(fuzzyComp(result.steps.at(1).amount_l, ratio * mashVolume_l, tolerance));

   // Check by heat balance: decoction at 100 °C mixed back into the rest of the mash (and tun) at 50 °C
   QVERIFY(infusionBalances(0.0, 0.0, {{ratio * 18.0, 100.0}, {(1.0 - ratio) * 18.0 + 2.0, 50.0}}, 66.0));

   // A decoction doesn't add water, so the sparge gets 30 - 16 = 14 L
   QVERIFY(fuzzyComp(result.steps.at(2).amount_l, 14.0, tolerance));

   //
   // Fixed decoction amount that is too small gives a warning
   //
   steps[1].amount_l    = 5.0;
   steps[1].amountFixed = true;
   auto const result2 = MashCalculator::calculate(makeInputs(), steps, thickness(3.0));
   QVERIFY(result2.ok());
   QCOMPARE(numMessages(result2, Severity::Warning), 1);
   QCOMPARE(result2.steps.at(1).amount_l, 5.0);

   // More than the whole mash is an error
   steps[1].amount_l = 50.0;
   QVERIFY(!MashCalculator::calculate(makeInputs(), steps, thickness(3.0)).ok());

   // A decoction can't cool the mash
   steps[1].amountFixed = false;
   steps[1].startTemp_c = 45.0;
   QVERIFY(!MashCalculator::calculate(makeInputs(), steps, thickness(3.0)).ok());

   // A decoction can't be the first step
   QList<MashCalculator::Step> badSteps{makeStep("Decoction", MashStep::Type::Decoction, 66.0)};
   QVERIFY(!MashCalculator::calculate(makeInputs(), badSteps, thickness(3.0)).ok());
   return;
}

void MashCalculatorTests::testSparge() {
   QList<MashCalculator::Step> steps{makeStep("Saccharification", MashStep::Type::Infusion, 66.0)};

   //
   // setSparge
   //
   QCOMPARE(MashCalculator::spargeMethod(steps), MashCalculator::SpargeMethod::NoSparge);
   MashCalculator::setSparge(steps, MashCalculator::SpargeMethod::BatchSparge, 2, 76.0);
   QCOMPARE(steps.size(), 3);
   QCOMPARE(MashCalculator::spargeMethod(steps), MashCalculator::SpargeMethod::BatchSparge);
   QCOMPARE(steps.at(1).type, MashStep::Type::BatchSparge);
   QCOMPARE(steps.at(2).type, MashStep::Type::BatchSparge);
   QCOMPARE(steps.at(1).startTemp_c, 76.0);
   QCOMPARE(steps.at(1).sourceIndex, -1);

   // Asking for what's already there leaves the steps alone (so fixed values are kept)
   steps[2].amountFixed = true;
   steps[2].amount_l    = 8.0;
   MashCalculator::setSparge(steps, MashCalculator::SpargeMethod::BatchSparge, 2, 70.0);
   QCOMPARE(steps.size(), 3);
   QVERIFY(steps.at(2).amountFixed);
   QCOMPARE(steps.at(1).startTemp_c, 76.0);

   //
   // Batch sparge amounts and temperatures.  Total sparge water is 30 - 16 = 14 L, of which 8 L is fixed for the
   // second batch, leaving 6 L for the first.
   //
   // Drained grain bed heat capacity is 9 kcal/°C.  With 5 °C heat loss:
   //    First batch: bed at 66 - 5 = 61 °C.  Water temp = 76 + 9 × (76 - 61) / 6 = 98.5 °C
   //    Second batch: bed at 76 - 5 = 71 °C.  Water temp = 76 + 9 × (76 - 71) / 8 = 81.625 °C
   //
   MashCalculator::Options options = thickness(3.0);
   options.spargeHeatLoss_c = 5.0;
   auto const result = MashCalculator::calculate(makeInputs(), steps, options);
   QVERIFY(result.ok());
   QCOMPARE(numMessages(result, Severity::Warning), 0);
   QVERIFY(fuzzyComp(result.steps.at(1).amount_l, 6.0, tolerance));
   QVERIFY(fuzzyComp(*result.steps.at(1).infuseTemp_c, 98.5, tolerance));
   QCOMPARE(result.steps.at(2).amount_l, 8.0);
   QVERIFY(fuzzyComp(*result.steps.at(2).infuseTemp_c, 81.625, tolerance));
   QVERIFY(infusionBalances(6.0, 98.5, {{9.0, 61.0}}, 76.0));

   //
   // With the default 2 °C heat loss, two equal batches of 7 L:
   //    First batch: bed at 66 - 2 = 64 °C.  Water temp = 76 + 9 × 12 / 7 = 91.43 °C
   //    Second batch: bed at 76 - 2 = 74 °C.  Water temp = 76 + 9 × 2 / 7 = 78.57 °C
   //
   steps[2].amountFixed = false;
   auto const result2 = MashCalculator::calculate(makeInputs(), steps, thickness(3.0));
   QVERIFY(result2.ok());
   QCOMPARE(numMessages(result2, Severity::Warning), 0);
   QVERIFY(fuzzyComp(result2.steps.at(1).amount_l, 7.0, tolerance));
   QVERIFY(fuzzyComp(result2.steps.at(2).amount_l, 7.0, tolerance));
   QVERIFY(fuzzyComp(*result2.steps.at(1).infuseTemp_c, 76.0 + 9.0 * 12.0 / 7.0, tolerance));
   QVERIFY(fuzzyComp(*result2.steps.at(2).infuseTemp_c, 76.0 + 9.0 *  2.0 / 7.0, tolerance));

   //
   // With a 10 °C heat loss (as the old Mash Wizard assumed), the first batch would need water at
   // 76 + 9 × 20 / 7 = 101.7 °C.  So we use boiling water, and warn that the bed only gets to
   // (7 × 100 + 9 × 56) / (7 + 9) = 75.25 °C.  The second batch then starts from that (less the heat loss), so needs
   // water at 76 + 9 × (76 - 65.25) / 7 = 89.82 °C.
   //
   options.spargeHeatLoss_c = 10.0;
   auto const result2a = MashCalculator::calculate(makeInputs(), steps, options);
   QVERIFY(result2a.ok());
   QCOMPARE(numMessages(result2a, Severity::Warning), 1);
   QVERIFY(hasMessage(result2a, Severity::Warning, "Batch Sparge 1"));
   QCOMPARE(*result2a.steps.at(1).infuseTemp_c, 100.0);
   QVERIFY(fuzzyComp(*result2a.steps.at(2).infuseTemp_c, 76.0 + 9.0 * (76.0 - 65.25) / 7.0, tolerance));

   //
   // Changing to fly sparge replaces the batch sparges with one fly sparge step, which gets all the sparge water, at
   // the target grain bed temperature
   //
   MashCalculator::setSparge(steps, MashCalculator::SpargeMethod::FlySparge, 1, 77.0);
   QCOMPARE(steps.size(), 2);
   QCOMPARE(MashCalculator::spargeMethod(steps), MashCalculator::SpargeMethod::FlySparge);
   auto const result3 = MashCalculator::calculate(makeInputs(), steps, thickness(3.0));
   QVERIFY(result3.ok());
   QVERIFY(fuzzyComp(result3.steps.at(1).amount_l, 14.0, tolerance));
   QCOMPARE(*result3.steps.at(1).infuseTemp_c, 77.0);

   // And to no sparge removes it
   MashCalculator::setSparge(steps, MashCalculator::SpargeMethod::NoSparge, 1);
   QCOMPARE(steps.size(), 1);

   // Sparge steps must come at the end
   QList<MashCalculator::Step> badSteps{
      makeStep("Strike"  , MashStep::Type::Infusion  , 66.0),
      makeStep("Sparge"  , MashStep::Type::FlySparge , 76.0),
      makeStep("Mash out", MashStep::Type::Infusion  , 76.0),
   };
   QVERIFY(!MashCalculator::calculate(makeInputs(), badSteps, thickness(3.0)).ok());
   return;
}

void MashCalculatorTests::testNoSparge() {
   //
   // Single infusion: all 30 L of water goes in as strike water, regardless of thickness.  Water temp = 66 + 184 / 30
   //
   QList<MashCalculator::Step> steps{makeStep("Saccharification", MashStep::Type::Infusion, 66.0)};
   auto const result = MashCalculator::calculate(makeInputs(), steps, thickness(3.0));
   QVERIFY(result.ok());
   QVERIFY(fuzzyComp(result.steps.at(0).amount_l, 30.0, tolerance));
   QVERIFY(fuzzyComp(*result.steps.at(0).infuseTemp_c, 66.0 + 184.0 / 30.0, tolerance));

   //
   // Two infusions: the strike water comes from the thickness (16 L) and the second infusion makes up the rest (14 L).
   // Its temperature = 66 + 20 × (66 - 50) / 14
   //
   QList<MashCalculator::Step> twoSteps{
      makeStep("Protein rest"    , MashStep::Type::Infusion, 50.0),
      makeStep("Saccharification", MashStep::Type::Infusion, 66.0),
   };
   auto const result2 = MashCalculator::calculate(makeInputs(), twoSteps, thickness(3.0));
   QVERIFY(result2.ok());
   QVERIFY(fuzzyComp(result2.steps.at(0).amount_l, 16.0, tolerance));
   QVERIFY(fuzzyComp(result2.steps.at(1).amount_l, 14.0, tolerance));
   QVERIFY(fuzzyComp(*result2.steps.at(1).infuseTemp_c, 66.0 + 20.0 * 16.0 / 14.0, tolerance));

   //
   // If the second infusion's amount is fixed, the strike water makes up the rest
   //
   twoSteps[1].amount_l    = 12.0;
   twoSteps[1].amountFixed = true;
   auto const result3 = MashCalculator::calculate(makeInputs(), twoSteps, thickness(3.0));
   QVERIFY(result3.ok());
   QVERIFY(fuzzyComp(result3.steps.at(0).amount_l, 18.0, tolerance));

   // If that leaves nothing for the strike water, it's an error
   twoSteps[1].amount_l = 31.0;
   QVERIFY(!MashCalculator::calculate(makeInputs(), twoSteps, thickness(3.0)).ok());

   // If all the amounts are fixed and don't add up to the total, it's a warning
   twoSteps[0].amount_l    = 16.0;
   twoSteps[0].amountFixed = true;
   twoSteps[1].amount_l    = 12.0;
   auto const result4 = MashCalculator::calculate(makeInputs(), twoSteps, thickness(3.0));
   QVERIFY(result4.ok());
   QCOMPARE(numMessages(result4, Severity::Warning), 1);
   return;
}

void MashCalculatorTests::testErrorsAndWarnings() {
   QList<MashCalculator::Step> steps{makeStep("Saccharification", MashStep::Type::Infusion, 66.0)};
   MashCalculator::setSparge(steps, MashCalculator::SpargeMethod::FlySparge, 1);

   // No steps
   QVERIFY(!MashCalculator::calculate(makeInputs(), {}).ok());

   // No grain
   auto noGrain = makeInputs();
   noGrain.grain_kg = 0.0;
   QVERIFY(!MashCalculator::calculate(noGrain, steps).ok());

   // Bad thickness
   QVERIFY(!MashCalculator::calculate(makeInputs(), steps, thickness(0.0)).ok());

   //
   // Mash too thick: 0.8 L/kg gives 5 L, needing water at 66 + 184 / 5 = 102.8 °C.  Nothing should be changed.
   //
   auto const tooThick = MashCalculator::calculate(makeInputs(), steps, thickness(0.8));
   QVERIFY(!tooThick.ok());
   QVERIFY(hasMessage(tooThick, Severity::Error, "Saccharification"));
   QVERIFY(hasMessage(tooThick, Severity::Error, "thinner"));

   //
   // Lower boiling point (eg at altitude) makes a difference: 1.2 L/kg gives 7 L, needing water at 66 + 184 / 7 =
   // 92.3 °C, which is OK at sea level but not if water boils at 90 °C
   //
   QVERIFY(MashCalculator::calculate(makeInputs(), steps, thickness(1.2)).ok());
   auto highAltitude = makeInputs();
   highAltitude.boilingPoint_c = 90.0;
   QVERIFY(!MashCalculator::calculate(highAltitude, steps, thickness(1.2)).ok());

   //
   // Mash too thin: 6 L/kg gives 31 L of strike water, more than the 30 L needed in total.  This is a warning (as the
   // user might be happy to have extra wort), and the sparge gets no water.
   //
   auto const tooThin = MashCalculator::calculate(makeInputs(), steps, thickness(6.0));
   QVERIFY(tooThin.ok());
   QVERIFY(hasMessage(tooThin, Severity::Warning, "no water left for sparging"));
   QCOMPARE(tooThin.steps.at(1).amount_l, 0.0);

   //
   // Mash doesn't fit in the tun: 16 L water + 5.19 L grain is more than 20 L
   //
   auto smallTun = makeInputs();
   smallTun.mashVesselVolume_l = 20.0;
   auto const overflow = MashCalculator::calculate(smallTun, steps, thickness(3.0));
   QVERIFY(overflow.ok());
   QVERIFY(hasMessage(overflow, Severity::Warning, "capacity of the mash tun"));
   // But we don't check if we don't know the tun size
   smallTun.mashVesselVolume_l = std::nullopt;
   QCOMPARE(numMessages(MashCalculator::calculate(smallTun, steps, thickness(3.0)), Severity::Warning), 0);

   // An infusion that doesn't change the temperature can't have its amount calculated
   QList<MashCalculator::Step> sameTempSteps{
      makeStep("Strike"     , MashStep::Type::Infusion, 66.0),
      makeStep("More water" , MashStep::Type::Infusion, 66.0),
   };
   MashCalculator::setSparge(sameTempSteps, MashCalculator::SpargeMethod::FlySparge, 1);
   QVERIFY(!MashCalculator::calculate(makeInputs(), sameTempSteps, thickness(3.0)).ok());
   // But it's fine if its amount is fixed
   sameTempSteps[1].amount_l    = 2.0;
   sameTempSteps[1].amountFixed = true;
   auto const sameTemp = MashCalculator::calculate(makeInputs(), sameTempSteps, thickness(3.0));
   QVERIFY(sameTemp.ok());
   QVERIFY(fuzzyComp(*sameTemp.steps.at(1).infuseTemp_c, 66.0, tolerance));
   return;
}

void MashCalculatorTests::testWithRecipe() {
   double const grain_kg      = 5.0;
   double const thickness_LKg = 3.0;

   auto twoRow = UnitTests::makeFermentable_TwoRow();
   ObjectStoreWrapper::insert(twoRow);

   auto equipment = std::make_shared<Equipment>("Mash calculator test equipment");
   equipment->setKettleBoilSize_l(30.0);
   equipment->setFvBatchSize_l(23.0);
   equipment->setMashTunVolume_l(40.0);
   equipment->setMashTunWeight_kg(5.0);
   equipment->setMashTunSpecificHeat_calGC(0.4);
   equipment->setGrainAbsorption_LKg(1.0);
   equipment->setFoundationVolume_l(0.0);
   equipment->setMashTunLoss_l(1.0);
   equipment->setLauterTunDeadspaceLoss_l(0.0);
   equipment->setTopUpKettle_l(0.0);
   equipment->setTopUpWater_l(0.0);
   equipment->setBoilingPoint_c(99.0);
   ObjectStoreWrapper::insert(equipment);

   auto recipe = std::make_shared<Recipe>("Mash calculator test recipe");
   ObjectStoreWrapper::insert(recipe);
   recipe->setBatchSize_l(23.0);
   recipe->setEquipment(equipment);
   auto boil = recipe->nonOptBoil();
   boil->setPreBoilSize_l(30.0);
   boil->setBoilTime_mins(60.0);
   auto grain = std::make_shared<RecipeAdditionFermentable>("Mash calculator test grain");
   grain->setFermentable(twoRow.get());
   grain->setStage(RecipeAddition::Stage::Mash);
   grain->setQuantity(grain_kg);
   grain->setMeasure(Measurement::PhysicalQuantity::Mass);
   recipe->addAddition(grain);

   //
   // Mash with a strike and two batch sparges, with made-up amounts
   //
   auto mash = std::make_shared<Mash>("Mash calculator test mash");
   mash->setGrainTemp_c(18.0);
   mash->setSpargeTemp_c(75.0);
   ObjectStoreWrapper::insert(mash);
   auto strike = std::make_shared<MashStep>("Strike");
   strike->setType(MashStep::Type::Infusion);
   strike->setStartTemp_c(66.0);
   strike->setStepTime_mins(60.0);
   strike->setAmount_l(1.0);
   mash->add(strike);
   for (int ii = 1; ii <= 2; ++ii) {
      auto sparge = std::make_shared<MashStep>(QString("Sparge %1").arg(ii));
      sparge->setType(MashStep::Type::BatchSparge);
      sparge->setStartTemp_c(75.0);
      sparge->setStepTime_mins(15.0);
      sparge->setAmount_l(1.0);
      mash->add(sparge);
   }
   recipe->setMash(mash);

   //
   // Inputs come from the recipe, equipment and mash
   //
   auto const inputs = MashCalculator::Inputs::fromRecipe(*recipe);
   QVERIFY(fuzzyComp(inputs.grain_kg              , recipe->grainsInMash_kg()          , tolerance));
   QVERIFY(fuzzyComp(inputs.grainTemp_c           , 18.0                               , tolerance));
   QVERIFY(fuzzyComp(inputs.tunHeatCapacity_kcalC , 2.0                                , tolerance));
   QVERIFY(fuzzyComp(inputs.tunTemp_c             , 18.0                               , tolerance));
   QVERIFY(fuzzyComp(inputs.boilingPoint_c        , 99.0                               , tolerance));
   QVERIFY(fuzzyComp(inputs.grainAbsorption_LKg   , 1.0                                , tolerance));
   QVERIFY(fuzzyComp(inputs.strikeWaterAllowance_l, equipment->strikeWaterAllowance_l(), tolerance));
   QVERIFY(fuzzyComp(inputs.targetTotalWater_l    , recipe->targetTotalMashVol_l()     , tolerance));
   QVERIFY(inputs.mashVesselVolume_l);

   auto steps = MashCalculator::stepsFrom(*mash);
   QCOMPARE(steps.size(), 3);
   QCOMPARE(MashCalculator::spargeMethod(steps), MashCalculator::SpargeMethod::BatchSparge);
   QCOMPARE(steps.at(2).sourceIndex, 2);

   //
   // Switch to fly sparge and calculate
   //
   MashCalculator::setSparge(steps, MashCalculator::SpargeMethod::FlySparge, 1, *mash->spargeTemp_c());
   auto const result = MashCalculator::calculate(inputs, steps, MashCalculator::Options{.thickness_LKg = thickness_LKg});
   QVERIFY(result.ok());

   //
   // Apply the results to a copy of the mash for this recipe (as the Mash Wizard does when the mash is shared or
   // locked), and check the original is unchanged
   //
   mash->setLocked(true);
   std::shared_ptr<Mash> copy = RecipeUtils::useCopyFor<Mash>(*recipe);
   QVERIFY(copy);
   QVERIFY(copy.get() != mash.get());
   QCOMPARE(recipe->mash().get(), copy.get());
   MashCalculator::apply(result.steps, *copy);

   QCOMPARE(mash->mashSteps().size(), 3);
   QCOMPARE(mash->mashSteps().at(0)->amount_l(), 1.0);

   auto const copySteps = copy->mashSteps();
   QCOMPARE(copySteps.size(), 2);
   QCOMPARE(copySteps.at(0)->name(), QString("Strike"));
   QCOMPARE(copySteps.at(1)->type(), MashStep::Type::FlySparge);

   double const strike_l = recipe->targetStrikeWater_l(thickness_LKg);
   QVERIFY(fuzzyComp(copySteps.at(0)->amount_l(), strike_l, tolerance));
   QVERIFY(copySteps.at(0)->infuseTemp_c());
   QVERIFY(infusionBalances(strike_l, *copySteps.at(0)->infuseTemp_c(), {{inputs.grain_kg * PhysicalConstants::grainSpecificHeat_calGC, 18.0}, {2.0, 18.0}}, 66.0));
   QVERIFY(fuzzyComp(copySteps.at(1)->amount_l(), recipe->targetTotalMashVol_l() - strike_l, tolerance));
   QVERIFY(fuzzyComp(copy->totalMashWater_l(), recipe->targetTotalMashVol_l(), tolerance));
   QCOMPARE(*copySteps.at(1)->infuseTemp_c(), 75.0);
   QCOMPARE(copySteps.at(1)->startTemp_c(), 75.0);
   return;
}

void MashCalculatorTests::testExampleProfiles() {
   QString const filePath{QDir{CONFIG_TEST_DATA_DIR}.filePath("../data/DefaultContent006-MashProfiles.beer")};
   QString userMessage;
   QTextStream userMessageAsStream{&userMessage};
   bool const imported = DotBeer::import("", filePath, userMessageAsStream);
   userMessageAsStream.flush();
   QVERIFY2(imported, qPrintable(userMessage));

   QStringList const names{
      "Single Infusion, Light Body, Batch Sparge",
      "Single Infusion, Medium Body, Batch Sparge",
      "Single Infusion, Full Body, Batch Sparge",
      "Single Infusion, Medium Body, Fly Sparge",
      "Single Infusion, Medium Body, No Sparge",
      "Step Mash with Protein Rest, Fly Sparge",
      "Step Mash, Direct Heat, No Sparge",
      "Double Decoction, Fly Sparge",
   };
   for (auto const & name : names) {
      qDebug() << Q_FUNC_INFO << "Checking" << name;
      auto const mash = UnitTests::findByName<Mash>(name);
      QVERIFY2(mash, qPrintable(name));
      QVERIFY2(mash->locked(), qPrintable(name));
      QVERIFY2(mash->mashSteps().size() >= 2, qPrintable(name));

      //
      // The profiles don't have water amounts -- they need to be calculated for a recipe.  Here we check that works
      // without errors or warnings for a typical recipe.  (We use the defaults for the thickness, so that any thickness
      // stored in the profile is used.)
      //
      auto const result = MashCalculator::calculate(makeInputs(), MashCalculator::stepsFrom(*mash));
      QVERIFY2(result.ok(), qPrintable(name + ": " + result.messages(Severity::Error).join("; ")));
      QVERIFY2(numMessages(result, Severity::Warning) == 0,
               qPrintable(name + ": " + result.messages(Severity::Warning).join("; ")));
      // All the water is accounted for
      double totalWater_l = 0.0;
      for (auto const & step : result.steps) {
         if (step.isInfusion() || step.isSparge()) {
            totalWater_l += step.amount_l;
         }
      }
      QVERIFY2(fuzzyComp(totalWater_l, makeInputs().targetTotalWater_l, tolerance), qPrintable(name));
   }
   return;
}
