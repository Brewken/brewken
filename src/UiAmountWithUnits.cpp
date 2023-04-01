/*======================================================================================================================
 * UiAmountWithUnits.cpp is part of Brewken, and is copyright the following authors 2009-2023:
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
#include "UiAmountWithUnits.h"

#include <tuple>

#include <QDebug>
#include <QVariant>
#include <QWidget>

#include "Localization.h"
#include "measurement/Measurement.h"
#include "utils/OptionalHelpers.h"

// This private implementation class holds all private non-virtual members of UiAmountWithUnits
class UiAmountWithUnits::impl {
public:
   impl(UiAmountWithUnits & self,
        QWidget const * const parent,
        Measurement::PhysicalQuantities const physicalQuantities) :
      m_self                     {self},
      m_parent                   {parent},
      m_allowedPhysicalQuantities{physicalQuantities},
      m_currentPhysicalQuantity  {
         // If the field supports more than one PhysicalQuantity (eg PqEitherMassOrVolume or
         // PqEitherMassOrVolumeConcentration), our starting assumption is that we hold the second one (eg Volume or
         // VolumeConcentration).  Currently this matters because the assumption is baked into the UI of MiscEditor,
         // but we should change that at some point.
         std::holds_alternative<Measurement::PhysicalQuantity>(physicalQuantities) ?
            std::get<Measurement::PhysicalQuantity>(physicalQuantities) :
            std::get<1>(std::get<Measurement::Mixed2PhysicalQuantities>(physicalQuantities))
      } {
      return;
   }

   ~impl() = default;

   /**
    * \brief Returns the contents of the field converted, if necessary, to SI units
    *
    * \param enteredText
    * \param previousScaleInfo
    */
   Measurement::Amount toCanonical(QString const & enteredText, PreviousScaleInfo previousScaleInfo) {
      qDebug() <<
         Q_FUNC_INFO << "enteredText:" << enteredText <<  ", old SystemOfMeasurement:" <<
         previousScaleInfo.oldSystemOfMeasurement << ", old ForcedScale: " << previousScaleInfo.oldForcedScale;

      Measurement::UnitSystem const & oldUnitSystem =
         Measurement::UnitSystem::getInstance(previousScaleInfo.oldSystemOfMeasurement,
                                              this->m_currentPhysicalQuantity);

      Measurement::Unit const * defaultUnit{
         previousScaleInfo.oldForcedScale ? oldUnitSystem.scaleUnit(*previousScaleInfo.oldForcedScale) : oldUnitSystem.unit()
      };

      // It's a coding error if defaultUnit is null, because it means previousScaleInfo.oldForcedScale was not valid for
      // oldUnitSystem.  However, we can recover.
      if (!defaultUnit) {
         qWarning() << Q_FUNC_INFO << "previousScaleInfo.oldForcedScale invalid?" << previousScaleInfo.oldForcedScale;
         defaultUnit = oldUnitSystem.unit();
      }

      //
      // Normally, we display units with the text.  If the user just edits the number, then the units will still be there.
      // Alternatively, if the user specifies different units in the text, we should try to honour those.  Otherwise, if,
      // no units are specified in the text, we need to go to defaults.  Defaults are either what is "forced" for this
      // specific field or, failing that, what is configured globally.
      //
      // Measurement::UnitSystem::qStringToSI will handle all the logic to deal with any units specified by the user in the
      // string.  (In theory, we just grab the units that the user has specified in the input text.  In reality, it's not
      // that easy as we sometimes need to disambiguate - eg between Imperial gallons and US customary ones.  So, if we
      // have old or current units then that helps with this - eg, if current units are US customary cups and user enters
      // gallons, then we'll go with US customary gallons over Imperial ones.)
      //
      auto amount = oldUnitSystem.qstringToSI(enteredText, *defaultUnit);
      qDebug() << Q_FUNC_INFO << "Converted to" << amount;
      return amount;
   }

   UiAmountWithUnits &                   m_self;
   QWidget const * const                 m_parent;
   Measurement::PhysicalQuantities const m_allowedPhysicalQuantities;
   Measurement::PhysicalQuantity         m_currentPhysicalQuantity;
   QString                               m_editField;
   QString                               m_configSection;

};


UiAmountWithUnits::UiAmountWithUnits(QWidget const * const parent,
                                     Measurement::PhysicalQuantities const physicalQuantities) :
   pimpl{std::make_unique<impl>(*this, parent, physicalQuantities)} {
   return;
}

UiAmountWithUnits::~UiAmountWithUnits() = default;

Measurement::PhysicalQuantity UiAmountWithUnits::getPhysicalQuantity() const {
   return this->pimpl->m_currentPhysicalQuantity;
}

