/*======================================================================================================================
 * UiAmountWithUnits.cpp is part of Brewken, and is copyright the following authors 2009-2022:
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

#include <QDebug>
#include <QVariant>
#include <QWidget>

#include "Localization.h"
#include "measurement/Measurement.h"
#include "utils/OptionalToStream.h"

UiAmountWithUnits::UiAmountWithUnits(QWidget * parent,
                                     BtFieldType fieldType,
                                     Measurement::Unit const * units) :
   parent{parent},
   fieldType{fieldType},
   units{units},
   forcedSystemOfMeasurement{std::nullopt},
   forcedRelativeScale(std::nullopt) {
   return;
}

UiAmountWithUnits::~UiAmountWithUnits() = default;

void UiAmountWithUnits::setForcedSystemOfMeasurement(std::optional<Measurement::SystemOfMeasurement> systemOfMeasurement) {
   this->forcedSystemOfMeasurement = systemOfMeasurement;
   return;
}

std::optional<Measurement::SystemOfMeasurement> UiAmountWithUnits::getForcedSystemOfMeasurement() const {
   return this->forcedSystemOfMeasurement;
}

void UiAmountWithUnits::setForcedSystemOfMeasurementViaString(QString systemOfMeasurementAsString) {
   this->forcedSystemOfMeasurement = Measurement::getFromUniqueName(systemOfMeasurementAsString);
   return;
}

QString UiAmountWithUnits::getForcedSystemOfMeasurementViaString() const {
   return this->forcedSystemOfMeasurement ? Measurement::getUniqueName(*this->forcedSystemOfMeasurement) : "";
}

void UiAmountWithUnits::setForcedRelativeScale(std::optional<Measurement::UnitSystem::RelativeScale> relativeScale) {
   this->forcedRelativeScale = relativeScale;
   return;
}

std::optional<Measurement::UnitSystem::RelativeScale> UiAmountWithUnits::getForcedRelativeScale() const {
   return this->forcedRelativeScale;
}

void UiAmountWithUnits::setForcedRelativeScaleViaString(QString relativeScaleAsString) {
   if (relativeScaleAsString == "") {
      this->forcedRelativeScale = std::nullopt;
   } else {
      this->forcedRelativeScale = Measurement::UnitSystem::getScaleFromUniqueName(relativeScaleAsString);
   }
   return;
}

QString UiAmountWithUnits::getForcedRelativeScaleViaString() const {
   return this->forcedRelativeScale ? Measurement::UnitSystem::getUniqueName(*this->forcedRelativeScale) : "";
}

void UiAmountWithUnits::setEditField(QString editField) {
   this->editField = editField;
   return;
}

QString UiAmountWithUnits::getEditField() const {
   return this->editField;
}

void UiAmountWithUnits::setConfigSection(QString configSection) {
   // The cascade looks a little odd, but it is intentional.
   this->configSection = configSection;
   if (this->configSection.isEmpty()) {
      this->configSection = this->parent->property("configSection").toString();
   }
   if (this->configSection.isEmpty()) {
      this->configSection = this->parent->objectName();
   }
   return;
}

QString UiAmountWithUnits::getConfigSection() {
   if (this->configSection.isEmpty()) {
      // Setting the config section to blank will actually attempt to populate it with default values -- see
      // UiAmountWithUnits::setConfigSection()
      this->setConfigSection("");
   }

   return this->configSection;
}

/*void UiAmountWithUnits::setType(int type) {
   // .:TBD:. Why do we need to pass in int and then cast?  Why not pass PhysicalQuantity?
   this->physicalQuantity = static_cast<Measurement::PhysicalQuantity>(type);
   return;
}*/

/*int UiAmountWithUnits::type() const {
   // .:TBD:. Why can't we just return PhysicalQuantity?
   return static_cast<int>(this->physicalQuantity);
}*/

double UiAmountWithUnits::toDoubleRaw(bool * ok) const {
   if (ok) {
      *ok = true;
   }

   // Make sure we get the right decimal point (. or ,) and the right grouping
   // separator (, or .). Some locales write 1.000,10 and other write
   // 1,000.10. We need to catch both
   QString decimal = QRegExp::escape(QLocale::system().decimalPoint());
   QString grouping = QRegExp::escape(QLocale::system().groupSeparator());

   QRegExp amtUnit;
   amtUnit.setPattern("((?:\\d+" + grouping + ")?\\d+(?:" + decimal + "\\d+)?|" + decimal + "\\d+)\\s*(\\w+)?");
   amtUnit.setCaseSensitivity(Qt::CaseInsensitive);

   // if the regex dies, return 0.0
   if (amtUnit.indexIn(this->getWidgetText()) == -1) {
      if (ok) {
         *ok = false;
      }
      return 0.0;
   }

   return Localization::toDouble(amtUnit.cap(1), Q_FUNC_INFO);
}

