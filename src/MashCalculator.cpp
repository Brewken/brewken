/*======================================================================================================================
 * MashCalculator.cpp is part of Brewken, and is copyright the following authors 2026:
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
#include "MashCalculator.h"

#include <algorithm>
#include <cmath>

#include <QDebug>
#include <QSet>

#include "measurement/Measurement.h"
#include "measurement/PhysicalConstants.h"
#include "model/Equipment.h"
#include "model/Mash.h"
#include "model/MashStep.h"
#include "model/Recipe.h"

namespace {
   double constexpr cW = PhysicalConstants::waterSpecificHeat_calGC;
   double constexpr cG = PhysicalConstants::grainSpecificHeat_calGC;

   //! For messages to the user
   QString showTemp(double const temp_c) {
      return Measurement::displayAmount(Measurement::Amount{temp_c, Measurement::Units::celsius});
   }
   QString showVolume(double const volume_l) {
      return Measurement::displayAmount(Measurement::Amount{volume_l, Measurement::Units::liters});
   }

   /**
    * \brief What the water added at a step is mixed with: one or more things, each with a heat capacity and a
    *        temperature.  For the strike water, this is the grain and the tun (which can be at different temperatures).
    *        For later steps, it is the whole mash (grain, water and tun).  For batch sparges, it is the drained grain
    *        bed.
    *
    *        If we add amount A of water at temperature T_w, then, by conservation of energy, the final temperature T
    *        satisfies:
    *
    *           A × c_w × (T_w - T) = Σ HC_k × (T - T_k)
    *
    *        where HC_k and T_k are the heat capacity and initial temperature of each thing in the mix.  Given any two of
    *        A, T_w and T, we can work out the third.
    */
   struct Contents {
      struct Part {
         double heatCapacity_kcalC;
         double temp_c;
      };
      QList<Part> parts;

      double heatCapacity_kcalC() const {
         double total = 0.0;
         for (auto const & part : this->parts) {
            total += part.heatCapacity_kcalC;
         }
         return total;
      }

      //! Heat (in kcal) needed to bring the contents to the supplied temperature (negative if they need cooling)
      double heatNeeded_kcal(double const temp_c) const {
         double total = 0.0;
         for (auto const & part : this->parts) {
            total += part.heatCapacity_kcalC * (temp_c - part.temp_c);
         }
         return total;
      }

      //! Temperature we end up at if we add \c amount_l of water at \c waterTemp_c
      double finalTemp_c(double const amount_l, double const waterTemp_c) const {
         double heatCapacity = amount_l * cW;
         double energy       = amount_l * cW * waterTemp_c;
         for (auto const & part : this->parts) {
            heatCapacity += part.heatCapacity_kcalC;
            energy       += part.heatCapacity_kcalC * part.temp_c;
         }
         return heatCapacity > 0.0 ? energy / heatCapacity : waterTemp_c;
      }

      //! Water temperature needed to reach \c temp_c if we add \c amount_l of water.  Caller must ensure amount_l > 0.
      double waterTempFor(double const temp_c, double const amount_l) const {
         return temp_c + this->heatNeeded_kcal(temp_c) / (amount_l * cW);
      }

      /**
       * \brief Amount of water needed to reach \c temp_c if we add water at \c waterTemp_c, or \c std::nullopt if it
       *        can't be done (eg water is not hotter than the target temperature but we need to heat the mash).
       */
      std::optional<double> amountFor(double const temp_c, double const waterTemp_c) const {
         double const heatNeeded_kcal = this->heatNeeded_kcal(temp_c);
         double const waterTempDiff_c = waterTemp_c - temp_c;
         if (std::abs(heatNeeded_kcal) < 1e-9) {
            return 0.0;
         }
         if (std::abs(waterTempDiff_c) < 1e-9) {
            return std::nullopt;
         }
         double const amount_l = heatNeeded_kcal / (cW * waterTempDiff_c);
         if (amount_l < 0.0) {
            return std::nullopt;
         }
         return amount_l;
      }
   };

}

