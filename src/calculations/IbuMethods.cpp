/*======================================================================================================================
 * calculations/IbuMethods.cpp is part of Brewken, and is copyright the following authors 2009-2026:
 *   • Daniel Pettersson <pettson81@gmail.com>
 *   • Mattias Måhl <mattias@kejsarsten.com>
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
#include "calculations/IbuMethods.h"

#include <algorithm>        // For std::max, std::min
#include <initializer_list>
#include <limits>
#include <numbers>          // For std::numbers::pi
#include <utility>          // For std::pair

#include <cmath>

#include <QDebug>
#include <QObject>
#include <QString>
#include <qglobal.h> // For Q_ASSERT and Q_UNREACHABLE

#include "calculations/Algorithms.h"
#include "measurement/Unit.h"
#include "PersistentSettings.h"

namespace {
   double circleAreaFromRadius(double const radius) {
      return std::numbers::pi * radius * radius;
   }

   /**
    * \brief This intermediate calculation is used in Tinseth's formula and the mIBU formula
    *
    * \param wortGravity_sg
    * \param timeInBoil_minutes usually measured from the point at which hops are added until flameout
    */
   double calculateDecimalAlphaAcidUtilization(double const wortGravity_sg,
                                               double const timeInBoil_minutes) {
      //
      // TODO This is Tinseth's "Utilization Table" from which we could probably get a better value for
      //      decimalAlphaAcidUtilization via look-up and interpolation.
      //
      // Decimal Alpha Acid Utilization vs. Boil Time and Wort Original Gravity
      //
      // Boil  | Original Gravity
      // Time  |
      // (min) | 1.030  1.040  1.050  1.060  1.070  1.080  1.090  1.100  1.110  1.120  1.130
      // ------+ -----  -----  -----  -----  -----  -----  -----  -----  -----  -----  -----
      //    0  | 0.000  0.000  0.000  0.000  0.000  0.000  0.000  0.000  0.000  0.000  0.000
      //    3  | 0.034  0.031  0.029  0.026  0.024  0.022  0.020  0.018  0.017  0.015  0.014
      //    6  | 0.065  0.059  0.054  0.049  0.045  0.041  0.038  0.035  0.032  0.029  0.026
      //    9  | 0.092  0.084  0.077  0.070  0.064  0.059  0.054  0.049  0.045  0.041  0.037
      //   12  | 0.116  0.106  0.097  0.088  0.081  0.074  0.068  0.062  0.056  0.052  0.047
      //   15  | 0.137  0.125  0.114  0.105  0.096  0.087  0.080  0.073  0.067  0.061  0.056
      //   18  | 0.156  0.142  0.130  0.119  0.109  0.099  0.091  0.083  0.076  0.069  0.063
      //   21  | 0.173  0.158  0.144  0.132  0.120  0.110  0.101  0.092  0.084  0.077  0.070
      //   24  | 0.187  0.171  0.157  0.143  0.131  0.120  0.109  0.100  0.091  0.083  0.076
      //   27  | 0.201  0.183  0.168  0.153  0.140  0.128  0.117  0.107  0.098  0.089  0.082
      //   30  | 0.212  0.194  0.177  0.162  0.148  0.135  0.124  0.113  0.103  0.094  0.086
      //   33  | 0.223  0.203  0.186  0.170  0.155  0.142  0.130  0.119  0.108  0.099  0.091
      //   36  | 0.232  0.212  0.194  0.177  0.162  0.148  0.135  0.124  0.113  0.103  0.094
      //   39  | 0.240  0.219  0.200  0.183  0.167  0.153  0.140  0.128  0.117  0.107  0.098
      //   42  | 0.247  0.226  0.206  0.189  0.172  0.158  0.144  0.132  0.120  0.110  0.101
      //   45  | 0.253  0.232  0.212  0.194  0.177  0.162  0.148  0.135  0.123  0.113  0.103
      //   48  | 0.259  0.237  0.216  0.198  0.181  0.165  0.151  0.138  0.126  0.115  0.105
      //   51  | 0.264  0.241  0.221  0.202  0.184  0.169  0.154  0.141  0.129  0.118  0.108
      //   54  | 0.269  0.246  0.224  0.205  0.188  0.171  0.157  0.143  0.131  0.120  0.109
      //   57  | 0.273  0.249  0.228  0.208  0.190  0.174  0.159  0.145  0.133  0.121  0.111
      //   60  | 0.276  0.252  0.231  0.211  0.193  0.176  0.161  0.147  0.135  0.123  0.112
      //   70  | 0.285  0.261  0.238  0.218  0.199  0.182  0.166  0.152  0.139  0.127  0.116
      //   80  | 0.291  0.266  0.243  0.222  0.203  0.186  0.170  0.155  0.142  0.130  0.119
      //   90  | 0.295  0.270  0.247  0.226  0.206  0.188  0.172  0.157  0.144  0.132  0.120
      //  120  | 0.301  0.275  0.252  0.230  0.210  0.192  0.176  0.161  0.147  0.134  0.123
      //

      //
      // This is the short-cut way to get decimalAlphaAcidUtilization
      //
      double const boilTimeFactor = (1.0 - exp(-0.04 * timeInBoil_minutes)) / 4.15;
      double const bignessFactor  = 1.65 * pow(0.000125, (wortGravity_sg - 1.0));
      double const decimalAlphaAcidUtilization = bignessFactor * boilTimeFactor;
      qDebug() <<
         Q_FUNC_INFO << "boilTimeFactor:" << boilTimeFactor << "; bignessFactor:" << bignessFactor <<
         "; decimalAlphaAcidUtilization" << decimalAlphaAcidUtilization;
      return decimalAlphaAcidUtilization;
   }

   /*!
    * \brief Calculates the IBU by Tinseth's formula, as described at http://www.realbeer.com/hops/research.html
    */
   IbuMethods::IbuBreakdown tinseth(IbuMethods::IbuCalculationParms const & parms) {
      //
      // Tinseth's formula only models isomerization during the boil
      //
      if (parms.postBoilAddition) {
         return {};
      }
      //
      // Tinseth's formula is:
      //
      //    IBUs = decimal alpha acid utilization * mg/l of added alpha acids
      //
      // where:
      //
      //    mg/l of added alpha acids = decimal AA rating * grams hops * 1000
      //                                -------------------------------------
      //                                  volume of finished beer in liters
      //
      // and either:
      //
      //    decimal alpha acid utilization comes from look-up table
      //
      // or:
      //
      //    decimal alpha acid utilization = Bigness factor * Boil Time factor
      //
      // where:
      //
      //    Bigness factor = 1.65 * 0.000125^(wort gravity - 1)
      //
      //       "The Bigness factor accounts for reduced utilization due to higher wort gravities.  Use an average
      //       gravity value for the entire boil to account for changes in the wort volume."
      //
      // and:
      //
      //    Boil Time factor = 1 - e^(-0.04 * time in mins)
      //                       ----------------------------
      //                                   4.15
      //
      // Tinseth comments:
      //       "The numbers 1.65 and 0.000125 are empirically derived to fit my data.  The number 0.04 controls the
      //    shape of the utilization vs. time curve.  The factor 4.15 controls the maximum utilization value--make it
      //    smaller if your kettle utilization is higher than mine.
      //       "I'd suggest fiddling with 4.15 if necessary to match your system; only play with the other three if you
      //    like to muck around. I make no guarantees if you do.
      //       "The really cool thing about these new equations is that they are easily customizable.  I believe the
      //    basic form is correct; by playing with the different factors, different brewers should be able to make them
      //    fit their breweries perfectly."
      //
      double const mgPerLiterOfAddedAlphaAcids = (parms.AArating * parms.hops_grams * 1000) / parms.postBoilVolume_liters;
      double const decimalAlphaAcidUtilization = calculateDecimalAlphaAcidUtilization(parms.wortGravity_sg,
                                                                                      parms.timeInBoil_minutes);
      double const result = decimalAlphaAcidUtilization * mgPerLiterOfAddedAlphaAcids;
      qDebug() <<
         Q_FUNC_INFO << "mgPerLiterOfAddedAlphaAcids: " << mgPerLiterOfAddedAlphaAcids <<
         "; decimalAlphaAcidUtilization" << decimalAlphaAcidUtilization << " = " << result;
      return {.boil_ibus = result};
   }

   IbuMethods::IbuBreakdown rager(IbuMethods::IbuCalculationParms const & parms) {
      //
      // Rager's formula only models isomerization during the boil.  Note that, because of the shape of the tanh curve,
      // it gives a non-zero utilization (about 5%) at 0 minutes in the boil.  That's part of the formula for additions
      // at the end of the boil, but it would be wrong to apply it to hops added after flameout, as it would give the
      // same IBUs regardless of how long the hops steep or at what temperature.
      //
      if (parms.postBoilAddition) {
         return {};
      }
      double const utilization = (18.11 + 13.86 * tanh((parms.timeInBoil_minutes - 31.32) / 18.17)) / 100.0;

      double const gravityFactor = (parms.wortGravity_sg > 1.050) ? (parms.wortGravity_sg - 1.050)/0.2 : 0.0;

      return {
         .boil_ibus =
            (parms.hops_grams * utilization * parms.AArating * 1000) / (parms.postBoilVolume_liters * (1 + gravityFactor))
      };
   }

   /*!
    * \brief Calculates the IBU by Greg Noonan's formula
    */
   IbuMethods::IbuBreakdown noonan(IbuMethods::IbuCalculationParms const & parms) {
      //
      // As with Rager, Noonan's formula gives non-zero IBUs at 0 minutes in the boil, but does not model post-boil
      // isomerization, so we don't want to apply it to hops added after flameout.
      //
      if (parms.postBoilAddition) {
         return {};
      }
      double const volumeFactor = (Measurement::Units::us_gallons.toCanonical(5.0).quantity)/ parms.postBoilVolume_liters;
      double const hopsFactor = parms.hops_grams/ (Measurement::Units::ounces.toCanonical(1.0).quantity * 1000.0);
      static const Polynomial p(Polynomial() << 0.7000029428 << -0.08868853463 << 0.02720809386 << -0.002340415323 << 0.00009925450081 << -0.000002102006144 << 0.00000002132644293 << -0.00000000008229488217);

      //using 60 minutes as a general table
      static double const utilizationFactorTable[4][2] =  {
                        {1.050, 1},
                        {1.065, 0.9286},
                        {1.085, 0.8571},
                        {1.100, 0.75}
                     };

      double utilizationFactor;

      if (parms.wortGravity_sg <= utilizationFactorTable[0][0]) {
         utilizationFactor = utilizationFactorTable[0][1];
      } else if (parms.wortGravity_sg <= utilizationFactorTable[1][0]) {
         utilizationFactor = utilizationFactorTable[1][1];
      } else if (parms.wortGravity_sg <= utilizationFactorTable[2][0]) {
         utilizationFactor = utilizationFactorTable[2][1];
      } else {
         utilizationFactor = utilizationFactorTable[3][1];
      }

      return {
         .boil_ibus =
            volumeFactor * ( hopsFactor * (100 * parms.AArating) * p.eval(parms.timeInBoil_minutes) ) * utilizationFactor
      };
   }

   /**
    * \brief Look up a utilization (in percent) from a table of (upper bound of boil time, utilization) pairs, where the
    *        boil time ranges are of the form (previous upper bound, this upper bound].  Boil times beyond the last
    *        entry get the last entry's utilization.
    */
   double lookUpUtilization_pct(std::initializer_list<std::pair<double, double>> const table,
                                double const timeInBoil_minutes) {
      double utilization_pct = 0.0;
      for (auto const & [upperBound_minutes, entryUtilization_pct] : table) {
         utilization_pct = entryUtilization_pct;
         if (timeInBoil_minutes <= upperBound_minutes) {
            break;
         }
      }
      return utilization_pct;
   }

   /**
    * \brief For formulae (Rager, Daniels) that use Rager's boil gravity correction: 1 + (SG - 1.050) / 0.2 for gravities
    *        above 1.050, and 1 otherwise.  (Michael Hall writes this as a utilization factor Fbg = 1 / (1 + 5(SG -
    *        1.050)), which is the same thing.)
    */
   double ragerGravityCorrection(double const boilGravity_sg) {
      return (boilGravity_sg > 1.050) ? 1.0 + (boilGravity_sg - 1.050) / 0.2 : 1.0;
   }

   bool isPelletOrPowder(IbuMethods::IbuCalculationParms const & parms) {
      return parms.hopForm && (*parms.hopForm == IbuMethods::HopForm::Pellet ||
                               *parms.hopForm == IbuMethods::HopForm::Powder);
   }

   /*!
    * \brief Calculates the IBU by Mark Garetz's formula, from his book "Using Hops: The Complete Guide to Hops for the
    *        Craft Brewer" (HopTech, 1994), as summarised by Michael L. Hall in "What's Your IBU?" (Zymurgy Special
    *        Edition, 1997) and elsewhere.
    *
    *        IBU = (Utilization × AA × hop grams × 1000) / (Volume (l) × CA)
    *
    *        where the combined adjustment CA = GF × HF × TF and:
    *
    *         - the concentration factor CF = final volume / pre-boil volume
    *         - the boil gravity BG = (CF × (OG − 1)) + 1
    *         - the gravity factor GF = ((BG − 1.050) / 0.2) + 1 (for BG > 1.050; otherwise 1)
    *         - the hopping rate factor HF = (CF × total IBUs / 260) + 1 -- see \c IbuMethods::garetzHoppingRateFactor
    *         - the temperature (ie altitude) factor TF = (elevation in feet / 27,500) + 1.  We don't currently know the
    *           brewer's elevation, so we assume sea level, ie TF = 1.
    *
    *        Garetz also has a pellet factor of 1.10 for pellets boiled for up to 30 minutes.  (Hall gives it as for
    *        10-30 minutes, but the utilization is 0 for under 10 minutes anyway.)
    *
    *        Garetz's other optional factors (yeast, filtering, hop bags) are all nominally 1.0 and, per Hall, "you may
    *        omit any that don't seem necessary".  We do not currently have the information to set them, so we omit
    *        them.
    */
   IbuMethods::IbuBreakdown garetz(IbuMethods::IbuCalculationParms const & parms) {
      // Garetz does not model post-boil isomerization
      if (parms.postBoilAddition) {
         return {};
      }

      //
      // Garetz's utilization table.  Note that there is no utilization below 10 minutes.  Values are as tabulated by
      // Michael Hall, who gives values to 90 minutes; we assume no further increase beyond that.
      //
      double utilization_pct = lookUpUtilization_pct({{10.0,  0.0},
                                                      {15.0,  2.0},
                                                      {20.0,  5.0},
                                                      {25.0,  8.0},
                                                      {30.0, 11.0},
                                                      {35.0, 14.0},
                                                      {40.0, 16.0},
                                                      {45.0, 18.0},
                                                      {50.0, 19.0},
                                                      {60.0, 20.0},
                                                      {70.0, 21.0},
                                                      {80.0, 22.0},
                                                      {90.0, 23.0}},
                                                     parms.timeInBoil_minutes);
      if (isPelletOrPowder(parms) && parms.timeInBoil_minutes <= 30.0) {
         utilization_pct *= 1.10;
      }

      double const finalVolume_liters = parms.kettleVolumeAtFlameout_liters.value_or(parms.postBoilVolume_liters);
      double const concentrationFactor =
         (parms.preBoilVolume_liters && *parms.preBoilVolume_liters > 0.0) ?
         finalVolume_liters / *parms.preBoilVolume_liters : 1.0;
      double const originalGravity_sg = parms.originalGravity_sg.value_or(parms.wortGravity_sg);
      double const boilGravity_sg = (concentrationFactor * (originalGravity_sg - 1.0)) + 1.0;
      double const gravityFactor = ragerGravityCorrection(boilGravity_sg);
      double const temperatureFactor = 1.0;
      double const combinedAdjustments = gravityFactor * parms.hoppingRateFactor * temperatureFactor;

      double const result =
         (utilization_pct / 100.0) * parms.AArating * parms.hops_grams * 1000.0 /
         (parms.postBoilVolume_liters * combinedAdjustments);
      qDebug() <<
         Q_FUNC_INFO << "utilization_pct:" << utilization_pct << "; concentrationFactor:" << concentrationFactor <<
         "; boilGravity_sg:" << boilGravity_sg << "; gravityFactor:" << gravityFactor << "; hoppingRateFactor:" <<
         parms.hoppingRateFactor << " = " << result;
      return {.boil_ibus = result};
   }

   /*!
    * \brief Calculates the IBU by Ray Daniels' formula, from his book "Designing Great Beers" (Brewers Publications,
    *        1996), as summarised by Michael L. Hall in "What's Your IBU?" (Zymurgy Special Edition, 1997).
    *
    *        IBU = (Utilization × AA × hop grams × 1000) / (Volume (l) × C_gravity × HF)
    *
    *        Daniels' utilization is a step function of boil time, with separate tables for whole hops and pellets.  Per
    *        Hall, Daniels uses Rager's boil gravity correction (C_gravity) and Garetz's hopping rate factor (HF).
    *
    *        NOTE: The pellet value for 75+ minutes (34%) is from the book as best we know, but could not be checked
    *              against a second source.  It is consistent with the other pellet values being about 125% of the
    *              whole-hop ones (which also matches Daniels' recommendation to use a factor of 1 to 1.25 for pellets).
    */
   IbuMethods::IbuBreakdown daniels(IbuMethods::IbuCalculationParms const & parms) {
      // Daniels does not model post-boil isomerization
      if (parms.postBoilAddition) {
         return {};
      }

      //
      // Here the time ranges are [lower bound, next lower bound), so we use "upper bound" values just below the next
      // lower bound.  Eg 0-9 minutes (inclusive) is 5% for whole hops, and 10 minutes is in the next band.
      //
      static double constexpr justUnder = 1.0e-9;
      double const utilization_pct = isPelletOrPowder(parms) ?
         lookUpUtilization_pct({{10.0 - justUnder,  6.0},
                                {20.0 - justUnder, 15.0},
                                {30.0 - justUnder, 19.0},
                                {45.0 - justUnder, 24.0},
                                {60.0 - justUnder, 27.0},
                                {75.0 - justUnder, 30.0},
                                {std::numeric_limits<double>::max(), 34.0}},
                               parms.timeInBoil_minutes) :
         lookUpUtilization_pct({{10.0 - justUnder,  5.0},
                                {20.0 - justUnder, 12.0},
                                {30.0 - justUnder, 15.0},
                                {45.0 - justUnder, 19.0},
                                {60.0 - justUnder, 22.0},
                                {75.0 - justUnder, 24.0},
                                {std::numeric_limits<double>::max(), 27.0}},
                               parms.timeInBoil_minutes);

      double const gravityCorrection = ragerGravityCorrection(parms.wortGravity_sg);
      double const result =
         (utilization_pct / 100.0) * parms.AArating * parms.hops_grams * 1000.0 /
         (parms.postBoilVolume_liters * gravityCorrection * parms.hoppingRateFactor);
      qDebug() <<
         Q_FUNC_INFO << "utilization_pct:" << utilization_pct << "; gravityCorrection:" << gravityCorrection <<
         "; hoppingRateFactor:" << parms.hoppingRateFactor << " = " << result;
      return {.boil_ibus = result};
   }

   /**
    * \brief Intermediate step used by mIBU formula: the additional decimal alpha acid utilization from isomerization
    *        after flameout.
    *
    *        Per Hosom, the instantaneous rate of utilization is the derivative of Tinseth's utilization curve (with
    *        respect to time since the hops were added), multiplied by a relative rate that depends on the wort
    *        temperature -- see \c IbuMethods::relativeUtilization.  We integrate this from flameout (or, for post-boil
    *        additions, from when the hops are added) for \c coolTime_minutes.  Per Hosom, after forced cooling, the
    *        contribution is zero.
    *
    *        Also per Hosom, the relative rate is taken to be 1.0 (ie as at boiling) for the first 5 minutes after the
    *        hops are added.  He describes this as an oversimplification to account for non-IAA components (which are
    *        less dependent on temperature), but better than ignoring them altogether.  This only affects hops added
    *        less than 5 minutes before flameout or after flameout.
    *
    * \param coolTime_minutes How long after flameout (or, for post-boil additions, after the hops are added) the hops
    *                         are in hot wort.
    */
   double computePostBoilUtilization(IbuMethods::IbuCalculationParms const & parms,
                                     double const coolTime_minutes) {
      if (coolTime_minutes <= 0.0) {
         return 0.0;
      }

      double const b = IbuMethods::naturalCoolingRate(parms.postBoilVolume_liters,
                                                      parms.kettleInternalDiameter_cm.value_or(45.0),
                                                      parms.kettleOpeningDiameter_cm .value_or(45.0));

      //
      // The integrand is smooth (apart from the step at 5 minutes, and at segment boundaries), so the midpoint rule with
      // steps of 0.01 minutes is plenty accurate.  (We used to use steps of 0.001 minutes with the left-hand rule, which
      // was about 30,000 iterations per hop addition for a 30-minute cool time.)
      //
      double const bignessFactor = 1.65 * pow(0.000125, (parms.wortGravity_sg - 1.0));
      int    const numSteps      = std::max(1, static_cast<int>(std::ceil(coolTime_minutes / 0.01)));
      double const stepSize      = coolTime_minutes / numSteps;
      double decimalAlphaAcidUtilization = 0.0;
      for (int ii = 0; ii < numSteps; ++ii) {
         double const timeIntoCooling_minutes = (ii + 0.5) * stepSize;
         double const timeSinceAddition_minutes = parms.timeInBoil_minutes + timeIntoCooling_minutes;
         // Derivative of Tinseth's utilization with respect to time since the hops were added
         double const dU = bignessFactor * 0.04 * exp(-0.04 * timeSinceAddition_minutes) / 4.15;
         double const relativeUtilization =
            (timeSinceAddition_minutes < 5.0) ? 1.0 : IbuMethods::relativeUtilization(
               IbuMethods::postBoilWortTemperature_c(parms.postBoilDelay_minutes + timeIntoCooling_minutes,
                                                     parms.postBoilTemperatureProfile,
                                                     b)
            );
         decimalAlphaAcidUtilization += dU * relativeUtilization * stepSize;
      }
      return decimalAlphaAcidUtilization;
   }

   /*!
    * \brief Calculates the IBU by the mIBU formula, developed by John-Paul Hosom, and described at
    *        https://alchemyoverlord.wordpress.com/2015/05/12/a-modified-ibu-measurement-especially-for-late-hopping/
    */
   IbuMethods::IbuBreakdown mIbu(IbuMethods::IbuCalculationParms const & parms) {
      //
      // If we don't know how long the hops are in hot wort after flameout, we assume immediate forced cooling, in which
      // case, per Hosom, mIBU gives the same results as Tinseth.
      //
      double const coolTime_minutes = parms.coolTime_minutes.value_or(0.0);
      if (!parms.coolTime_minutes) {
         qDebug() << Q_FUNC_INFO << "Cool time not set, so assuming immediate forced cooling";
      } else if (coolTime_minutes > 0.0) {
         //
         // We supply fallback values below, but they likely won't be great.  (Kettle geometry only matters when we are
         // modelling natural cooling, but it's simplest to check here.)
         //
         if (!parms.kettleInternalDiameter_cm) { qInfo() << Q_FUNC_INFO << "kettleInternalDiameter_cm not set!"; }
         if (!parms.kettleOpeningDiameter_cm ) { qInfo() << Q_FUNC_INFO << "kettleOpeningDiameter_cm  not set!"; }
      }

      double const mgPerLiterOfAddedAlphaAcids =
         (parms.AArating * parms.hops_grams * 1000.0) / parms.postBoilVolume_liters;

      // For post-boil additions, the hops have spent no time in the boil
      double const boilUtilization =
         parms.postBoilAddition ? 0.0 : calculateDecimalAlphaAcidUtilization(parms.wortGravity_sg,
                                                                             parms.timeInBoil_minutes);
      double const postBoilUtilization = computePostBoilUtilization(parms, coolTime_minutes);

      qDebug() <<
         Q_FUNC_INFO << "boilUtilization:" << boilUtilization << "; postBoilUtilization:" << postBoilUtilization <<
         "; mgPerLiterOfAddedAlphaAcids:" << mgPerLiterOfAddedAlphaAcids;

      return {
         .boil_ibus     = boilUtilization     * mgPerLiterOfAddedAlphaAcids,
         .postBoil_ibus = postBoilUtilization * mgPerLiterOfAddedAlphaAcids,
      };
   }

   /**
    * \brief Constants and helper functions for the SMPH model.
    *
    *        The SMPH model (named after Shellhammer, Malowicki, Peacock and Hosom) was developed by John-Paul Hosom.  It
    *        is described in "A Summary of Factors Affecting IBUs" (referred to below as [Summary]):
    *        https://jphosom.github.io/alchemyoverlord/blog/11-a-summary-of-factors-affecting-ibus/alchemyoverlord-blog-content11.html
    *        with an overview at
    *        https://jphosom.github.io/alchemyoverlord/blog/31-ibus-and-the-smph-model/alchemyoverlord-blog-content31.html
    *        Equation numbers below are those in [Summary].
    *
    *        In outline:
    *
    *          IBU = 5/7 × ([IAA]_beer + [oAA]_beer × scale_oAA + [oBA]_beer × scale_oBA + [PP_hops]_beer × scale_PPhops
    *                       + [PP_malt]_beer × scale_PPmalt)                                                       [46]
    *
    *        where IAA = isomerized alpha acids, oAA = oxidized alpha acids, oBA = oxidized beta acids, PP = polyphenols,
    *        all concentrations in ppm.  The malt polyphenol term doesn't depend on the hops, so we return it from
    *        \c IbuMethods::recipeLevelIbus rather than including it in any one hop addition.
    *
    *        [IAA] comes from Mark Malowicki's model of alpha-acid isomerization and IAA degradation as first-order
    *        reactions with temperature-dependent rate constants, integrated over the boil and post-boil temperature
    *        profile.  The other components dissolve quickly and so don't depend on boil time.  Each component is
    *        then reduced by various loss factors (LF_xxx).
    *
    *        Where the model has inputs that we do not currently have in Brewken, we use the following assumptions (all
    *        of which give a loss factor of 1.0, except beer age):
    *
    *         - Wort clarity: average
    *         - Krausen: medium deposits on the fermenter walls (ie neither mixed back in nor blown off)
    *         - Filtering: none
    *         - Age of beer: 1 week, which is what Hosom uses in his worked examples
    *         - Hops: well-preserved, ie the alpha-acid rating is correct on brew day (AA_decayfactor = 1.0)
    *         - Elevation: sea level (so boiling is 100°C)
    *         - pH: no pH adjustment between mash and end of boil (which only matters for malt polyphenols)
    *
    *        On the question of how the alpha-acid solubility limit applies when there are several hop additions,
    *        [Summary] is silent.  Following Hosom's worked examples, which are reproduced if (and only if) the limit is
    *        applied to the concentration in the kettle at the time the hops are added, we use the concentration of
    *        alpha acids from all kettle additions made up to that point, in the volume of wort at that point -- see
    *        \c IbuCalculationParms::kettleAlphaAcids_ppm.
    */
   namespace Smph {
      // Parameter values estimated by Hosom from 347 measured IBU values -- [Summary] section 4.3.  (Earlier versions
      // of the model used different values -- eg 240 and 490 ppm for the solubility limits.)
      double constexpr lossFactorBoil           = 0.51;
      double constexpr oAABoilFactor            = 0.11;
      double constexpr oAAAgeScale              = 0.33;
      double constexpr alphaAcidLimitMin_ppm    = 200.0;
      double constexpr alphaAcidLimitMax_ppm    = 580.0;

      // Other parameters from [Summary]
      double constexpr lossFactorFerment        = 0.85;          // Section 3.3
      double constexpr scaleOAA                 = 0.9155;        // [38]
      double constexpr oBABoilFactor            = 0.071;         // [40]
      double constexpr scaleOBA                 = 0.85;          // [44]
      double constexpr defaultPolyphenolRating  = 0.04;          // Section 3.4.4
      double constexpr lossFactorPPPrecipitate  = 0.20;          // [48]
      double constexpr lossFactorPPFerment      = 0.70;          // Section 3.4.4
      double constexpr scalePPHops              = 7.0/5.0 * 0.022; // [52]
      double constexpr pelletOAAFactor          = 2.0;           // F(form) in [35] -- Section 3.4.1
      double constexpr freshHopsDryingDays      = 3.5;           // [35]
      double constexpr defaultPostBoilPh        = 5.75;          // Where both pH loss factors are 1.0
      double constexpr ibuPerIaaPpm             = 5.0/7.0;       // [4]
      double constexpr beerAge_weeks            = 1.0;           // See comment above

      double constexpr boilingTemp_K            = 373.15;

      //! Malowicki's rate constant (per minute) for isomerization of alpha acids [13]
      double k1(double const temp_K) { return 7.9e11 * std::exp(-11858.0 / temp_K); }
      //! Malowicki's rate constant (per minute) for degradation of isomerized alpha acids [15]
      double k2(double const temp_K) { return 4.1e12 * std::exp(-12994.0 / temp_K); }

      /**
       * \brief Proportion of alpha acids that are available (ie dissolved), given the initial concentration, per the
       *        "soft" solubility limit of [24] and [25].
       */
      double lossFactorHoppingRate(double const alphaAcids_ppm) {
         if (alphaAcids_ppm <= 0.0) {
            return 1.0;
         }
         double const slope = std::log(1.0 - (alphaAcidLimitMin_ppm / alphaAcidLimitMax_ppm)) / alphaAcidLimitMin_ppm;
         double const limit_ppm = alphaAcidLimitMax_ppm * (1.0 - std::exp(slope * alphaAcids_ppm));
         return std::min(1.0, limit_ppm / alphaAcids_ppm);
      }

      //! Gravity loss factor [26].  \c steepTime_minutes is the time the hops are in contact with boiling wort.
      double lossFactorGravity(double const originalGravity_sg, double const steepTime_minutes) {
         double const s = (steepTime_minutes <= 30.0) ? 1.0 :
                          (steepTime_minutes >= 40.0) ? 4.9 : 0.39 * (steepTime_minutes - 30.0) + 1.0;
         double const points = originalGravity_sg - 1.0;
         if (points <= 0.0) {
            return 1.0;
         }
         return 1.0 - 2.0 * std::exp(-1.0 / (s * points));
      }

      //! [28]
      double lossFactorAge(double const age_weeks) {
         return 0.35 * std::exp(-0.073 * std::min(age_weeks, 16.0)) + 0.65;
      }

      //! Section 3.3.  (Clamped at 0 to avoid a negative loss factor at implausibly low pH.)
      double lossFactorPhIaa   (double const pH) { return std::max(0.0, 0.071  * pH + 0.592); }
      //! Section 3.4.2
      double lossFactorPhNonIaa(double const pH) { return std::max(0.0, 0.8948 * pH - 4.145); }

      //! Per Garetz, as cited in [Summary] Section 3.3
      double lossFactorFlocculation(std::optional<IbuMethods::YeastFlocculation> const flocculation) {
         if (flocculation) {
            switch (*flocculation) {
               case IbuMethods::YeastFlocculation::Low   : return 1.05;
               case IbuMethods::YeastFlocculation::Medium: return 1.00;
               case IbuMethods::YeastFlocculation::High  : return 0.95;
            }
         }
         return 1.0;
      }

      /**
       * \brief Proportion of the hops' weight that is alpha (or beta) acids oxidized before the hops reach the kettle --
       *        ie the "oAA_fresh + oAA_storage" part of [35] or the bracketed part of [41], divided by AA_harvest (or
       *        BA_harvest).  With well-preserved hops (AA_decayfactor = 1), the storage part is zero, so this is just
       *        the oxidation during drying.
       *
       *        The Garetz hop decay rate constant k (per day) is such that a proportion \c sixMonthAlphaLoss of alpha
       *        acids is lost after 180 days at room temperature in loose storage.
       */
      double preKettleOxidation(std::optional<double> const sixMonthAlphaLoss) {
         if (!sixMonthAlphaLoss || *sixMonthAlphaLoss <= 0.0 || *sixMonthAlphaLoss >= 1.0) {
            return 0.0;
         }
         double const k_perDay = -std::log(1.0 - *sixMonthAlphaLoss) / 180.0;
         double const alphaAcidDecayFactor = 1.0;
         return (1.0 - std::exp(-k_perDay * freshHopsDryingDays)) + oAAAgeScale * (1.0 - alphaAcidDecayFactor);
      }

      /**
       * \brief Advance the alpha acid and IAA concentrations by \c duration_minutes at a constant temperature, using the
       *        exact solution of Malowicki's differential equations ([20] and [21]) for constant rate constants.
       */
      void advance(double & alphaAcids_ppm, double & iaa_ppm, double const temp_K, double const duration_minutes) {
         double const rateK1 = k1(temp_K);
         double const rateK2 = k2(temp_K);
         double const decayAA  = std::exp(-rateK1 * duration_minutes);
         double const decayIAA = std::exp(-rateK2 * duration_minutes);
         double const formedIaa_ppm = (std::abs(rateK2 - rateK1) > 1.0e-12) ?
            alphaAcids_ppm * rateK1 / (rateK2 - rateK1) * (decayAA - decayIAA) :
            alphaAcids_ppm * rateK1 * duration_minutes * decayAA;
         iaa_ppm = iaa_ppm * decayIAA + formedIaa_ppm;
         alphaAcids_ppm *= decayAA;
         return;
      }
   }

   /*!
    * \brief Calculates the IBU by the SMPH model -- see comments on \c Smph namespace above.
    */
   IbuMethods::IbuBreakdown smph(IbuMethods::IbuCalculationParms const & parms) {
      //
      // Per [17], concentrations are relative to the post-boil volume of wort in the kettle (since any losses after the
      // end of the boil don't change concentrations).
      //
      double const volume_liters = parms.kettleVolumeAtFlameout_liters.value_or(parms.postBoilVolume_liters);
      double const ppmPerGramOfHops = 1000.0 / volume_liters;
      double const originalGravity_sg = parms.originalGravity_sg.value_or(parms.wortGravity_sg);
      double const postBoilPh = parms.postBoilPh.value_or(Smph::defaultPostBoilPh);

      double const alphaAcids_ppm = parms.AArating * parms.hops_grams * ppmPerGramOfHops; // [17]
      double const lossFactorHoppingRate =
         Smph::lossFactorHoppingRate(parms.kettleAlphaAcids_ppm.value_or(alphaAcids_ppm));

      //
      // Isomerized alpha acids.  During the boil proper, the temperature is constant (boiling), so we can use the exact
      // solution (equivalent to [16]).  After flameout, we step through the temperature profile, using the exact
      // solution for each (small) step.  As with mIBU, if we don't know how long the hops are in hot wort after
      // flameout, we assume immediate forced cooling.
      //
      double dissolvedAlphaAcids_ppm = alphaAcids_ppm * lossFactorHoppingRate;
      double iaa_ppm = 0.0;
      if (!parms.postBoilAddition && parms.timeInBoil_minutes > 0.0) {
         Smph::advance(dissolvedAlphaAcids_ppm, iaa_ppm, Smph::boilingTemp_K, parms.timeInBoil_minutes);
      }
      double const iaaAtFlameout_ppm = iaa_ppm;

      double const coolTime_minutes = std::max(0.0, parms.coolTime_minutes.value_or(0.0));
      if (coolTime_minutes > 0.0) {
         double const b = IbuMethods::naturalCoolingRate(volume_liters,
                                                         parms.kettleInternalDiameter_cm.value_or(45.0),
                                                         parms.kettleOpeningDiameter_cm .value_or(45.0));
         int    const numSteps = std::max(1, static_cast<int>(std::ceil(coolTime_minutes / 0.01)));
         double const stepSize = coolTime_minutes / numSteps;
         for (int ii = 0; ii < numSteps; ++ii) {
            double const midStep_minutes = parms.postBoilDelay_minutes + (ii + 0.5) * stepSize;
            double const temp_c = IbuMethods::postBoilWortTemperature_c(midStep_minutes,
                                                                       parms.postBoilTemperatureProfile,
                                                                       b);
            Smph::advance(dissolvedAlphaAcids_ppm, iaa_ppm, temp_c + 273.15, stepSize);
         }
      }

      //
      // Loss factors.  Note that the gravity loss factor depends on how long the hops were in boiling wort.
      //
      double const steepTime_minutes = parms.postBoilAddition ? 0.0 : parms.timeInBoil_minutes;
      double const lossFactorGravity      = Smph::lossFactorGravity(originalGravity_sg, steepTime_minutes);
      double const lossFactorFlocculation = Smph::lossFactorFlocculation(parms.yeastFlocculation);
      double const lossFactorAge          = Smph::lossFactorAge(Smph::beerAge_weeks);
      // Wort clarity, krausen and filtering loss factors are all 1.0 -- see comments above
      double const commonLossFactors = Smph::lossFactorFerment * lossFactorFlocculation * lossFactorAge;

      double const lossFactorIaa =                                                                        // [30]
         Smph::lossFactorBoil * lossFactorGravity * Smph::lossFactorPhIaa(postBoilPh) * commonLossFactors;

      double const lossFactorNonIaa =                                                                     // [37]
         Smph::lossFactorBoil * lossFactorGravity * Smph::lossFactorPhNonIaa(postBoilPh) * commonLossFactors;

      //
      // Oxidized alpha acids [35], [36] and [37].  Per Section 3.4.2, these are produced quickly when hops are added to
      // hot wort, so (unlike IAA) they don't depend on how long the hops are in the wort.  (Hosom also found that
      // production doesn't depend much on temperature, at least between 80°C and boiling -- see
      // https://jphosom.github.io/alchemyoverlord/blog/26-the-production-of-oxidized-alpha-acids-at-hop-stand-temperatures/alchemyoverlord-blog-content26.html)
      //
      bool const pellets = isPelletOrPowder(parms);
      double const preKettleOxidation = Smph::preKettleOxidation(parms.sixMonthAlphaLoss);
      double const oAA = std::max(parms.AArating * preKettleOxidation,
                                  parms.AArating * (pellets ? Smph::pelletOAAFactor : 1.0) *
                                  lossFactorHoppingRate * Smph::oAABoilFactor);
      double const oAABeer_ppm = oAA * parms.hops_grams * ppmPerGramOfHops * lossFactorNonIaa;

      //
      // Oxidized beta acids [41], [42] and [43].  Here the "boil" loss factor is 1.0, because oBA_boilFactor is already
      // the proportion of oxidized beta acids that survive the boil.
      //
      double const oBA = Smph::oBABoilFactor * parms.betaAcidRating.value_or(0.0) * preKettleOxidation;
      double const oBABeer_ppm = oBA * parms.hops_grams * ppmPerGramOfHops * lossFactorNonIaa / Smph::lossFactorBoil;

      //
      // Hop polyphenols [47], [48] and [49].  (These are assumed not to be lost as the beer ages.)
      //
      double const ppHopsBeer_ppm =
         parms.polyphenolRating.value_or(Smph::defaultPolyphenolRating) * parms.hops_grams * ppmPerGramOfHops *
         Smph::lossFactorPPPrecipitate * Smph::lossFactorPPFerment * lossFactorFlocculation;

      IbuMethods::IbuBreakdown result{
         .boil_ibus     = Smph::ibuPerIaaPpm * iaaAtFlameout_ppm * lossFactorIaa,
         .postBoil_ibus = Smph::ibuPerIaaPpm * (iaa_ppm - iaaAtFlameout_ppm) * lossFactorIaa,
         .nonIaa_ibus   = Smph::ibuPerIaaPpm * (oAABeer_ppm    * Smph::scaleOAA +
                                                oBABeer_ppm    * Smph::scaleOBA +
                                                ppHopsBeer_ppm * Smph::scalePPHops)
      };
      qDebug() <<
         Q_FUNC_INFO << "alphaAcids_ppm:" << alphaAcids_ppm << "; lossFactorHoppingRate:" << lossFactorHoppingRate <<
         "; iaaAtFlameout_ppm:" << iaaAtFlameout_ppm << "; iaa_ppm:" << iaa_ppm << "; lossFactorIaa:" <<
         lossFactorIaa << "; oAABeer_ppm:" << oAABeer_ppm << "; oBABeer_ppm:" << oBABeer_ppm << "; ppHopsBeer_ppm:" <<
         ppHopsBeer_ppm << " = " << result.boil_ibus << "+" << result.postBoil_ibus << "+" << result.nonIaa_ibus;
      return result;
   }

   /**
    * \brief For formulae that don't handle hop form themselves, this is Brewken's long-standing adjustment.  Tinseth's
    *        table was created from whole cone data, and it seems other formulae are optimized that way as well.  So,
    *        the utilization is considered unadjusted for whole cones, and adjusted up for plugs and pellets.
    *
    *         - http://www.realbeer.com/hops/FAQ.html
    *         - https://groups.google.com/forum/#!topic/brewtarget-help/mv2qvWBC4sU
    */
   double genericHopFormFactor(std::optional<IbuMethods::HopForm> const hopForm) {
      if (hopForm) {
         switch (*hopForm) {
            case IbuMethods::HopForm::Plug  : return 1.02;
            case IbuMethods::HopForm::Pellet: return 1.10;
            default:
               break;
         }
      }
      return 1.0;
   }

   /**
    * \return \c true if the formula takes account of hop form itself, \c false if we should apply
    *         \c genericHopFormFactor
    */
   bool handlesHopForm(IbuMethods::IbuFormula const ibuFormula) {
      switch (ibuFormula) {
         case IbuMethods::IbuFormula::Tinseth: return false;
         case IbuMethods::IbuFormula::Rager  : return false;
         case IbuMethods::IbuFormula::Noonan : return false;
         case IbuMethods::IbuFormula::Garetz : return true;
         case IbuMethods::IbuFormula::Daniels: return true;
         case IbuMethods::IbuFormula::mIbu   : return false;
         case IbuMethods::IbuFormula::Smph   : return true;
      }
      Q_UNREACHABLE();
   }

}

