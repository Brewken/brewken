/*======================================================================================================================
 * RefractoDialog.cpp is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Eric Tamme <etamme@gmail.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Théophane Martin <theophane.m@gmail.com>
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
#include "RefractoDialog.h"

#include <cmath>

#include <QDebug>
#include <QMessageBox>
#include <QString>

#include "Algorithms.h"
#include "measurement/Measurement.h"

RefractoDialog::RefractoDialog(QWidget* parent) : QDialog(parent) {
   setupUi(this);

   SMART_LINE_EDIT_INIT_FS_FIXED(RefractoDialog, lineEdit_op     , double, Measurement::PhysicalQuantity::Density      , 1); // Original Plato
   SMART_LINE_EDIT_INIT_FS_FIXED(RefractoDialog, lineEdit_inputOG, double, Measurement::PhysicalQuantity::Density      , 3); // Original gravity in
   SMART_LINE_EDIT_INIT_FS_FIXED(RefractoDialog, lineEdit_cp     , double, Measurement::PhysicalQuantity::Density      , 1); // Current Plato
   SMART_LINE_EDIT_INIT_FS_FIXED(RefractoDialog, lineEdit_ri     , double,           NonPhysicalQuantity::Dimensionless   ); // Refractive index
   SMART_LINE_EDIT_INIT_FS_FIXED(RefractoDialog, lineEdit_og     , double, Measurement::PhysicalQuantity::Density      , 3); // Original gravity out
   SMART_LINE_EDIT_INIT_FS_FIXED(RefractoDialog, lineEdit_sg     , double, Measurement::PhysicalQuantity::Density      , 3); // Specific gravity out
   SMART_LINE_EDIT_INIT_FS_FIXED(RefractoDialog, lineEdit_abv    , double,           NonPhysicalQuantity::Percentage      ); // Alcohol by volume
   SMART_LINE_EDIT_INIT_FS_FIXED(RefractoDialog, lineEdit_abw    , double,           NonPhysicalQuantity::Percentage      ); // Alcohol by weight
   SMART_LINE_EDIT_INIT_FS_FIXED(RefractoDialog, lineEdit_re     , double, Measurement::PhysicalQuantity::Density      , 1); // Real extract Plato

   this->lineEdit_op     ->getUiAmountWithUnits().setForcedSystemOfMeasurement(Measurement::SystemOfMeasurement::Plato          );
   this->lineEdit_inputOG->getUiAmountWithUnits().setForcedSystemOfMeasurement(Measurement::SystemOfMeasurement::SpecificGravity);
   this->lineEdit_cp     ->getUiAmountWithUnits().setForcedSystemOfMeasurement(Measurement::SystemOfMeasurement::Plato          );
   this->lineEdit_og     ->getUiAmountWithUnits().setForcedSystemOfMeasurement(Measurement::SystemOfMeasurement::SpecificGravity);
   this->lineEdit_sg     ->getUiAmountWithUnits().setForcedSystemOfMeasurement(Measurement::SystemOfMeasurement::SpecificGravity);
   this->lineEdit_re     ->getUiAmountWithUnits().setForcedSystemOfMeasurement(Measurement::SystemOfMeasurement::Plato          );

   connect(this->pushButton_calculate, &QAbstractButton::clicked, this, &RefractoDialog::calculate );
   return;
}

RefractoDialog::~RefractoDialog() = default;

void RefractoDialog::calculate() {
   bool haveCP = true;
   bool haveOP = true;
   bool haveOG = true;

   // User can enter in specific gravity or Plato, but the lineEdit is going to convert it to Plato, so we can just
   // grab the number
   double originalPlato = Measurement::extractRawFromString<double>(lineEdit_op     ->text(), &haveOP);
   double inputOG       = Measurement::extractRawFromString<double>(lineEdit_inputOG->text(), &haveOG);
   double currentPlato  = Measurement::extractRawFromString<double>(lineEdit_cp     ->text(), &haveCP);

   this->clearOutputFields();

   // Abort if we don't have the current plato.
   // I really dislike just doing nothing as the user is POUNDING on the
   // calculate button, waiting for something to happen. Maybe we should
   // provide some ... oh ... feedback that they are doing something wrong?
   if (!haveCP) {
      return;
   }

   double ri = Algorithms::refractiveIndex(currentPlato);
   this->lineEdit_ri->setText(Measurement::displayQuantity(ri, 3));

   if (!haveOG && haveOP) {
      inputOG = Algorithms::PlatoToSG_20C20C(originalPlato);
      this->lineEdit_inputOG->setAmount(inputOG);
   } else if (!haveOP && haveOG) {
      originalPlato = Algorithms::SG_20C20C_toPlato(inputOG);
      this->lineEdit_op->setAmount(inputOG);
   } else if (!haveOP && !haveOG) {
      qDebug() << Q_FUNC_INFO << "no plato or og";
      return; // Can't do much if we don't have OG or OP.
   }

   double og = Algorithms::PlatoToSG_20C20C( originalPlato );
   double sg = 0;
   if (originalPlato != currentPlato) {
     sg = Algorithms::sgByStartingPlato( originalPlato, currentPlato );
   } else {
     sg = og;
   }

   double re  = Algorithms::realExtract    (sg, currentPlato);
   double abv = Algorithms::getABVBySGPlato(sg, currentPlato);
   double abw = Algorithms::getABWBySGPlato(sg, currentPlato);

   // Warn the user if the inputOG and calculated og don't match.
   if( qAbs(og - inputOG) > 0.002 ) {
      QMessageBox::warning(
         this,
         tr("OG Mismatch"),
         tr("Based on the given original plato, the OG should be %1, but you have entered %2. "
            "Continuing with the calculated OG.").arg(og,0,'f',3).arg(inputOG,0,'f',3)
      );
   }

   this->lineEdit_og->setAmount(og);
   this->lineEdit_sg->setAmount(sg);
   // Even if the real extract if display in Plato, it must be given in system unit.
   // Conversion is made by SmartLineEdit
   this->lineEdit_re ->setAmount(Algorithms::PlatoToSG_20C20C(re));
   this->lineEdit_abv->setAmount(abv);
   this->lineEdit_abw->setAmount(abw);
   return;
}

void RefractoDialog::clearOutputFields() {
   this->lineEdit_ri ->clear();
   this->lineEdit_og ->clear();
   this->lineEdit_sg ->clear();
   this->lineEdit_re ->clear();
   this->lineEdit_abv->clear();
   this->lineEdit_abw->clear();
   return;
}