void UiAmountWithUnits::selectPhysicalQuantity(Measurement::PhysicalQuantity const physicalQuantity) {
   // It's a coding error to call this if we only hold one PhysicalQuantity
   Q_ASSERT(!std::holds_alternative<Measurement::PhysicalQuantity>(this->pimpl->m_allowedPhysicalQuantities));

   // It's a coding error to try to select a PhysicalQuantity that was not specified in the constructor
   auto const & tupleOfPqs = std::get<Measurement::Mixed2PhysicalQuantities>(this->pimpl->m_allowedPhysicalQuantities);
   Q_ASSERT(std::get<0>(tupleOfPqs) == physicalQuantity ||
            std::get<1>(tupleOfPqs) == physicalQuantity);

   this->pimpl->m_currentPhysicalQuantity = physicalQuantity;
   return;
}

void UiAmountWithUnits::setForcedSystemOfMeasurement(std::optional<Measurement::SystemOfMeasurement> systemOfMeasurement) {
   Measurement::setForcedSystemOfMeasurementForField(this->pimpl->m_editField,
                                                     this->pimpl->m_configSection,
                                                     systemOfMeasurement);
   return;
}

std::optional<Measurement::SystemOfMeasurement> UiAmountWithUnits::getForcedSystemOfMeasurement() const {
   return Measurement::getForcedSystemOfMeasurementForField(this->pimpl->m_editField, this->pimpl->m_configSection);
}

void UiAmountWithUnits::setForcedSystemOfMeasurementViaString(QString systemOfMeasurementAsString) {
   qDebug() <<
      Q_FUNC_INFO << "Measurement system" << systemOfMeasurementAsString << "for" << this->pimpl->m_configSection <<
      ">" << this->pimpl->m_editField;
   this->setForcedSystemOfMeasurement(Measurement::getFromUniqueName(systemOfMeasurementAsString));
   return;
}

QString UiAmountWithUnits::getForcedSystemOfMeasurementViaString() const {
   auto forcedSystemOfMeasurement = this->getForcedSystemOfMeasurement();
   return forcedSystemOfMeasurement ? Measurement::getUniqueName(*forcedSystemOfMeasurement) : "";
}

void UiAmountWithUnits::setForcedRelativeScale(std::optional<Measurement::UnitSystem::RelativeScale> relativeScale) {
   Measurement::setForcedRelativeScaleForField(this->pimpl->m_editField, this->pimpl->m_configSection, relativeScale);
   return;
}

std::optional<Measurement::UnitSystem::RelativeScale> UiAmountWithUnits::getForcedRelativeScale() const {
   return Measurement::getForcedRelativeScaleForField(this->pimpl->m_editField, this->pimpl->m_configSection);
}

void UiAmountWithUnits::setForcedRelativeScaleViaString(QString relativeScaleAsString) {
   qDebug() <<
      Q_FUNC_INFO << "Scale" << relativeScaleAsString << "for" << this->pimpl->m_configSection << ">" <<
      this->pimpl->m_editField;
   this->setForcedRelativeScale(Measurement::UnitSystem::getScaleFromUniqueName(relativeScaleAsString));
   return;
}

QString UiAmountWithUnits::getForcedRelativeScaleViaString() const {
   auto forcedRelativeScale = this->getForcedRelativeScale();
   return forcedRelativeScale ? Measurement::UnitSystem::getUniqueName(*forcedRelativeScale) : "";
}

void UiAmountWithUnits::setEditField(QString editField) {
   this->pimpl->m_editField = editField;
   return;
}

QString UiAmountWithUnits::getEditField() const {
   return this->pimpl->m_editField;
}

void UiAmountWithUnits::setConfigSection(QString configSection) {
   // The cascade looks a little odd, but it is intentional.
   this->pimpl->m_configSection = configSection;
   if (this->pimpl->m_configSection.isEmpty()) {
      this->pimpl->m_configSection =
         this->pimpl->m_parent->property(*PropertyNames::UiAmountWithUnits::configSection).toString();
   }
   if (this->pimpl->m_configSection.isEmpty()) {
      this->pimpl->m_configSection = this->pimpl->m_parent->objectName();
   }
   return;
}

QString UiAmountWithUnits::getConfigSection() {
   if (this->pimpl->m_configSection.isEmpty()) {
      // Setting the config section to blank will actually attempt to populate it with default values -- see
      // UiAmountWithUnits::setConfigSection()
      this->setConfigSection("");
   }

   return this->pimpl->m_configSection;
}

///double UiAmountWithUnits::toDoubleRaw(bool * ok) const {
///   return Measurement::extractRawFromString<double>(this->getWidgetText(), ok);
///}

Measurement::Amount UiAmountWithUnits::rawToCanonical(QString const & rawValue) const {
   return Measurement::qStringToSI(rawValue,
                                   this->pimpl->m_currentPhysicalQuantity,
                                   this->getForcedSystemOfMeasurement(),
                                   this->getForcedRelativeScale());
}

