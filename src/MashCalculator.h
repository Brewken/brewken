/*======================================================================================================================
 * MashCalculator.h is part of Brewken, and is copyright the following authors 2026:
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
#ifndef MASHCALCULATOR_H
#define MASHCALCULATOR_H
#pragma once

#include <optional>

#include <QCoreApplication> // For Q_DECLARE_TR_FUNCTIONS
#include <QList>
#include <QString>

#include "model/MashStep.h"

class Mash;
class Recipe;

/**
 * \brief Works out all or part of a mash: the amounts and temperatures of infusions (including the strike water),
 *        the amounts of decoctions, and the amounts and temperatures of sparge water.
 *
 *        This replaces the calculations that were previously spread (with some differences between them) across the
 *        old Mash Wizard and Mash Designer.  It has no UI, and, apart from \c Inputs::fromRecipe and \c apply, does not
 *        touch the model, so it is easy to unit test.
 *
 *        Usage is:
 *          1. Get the \c Inputs (normally from the \c Recipe -- see \c Inputs::fromRecipe) and the current steps (see
 *             \c stepsFrom).
 *          2. Optionally change the structure of the steps -- eg \c setSparge to change the sparge method.
 *          3. Call \c calculate, which returns the steps with all the values that are not "fixed" (see below) worked
 *             out, plus any problems.
 *          4. If there were no errors, call \c apply to write the results back to the \c Mash.  (It is the caller's
 *             job to make sure the mash can be changed -- see \c MashEditor::calculate.)
 *
 *        Which values get calculated:
 *
 *          - Each \c MashStep has \c amountFixed and \c infuseTempFixed flags.  A value that is fixed is never changed
 *            by the calculator.  This is what allows the user to have the tool work out "all or part" of the mash: eg
 *            to fix the amount of the strike water and have the tool work out its temperature.
 *
 *          - Step temperatures (\c startTemp_c and \c endTemp_c) are always inputs, never outputs.  They are what the
 *            brewer is trying to achieve.
 *
 *          - First step (must be an infusion -- the strike water):
 *               amount fixed, temp not fixed : temp calculated to hit the step temperature
 *               temp fixed, amount not fixed : amount calculated to hit the step temperature
 *               neither fixed                : amount from the mash thickness (except see "no sparge" below); then
 *                                              temp calculated
 *               both fixed                   : nothing calculated; warning if the step temperature won't be hit
 *
 *          - Later infusions: as for the first step, except that, if neither is fixed, we use water at boiling point
 *            (or, if the infusion is to cool the mash, water at the grain temperature) as this needs the least water.
 *
 *          - Decoctions: unless the amount is fixed, we calculate how much of the mash to remove, boil and return to
 *            hit the step temperature.
 *
 *          - Temperature steps (ie heating with an external heat source): nothing to calculate.
 *
 *          - Sparge steps: the total sparge water is whatever is needed, on top of the infusions, to give the target
 *            total mash water (see \c Recipe::targetTotalMashVol_l).  It is split equally between the sparge steps that
 *            don't have a fixed amount.  For a batch sparge, the water temperature is calculated so that, when added to
 *            the drained grain bed, the bed reaches the step temperature.  For a fly sparge, we just use the step
 *            temperature.
 *
 *          - No sparge (ie there are no sparge steps): all the water has to go in as infusions, so, instead of using
 *            the mash thickness, the last infusion that doesn't have a fixed amount makes up the total.  (If it's the
 *            only infusion, this means the strike water is all the water.)
 *
 *        Physics: we use the same model as the old Mash Wizard (adapted from https://jansson.us/MashPhysics.html),
 *        with the heat capacity (aka thermal mass) of the mash being the sum of those of the grain, the water and the
 *        mash tun.  As elsewhere, we take 1 liter of water to weigh 1 kg.  The tun starts at \c Inputs::tunTemp_c
 *        and, from the end of the first step, is at the same temperature as the mash.  When a step has an end
 *        temperature, the next step starts from that, otherwise from the step's start temperature.
 */
class MashCalculator {
   Q_DECLARE_TR_FUNCTIONS(MashCalculator)

public:
   /**
    * \brief The things the calculation needs to know that are not part of the mash steps.  Usually obtained from the
    *        \c Recipe (see \c fromRecipe) but can be set directly (eg in unit tests).
    */
   struct Inputs {
      //! Mass of the grains (and other fermentables) in the mash -- see \c Recipe::grainsInMash_kg
      double grain_kg               = 0.0;
      //! See \c Mash::grainTemp_c
      double grainTemp_c            = 20.0;
      //! See \c Recipe::mashTunHeatCapacity_kcalC
      double tunHeatCapacity_kcalC  = 0.0;
      //! See \c Recipe::mashTunTemp_c
      double tunTemp_c              = 20.0;
      //! See \c Equipment::boilingPoint_c
      double boilingPoint_c         = 100.0;
      //! See \c Equipment::grainAbsorption_LKg
      double grainAbsorption_LKg    = 0.0;
      //! See \c Equipment::strikeWaterAllowance_l
      double strikeWaterAllowance_l = 0.0;
      //! See \c Recipe::targetTotalMashVol_l
      double targetTotalWater_l     = 0.0;
      //! See \c Equipment::mashVesselVolume_l.  If not set, we don't check whether the mash fits in the vessel.
      std::optional<double> mashVesselVolume_l = std::nullopt;

      /**
       * \brief Get the inputs for calculating \c mash for \c recipe.  Normally \c mash is the recipe's mash, but it
       *        doesn't have to be (eg it might be a new mash that will be used by the recipe once it is saved).
       */
      static Inputs fromRecipe(Recipe const & recipe, Mash const & mash);
      //! As above, for the recipe's own mash (or default values for the mash-related inputs if it doesn't have one)
      static Inputs fromRecipe(Recipe const & recipe);
   };

