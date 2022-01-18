/*======================================================================================================================
 * StrikeWaterDialog.cpp is part of Brewken, and is copyright the following authors 2009-2014:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Maxime Lavigne <duguigne@gmail.com>
 *   • Mik Firestone <mikfire@gmail.com>
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
#include "StrikeWaterDialog.h"

#include <limits>

#include <Algorithms.h>

// From Northern Brewer ~0.38 but Jon Palmer suggest 0.41
// to compensate for the lost to the tun even if the tun is pre-heaten
const double StrikeWaterDialog::specificHeatBarley = 0.41;

StrikeWaterDialog::StrikeWaterDialog(QWidget* parent) : QDialog(parent)
{
   setupUi(this);
   connect(pushButton_calculate, &QAbstractButton::clicked, this, &StrikeWaterDialog::calculate);
}

StrikeWaterDialog::~StrikeWaterDialog() {}

void StrikeWaterDialog::calculate()
{
  double initial = computeInitialInfusion();
  double mash = computeMashInfusion();

  initialResultTxt->setText(initial);
  mashResultTxt->setText(mash);
}

double StrikeWaterDialog::computeInitialInfusion()
{
  double grainTemp   = grainTempVal->toSiRaw();
  double targetMash  = targetMashVal->toSiRaw();
  double waterVolume = waterVolumeVal->toSiRaw();
  double grainWeight = grainWeightInitVal->toSiRaw();

  if ( grainWeight == 0.0 )
     return 0.0;

  return initialInfusionSi( grainTemp, targetMash, waterVolume / grainWeight);
}

double StrikeWaterDialog::computeMashInfusion()
{
  double mashVol       = mashVolVal->toSiRaw();
  double grainWeight   = grainWeightVal->toSiRaw();
  double actualMash    = actualMashVal->toSiRaw();
  double targetMashInf = targetMashInfVal->toSiRaw();
  double infusionWater = infusionWaterVal->toSiRaw();

  return mashInfusionSi(actualMash, targetMashInf, grainWeight, infusionWater, mashVol);
}

double StrikeWaterDialog::initialInfusionSi(double grainTemp, double targetTemp, double waterToGrain)
{
   if ( waterToGrain == 0.0 )
      return 0.0;
   return (specificHeatBarley / waterToGrain) * (targetTemp - grainTemp) + targetTemp;
}
double StrikeWaterDialog::mashInfusionSi(double initialTemp, double targetTemp, double grainWeight, double infusionWater, double mashVolume)
{
   if ( infusionWater - targetTemp == 0.0 )
      return 0.0;

  return ((targetTemp - initialTemp) * (specificHeatBarley * grainWeight + mashVolume)) / (infusionWater - targetTemp);
}