QString UiAmountWithUnits::displayAmount(double amount, int precision) const {
   // I find this a nice level of abstraction. This lets all of the setText()
   // methods make a single call w/o having to do the logic for finding the
   // unit and scale.
   return Measurement::displayAmount(
      Measurement::Amount{amount, Measurement::Unit::getCanonicalUnit(this->pimpl->m_currentPhysicalQuantity)},
      precision,
      this->getForcedSystemOfMeasurement(),
      this->getForcedRelativeScale()
   );
}

QString UiAmountWithUnits::correctEnteredText(QString const & enteredText,
                                              int precision,
                                              PreviousScaleInfo previousScaleInfo) {
   QString correctedText;

   qDebug() << Q_FUNC_INFO << "enteredText:" << enteredText;

   if (enteredText.isEmpty()) {
      return enteredText;
   }

   // The idea here is we need to first translate the field into a known
   // amount (aka to SI) and then into the unit we want.
   Measurement::Amount amountAsCanonical = this->pimpl->toCanonical(enteredText, previousScaleInfo);

   correctedText = this->displayAmount(amountAsCanonical.quantity(), precision);
   qDebug() <<
      Q_FUNC_INFO << "Interpreted" << enteredText << "as" << amountAsCanonical << "and corrected to" << correctedText <<
      "(Edit Field =" << this->pimpl->m_editField << "Config Section =" << this->pimpl->m_configSection << ")";

   return correctedText;
}

///void UiAmountWithUnits::textOrUnitsChanged(PreviousScaleInfo previousScaleInfo) {
///   // This is where it gets hard
///   //
///   // We may need to fix the text that the user entered, eg if this field is set to show US Customary volumes and user
///   // enters an amount in litres then we need to convert it to display in pints or quarts etc.
///   QString correctedText;
///
///   QString rawValue = this->getWidgetText();
///   qDebug() << Q_FUNC_INFO << "rawValue:" << rawValue;
///
///   if (rawValue.isEmpty()) {
///      return;
///   }
///
///   // The idea here is we need to first translate the field into a known
///   // amount (aka to SI) and then into the unit we want.
///   Measurement::Amount amountAsCanonical = this->convertToSI(previousScaleInfo);
///
///   Measurement::PhysicalQuantity physicalQuantity = this->getPhysicalQuantity();
///   int precision = 3;
///   if (physicalQuantity == Measurement::PhysicalQuantity::Color) {
///      precision = 0;
///   }
///   correctedText = this->displayAmount(amountAsCanonical.quantity(), precision);
///   qDebug() <<
///      Q_FUNC_INFO << "Interpreted" << rawValue << "as" << amountAsCanonical << "and corrected to" << correctedText;
///
///   this->setWidgetText(correctedText);
///   return;
///}

///Measurement::Amount UiAmountWithUnits::convertToSI(PreviousScaleInfo previousScaleInfo) {
///   QString rawValue = this->getWidgetText();
///   qDebug() <<
///      Q_FUNC_INFO << "rawValue:" << rawValue <<  ", old SystemOfMeasurement:" <<
///      previousScaleInfo.oldSystemOfMeasurement << ", old ForcedScale: " << previousScaleInfo.oldForcedScale;
///
///   Measurement::UnitSystem const & oldUnitSystem =
///      Measurement::UnitSystem::getInstance(previousScaleInfo.oldSystemOfMeasurement, this->pimpl->m_currentPhysicalQuantity);
///
///   Measurement::Unit const * defaultUnit{
///      previousScaleInfo.oldForcedScale ? oldUnitSystem.scaleUnit(*previousScaleInfo.oldForcedScale) : oldUnitSystem.unit()
///   };
///
///   // It's a coding error if defaultUnit is null, because it means previousScaleInfo.oldForcedScale was not valid for
///   // oldUnitSystem.  However, we can recover.
///   if (!defaultUnit) {
///      qWarning() << Q_FUNC_INFO << "previousScaleInfo.oldForcedScale invalid?" << previousScaleInfo.oldForcedScale;
///      defaultUnit = oldUnitSystem.unit();
///   }
///
///   //
///   // Normally, we display units with the text.  If the user just edits the number, then the units will still be there.
///   // Alternatively, if the user specifies different units in the text, we should try to honour those.  Otherwise, if,
///   // no units are specified in the text, we need to go to defaults.  Defaults are either what is "forced" for this
///   // specific field or, failing that, what is configured globally.
///   //
///   // Measurement::UnitSystem::qStringToSI will handle all the logic to deal with any units specified by the user in the
///   // string.  (In theory, we just grab the units that the user has specified in the input text.  In reality, it's not
///   // that easy as we sometimes need to disambiguate - eg between Imperial gallons and US customary ones.  So, if we
///   // have old or current units then that helps with this - eg, if current units are US customary cups and user enters
///   // gallons, then we'll go with US customary gallons over Imperial ones.)
///   //
///   auto amount = oldUnitSystem.qstringToSI(rawValue, *defaultUnit);
///   qDebug() << Q_FUNC_INFO << "Converted to" << amount;
///   return amount;
///}
