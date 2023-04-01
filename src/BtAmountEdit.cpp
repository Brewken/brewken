/*======================================================================================================================
 * BtAmountEdit.cpp is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Mark de Wever <koraq@xs4all.nl>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mike Evans <mikee@saxicola.co.uk>
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
#include "BtAmountEdit.h"

#include <QDebug>

#include "Localization.h"
#include "measurement/Measurement.h"
#include "model/NamedEntity.h"
#include "utils/OptionalHelpers.h"

BtAmountEdit::BtAmountEdit(QWidget *parent,
                           Measurement::PhysicalQuantities const physicalQuantities,
                           int const defaultPrecision,
                           QString const & maximalDisplayString) :
   BtLineEdit{parent,
              ConvertToBtFieldType(physicalQuantities),
              defaultPrecision,
              maximalDisplayString},
   UiAmountWithUnits{parent, physicalQuantities} {
   this->setConfigSection(this->property("configSection").toString());

   connect(this, &QLineEdit::editingFinished, this, &BtAmountEdit::onLineChanged);

   return;
}

BtAmountEdit::~BtAmountEdit() = default;

Measurement::Amount BtAmountEdit::toCanonical() const {
   return this->rawToCanonical(this->text());
}

void BtAmountEdit::setText(double amount) {
   this->setText(amount, this->defaultPrecision);
   return;
}

void BtAmountEdit::setText(double amount, int precision) {
   this->QLineEdit::setText(this->displayAmount(amount, precision));
   this->setDisplaySize();
   return;
}

void BtAmountEdit::setText(NamedEntity * element) {
   this->setText(element, this->defaultPrecision);
   return;
}

void BtAmountEdit::setText(NamedEntity * element, int precision) {
   QString const propertyName = this->getEditField();
   QVariant const propertyValue = element->property(propertyName.toLatin1().constData());
   qDebug() <<
      Q_FUNC_INFO << "Read property" << propertyName << "(" << propertyName << ") of" << *element << "as" <<
      propertyValue;

   bool force = false;
   QString display;
   if (propertyValue.canConvert(QVariant::Double)) {
      bool ok = false;
      // It is important here to use QVariant::toDouble() instead of going
      // through toString() and then Localization::toDouble().
      double amount = propertyValue.toDouble(&ok);
      if (!ok) {
         qWarning() <<
            Q_FUNC_INFO << "Could not convert " << propertyValue.toString() << " (" << this->getConfigSection() << ":" <<
            propertyName << ") to double";
      }

      display = this->displayAmount(amount, precision);
   } else {
      display = "?";
   }

   this->QLineEdit::setText(display);
   this->setDisplaySize(force);
   return;
}

void BtAmountEdit::setText(QString amount) {
   this->setText(amount, this->defaultPrecision);
   return;
}

void BtAmountEdit::setText(QString amount, int precision) {
   bool ok = false;
   double amt = Localization::toDouble(amount, &ok);
   if (!ok) {
      qWarning() <<
         Q_FUNC_INFO << "Could not convert" << amount << "(" << this->getConfigSection() << ":" << this->getEditField() <<
         ") to double";
   }
   this->QLineEdit::setText(this->displayAmount(amt, precision));

   this->setDisplaySize(false);
   return;
}

void BtAmountEdit::setText(QVariant amount) {
   this->setText(amount, this->defaultPrecision);
   return;
}

void BtAmountEdit::setText(QVariant amount, int precision) {
   this->setText(amount.toString(), precision);
   return;
}

void BtAmountEdit::onLineChanged() {
   auto const myFieldType = this->getFieldType();
   qDebug() <<
      Q_FUNC_INFO << "this->fieldType=" << myFieldType << ", this->m_canonicalUnits=" << this->m_canonicalUnits <<
      ", forcedSystemOfMeasurement=" << this->getForcedSystemOfMeasurement() << ", forcedRelativeScale=" <<
      this->getForcedRelativeScale() << ", value=" << this->text();

   Measurement::PhysicalQuantities const physicalQuantities = ConvertToPhysicalQuantities(myFieldType);

   QString const propertyName = this->getEditField();
   QString const configSection = this->getConfigSection();
   Measurement::SystemOfMeasurement const oldSystemOfMeasurement =
      Measurement::getSystemOfMeasurementForField(propertyName, configSection, physicalQuantities);
   auto oldForcedRelativeScale = Measurement::getForcedRelativeScaleForField(propertyName, configSection);
   PreviousScaleInfo previousScaleInfo{
      oldSystemOfMeasurement,
      oldForcedRelativeScale
   };

   qDebug() <<
      Q_FUNC_INFO << "oldSystemOfMeasurement=" << oldSystemOfMeasurement << ", oldForcedRelativeScale=" <<
      oldForcedRelativeScale;

   this->lineChanged(previousScaleInfo);
   return;
}

void BtAmountEdit::lineChanged(PreviousScaleInfo previousScaleInfo) {
   // editingFinished happens on focus being lost, regardless of anything
   // being changed. I am hoping this short circuits properly and we do
   // nothing if nothing changed.
   if (this->sender() == this && !isModified()) {
      qDebug() << Q_FUNC_INFO << "Nothing changed; field holds" << this->text();
      return;
   }

   this->QLineEdit::setText(this->correctEnteredText(this->text(), 3, previousScaleInfo));

   if (sender() == this) {
      emit textModified();
   }

   return;
}

BtMassEdit                ::BtMassEdit                (QWidget *parent) : BtAmountEdit(parent, Measurement::PhysicalQuantity::Mass                   ) { return; }
BtVolumeEdit              ::BtVolumeEdit              (QWidget *parent) : BtAmountEdit(parent, Measurement::PhysicalQuantity::Volume                 ) { return; }
BtTimeEdit                ::BtTimeEdit                (QWidget *parent) : BtAmountEdit(parent, Measurement::PhysicalQuantity::Time                , 3) { return; }
BtTemperatureEdit         ::BtTemperatureEdit         (QWidget *parent) : BtAmountEdit(parent, Measurement::PhysicalQuantity::Temperature         , 1) { return; }
BtColorEdit               ::BtColorEdit               (QWidget *parent) : BtAmountEdit(parent, Measurement::PhysicalQuantity::Color                  ) { return; }
BtDensityEdit             ::BtDensityEdit             (QWidget *parent) : BtAmountEdit(parent, Measurement::PhysicalQuantity::Density                ) { return; }
BtDiastaticPowerEdit      ::BtDiastaticPowerEdit      (QWidget *parent) : BtAmountEdit(parent, Measurement::PhysicalQuantity::DiastaticPower         ) { return; }
BtAcidityEdit             ::BtAcidityEdit             (QWidget *parent) : BtAmountEdit(parent, Measurement::PhysicalQuantity::Acidity                ) { return; }
BtBitternessEdit          ::BtBitternessEdit          (QWidget *parent) : BtAmountEdit(parent, Measurement::PhysicalQuantity::Bitterness             ) { return; }
BtCarbonationEdit         ::BtCarbonationEdit         (QWidget *parent) : BtAmountEdit(parent, Measurement::PhysicalQuantity::Carbonation            ) { return; }
BtMassConcentrationEdit   ::BtMassConcentrationEdit   (QWidget *parent) : BtAmountEdit(parent, Measurement::PhysicalQuantity::MassConcentration      ) { return; }
BtVolumeConcentrationEdit ::BtVolumeConcentrationEdit (QWidget *parent) : BtAmountEdit(parent, Measurement::PhysicalQuantity::VolumeConcentration    ) { return; }
BtViscosityEdit           ::BtViscosityEdit           (QWidget *parent) : BtAmountEdit(parent, Measurement::PhysicalQuantity::Viscosity              ) { return; }
BtSpecificHeatCapacityEdit::BtSpecificHeatCapacityEdit(QWidget *parent) : BtAmountEdit(parent, Measurement::PhysicalQuantity::SpecificHeatCapacity   ) { return; }
BtMixedMassOrVolumeEdit   ::BtMixedMassOrVolumeEdit   (QWidget *parent) : BtAmountEdit(parent, Measurement::PqEitherMassOrVolume                     ) { return; }

void BtMixedMassOrVolumeEdit::setIsWeight(bool state) {
   qDebug() << Q_FUNC_INFO << "state is" << state;
   // But you have to admit, this is clever
   this->selectPhysicalQuantity(state ? Measurement::PhysicalQuantity::Mass : Measurement::PhysicalQuantity::Volume);

   // maybe? My head hurts now
   this->onLineChanged();

   // Strictly, if we change a Misc to be measured by mass instead of volume (or vice versa) we should also somehow tell
   // any other bit of the UI that is showing that Misc (eg a RecipeEditor or MainWindow) to redisplay the relevant
   // field.  Currently we don't do this, on the assumption that it's rare you will change how a Misc is measured after
   // you started using it in recipes.
   return;
}
