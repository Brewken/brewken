/*======================================================================================================================
 * measurement/Measurement.cpp is part of Brewken, and is copyright the following authors 2010-2023:
 *   • Mark de Wever <koraq@xs4all.nl>
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
#include "measurement/Measurement.h"

#include <QDebug>
#include <QMap>
#include <QString>

#include "Algorithms.h"
#include "Localization.h"
#include "measurement/UnitSystem.h"
#include "model/NamedEntity.h"
#include "model/Style.h" // For PropertyNames::Style::colorMin_srm, PropertyNames::Style::colorMax_srm
#include "PersistentSettings.h"
#include "utils/BtStringConst.h"
#include "utils/OptionalHelpers.h"

namespace {

   int const fieldWidth = 0;
   char const format = 'f';

   /**
    * \brief Stores the current \c Measurement::UnitSystem being used for \b input and \b display for each
    *        \c Measurement::PhysicalQuantity.  Note that we always convert to a standard ("canonical")
    *        \c Measurement::Unit (usually Metric/SI where that's an option) for storing in the DB.
    */
   QMap<Measurement::PhysicalQuantity, Measurement::UnitSystem const *> physicalQuantityToDisplayUnitSystem;

   //
   // Load the previous stored setting for which UnitSystem we use for a particular physical quantity
   //
   void loadDisplayScale(Measurement::PhysicalQuantity const physicalQuantity,
                         BtStringConst const &               settingName,
                         Measurement::UnitSystem    const &  defaultUnitSystem) {
      QString unitSystemName = PersistentSettings::value(settingName, defaultUnitSystem.uniqueName).toString();
      Measurement::UnitSystem const * unitSystem = Measurement::UnitSystem::getInstanceByUniqueName(unitSystemName);
      if (nullptr == unitSystem) {
         qWarning() <<
            Q_FUNC_INFO << "Unrecognised unit system," << unitSystemName << "for" <<
            Measurement::getDisplayName(physicalQuantity) << ", defaulting to" << defaultUnitSystem.uniqueName << "(" <<
            Measurement::getDisplayName(defaultUnitSystem.systemOfMeasurement) << ")";
         unitSystem = &defaultUnitSystem;
      }
      Measurement::setDisplayUnitSystem(physicalQuantity, *unitSystem);
      return;
   }

   /**
    * \brief Given a string of number plus, optionally, some units or pseudo-units, extract the number (and ignore the
    *        units or pseudo-units)
    */
   double extractRawDoubleFromString(QString const & input, bool * ok) {
      if (ok) {
         *ok = true;
      }

      // Make sure we get the right decimal point (. or ,) and the right grouping
      // separator (, or .). Some locales write 1.000,10 and other write
      // 1,000.10. We need to catch both
      QString const decimal  = QRegExp::escape(Localization::getLocale().decimalPoint());
      QString const grouping = QRegExp::escape(Localization::getLocale().groupSeparator());

      QRegExp amtUnit;
      amtUnit.setPattern("((?:\\d+" + grouping + ")?\\d+(?:" + decimal + "\\d+)?|" + decimal + "\\d+)\\s*(\\w+)?");
      amtUnit.setCaseSensitivity(Qt::CaseInsensitive);

      // If the regex dies, return 0.0
      if (amtUnit.indexIn(input) == -1) {
         if (ok) {
            *ok = false;
            qWarning() << Q_FUNC_INFO << "Error parsing" << input << "as number";
         }
         return 0.0;
      }

      QString numericPartOfInput = amtUnit.cap(1);
      double returnValue = 0.0;
      try {
         returnValue = Localization::toDouble(numericPartOfInput, ok);
      } catch (std::invalid_argument const & ex) {
         qWarning() << Q_FUNC_INFO << "Could not parse" << numericPartOfInput << "as number:" << ex.what();
      } catch(std::out_of_range const & ex) {
         qWarning() << Q_FUNC_INFO << "Out of range parsing" << numericPartOfInput << "as number:" << ex.what();
      }

      return returnValue;
   }
}