double UiAmountWithUnits::toSiRaw() {
   // It's a coding error to call this function if we are not dealing with a physical quantity
   Q_ASSERT(std::holds_alternative<Measurement::PhysicalQuantity>(this->fieldType));
   Q_ASSERT(this->units);

   bool ok = false;
   double amt = this->toDoubleRaw(&ok);
   if (!ok) {
      qWarning() <<
         Q_FUNC_INFO << "Could not convert " << this->getWidgetText() << " (" << this->configSection << ":" <<
         this->editField << ") to double";
   }
   return this->units->toSI(amt);
}


QString UiAmountWithUnits::displayAmount(double amount, int precision) {
   // Our overrides, if any, take precedence over others, if any
   std::optional<Measurement::SystemOfMeasurement> overrideSystemOfMeasurement =
      this->forcedSystemOfMeasurement ? this->forcedSystemOfMeasurement :
                                        Measurement::getForcedSystemOfMeasurementForField(this->editField,
                                                                                          this->configSection);
   std::optional<Measurement::UnitSystem::RelativeScale> overrideRelativeScale =
      this->forcedRelativeScale ? this->forcedRelativeScale :
                                  Measurement::getForcedRelativeScaleForField(this->editField,
                                                                              this->configSection);

   // I find this a nice level of abstraction. This lets all of the setText()
   // methods make a single call w/o having to do the logic for finding the
   // unit and scale.
   return Measurement::displayAmount(amount,
                                     this->units,
                                     precision,
                                     overrideSystemOfMeasurement,
                                     overrideRelativeScale);
}

void UiAmountWithUnits::textOrUnitsChanged(PreviousScaleInfo previousScaleInfo) {
   // This is where it gets hard
   double val = -1.0;
   QString amt;

   if (this->getWidgetText().isEmpty()) {
      return;
   }

   if (!std::holds_alternative<Measurement::PhysicalQuantity>(this->fieldType) ||
       Measurement::PhysicalQuantity::None == std::get<Measurement::PhysicalQuantity>(this->fieldType)) {
      amt = this->getWidgetText();
   } else {

      Measurement::PhysicalQuantity physicalQuantity = std::get<Measurement::PhysicalQuantity>(this->fieldType);

      // The idea here is we need to first translate the field into a known
      // amount (aka to SI) and then into the unit we want.
      switch(physicalQuantity) {
         case Measurement::PhysicalQuantity::Mass:
         case Measurement::PhysicalQuantity::Volume:
         case Measurement::PhysicalQuantity::Temperature:
         case Measurement::PhysicalQuantity::Time:
         case Measurement::PhysicalQuantity::Density:
         case Measurement::PhysicalQuantity::DiastaticPower:
            val = this->convertToSI(previousScaleInfo);
            amt = this->displayAmount(val, 3);
            break;
         case Measurement::PhysicalQuantity::Color:
            val = this->convertToSI(previousScaleInfo);
            amt = this->displayAmount(val, 0);
            break;
         case Measurement::PhysicalQuantity::None:
         default:
            {
               bool ok = false;
               val = Localization::toDouble(this->getWidgetText(), &ok);
               if (!ok) {
                  qWarning() <<
                     Q_FUNC_INFO << " failed to convert" << this->getWidgetText() << "(" << this->configSection << ":" <<
                     this->editField << ") to double";
               }
               amt = displayAmount(val);
            }
      }
   }
   qDebug() << Q_FUNC_INFO << "Interpreted" << this->getWidgetText() << "as" << amt;
   this->setWidgetText(amt);
/////////
   // The idea here is we need to first translate the field into a known
   // amount (aka to SI) and then into the unit we want.
/*   QString amt;
   switch (this->m_type) {
      case Measurement::Unit::Mass:
         amt = this->displayAmount(this->m_lastNum, 2);
         break;

      case Measurement::Unit::String:
         amt = this->text();
         break;

      case Measurement::Unit::None:
      default:
         {
            bool ok = false;
            double val = Localization::toDouble(this->text(), &ok);
            if (!ok) {
               qWarning() <<
                  Q_FUNC_INFO << "failed to convert " << this->text() << "(" << this->m_section << ":" <<
                  this->m_editField << ") to double";
            }
            amt = this->displayAmount(val);
         }
         break;
   }*/

   ////////
   return;
}

