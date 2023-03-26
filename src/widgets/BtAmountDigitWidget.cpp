/*======================================================================================================================
 * widgets/BtAmountDigitWidget.cpp is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
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
#include "widgets/BtAmountDigitWidget.h"

BtAmountDigitWidget::BtAmountDigitWidget(QWidget * parent,
                                         Measurement::PhysicalQuantities const physicalQuantities) :
   BtDigitWidget{parent, ConvertToBtFieldType(physicalQuantities)},
   UiAmountWithUnits(parent, physicalQuantities) {
   return;
}

BtAmountDigitWidget::~BtAmountDigitWidget() = default;

///QString BtAmountDigitWidget::getWidgetText() const {
///   return this->text();
///}
///
///void BtAmountDigitWidget::setWidgetText(QString text) {
///   this->QLabel::setText(text);
///   return;
///}

void BtAmountDigitWidget::displayChanged(PreviousScaleInfo previousScaleInfo) {
   this->QLabel::setText(this->correctEnteredText(this->text(), previousScaleInfo));
   return;
}

BtMassDigit::BtMassDigit(QWidget* parent) :
   BtAmountDigitWidget{parent, Measurement::PhysicalQuantity::Mass} {
   return;
}