//
// There isn't a generic version of this function, just the specialisations below
//
// Note that Qt conversion functions are generally not very accepting of extra characters.  Eg
// \c QString::toInt() will give 0 when parsing "12.34" as it barfs on the decimal point, whereas
// \c std::stoi() will give 12 on the same string input.  Nonetheless, we want to use Localization for decimal
// separators etc.  So, we always convert everything to double first and then, if needed, convert the double
// to an int / unsigned int, as this will give the behaviour we want.
//
template<typename T> T Measurement::extractRawFromString(QString const & input, bool * ok) {
   // This compile-time assert relies on the fact that no type has size 0
   static_assert(sizeof(T) == 0, "Only specializations of stringTo() can be used");
}
template<> int          Measurement::extractRawFromString<int>         (QString const & input, bool * ok) { return static_cast<int         >(extractRawDoubleFromString(input, ok)); }
template<> unsigned int Measurement::extractRawFromString<unsigned int>(QString const & input, bool * ok) { return static_cast<unsigned int>(extractRawDoubleFromString(input, ok)); }
template<> double       Measurement::extractRawFromString<double>      (QString const & input, bool * ok) { return                           extractRawDoubleFromString(input, ok);  }

void Measurement::loadDisplayScales() {
   for (Measurement::PhysicalQuantity const physicalQuantity : Measurement::allPhysicalQuantites) {
      loadDisplayScale(physicalQuantity,
                       Measurement::getSettingsName(physicalQuantity),
                       Measurement::Unit::getCanonicalUnit(physicalQuantity).getUnitSystem());
   }
   return;
}

void Measurement::saveDisplayScales() {
   for (Measurement::PhysicalQuantity const physicalQuantity : Measurement::allPhysicalQuantites) {
      PersistentSettings::insert(Measurement::getSettingsName(physicalQuantity),
                                 Measurement::getDisplayUnitSystem(physicalQuantity).uniqueName);
   }
   return;
}

void Measurement::setDisplayUnitSystem(Measurement::PhysicalQuantity physicalQuantity,
                                       Measurement::UnitSystem const & unitSystem) {
   // It's a coding error if we try to store a UnitSystem against a PhysicalQuantity to which it does not relate!
   Q_ASSERT(physicalQuantity == unitSystem.getPhysicalQuantity());
   qDebug() <<
      Q_FUNC_INFO << "Setting UnitSystem for" << Measurement::getDisplayName(physicalQuantity) << "to" <<
      unitSystem.uniqueName;
   physicalQuantityToDisplayUnitSystem.insert(physicalQuantity, &unitSystem);
   return;
}

void Measurement::setDisplayUnitSystem(UnitSystem const & unitSystem) {
   // It's a coding error if we try to store a UnitSystem against a PhysicalQuantity to which it does not relate!
   auto physicalQuantity = unitSystem.getPhysicalQuantity();
   Measurement::setDisplayUnitSystem(physicalQuantity, unitSystem);
   return;
}

Measurement::UnitSystem const & Measurement::getDisplayUnitSystem(Measurement::PhysicalQuantity physicalQuantity) {
   // It is a coding error if physicalQuantityToDisplayUnitSystem has not had data loaded into it by the time this function is
   // called.
   Q_ASSERT(!physicalQuantityToDisplayUnitSystem.isEmpty());

   Measurement::UnitSystem const * unitSystem = physicalQuantityToDisplayUnitSystem.value(physicalQuantity, nullptr);
   if (nullptr == unitSystem) {
      // This is a coding error
      qCritical() <<
         Q_FUNC_INFO << "Unable to find display unit system for physical quantity" <<
         Measurement::getDisplayName(physicalQuantity);
      Q_ASSERT(false);
   }
   return *unitSystem;
}

QString Measurement::displayQuantity(double quantity, int precision) {
   return QString("%L1").arg(quantity, fieldWidth, format, precision);
}