double UiAmountWithUnits::convertToSI(PreviousScaleInfo previousScaleInfo) {
   QString rawValue = this->getWidgetText();
   qDebug() <<
      Q_FUNC_INFO << "rawValue:" << rawValue <<  ", old SystemOfMeasurement:" << previousScaleInfo.oldSystemOfMeasurement <<
      ", old ForcedScale: " << previousScaleInfo.oldForcedScale;

   // It is a coding error if this function is called for a field is not holding a physical quantity.  Eg, it makes no
   // sense to call convertToSI for a date or a string field, not least as there will be no sane value to supply for
   // oldSystemOfMeasurement.
   Q_ASSERT(std::holds_alternative<Measurement::PhysicalQuantity>(this->fieldType));
   Measurement::PhysicalQuantity physicalQuantity = std::get<Measurement::PhysicalQuantity>(this->fieldType);

   // .:TBD:. 2021-12-31 MY: My gut instinct is that the logic here is more complicated than it needs to be.  It would
   //                        be nice to see if we can add some unit tests for all the edge cases and then simplify.

   Measurement::UnitSystem const & oldUnitSystem = Measurement::UnitSystem::getInstance(previousScaleInfo.oldSystemOfMeasurement,
                                                                                        physicalQuantity);

   Measurement::Unit const * defaultUnit{
      previousScaleInfo.oldForcedScale ? oldUnitSystem.scaleUnit(*previousScaleInfo.oldForcedScale) : oldUnitSystem.unit()
   };

   //
   // Measurement::UnitSystem::qStringToSI will handle all the logic to deal with any units specified by the user in the
   // string.  (In theory, we just grab the units that the user has specified in the input text.  In reality, it's not
   // that easy as we sometimes need to disambiguate - eg between Imperial gallons and US customary ones.  So, if we
   // have old or current units then that helps with this - eg, if current units are US customary cups and user enters
   // gallons, then we'll go with US customary gallons over Imperial ones.)
   //
   return oldUnitSystem.qstringToSI(rawValue, defaultUnit, previousScaleInfo.oldForcedScale);

/*
   //
   // Normally, we display units with the text.  If the user just edits the number, then the units will still be there.
   // Alternatively, if the user specifies different units in the text, we should try to honour those.  Otherwise, if,
   // no units are specified in the text, we need to go to defaults.  Defaults are either what is "forced" for this
   // specific field or, failing that, what is configured globally.
   //
   if (Localization::hasUnits(rawValue)) {
      //
      // User has specified units
      //
      // In theory, we just grab the units that the user has specified in the input text.  In reality, it's not that
      // easy as we sometimes need to disambiguate - eg between Imperial gallons and US customary ones.  So, if we have
      // old or current units then that helps with this - eg, if current units are US customary cups and user enters
      // gallons, then we'll go with US customary gallons over Imperial ones.  (The logic to handle this is all inside
      // Measurement::UnitSystem::qStringToSI
      //
      if (nullptr == dspUnitSystem) {
         if (nullptr != this->units) {
            dspUnitSystem = &this->units->getUnitSystem();
         }
      }
   } else {
      // If the display unit system is forced, use this as the default one.
      if (this->forcedUnitSystem != nullptr) {
         dspUnitSystem = this->forcedUnitSystem;
         qDebug() << Q_FUNC_INFO << "Forced unit system:" << dspUnitSystem->uniqueName;
      } else {
         dspUnitSystem = Measurement::getUnitSystemForField(this->editField, this->configSection);
         qDebug() <<
            Q_FUNC_INFO << "Unit system for field:" << (nullptr == dspUnitSystem ? "NULL" : dspUnitSystem->uniqueName);
      }

      // If the display scale is forced, use this scale as the default one.
//      if (this->forcedRelativeScale != Measurement::UnitSystem::noScale) {
//         dspScale = this->forcedRelativeScale;
//      } else {
         dspScale = Measurement::getRelativeScaleForField(this->editField, this->configSection);
//      }
   }

   if (nullptr != dspUnitSystem) {
      Measurement::Unit const * works = dspUnitSystem->scaleUnit(dspScale);
      if (!works) {
         // If we didn't find the unit, default to the UnitSystem's default
         // unit
         works = dspUnitSystem->unit();
      }

      return dspUnitSystem->qstringToSI(rawValue, works, dspScale);
   }

/*   if (this->physicalQuantity == Measurement::PhysicalQuantity::String) {
      return 0.0;
   }*/

//      return Measurement::qStringToSI(this->text(), this->physicalQuantity, dspUnitSystem, dspScale)

   // If all else fails, simply try to force the contents of the field to a
   // double. This doesn't seem advisable?
/*   bool ok = false;
   double amt = this->toDouble(&ok);
   if (!ok) {
      qWarning() <<
         Q_FUNC_INFO << "Could not convert " << rawValue << " (" << this->configSection << ":" <<
         this->editField << ") to double";
   }

   return amt;*/
}
