/*======================================================================================================================
 * StrikeWaterDialog.cpp is part of Brewken, and is copyright the following authors 2009-2023:
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

namespace {
   // From Northern Brewer ~0.38 but Jon Palmer suggest 0.41
   // to compensate for the lost to the tun even if the tun is pre-heated
   double const specificHeatBarley = 0.41;

   /**
    * \brief
    */
   double initialInfusionSi(double grainTemp, double targetTemp, double waterToGrain) {
      if (waterToGrain == 0.0) {
         return 0.0;
      }
      return (specificHeatBarley / waterToGrain) * (targetTemp - grainTemp) + targetTemp;
   }

   /**
    * \brief
    */
   double mashInfusionSi(double initialTemp,
                         double targetTemp,
                         double grainWeight,
                         double infusionWater,
                         double mashVolume) {
      if (infusionWater - targetTemp == 0.0) {
         return 0.0;
      }

      return ((targetTemp - initialTemp) * (specificHeatBarley * grainWeight + mashVolume)) / (infusionWater - targetTemp);
   }

}

StrikeWaterDialog::StrikeWaterDialog(QWidget* parent) : QDialog(parent) {
   setupUi(this);

   // .:TBD:. These label and lineEdit fields could be slightly better named...
   SMART_FIELD_INIT_FS(StrikeWaterDialog, grainTempLbl      , grainTempVal      , double, Measurement::PhysicalQuantity::Temperature); // Initial Infusion: Original Grain Temperature
   SMART_FIELD_INIT_FS(StrikeWaterDialog, targetMashLbl     , targetMashVal     , double, Measurement::PhysicalQuantity::Temperature); // Initial Infusion: Target Mash Temperature
   SMART_FIELD_INIT_FS(StrikeWaterDialog, grainWeightInitLbl, grainWeightInitVal, double, Measurement::PhysicalQuantity::Mass       ); // Initial Infusion: Weight of Grain
   SMART_FIELD_INIT_FS(StrikeWaterDialog, waterVolumeLbl    , waterVolumeVal    , double, Measurement::PhysicalQuantity::Volume     ); // Initial Infusion: Volume of Water
   SMART_FIELD_INIT_FS(StrikeWaterDialog, mashVolLbl        , mashVolVal        , double, Measurement::PhysicalQuantity::Volume     ); // Mash Infusion: Total Volume of Water
   SMART_FIELD_INIT_FS(StrikeWaterDialog, grainWeightLbl    , grainWeightVal    , double, Measurement::PhysicalQuantity::Mass       ); // Mash Infusion: Grain Weight
   SMART_FIELD_INIT_FS(StrikeWaterDialog, actualMashLbl     , actualMashVal     , double, Measurement::PhysicalQuantity::Temperature); // Mash Infusion: Actual Mash Temperature
   SMART_FIELD_INIT_FS(StrikeWaterDialog, targetMashInfLbl  , targetMashInfVal  , double, Measurement::PhysicalQuantity::Temperature); // Mash Infusion: Target Mash Temperature
   SMART_FIELD_INIT_FS(StrikeWaterDialog, infusionWaterLbl  , infusionWaterVal  , double, Measurement::PhysicalQuantity::Temperature); // Mash Infusion: Infusion Water Temperature
   SMART_FIELD_INIT_FS(StrikeWaterDialog, initialResultLbl  , initialResultTxt  , double, Measurement::PhysicalQuantity::Temperature); // Result: Strike Water Temperature
   SMART_FIELD_INIT_FS(StrikeWaterDialog, mashResultLbl     , mashResultTxt     , double, Measurement::PhysicalQuantity::Volume     ); // Result: Volume to add

   connect(pushButton_calculate, &QAbstractButton::clicked, this, &StrikeWaterDialog::calculate);
   return;
}

StrikeWaterDialog::~StrikeWaterDialog() = default;

void StrikeWaterDialog::calculate() {
  double strikeWaterTemp = computeInitialInfusion();
  double volumeToAdd     = computeMashInfusion();

  this->initialResultTxt->setQuantity(strikeWaterTemp);
  this->mashResultTxt   ->setQuantity(volumeToAdd);
  return;
}

double StrikeWaterDialog::computeInitialInfusion() {
   double grainTemp   = this->grainTempVal      ->getNonOptCanonicalQty();
   double targetMash  = this->targetMashVal     ->getNonOptCanonicalQty();
   double waterVolume = this->waterVolumeVal    ->getNonOptCanonicalQty();
   double grainWeight = this->grainWeightInitVal->getNonOptCanonicalQty();

   if (grainWeight == 0.0) {
      return 0.0;
   }

   return initialInfusionSi(grainTemp, targetMash, waterVolume / grainWeight);
}

double StrikeWaterDialog::computeMashInfusion() {
   double mashVol       = this->mashVolVal      ->getNonOptCanonicalQty();
   double grainWeight   = this->grainWeightVal  ->getNonOptCanonicalQty();
   double actualMash    = this->actualMashVal   ->getNonOptCanonicalQty();
   double targetMashInf = this->targetMashInfVal->getNonOptCanonicalQty();
   double infusionWater = this->infusionWaterVal->getNonOptCanonicalQty();

   return mashInfusionSi(actualMash, targetMashInf, grainWeight, infusionWater, mashVol);
}