QString Measurement::displayAmount(Measurement::Amount const & amount,
                                   int precision,
                                   std::optional<Measurement::SystemOfMeasurement> forcedSystemOfMeasurement,
                                   std::optional<Measurement::UnitSystem::RelativeScale> forcedScale) {
   // Check for insane values.
   if (Algorithms::isNan(amount.quantity()) || Algorithms::isInf(amount.quantity())) {
      return "-";
   }

   // If the caller told us (via forced system of measurement) what UnitSystem to use, use that, otherwise get whatever
   // one we're using generally for related physical property.
   PhysicalQuantity const physicalQuantity = amount.unit()->getPhysicalQuantity();
   Measurement::UnitSystem const & displayUnitSystem =
      forcedSystemOfMeasurement ? UnitSystem::getInstance(*forcedSystemOfMeasurement, physicalQuantity) :
                                  Measurement::getDisplayUnitSystem(physicalQuantity);

//      qDebug() << Q_FUNC_INFO << "Display" << amount << units->getUnitName() << "in" << temp->unitType();
   return displayUnitSystem.displayAmount(amount, precision, forcedScale);
}

QString Measurement::displayAmount(NamedEntity * namedEntity,
                                   QObject * guiObject,
                                   BtStringConst const & propertyName,
                                   Measurement::Unit const & units,
                                   int precision ) {

   if (namedEntity->property(*propertyName).canConvert(QVariant::Double)) {
      // Get the amount
      QString value = namedEntity->property(*propertyName).toString();
      bool ok = false;
      double quantity = Localization::toDouble(value, &ok);
      if (!ok) {
         qWarning() << Q_FUNC_INFO << "Could not convert " << value << " to double";
      }

      return Measurement::displayAmount(
         Measurement::Amount{quantity, units},
         precision,
         Measurement::getForcedSystemOfMeasurementForField(*propertyName, guiObject->objectName()),
         Measurement::getForcedRelativeScaleForField(*propertyName, guiObject->objectName())
      );
   }

   return "?";
}

QString Measurement::displayAmount(Measurement::Amount const & amount,
                                   BtStringConst const & section,
                                   BtStringConst const & propertyName,
                                   int precision) {
   return Measurement::displayAmount(amount,
                                     precision,
                                     Measurement::getForcedSystemOfMeasurementForField(*propertyName, *section),
                                     Measurement::getForcedRelativeScaleForField(*propertyName, *section));

}

double Measurement::amountDisplay(Measurement::Amount const & amount,
                                  std::optional<Measurement::SystemOfMeasurement> forcedSystemOfMeasurement,
                                  std::optional<Measurement::UnitSystem::RelativeScale> forcedScale) {

   // Check for insane values.
   if (Algorithms::isNan(amount.quantity()) || Algorithms::isInf(amount.quantity())) {
      return -1.0;
   }

   // If the caller told us (via forced system of measurement) what UnitSystem to use, use that, otherwise get whatever
   // one we're using generally for related physical property.
   PhysicalQuantity const physicalQuantity = amount.unit()->getPhysicalQuantity();
   Measurement::UnitSystem const & displayUnitSystem =
      forcedSystemOfMeasurement ? UnitSystem::getInstance(*forcedSystemOfMeasurement, physicalQuantity) :
                                  Measurement::getDisplayUnitSystem(physicalQuantity);

   return displayUnitSystem.amountDisplay(amount, forcedScale);
}

double Measurement::amountDisplay(NamedEntity * namedEntity,
                                  QObject * guiObject,
                                  BtStringConst const & propertyName,
                                  Measurement::Unit const * units) {

   if (namedEntity->property(*propertyName).canConvert(QVariant::Double)) {
      // Get the amount
      QString value = namedEntity->property(*propertyName).toString();
      bool ok = false;
      double amount = Localization::toDouble(value, &ok);
      if (!ok) {
         qWarning() << Q_FUNC_INFO << "Could not convert" << value << "to double";
      }

      // Special case: we don't know the units of the supplied amount, so just return it as is
      if (units == nullptr) {
         return amount;
      }

      return Measurement::amountDisplay(
         Measurement::Amount{amount, *units},
         Measurement::getForcedSystemOfMeasurementForField(*propertyName, guiObject->objectName()),
         Measurement::getForcedRelativeScaleForField(*propertyName, guiObject->objectName())
      );
   }

   return -1.0;
}