QString IbuMethods::localisedName_formula() { return QObject::tr("Formula"); }

EnumStringMapping const IbuMethods::formulaStringMapping {
   {IbuMethods::IbuFormula::Tinseth, "tinseth"},
   {IbuMethods::IbuFormula::Rager  , "rager"  },
   {IbuMethods::IbuFormula::Noonan , "noonan" },
   {IbuMethods::IbuFormula::Garetz , "garetz" },
   {IbuMethods::IbuFormula::Daniels, "daniels"},
   {IbuMethods::IbuFormula::mIbu   , "mibu"   },
   {IbuMethods::IbuFormula::Smph   , "smph"   },
};

//
// This is what we write to dotBeer files, so the values have to be in the enum for ibu_estimate_formula in
// schemas/dotBeer/Recipe.beer.schema.  Note that the schema has "Daniel" rather than "Daniels" (Ray Daniels being the
// person after whom the formula is named).
//
EnumStringMapping const IbuMethods::formulaStringMappingUc {
   {IbuMethods::IbuFormula::Tinseth, "Tinseth"},
   {IbuMethods::IbuFormula::Rager  , "Rager"  },
   {IbuMethods::IbuFormula::Noonan , "Noonan" },
   {IbuMethods::IbuFormula::Garetz , "Garetz" },
   {IbuMethods::IbuFormula::Daniels, "Daniel" },
   {IbuMethods::IbuFormula::mIbu   , "mIBU"   },
   {IbuMethods::IbuFormula::Smph   , "SMPH"   },
};