MashCalculator::Inputs MashCalculator::Inputs::fromRecipe(Recipe const & recipe, Mash const & mash) {
   Inputs inputs = fromRecipe(recipe);
   //
   // This mirrors Recipe::mashTunHeatCapacity_kcalC and Recipe::mashTunTemp_c, but for the supplied mash
   //
   inputs.grainTemp_c = mash.grainTemp_c();
   inputs.tunTemp_c   = mash.initialTunTemp_c().value_or(mash.grainTemp_c());
   auto const equipment = recipe.equipment();
   std::optional<double> const equipmentHeatCapacity_kcalC =
      equipment ? equipment->mashVesselHeatCapacity_kcalC() : std::nullopt;
   inputs.tunHeatCapacity_kcalC = equipmentHeatCapacity_kcalC.value_or(
      // kg × cal/(g·°C) = kcal/°C
      mash.tunWeight_kg().value_or(0.0) * mash.tunSpecificHeat_calGC().value_or(0.0)
   );
   return inputs;
}

MashCalculator::Inputs MashCalculator::Inputs::fromRecipe(Recipe const & recipe) {
   Inputs inputs;
   auto const equipment = recipe.equipment();
   auto const mash      = recipe.mash();
   inputs.grain_kg              = recipe.grainsInMash_kg();
   inputs.grainTemp_c           = mash ? mash->grainTemp_c() : 20.0;
   inputs.tunHeatCapacity_kcalC = recipe.mashTunHeatCapacity_kcalC();
   inputs.tunTemp_c             = recipe.mashTunTemp_c();
   inputs.boilingPoint_c        = equipment ? equipment->boilingPoint_c() : 100.0;
   inputs.grainAbsorption_LKg   =
      equipment ? equipment->grainAbsorption_LKg().value_or(Equipment::default_grainAbsorption_LKg) :
                  PhysicalConstants::grainAbsorption_Lkg;
   inputs.strikeWaterAllowance_l = equipment ? equipment->strikeWaterAllowance_l() : 0.0;
   inputs.targetTotalWater_l     = recipe.targetTotalMashVol_l();
   if (equipment && equipment->mashVesselVolume_l() > 0.0) {
      inputs.mashVesselVolume_l = equipment->mashVesselVolume_l();
   }
   return inputs;
}

bool MashCalculator::Step::isInfusion() const {
   return this->type == MashStep::Type::Infusion;
}

bool MashCalculator::Step::isSparge() const {
   return this->type == MashStep::Type::BatchSparge || this->type == MashStep::Type::FlySparge;
}

double MashCalculator::Step::finalTemp_c() const {
   return this->endTemp_c.value_or(this->startTemp_c);
}

bool MashCalculator::Result::ok() const {
   return std::none_of(
      this->problems.cbegin(),
      this->problems.cend(),
      [](Problem const & problem) { return problem.severity == Problem::Severity::Error; }
   );
}

QStringList MashCalculator::Result::messages(Problem::Severity const severity) const {
   QStringList messages;
   for (auto const & problem : this->problems) {
      if (problem.severity != severity) {
         continue;
      }
      if (problem.stepIndex < 0 || problem.stepIndex >= this->steps.size()) {
         messages.append(problem.message);
      } else {
         QString const & stepName = this->steps.at(problem.stepIndex).name;
         messages.append(
            tr("%1: %2").arg(stepName.isEmpty() ? tr("Step %1").arg(problem.stepIndex + 1) : stepName, problem.message)
         );
      }
   }
   return messages;
}

QList<MashCalculator::Step> MashCalculator::stepsFrom(Mash const & mash) {
   QList<Step> steps;
   int index = 0;
   for (auto const & mashStep : mash.mashSteps()) {
      steps.append(Step{
         .sourceIndex     = index++,
         .name            = mashStep->name(),
         .type            = mashStep->type(),
         .startTemp_c     = mashStep->startTemp_c(),
         .endTemp_c       = mashStep->endTemp_c(),
         .stepTime_mins   = mashStep->stepTime_mins(),
         .amount_l        = mashStep->amount_l(),
         .infuseTemp_c    = mashStep->infuseTemp_c(),
         .amountFixed     = mashStep->amountFixed(),
         .infuseTempFixed = mashStep->infuseTempFixed(),
         .liquorToGristRatio_lKg = mashStep->liquorToGristRatio_lKg(),
      });
   }
   return steps;
}

MashCalculator::SpargeMethod MashCalculator::spargeMethod(QList<Step> const & steps) {
   bool hasFlySparge = false;
   for (auto const & step : steps) {
      if (step.type == MashStep::Type::BatchSparge) {
         return SpargeMethod::BatchSparge;
      }
      if (step.type == MashStep::Type::FlySparge) {
         hasFlySparge = true;
      }
   }
   return hasFlySparge ? SpargeMethod::FlySparge : SpargeMethod::NoSparge;
}