QPair<double,double> Measurement::displayRange(NamedEntity* namedEntity,
                                               QObject *guiObject,
                                               BtStringConst const & propertyNameMin,
                                               BtStringConst const & propertyNameMax,
                                               Unit const * units) {
   QPair<double,double> range;

   if (!namedEntity) {
      range.first  = 0.0;
      range.second = 100.0;
   } else {
      range.first  = amountDisplay(namedEntity, guiObject, propertyNameMin, units);
      range.second = amountDisplay(namedEntity, guiObject, propertyNameMax, units);
   }

   return range;
}

QPair<double,double> Measurement::displayRange(QObject *guiObject,
                                               BtStringConst const & propertyName,
                                               double min,
                                               double max,
                                               Unit const & units) {
   auto forcedSystemOfMeasurement = Measurement::getForcedSystemOfMeasurementForField(*propertyName,
                                                                                      guiObject->objectName());
   auto forcedRelativeScale = Measurement::getForcedRelativeScaleForField(*propertyName, guiObject->objectName());

   QPair<double,double> range;
   range.first  = Measurement::amountDisplay(Measurement::Amount{min, units}, forcedSystemOfMeasurement, forcedRelativeScale);
   range.second = Measurement::amountDisplay(Measurement::Amount{max, units}, forcedSystemOfMeasurement, forcedRelativeScale);
   return range;
}

void Measurement::getThicknessUnits(Unit const ** volumeUnit, Unit const ** weightUnit) {
   *volumeUnit = Measurement::getDisplayUnitSystem(Measurement::PhysicalQuantity::Volume).thicknessUnit();
   *weightUnit = Measurement::getDisplayUnitSystem(Measurement::PhysicalQuantity::Mass).thicknessUnit();
   return;
}

QString Measurement::displayThickness( double thick_lkg, bool showUnits ) {
   int fieldWidth = 0;
   char format = 'f';
   int precision = 2;

   Measurement::Unit const * volUnit;
   Measurement::Unit const * weightUnit;
   Measurement::getThicknessUnits(&volUnit, &weightUnit);

   double num = volUnit->fromCanonical(thick_lkg);
   double den = weightUnit->fromCanonical(1.0);

   if (showUnits) {
      return QString("%L1 %2/%3").arg(num/den, fieldWidth, format, precision).arg(volUnit->name).arg(weightUnit->name);
   }

   return QString("%L1").arg(num/den, fieldWidth, format, precision).arg(volUnit->name).arg(weightUnit->name);
}

Measurement::Amount Measurement::qStringToSI(QString qstr,
                                             Measurement::PhysicalQuantity const physicalQuantity,
                                             std::optional<Measurement::SystemOfMeasurement> forcedSystemOfMeasurement,
                                             std::optional<Measurement::UnitSystem::RelativeScale> forcedScale) {
   qDebug() <<
      Q_FUNC_INFO << "Input" << qstr << "of" << physicalQuantity << "; forcedSystemOfMeasurement=" <<
      forcedSystemOfMeasurement << "; forcedScale=" << forcedScale;

   //
   // If the caller told us that the SystemOfMeasurement and/or RelativeScale on the input (qstr) are "forced" then that
   // information can be used to interpret a case where no (valid) unit is supplied in the input (ie it's just a number
   // rather than number plus units) or where the supplied unit is ambiguous (eg US pints are different than Imperial
   // pints).  Otherwise, just otherwise get whatever UnitSystem we're using generally for related physical property.
   //
   Measurement::UnitSystem const & displayUnitSystem {
      forcedSystemOfMeasurement ? UnitSystem::getInstance(*forcedSystemOfMeasurement, physicalQuantity) :
                                  Measurement::getDisplayUnitSystem(physicalQuantity)
   };
   Measurement::Unit const * defaultUnit {
      forcedScale ? displayUnitSystem.scaleUnit(*forcedScale) : displayUnitSystem.unit()
   };
   // It's a coding error if defaultUnit is null, because it means previousScaleInfo.oldForcedScale was not valid for
   // oldUnitSystem.  However, we can recover.
   if (!defaultUnit) {
      qWarning() << Q_FUNC_INFO << "forcedScale invalid?" << forcedScale;
      defaultUnit = &Measurement::Unit::getCanonicalUnit(physicalQuantity);
   }

   return displayUnitSystem.qstringToSI(qstr, *defaultUnit);
}