EnumStringMapping const IbuMethods::formulaDisplayNames {
   {IbuMethods::IbuFormula::Tinseth, QObject::tr("Tinseth's approximation")},
   {IbuMethods::IbuFormula::Rager  , QObject::tr("Rager's approximation"  )},
   {IbuMethods::IbuFormula::Noonan , QObject::tr("Noonan's approximation" )},
   {IbuMethods::IbuFormula::Garetz , QObject::tr("Garetz's approximation" )},
   {IbuMethods::IbuFormula::Daniels, QObject::tr("Daniels' approximation" )},
   {IbuMethods::IbuFormula::mIbu   , QObject::tr("mIBU"                   )},
   {IbuMethods::IbuFormula::Smph   , QObject::tr("SMPH"                   )},
};

TypeLookup const IbuMethods::typeLookup {
   "IbuMethods",
   {
      PROPERTY_TYPE_LOOKUP_ENTRY(IbuMethods, formula, formula, ENUM_INFO(IbuMethods::formula)),
   }
};

IbuMethods::IbuFormula IbuMethods::formula = IbuMethods::IbuFormula::Tinseth;


void IbuMethods::loadFormula() {
   PersistentSettings::readEnum(PersistentSettings::Names::ibu_formula,
                                IbuMethods::formulaStringMapping,
                                IbuMethods::formula,
                                IbuMethods::IbuFormula::Tinseth);
   return;
}

