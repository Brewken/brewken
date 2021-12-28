/*======================================================================================================================
 * PrimingDialog.cpp is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Brian Rower <brian.rower@gmail.com>
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
#include "PrimingDialog.h"

#include <cmath>

#include "Brewken.h"
#include "measurement/Unit.h"

PrimingDialog::PrimingDialog(QWidget* parent) : QDialog(parent) {
   setupUi(this);

   sugarGroup = new QButtonGroup(this);
   sugarGroup->setExclusive(true); // Can select only one.

   sugarGroup->addButton(radioButton_glucMono);
   sugarGroup->addButton(radioButton_gluc);
   sugarGroup->addButton(radioButton_sucrose);
   sugarGroup->addButton(radioButton_dme);

   connect( pushButton_calculate, &QAbstractButton::clicked, this, &PrimingDialog::calculate );
   return;
}

PrimingDialog::~PrimingDialog() = default;

void PrimingDialog::calculate()
{
   QAbstractButton* button;

   double beer_l;
   double temp_c;
   double desiredVols;

   double addedVols;
   double residualVols;

   double co2_l;
   double co2_mol;

   double sugar_mol;
   double sugar_g;

   beer_l = lineEdit_beerVol->toSI();
   temp_c = lineEdit_temp->toSI();
   desiredVols = lineEdit_vols->toSI();

   residualVols = 1.57 * pow( 0.97, temp_c ); // Amount of CO2 still in suspension.
   addedVols = desiredVols - residualVols;
   co2_l = addedVols * beer_l; // Liters of CO2 we need to generate (at 273 K and 1 atm).
   co2_mol = co2_l / 22.4; // Mols of CO2 we need.

   button = sugarGroup->checkedButton();

   if( button == radioButton_glucMono )
   {
      sugar_mol = co2_mol / 2;
      sugar_g = sugar_mol * 198; // Glucose monohydrate is 198 g/mol.
   }
   else if( button == radioButton_gluc )
   {
      sugar_mol = co2_mol / 2;
      sugar_g = sugar_mol * 180; // Glucose is 180g/mol.
   }
   else if( button == radioButton_sucrose )
   {
      sugar_mol = co2_mol / 4;
      sugar_g = sugar_mol * 342; // Sucrose is 342 g/mol.
   }
   else if( button == radioButton_dme )
   {
      sugar_mol = co2_mol / 2;
      sugar_g = sugar_mol * 180 / 0.60; // DME is equivalently about 60% glucose.
   }
   else
      sugar_g = 0;

   //The amount have to be set in default unit to BtLineEdit.
   //We should find a better solution, but until it is not, we must do it this way.
   lineEdit_output->setText( sugar_g/1000 );
}