std::optional<Measurement::SystemOfMeasurement> Measurement::getForcedSystemOfMeasurementForField(QString const & field,
                                                                                                  QString const & section) {
   if (field.isEmpty()) {
      return std::nullopt;
   }
   return Measurement::getFromUniqueName(
      PersistentSettings::value(field,
                                "None", // This, or any invalid name, will give "no value" return from getFromUniqueName()
                                section,
                                PersistentSettings::Extension::UNIT).toString()
   );
}

std::optional<Measurement::UnitSystem::RelativeScale> Measurement::getForcedRelativeScaleForField(QString const & field,
                                                                                                  QString const & section) {
   if (field.isEmpty()) {
      return std::nullopt;
   }
   return Measurement::UnitSystem::getScaleFromUniqueName(
      PersistentSettings::value(field,
                                "None", // This, or any invalid name, will give "no value" return from getFromUniqueName()
                                section,
                                PersistentSettings::Extension::SCALE).toString()
   );
}

void Measurement::setForcedSystemOfMeasurementForField(QString const & field,
                                                       QString const & section,
                                                       std::optional<Measurement::SystemOfMeasurement> forcedSystemOfMeasurement) {
   if (field.isEmpty()) {
      return;
   }
   if (forcedSystemOfMeasurement) {
      PersistentSettings::insert(field,
                                 Measurement::getUniqueName(*forcedSystemOfMeasurement),
                                 section,
                                 PersistentSettings::Extension::UNIT);
   } else {
      PersistentSettings::remove(field,
                                 section,
                                 PersistentSettings::Extension::UNIT);
   }
   return;
}

void Measurement::setForcedRelativeScaleForField(QString const & field,
                                                 QString const & section,
                                                 std::optional<Measurement::UnitSystem::RelativeScale> forcedScale) {
   if (field.isEmpty()) {
      return;
   }
   if (forcedScale) {
      PersistentSettings::insert(field,
                                 Measurement::UnitSystem::getUniqueName(*forcedScale),
                                 section,
                                 PersistentSettings::Extension::SCALE);
   } else {
      PersistentSettings::remove(field,
                                 section,
                                 PersistentSettings::Extension::SCALE);
   }
   return;
}

Measurement::SystemOfMeasurement Measurement::getSystemOfMeasurementForField(QString const & field,
                                                                             QString const & section,
                                                                             Measurement::PhysicalQuantities physicalQuantities) {
   auto forcedSystemOfMeasurement = Measurement::getForcedSystemOfMeasurementForField(field, section);
   if (forcedSystemOfMeasurement) {
      return *forcedSystemOfMeasurement;
   }

   // If there is no forced System Of Measurement for the field, then we can look to the globally-set UnitSystem for
   // this PhysicalQuantity -- except that, if there are two values of PhysicalQuantity, we have to
   // choose one arbitrarily.  The end result should be the same, because Mass & Volume share the same
   // SystemOfMeasurement, as do MassConcentration & VolumeConcentration.
   Measurement::PhysicalQuantity const physicalQuantity =
      std::holds_alternative<Measurement::PhysicalQuantity>(physicalQuantities) ?
         std::get<Measurement::PhysicalQuantity>(physicalQuantities) :
         std::get<0>(std::get<Mixed2PhysicalQuantities>(physicalQuantities));

   return Measurement::getDisplayUnitSystem(physicalQuantity).systemOfMeasurement;
}


Measurement::UnitSystem const & Measurement::getUnitSystemForField(QString const & field,
                                                                   QString const & section,
                                                                   Measurement::PhysicalQuantity physicalQuantity) {
   auto forcedSystemOfMeasurement = Measurement::getForcedSystemOfMeasurementForField(field, section);
   if (forcedSystemOfMeasurement) {
      return Measurement::UnitSystem::getInstance(*forcedSystemOfMeasurement, physicalQuantity);
   }
   return Measurement::getDisplayUnitSystem(physicalQuantity);
}