void IbuMethods::saveFormula() {
   PersistentSettings::insert_ck(PersistentSettings::Names::ibu_formula,
                                 IbuMethods::formulaStringMapping[IbuMethods::formula]);
   return;
}

QString IbuMethods::localisedFormulaName() {
   return IbuMethods::formulaDisplayNames[IbuMethods::formula];
}

bool IbuMethods::modelsPostBoilIsomerization(IbuMethods::IbuFormula const ibuFormula) {
   switch(ibuFormula) {
      case IbuMethods::IbuFormula::Tinseth: return false;
      case IbuMethods::IbuFormula::Rager  : return false;
      case IbuMethods::IbuFormula::Noonan : return false;
      case IbuMethods::IbuFormula::Garetz : return false;
      case IbuMethods::IbuFormula::Daniels: return false;
      case IbuMethods::IbuFormula::mIbu   : return true;
      case IbuMethods::IbuFormula::Smph   : return true;
   }
   Q_UNREACHABLE();
}

bool IbuMethods::usesHoppingRateFactor(IbuMethods::IbuFormula const ibuFormula) {
   return ibuFormula == IbuMethods::IbuFormula::Garetz || ibuFormula == IbuMethods::IbuFormula::Daniels;
}

double IbuMethods::garetzHoppingRateFactor(double const totalIbusBeforeHoppingRateFactor,
                                           double const concentrationFactor) {
   //
   // HF = 1 + a × IBU, where a = CF / 260, and IBU = U / HF.  So HF = 1 + a × U / HF, ie HF² - HF - a × U = 0, whose
   // positive root is HF = (1 + sqrt(1 + 4 × a × U)) / 2.
   //
   double const aTimesU = std::max(0.0, concentrationFactor / 260.0 * totalIbusBeforeHoppingRateFactor);
   return (1.0 + std::sqrt(1.0 + 4.0 * aTimesU)) / 2.0;
}