void MashCalculator::setSparge(QList<Step> & steps,
                               SpargeMethod const method,
                               int const numBatches,
                               double const spargeTemp_c) {
   int const numBatchSparges = std::count_if(
      steps.cbegin(), steps.cend(), [](Step const & step) { return step.type == MashStep::Type::BatchSparge; }
   );
   int const numFlySparges = std::count_if(
      steps.cbegin(), steps.cend(), [](Step const & step) { return step.type == MashStep::Type::FlySparge; }
   );

   //
   // If the existing sparge steps already match what's asked for, leave them alone, so we don't lose any fixed values
   //
   switch (method) {
      case SpargeMethod::NoSparge:
         if (numBatchSparges == 0 && numFlySparges == 0) {
            return;
         }
         break;
      case SpargeMethod::BatchSparge:
         if (numBatchSparges == std::max(numBatches, 1) && numFlySparges == 0) {
            return;
         }
         break;
      case SpargeMethod::FlySparge:
         if (numBatchSparges == 0 && numFlySparges == 1) {
            return;
         }
         break;
   }

   steps.removeIf([](Step const & step) { return step.isSparge(); });

   auto makeSpargeStep = [spargeTemp_c](QString const & name, MashStep::Type const type) {
      return Step{
         .name          = name,
         .type          = type,
         .startTemp_c   = spargeTemp_c,
         .stepTime_mins = 15.0,
      };
   };
   if (method == SpargeMethod::BatchSparge) {
      for (int ii = 1; ii <= std::max(numBatches, 1); ++ii) {
         steps.append(makeSpargeStep(tr("Batch Sparge %1").arg(ii), MashStep::Type::BatchSparge));
      }
   } else if (method == SpargeMethod::FlySparge) {
      steps.append(makeSpargeStep(tr("Fly Sparge"), MashStep::Type::FlySparge));
   }
   return;
}

