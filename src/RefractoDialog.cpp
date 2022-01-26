/*======================================================================================================================
 * RefractoDialog.cpp is part of Brewken, and is copyright the following authors 2009-2022:
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

   connect( pushButton_calculate, &QAbstractButton::clicked, this, &RefractoDialog::calculate );
   return;
}

RefractoDialog::~RefractoDialog() = default;

void RefractoDialog::calculate() {
   bool haveCP = true;
   bool haveOP = true;
   bool haveOG = true;

   double originalPlato = lineEdit_op->toDoubleRaw(&haveOP);
   double inputOG       = lineEdit_inputOG->toDoubleRaw(&haveOG);
   double currentPlato  = lineEdit_cp->toDoubleRaw(&haveCP);

   clearOutputFields();

   // Abort if we don't have the current plato.
   // I really dislike just doing nothing as the user is POUNDING on the
   // calculate button, waiting for something to happen. Maybe we should
   // provide some ... oh ... feedback that they are doing something wrong?
   if (!haveCP) {
      return;
   }

   double ri = Algorithms::refractiveIndex(currentPlato);
   lineEdit_ri->setText(Measurement::displayQuantity(ri, 3));

   if (!haveOG && haveOP) {
      inputOG = Algorithms::PlatoToSG_20C20C( originalPlato );
      lineEdit_inputOG->setText(inputOG);
   } else if (!haveOP && haveOG) {
      originalPlato = Algorithms::SG_20C20C_toPlato( inputOG );
      lineEdit_op->setText(inputOG);
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

   double re = Algorithms::realExtract( sg, currentPlato );
   double abv = Algorithms::getABVBySGPlato( sg, currentPlato );
   double abw = Algorithms::getABWBySGPlato( sg, currentPlato );

   // Warn the user if the inputOG and calculated og don't match.
   if( qAbs(og - inputOG) > 0.002 ) {
      QMessageBox::warning(
         this,
         tr("OG Mismatch"),
         tr("Based on the given original plato, the OG should be %1, but you have entered %2. "
            "Continuing with the calculated OG.").arg(og,0,'f',3).arg(inputOG,0,'f',3)
      );
   }

   lineEdit_og->setText(og);
   lineEdit_sg->setText(sg);
   //Even if the real extract if display in Plato, it must be given in system unit.
   //Conversion is made by BtLineEdit
   lineEdit_re->setText(Algorithms::PlatoToSG_20C20C(re));
   lineEdit_abv->setText(abv);
   lineEdit_abw->setText(abw);
   return;
}

void RefractoDialog::clearOutputFields() {
   lineEdit_ri->clear();
   lineEdit_og->clear();
   lineEdit_sg->clear();
   lineEdit_re->clear();
   lineEdit_abv->clear();
   lineEdit_abw->clear();
   return;
}