double IbuMethods::recipeLevelIbus(double const originalGravity_sg) {
   if (IbuMethods::formula != IbuMethods::IbuFormula::Smph) {
      return 0.0;
   }
   //
   // SMPH malt polyphenols, per [45] in Hosom's "A Summary of Factors Affecting IBUs".  We assume no difference between
   // the wort pH before any pH adjustment and the post-boil pH (because we don't know the former), so the pH term is
   // 1.0.
   //
   double const scaledMaltPolyphenols_ppm = (69.68 / 51.2) * (std::max(0.0, originalGravity_sg - 1.0) * 19.0);
   return Smph::ibuPerIaaPpm * scaledMaltPolyphenols_ppm;
}

IbuMethods::IbuBreakdown IbuMethods::getIbuBreakdown(IbuMethods::IbuCalculationParms const & parms) {
   qDebug() << Q_FUNC_INFO << "Using" << formulaDisplayNames[formula] << "with" << parms;
   IbuMethods::IbuBreakdown result;
   switch(IbuMethods::formula) {
      case IbuMethods::IbuFormula::Tinseth: result = tinseth(parms); break;
      case IbuMethods::IbuFormula::Rager  : result = rager  (parms); break;
      case IbuMethods::IbuFormula::Noonan : result = noonan (parms); break;
      case IbuMethods::IbuFormula::Garetz : result = garetz (parms); break;
      case IbuMethods::IbuFormula::Daniels: result = daniels(parms); break;
      case IbuMethods::IbuFormula::mIbu   : result = mIbu   (parms); break;
      case IbuMethods::IbuFormula::Smph   : result = smph   (parms); break;
   }
   if (!handlesHopForm(IbuMethods::formula)) {
      result *= genericHopFormFactor(parms.hopForm);
   }
   return result;
}