MashCalculator::Result MashCalculator::calculate(Inputs const & inputs,
                                                 QList<Step> const & stepsIn,
                                                 Options const & options) {
   Result result{stepsIn, {}};
   QList<Step> & steps = result.steps;

   auto warn = [&result](int const stepIndex, QString const & message) {
      qDebug() << Q_FUNC_INFO << "Warning for step" << stepIndex << ":" << message;
      result.problems.append(Problem{Problem::Severity::Warning, stepIndex, message});
   };
   auto error = [&result](int const stepIndex, QString const & message) {
      qInfo() << Q_FUNC_INFO << "Error for step" << stepIndex << ":" << message;
      result.problems.append(Problem{Problem::Severity::Error, stepIndex, message});
      return result;
   };

   //
   // Basic checks on the structure
   //
   if (steps.isEmpty()) {
      return error(-1, tr("There are no mash steps."));
   }
   if (inputs.grain_kg <= 0.0) {
      return error(-1, tr("There are no grains in the mash.  You need some mash-stage fermentable additions before the "
                          "mash can be calculated."));
   }
   if (!steps.first().isInfusion()) {
      return error(0, tr("The first step must be an infusion (ie adding the strike water)."));
   }
   //
   // Sparge steps, if any, have to come after all the other steps.  We call the other steps "mash steps".
   //
   qsizetype const numMashSteps = std::find_if(
      steps.cbegin(), steps.cend(), [](Step const & step) { return step.isSparge(); }
   ) - steps.cbegin();
   for (qsizetype ii = numMashSteps; ii < steps.size(); ++ii) {
      if (!steps[ii].isSparge()) {
         return error(ii, tr("Sparge steps must come after all the other steps."));
      }
   }
   bool const noSparge = (numMashSteps == steps.size());

   //
   // With no sparge, the last infusion whose amount isn't fixed has to make the water up to the total required.  See
   // class comment.
   //
   qsizetype balancingIndex = -1;
   if (noSparge) {
      for (qsizetype ii = 0; ii < numMashSteps; ++ii) {
         if (steps[ii].isInfusion() && !steps[ii].amountFixed) {
            balancingIndex = ii;
         }
      }
   }

   double thickness_LKg = defaultThickness_LKg;
   if (options.thickness_LKg) {
      thickness_LKg = *options.thickness_LKg;
      if (thickness_LKg <= 0.0) {
         return error(-1, tr("The mash thickness must be positive."));
      }
   } else if (steps.first().liquorToGristRatio_lKg && *steps.first().liquorToGristRatio_lKg > 0.0) {
      thickness_LKg = *steps.first().liquorToGristRatio_lKg;
   } else if (steps.first().amount_l > inputs.strikeWaterAllowance_l) {
      thickness_LKg = (steps.first().amount_l - inputs.strikeWaterAllowance_l) / inputs.grain_kg;
   }

   double const grainHeatCapacity_kcalC = inputs.grain_kg * cG;
   double const grainVolume_l           = inputs.grain_kg / PhysicalConstants::grainDensity_kgL;

   double water_l           = 0.0; // Water in the mash so far (not counting sparge water)
   double priorTemp_c       = 0.0; // Final temperature of the previous step
   double maxMashVolume_l   = 0.0; // Largest the mash gets

   for (qsizetype ii = 0; ii < numMashSteps; ++ii) {
      Step & step = steps[ii];
      double const targetTemp_c = step.startTemp_c;

      //
      // What the water (or, for a decoction, the boiled part of the mash) is being mixed with
      //
      Contents contents;
      if (ii == 0) {
         contents.parts = {{grainHeatCapacity_kcalC, inputs.grainTemp_c}, {inputs.tunHeatCapacity_kcalC, inputs.tunTemp_c}};
      } else {
         contents.parts = {{grainHeatCapacity_kcalC + water_l * cW + inputs.tunHeatCapacity_kcalC, priorTemp_c}};
      }

      switch (step.type) {
         case MashStep::Type::Infusion:
            {
               std::optional<double> amount_l;
               std::optional<double> waterTemp_c;
               if (step.infuseTempFixed) {
                  if (!step.infuseTemp_c) {
                     return error(ii, tr("The infusion temperature is marked as fixed, but has not been set."));
                  }
                  waterTemp_c = *step.infuseTemp_c;
               }

               //
               // Work out the amount, if it isn't fixed
               //
               if (step.amountFixed) {
                  amount_l = step.amount_l;
               } else if (ii == balancingIndex) {
                  double waterInOtherSteps_l = water_l;
                  for (qsizetype jj = ii + 1; jj < numMashSteps; ++jj) {
                     // By definition of balancingIndex, all later infusions have fixed amounts
                     if (steps[jj].isInfusion()) {
                        waterInOtherSteps_l += steps[jj].amount_l;
                     }
                  }
                  amount_l = inputs.targetTotalWater_l - waterInOtherSteps_l;
                  if (*amount_l <= 0.0) {
                     return error(
                        ii,
                        tr("With no sparge, this step has to make up the total water needed (%1), but the other steps "
                           "already add %2.").arg(showVolume(inputs.targetTotalWater_l), showVolume(waterInOtherSteps_l))
                     );
                  }
               } else if (waterTemp_c) {
                  amount_l = contents.amountFor(targetTemp_c, *waterTemp_c);
                  if (!amount_l) {
                     return error(ii, tr("Cannot reach %1 with water at %2.").arg(showTemp(targetTemp_c),
                                                                                    showTemp(*waterTemp_c)));
                  }
               } else if (ii == 0) {
                  amount_l = thickness_LKg * inputs.grain_kg + inputs.strikeWaterAllowance_l;
               } else {
                  //
                  // We use the hottest (or, if cooling, coldest) water we reasonably can, as that minimises the amount
                  // of water needed.
                  //
                  bool const heating = contents.heatNeeded_kcal(targetTemp_c) >= 0.0;
                  double const usualWaterTemp_c = heating ? inputs.boilingPoint_c : inputs.grainTemp_c;
                  amount_l = contents.amountFor(targetTemp_c, usualWaterTemp_c);
                  if (!amount_l) {
                     return error(ii, tr("Cannot reach %1 with water at %2.").arg(showTemp(targetTemp_c),
                                                                                    showTemp(usualWaterTemp_c)));
                  }
                  waterTemp_c = usualWaterTemp_c;
               }

               if (*amount_l <= 0.0) {
                  if (step.amountFixed) {
                     return error(ii, tr("The infusion amount must be more than zero."));
                  }
                  //
                  // Otherwise, this can only happen if the step doesn't need any heat, ie it is at the same temperature
                  // as the previous one, in which case the amount of water is up to the brewer.
                  //
                  return error(ii, tr("This infusion does not change the temperature of the mash, so its amount cannot "
                                      "be calculated.  It needs to be set and marked as fixed."));
               }

               //
               // Work out the water temperature, if it isn't fixed (or already set above)
               //
               bool const bothGiven = waterTemp_c && (step.amountFixed || ii == balancingIndex);
               if (!waterTemp_c) {
                  waterTemp_c = contents.waterTempFor(targetTemp_c, *amount_l);
               }

               if (*waterTemp_c > inputs.boilingPoint_c + 1e-6) {
                  QString const advice = (ii == 0 && !step.amountFixed && ii != balancingIndex) ?
                     tr("Try a thinner mash (ie more water).") : tr("Try more water or a lower step temperature.");
                  return error(
                     ii,
                     tr("The water would need to be at %1, which is above boiling point (%2).  %3").arg(
                        showTemp(*waterTemp_c), showTemp(inputs.boilingPoint_c), advice
                     )
                  );
               }
               if (*waterTemp_c < 0.0) {
                  return error(ii, tr("The water would need to be at %1, which is below freezing.").arg(
                     showTemp(*waterTemp_c)
                  ));
               }

               if (bothGiven) {
                  double const actualTemp_c = contents.finalTemp_c(*amount_l, *waterTemp_c);
                  if (std::abs(actualTemp_c - targetTemp_c) > temperatureTolerance_c) {
                     warn(ii, tr("%1 of water at %2 will give a mash temperature of %3 rather than %4.").arg(
                        showVolume(*amount_l), showTemp(*waterTemp_c), showTemp(actualTemp_c), showTemp(targetTemp_c)
                     ));
                  }
               }

               step.amount_l     = *amount_l;
               step.infuseTemp_c = *waterTemp_c;
               water_l += *amount_l;
               step.liquorToGristRatio_lKg = std::max(water_l - inputs.strikeWaterAllowance_l, 0.0) / inputs.grain_kg;
            }
            break;

         case MashStep::Type::Decoction:
            {
               //
               // We take out part of the mash (proportion r), heat it to boiling, and put it back.  Assuming that, while
               // this happens, the rest of the mash (and the tun) stay at priorTemp_c, we have:
               //
               //    r × HC_wg × (T_boil - T) = (HC_total - r × HC_wg) × (T - T_prior)
               //
               // where HC_wg is the heat capacity of the water and grain (ie not the tun) and HC_total includes the tun.
               // So:
               //
               //    r = HC_total × (T - T_prior) / (HC_wg × (T_boil - T_prior))
               //
               double const waterAndGrainHeatCapacity_kcalC = grainHeatCapacity_kcalC + water_l * cW;
               double const totalHeatCapacity_kcalC = waterAndGrainHeatCapacity_kcalC + inputs.tunHeatCapacity_kcalC;
               double const mashVolume_l = water_l + grainVolume_l;
               double const boilingDiff_c = inputs.boilingPoint_c - priorTemp_c;
               if (boilingDiff_c <= 0.0) {
                  return error(ii, tr("The mash is already at boiling point."));
               }
               if (step.amountFixed) {
                  double const ratio = step.amount_l / mashVolume_l;
                  if (ratio < 0.0 || ratio > 1.0) {
                     return error(ii, tr("The decoction amount (%1) must be between zero and the volume of the mash "
                                         "(%2).").arg(showVolume(step.amount_l), showVolume(mashVolume_l)));
                  }
                  double const actualTemp_c =
                     priorTemp_c + ratio * waterAndGrainHeatCapacity_kcalC * boilingDiff_c / totalHeatCapacity_kcalC;
                  if (std::abs(actualTemp_c - targetTemp_c) > temperatureTolerance_c) {
                     warn(ii, tr("A decoction of %1 will give a mash temperature of %2 rather than %3.").arg(
                        showVolume(step.amount_l), showTemp(actualTemp_c), showTemp(targetTemp_c)
                     ));
                  }
               } else {
                  if (targetTemp_c < priorTemp_c) {
                     return error(ii, tr("A decoction cannot lower the temperature of the mash."));
                  }
                  double const ratio = totalHeatCapacity_kcalC * (targetTemp_c - priorTemp_c) /
                                       (waterAndGrainHeatCapacity_kcalC * boilingDiff_c);
                  if (ratio > 1.0) {
                     return error(ii, tr("Cannot reach %1 by decoction, even by boiling the whole mash.").arg(
                        showTemp(targetTemp_c)
                     ));
                  }
                  step.amount_l = ratio * mashVolume_l;
               }
               // A decoction doesn't change the amount of water in the mash
            }
            break;

         case MashStep::Type::Temperature:
         case MashStep::Type::SouringMash:
         case MashStep::Type::SouringWort:
            // Nothing to calculate
            break;

         case MashStep::Type::FlySparge:
         case MashStep::Type::BatchSparge:
            // Should be impossible, because of how we calculated numMashSteps
            qCritical() << Q_FUNC_INFO << "Unexpected sparge step at" << ii;
            break;
      }

      priorTemp_c = step.finalTemp_c();
      maxMashVolume_l = std::max(maxMashVolume_l, water_l + grainVolume_l);
   }

   if (inputs.mashVesselVolume_l && maxMashVolume_l > *inputs.mashVesselVolume_l + volumeTolerance_l) {
      warn(-1, tr("At its fullest, the mash will be about %1, which is more than the capacity of the mash tun (%2).").arg(
         showVolume(maxMashVolume_l), showVolume(*inputs.mashVesselVolume_l)
      ));
   }

   if (noSparge) {
      if (balancingIndex < 0 && std::abs(water_l - inputs.targetTotalWater_l) > volumeTolerance_l) {
         warn(-1, tr("The infusions add up to %1 of water, but %2 is needed in total.").arg(
            showVolume(water_l), showVolume(inputs.targetTotalWater_l)
         ));
      }
      return result;
   }

   //
   // Now the sparge steps.  First work out how much water goes in each one.
   //
   double fixedSpargeWater_l = 0.0;
   int numUnfixedSparges = 0;
   for (qsizetype ii = numMashSteps; ii < steps.size(); ++ii) {
      if (steps[ii].amountFixed) {
         fixedSpargeWater_l += steps[ii].amount_l;
      } else {
         ++numUnfixedSparges;
      }
   }
   double const remainingWater_l = inputs.targetTotalWater_l - water_l - fixedSpargeWater_l;
   double spargeAmount_l = 0.0;
   if (numUnfixedSparges > 0) {
      if (remainingWater_l < volumeTolerance_l) {
         warn(-1, tr("There is no water left for sparging: the total water needed is %1, but the mash steps already "
                     "add %2%3.  You will collect more wort than needed.  Try a thicker mash (ie less water).").arg(
            showVolume(inputs.targetTotalWater_l),
            showVolume(water_l),
            fixedSpargeWater_l > 0.0 ? tr(" and the fixed sparge amounts add %1").arg(showVolume(fixedSpargeWater_l)) :
                                       QString{}
         ));
      } else {
         spargeAmount_l = remainingWater_l / numUnfixedSparges;
      }
   } else if (std::abs(remainingWater_l) > volumeTolerance_l) {
      warn(-1, tr("The mash and sparge water add up to %1, but %2 is needed in total.").arg(
         showVolume(water_l + fixedSpargeWater_l), showVolume(inputs.targetTotalWater_l)
      ));
   }

   //
   // Now the sparge water temperatures.  For a batch sparge, the water is mixed with the drained grain bed, ie the
   // grain, the water it has absorbed, and the tun.
   //
   double const bedHeatCapacity_kcalC =
      grainHeatCapacity_kcalC + inputs.grainAbsorption_LKg * inputs.grain_kg * cW + inputs.tunHeatCapacity_kcalC;
   for (qsizetype ii = numMashSteps; ii < steps.size(); ++ii) {
      Step & step = steps[ii];
      if (!step.amountFixed) {
         step.amount_l = spargeAmount_l;
      }
      double const targetTemp_c = step.startTemp_c;

      if (step.infuseTempFixed && !step.infuseTemp_c) {
         return error(ii, tr("The sparge water temperature is marked as fixed, but has not been set."));
      }

      if (step.type == MashStep::Type::FlySparge) {
         //
         // A fly sparge is a continuous process, so a simple heat balance doesn't really apply.  The sparge water is
         // normally just at the temperature we want the grain bed to be.
         //
         if (!step.infuseTempFixed) {
            step.infuseTemp_c = targetTemp_c;
         }
      } else if (step.amount_l > 0.0) {
         Contents const bed{{{bedHeatCapacity_kcalC, priorTemp_c - options.spargeHeatLoss_c}}};
         if (step.infuseTempFixed) {
            double const actualTemp_c = bed.finalTemp_c(step.amount_l, *step.infuseTemp_c);
            if (std::abs(actualTemp_c - targetTemp_c) > temperatureTolerance_c) {
               warn(ii, tr("%1 of water at %2 will give a grain bed temperature of %3 rather than %4.").arg(
                  showVolume(step.amount_l), showTemp(*step.infuseTemp_c), showTemp(actualTemp_c), showTemp(targetTemp_c)
               ));
            }
         } else {
            step.infuseTemp_c = bed.waterTempFor(targetTemp_c, step.amount_l);
         }
      } else if (!step.infuseTempFixed) {
         // No water, so no water temperature
         step.infuseTemp_c = std::nullopt;
      }

      if (step.infuseTemp_c && *step.infuseTemp_c > inputs.boilingPoint_c + 1e-6) {
         return error(ii, tr("The sparge water would need to be at %1, which is above boiling point (%2).  Try a lower "
                             "sparge temperature or more sparge water.").arg(
            showTemp(*step.infuseTemp_c), showTemp(inputs.boilingPoint_c)
         ));
      }

      priorTemp_c = step.finalTemp_c();
   }

   return result;
}

