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

#include <algorithm> // For std::max
#include <numbers>   // For std::numbers::pi

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

//   /*!
//    * \brief Calculates the IBU by the SMPH formula, developed by Tom Shellhammer, Mark Malowicki, Val Peacock and
//    *        John-Paul Hosom, and described at
//    *        https://alchemyoverlord.wordpress.com/2021/11/10/ibus-and-the-smph-model/
//    */
//   double smph(IbuMethods::IbuCalculationParms const & parms) {
//      //
//      // TODO: Need to implement this!
//      //
//      return 0.0;
//   }

}

QString IbuMethods::localisedName_formula() { return QObject::tr("Formula"); }

EnumStringMapping const IbuMethods::formulaStringMapping {
   {IbuMethods::IbuFormula::Tinseth, "tinseth"},
   {IbuMethods::IbuFormula::Rager  , "rager"  },
   {IbuMethods::IbuFormula::Noonan , "noonan" },
   {IbuMethods::IbuFormula::mIbu   , "mibu"   },
//   {IbuMethods::IbuFormula::Smph   , "smph"   },
};

EnumStringMapping const IbuMethods::formulaStringMappingUc {
      {IbuMethods::IbuFormula::Tinseth, "Tinseth"},
      {IbuMethods::IbuFormula::Rager  , "Rager"  },
      {IbuMethods::IbuFormula::Noonan , "Noonan" },
      {IbuMethods::IbuFormula::mIbu   , "mIBU"   },
   //   {IbuMethods::IbuFormula::Smph   , "smph"   },
};

EnumStringMapping const IbuMethods::formulaDisplayNames {
   {IbuMethods::IbuFormula::Tinseth, QObject::tr("Tinseth's approximation")},
   {IbuMethods::IbuFormula::Rager  , QObject::tr("Rager's approximation"  )},
   {IbuMethods::IbuFormula::Noonan , QObject::tr("Noonan's approximation" )},
   {IbuMethods::IbuFormula::mIbu   , QObject::tr("mIBU"   )},
//   {IbuMethods::IbuFormula::Smph   , QObject::tr("Shellhammer, Malowicki, Peacock & Hosom (SMPH) approximation")},
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
      case IbuMethods::IbuFormula::mIbu   : return true;
//      case IbuMethods::IbuFormula::Smph   : return true;
   }
   Q_UNREACHABLE();
}

IbuMethods::IbuBreakdown IbuMethods::getIbuBreakdown(IbuMethods::IbuCalculationParms const & parms) {
   qDebug() << Q_FUNC_INFO << "Using" << formulaDisplayNames[formula] << "with" << parms;
   switch(IbuMethods::formula) {
      case IbuMethods::IbuFormula::Tinseth: return tinseth(parms);
      case IbuMethods::IbuFormula::Rager  : return rager  (parms);
      case IbuMethods::IbuFormula::Noonan : return noonan (parms);
      case IbuMethods::IbuFormula::mIbu   : return mIbu   (parms);
//      case IbuMethods::IbuFormula::Smph   : return smph   (parms);
   }
   Q_UNREACHABLE();
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