double IbuMethods::getIbus(IbuMethods::IbuCalculationParms const & parms) {
   return IbuMethods::getIbuBreakdown(parms).total();
}

namespace {
   //
   // Constants from Hosom's natural cooling curve, which is
   //
   //    T(t) = 53.70 * exp(-b * t) + 319.55
   //
   // where T is wort temperature in Kelvin and t is minutes since flameout.  So, in Celsius, the wort starts at 100.10°C
   // at flameout and tends towards 46.40°C.
   //
   double constexpr hosomCoolingAmplitude_c = 53.70;
   double constexpr hosomCoolingAsymptote_c = 319.55 - 273.15;
   double constexpr hosomFlameoutTemp_c     = hosomCoolingAsymptote_c + hosomCoolingAmplitude_c;

   /**
    * \brief Natural cooling, per Hosom's curve, from a given starting temperature.  Because the curve is exponential,
    *        starting part-way along it just means scaling the amplitude.
    */
   double naturalCooling_c(double const startTemp_c,
                           double const elapsed_minutes,
                           double const naturalCoolingRate_perMinute) {
      // If we're already at or below the asymptote, we just assume the temperature stays where it is
      if (startTemp_c <= hosomCoolingAsymptote_c) {
         return startTemp_c;
      }
      //
      // Anything at or above (nominal) boiling, we treat as starting from flameout on Hosom's curve.  (Otherwise
      // starting at exactly 100°C would give very slightly different results from Hosom's own formula.)
      //
      double const effectiveStartTemp_c = (startTemp_c >= 100.0) ? hosomFlameoutTemp_c : startTemp_c;
      return hosomCoolingAsymptote_c +
             (effectiveStartTemp_c - hosomCoolingAsymptote_c) * exp(-naturalCoolingRate_perMinute * elapsed_minutes);
   }
}