MashCalculator::Result MashCalculator::calculate(Inputs const & inputs, QList<Step> const & steps) {
   return calculate(inputs, steps, Options{});
}

void MashCalculator::apply(QList<Step> const & steps, Mash & mash) {
   QList<std::shared_ptr<MashStep>> const existingSteps = mash.mashSteps();

   QSet<int> kept;
   int lastSourceIndex = -1;
   bool seenNewStep = false;
   for (auto const & step : steps) {
      if (step.sourceIndex < 0) {
         seenNewStep = true;
         continue;
      }
      if (step.sourceIndex >= existingSteps.size() || step.sourceIndex <= lastSourceIndex || seenNewStep) {
         //
         // This is a coding error.  We carry on (as there is no harm in updating the steps we can), but the user
         // might not get all the changes.
         //
         qCritical() <<
            Q_FUNC_INFO << "Step" << step.name << "has unexpected source index" << step.sourceIndex << "(mash has" <<
            existingSteps.size() << "steps; last source index" << lastSourceIndex << "; seenNewStep" << seenNewStep <<
            ")";
         continue;
      }
      lastSourceIndex = step.sourceIndex;
      kept.insert(step.sourceIndex);
      MashStep & mashStep = *existingSteps.at(step.sourceIndex);
      if (mashStep.amount_l() != step.amount_l) {
         mashStep.setAmount_l(step.amount_l);
      }
      if (mashStep.infuseTemp_c() != step.infuseTemp_c) {
         mashStep.setInfuseTemp_c(step.infuseTemp_c);
      }
      if (mashStep.liquorToGristRatio_lKg() != step.liquorToGristRatio_lKg) {
         mashStep.setLiquorToGristRatio_lKg(step.liquorToGristRatio_lKg);
      }
   }

   bool stepsAddedOrRemoved = false;
   for (int ii = 0; ii < existingSteps.size(); ++ii) {
      if (!kept.contains(ii)) {
         mash.remove(existingSteps.at(ii));
         stepsAddedOrRemoved = true;
      }
   }

   for (auto const & step : steps) {
      if (step.sourceIndex >= 0) {
         continue;
      }
      auto mashStep = std::make_shared<MashStep>(step.name);
      mashStep->setType           (step.type           );
      mashStep->setStartTemp_c    (step.startTemp_c    );
      mashStep->setEndTemp_c      (step.endTemp_c      );
      mashStep->setStepTime_mins  (step.stepTime_mins  );
      mashStep->setAmount_l       (step.amount_l       );
      mashStep->setInfuseTemp_c   (step.infuseTemp_c   );
      mashStep->setAmountFixed    (step.amountFixed    );
      mashStep->setInfuseTempFixed(step.infuseTempFixed);
      mashStep->setLiquorToGristRatio_lKg(step.liquorToGristRatio_lKg);
      // Setting of step owner and storing in DB ultimately happens in OwnedSet::extend()
      mash.add(mashStep);
      stepsAddedOrRemoved = true;
   }

   if (stepsAddedOrRemoved) {
      emit mash.ownedItemsChanged();
   }
   return;
}