   /**
    * \brief The parts of a \c MashStep that the calculator uses or changes.
    */
   struct Step {
      /**
       * \brief Position of the corresponding step in the \c Mash when we got the steps from it (see \c stepsFrom),
       *        counted from 0, or -1 for a step that does not exist in the mash (eg one added by \c setSparge).
       *
       *        We use the position rather than a pointer to the \c MashStep so that the results can be applied to a
       *        copy of the mash (see \c apply).
       */
      int                   sourceIndex     = -1;
      QString               name            = "";
      MashStep::Type        type            = MashStep::Type::Infusion;
      double                startTemp_c     = 0.0;
      std::optional<double> endTemp_c       = std::nullopt;
      double                stepTime_mins   = 0.0;
      double                amount_l        = 0.0;
      std::optional<double> infuseTemp_c    = std::nullopt;
      bool                  amountFixed     = false;
      bool                  infuseTempFixed = false;
      /**
       * \brief See \c MashStep::liquorToGristRatio_lKg.  For the first step, if set (and \c Options::thickness_LKg is
       *        not), this is the mash thickness we use for the strike water.  The calculation sets it on all the
       *        infusion steps (based on the water in the mash at the end of the step, not counting the strike water
       *        allowance), so the mash "remembers" its thickness.
       */
      std::optional<double> liquorToGristRatio_lKg = std::nullopt;

      bool isInfusion() const;
      bool isSparge  () const;

      //! The temperature of the mash at the end of the step
      double finalTemp_c() const;
   };

   enum class SpargeMethod {
      NoSparge   ,
      BatchSparge,
      FlySparge
   };

   struct Options {
      /**
       * \brief The mash thickness (see \c Recipe::targetStrikeWater_l) to use for the strike water, if its amount is
       *        neither fixed nor determined by there being no sparge.  If not set, we use the first step's
       *        \c liquorToGristRatio_lKg, or, if that's not set, the thickness of the existing strike water, or, if
       *        there isn't any, \c defaultThickness_LKg.
       */
      std::optional<double> thickness_LKg  = std::nullopt;
      /**
       * \brief How much we assume the grain bed cools between draining the mash (or the previous batch sparge) and
       *        adding a batch of sparge water.  This was hard-coded in the old Mash Wizard and Mash Designer.
       */
      double                spargeHeatLoss_c = 10.0;
   };

   struct Problem {
      enum class Severity {
         //! The results can be used, but the user should check them
         Warning,
         //! The calculation could not be completed, so the results must not be used
         Error
      };
      Severity severity;
      //! The step the problem relates to, counted from 0, or -1 if it relates to the mash as a whole
      int      stepIndex;
      QString  message;
   };

   struct Result {
      QList<Step>    steps;
      QList<Problem> problems;

      //! \return \c true if there are no errors (there might still be warnings)
      bool ok() const;
      //! All the messages of the given severity, in order, each prefixed by the name of the step it relates to
      QStringList messages(Problem::Severity const severity) const;
   };

   //! Approximately 1.25 qt/lb, which is a common default
   static constexpr double defaultThickness_LKg = 2.6;
   //! Approximately 168°F, which is a common target for the grain bed when sparging
   static constexpr double defaultSpargeTemp_c  = 76.0;
   //! How much difference between the target and actual step temperature we allow before warning about it
   static constexpr double temperatureTolerance_c = 0.5;
   //! As \c temperatureTolerance_c but for volumes
   static constexpr double volumeTolerance_l      = 0.1;

   /**
    * \brief Get the steps of the supplied \c Mash in the form the calculator needs
    */
   static QList<Step> stepsFrom(Mash const & mash);

   /**
    * \brief Work out the sparge method from the steps.  (If there are both batch and fly sparge steps -- which would
    *        be odd -- we say batch, as that's what the calculations will treat the batch sparge steps as.)
    */
   static SpargeMethod spargeMethod(QList<Step> const & steps);

   /**
    * \brief Change the sparge steps to match the supplied method.  Any existing sparge steps are removed, and new
    *        ones (with amounts and temperatures to be calculated) are added at the end, except that, if the existing
    *        sparge steps already match the method (and number of batches), they are left alone, so that any fixed
    *        values on them are kept.
    *
    * \param steps
    * \param method
    * \param numBatches Ignored unless \c method is \c SpargeMethod::BatchSparge.  Must be at least 1.
    * \param spargeTemp_c The target temperature of the grain bed for the new sparge steps -- see \c Mash::spargeTemp_c
    */
   static void setSparge(QList<Step> & steps,
                         SpargeMethod const method,
                         int const numBatches,
                         double const spargeTemp_c = defaultSpargeTemp_c);

   /**
    * \brief Do the calculations.  See class comment for details.
    *
    *        If an error is found, the calculation stops, and the steps in the result should not be used.
    */
   static Result calculate(Inputs const & inputs, QList<Step> const & steps, Options const & options);
   //! As above, with default options.  (We can't use a default argument because Options is a nested class.)
   static Result calculate(Inputs const & inputs, QList<Step> const & steps);

   /**
    * \brief Write the results of \c calculate back to the supplied \c Mash, which must either be the one we got the
    *        steps from (see \c stepsFrom) or a copy of it.  Steps with a \c sourceIndex are updated, steps of the mash
    *        that are no longer in \c steps are removed, and steps without a \c sourceIndex are added at the end.
    *
    *        Steps that came from the mash must be in the same relative order as they were in the mash, and new steps
    *        must come after them.  (This is always the case with the steps returned by \c calculate after
    *        \c setSparge.)
    */
   static void apply(QList<Step> const & steps, Mash & mash);
};

#endif