double IbuMethods::naturalCoolingRate(double const postBoilVolume_liters,
                                      double const kettleInternalDiameter_cm,
                                      double const kettleOpeningDiameter_cm) {
   double const surfaceArea_cm2   = circleAreaFromRadius(kettleInternalDiameter_cm/2.0);
   double const openingArea_cm2   = circleAreaFromRadius(kettleOpeningDiameter_cm /2.0);
   double const effectiveArea_cm2 = sqrt(surfaceArea_cm2 * openingArea_cm2);
   return (0.0002925 * effectiveArea_cm2 / postBoilVolume_liters) + 0.00538;
}

double IbuMethods::relativeUtilization(double const temp_c) {
   double const temp_degK = temp_c + 273.15;
   return 2.39e11 * exp(-9773.0 / temp_degK);
}

double IbuMethods::postBoilWortTemperature_c(double const minutesSinceFlameout,
                                             std::vector<PostBoilTemperatureSegment> const & profile,
                                             double const naturalCoolingRate_perMinute) {
   double elapsed_minutes = std::max(0.0, minutesSinceFlameout);
   // At flameout, we are at boiling
   double temp_c = 100.0;
   for (auto const & segment : profile) {
      if (elapsed_minutes <= segment.duration_minutes) {
         if (segment.naturalCooling) {
            return std::max(segment.endTemp_c,
                            naturalCooling_c(segment.startTemp_c, elapsed_minutes, naturalCoolingRate_perMinute));
         }
         if (segment.duration_minutes <= 0.0) {
            return segment.endTemp_c;
         }
         return segment.startTemp_c +
                (segment.endTemp_c - segment.startTemp_c) * elapsed_minutes / segment.duration_minutes;
      }
      elapsed_minutes -= segment.duration_minutes;
      if (segment.naturalCooling) {
         temp_c = std::max(segment.endTemp_c,
                           naturalCooling_c(segment.startTemp_c, segment.duration_minutes, naturalCoolingRate_perMinute));
      } else {
         temp_c = segment.endTemp_c;
      }
   }
   // Beyond the end of the profile, we assume natural cooling
   return naturalCooling_c(temp_c, elapsed_minutes, naturalCoolingRate_perMinute);
}