/*======================================================================================================================
 * measurement/Measurement.cpp is part of Brewken, and is copyright the following authors 2010-2022:
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

namespace {

   /**
    * \brief Stores the current \c Measurement::UnitSystem being used for each \c Measurement::PhysicalQuantity
    *        Note that this is for input and display.  We always convert to a standard \c Measurement::Unit (usually
    *        Metric/SI where that's an option) for storing in the DB.
    */
   QMap<Measurement::PhysicalQuantity, Measurement::UnitSystem const *> physicalQuantityToUnitSystem;

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
    * For each \c PhysicalQuantity we have one \c Unit that we use for storing amounts internally (including in the DB)
    */
   QMap<Measurement::PhysicalQuantity, Measurement::Unit const *> const physicalQuantityToUnit {
      {Measurement::PhysicalQuantity::Mass,           &Measurement::Units::kilograms},
      {Measurement::PhysicalQuantity::Volume,         &Measurement::Units::liters},
      {Measurement::PhysicalQuantity::Time,           &Measurement::Units::minutes},
      {Measurement::PhysicalQuantity::Temperature,    &Measurement::Units::celsius},
      {Measurement::PhysicalQuantity::Color,          &Measurement::Units::srm},     // We will consider the standard unit of color to be SRM.
      {Measurement::PhysicalQuantity::Density,        &Measurement::Units::sp_grav}, // Specific gravity (aka, Sg) will be the standard unit, since that is how we store things in the database.
      {Measurement::PhysicalQuantity::DiastaticPower, &Measurement::Units::lintner}, // Lintner will be the standard unit, since that is how we store things in the database.
      {Measurement::PhysicalQuantity::None,           nullptr}
   };

}

void Measurement::loadDisplayScales() {
   loadDisplayScale(Measurement::PhysicalQuantity::Mass,           PersistentSettings::Names::unitSystem_weight,         Measurement::UnitSystems::mass_Metric);
   loadDisplayScale(Measurement::PhysicalQuantity::Volume,         PersistentSettings::Names::unitSystem_volume,         Measurement::UnitSystems::volume_Metric);
   loadDisplayScale(Measurement::PhysicalQuantity::Temperature,    PersistentSettings::Names::unitSystem_temperature,    Measurement::UnitSystems::temperature_MetricIsCelsius);
   loadDisplayScale(Measurement::PhysicalQuantity::Density,        PersistentSettings::Names::unitSystem_density,        Measurement::UnitSystems::density_SpecificGravity);
   loadDisplayScale(Measurement::PhysicalQuantity::Color,          PersistentSettings::Names::unitSystem_color,          Measurement::UnitSystems::color_StandardReferenceMethod);
   loadDisplayScale(Measurement::PhysicalQuantity::DiastaticPower, PersistentSettings::Names::unitSystem_diastaticPower, Measurement::UnitSystems::diastaticPower_Lintner);

   // These physical quantities only have one UnitSystem, so we don't bother storing it in PersistentSettings
   Measurement::setDisplayUnitSystem(Measurement::PhysicalQuantity::Time, Measurement::UnitSystems::time_CoordinatedUniversalTime);

   return;
}

void Measurement::saveDisplayScales() {
   PersistentSettings::insert(PersistentSettings::Names::unitSystem_weight,         Measurement::getDisplayUnitSystem(Measurement::PhysicalQuantity::Mass).uniqueName);
   PersistentSettings::insert(PersistentSettings::Names::unitSystem_volume,         Measurement::getDisplayUnitSystem(Measurement::PhysicalQuantity::Volume).uniqueName);
   PersistentSettings::insert(PersistentSettings::Names::unitSystem_temperature,    Measurement::getDisplayUnitSystem(Measurement::PhysicalQuantity::Temperature).uniqueName);
   PersistentSettings::insert(PersistentSettings::Names::unitSystem_density,        Measurement::getDisplayUnitSystem(Measurement::PhysicalQuantity::Density).uniqueName);
   PersistentSettings::insert(PersistentSettings::Names::unitSystem_color,          Measurement::getDisplayUnitSystem(Measurement::PhysicalQuantity::Color).uniqueName);
   PersistentSettings::insert(PersistentSettings::Names::unitSystem_diastaticPower, Measurement::getDisplayUnitSystem(Measurement::PhysicalQuantity::DiastaticPower).uniqueName);
   return;
}

void Measurement::setDisplayUnitSystem(Measurement::PhysicalQuantity physicalQuantity,
                                       Measurement::UnitSystem const & unitSystem) {
   // It's a coding error if we try to store a UnitSystem against a PhysicalQuantity to which it does not relate!
   Q_ASSERT(physicalQuantity == unitSystem.getPhysicalQuantity());
   qDebug() <<
      Q_FUNC_INFO << "Setting UnitSystem for" << Measurement::getDisplayName(physicalQuantity) << "to" <<
      unitSystem.uniqueName;
   physicalQuantityToUnitSystem.insert(physicalQuantity, &unitSystem);
   return;
}

void Measurement::setDisplayUnitSystem(UnitSystem const & unitSystem) {
   // It's a coding error if we try to store a UnitSystem against a PhysicalQuantity to which it does not relate!
   auto physicalQuantity = unitSystem.getPhysicalQuantity();
   Measurement::setDisplayUnitSystem(physicalQuantity, unitSystem);
   return;
}

// .:TODO:. Need to decide how to handle Mixed!
Measurement::UnitSystem const & Measurement::getDisplayUnitSystem(PhysicalQuantity physicalQuantity) {
   // It is a coding error if physicalQuantityToUnitSystem has not had data loaded into it by the time this function is
   // called.
   Q_ASSERT(!physicalQuantityToUnitSystem.isEmpty());

   Measurement::UnitSystem const * unitSystem = physicalQuantityToUnitSystem.value(physicalQuantity, nullptr);
   if (nullptr == unitSystem) {
      // This is a coding error
      qCritical() <<
         Q_FUNC_INFO << "Unable to find display unit system for physical quantity" <<
         Measurement::getDisplayName(physicalQuantity);
      Q_ASSERT(false);
   }
   return *unitSystem;
}

Measurement::Unit const & Measurement::getUnitForInternalStorage(PhysicalQuantity physicalQuantity) {
   // Every physical quantity we use should have a corresponding unit we use for measuring it...
   if (!physicalQuantityToUnit.contains(physicalQuantity) ||
       nullptr == physicalQuantityToUnit.value(physicalQuantity)) {
      // ...so it's a coding error if this is not the case
      qCritical() <<
         Q_FUNC_INFO << "No internal storage Unit defined for physical quantity" <<
         Measurement::getDisplayName(physicalQuantity);
      Q_ASSERT(false);
   }
   return *physicalQuantityToUnit.value(physicalQuantity);
}

QString Measurement::displayAmount(double amount,
                                   Measurement::Unit const * units,
                                   int precision,
                                   std::optional<Measurement::SystemOfMeasurement> forcedSystemOfMeasurement,
                                   std::optional<Measurement::UnitSystem::RelativeScale> forcedScale) {
   // Check for insane values.
   if (Algorithms::isNan(amount) || Algorithms::isInf(amount)) {
      return "-";
   }

   int const fieldWidth = 0;
   char const format = 'f';

   // Special case: we don't know the units of the supplied amount, so just return it as is
   if (units == nullptr) {
      return QString("%L1").arg(amount, fieldWidth, format, precision);
   }

   // If the caller told us (via forced system of measurement) what UnitSystem to use, use that, otherwise get whatever
   // one we're using generally for related physical property.
   PhysicalQuantity const physicalQuantity = units->getPhysicalQuantity();
   Measurement::UnitSystem const & displayUnitSystem =
      forcedSystemOfMeasurement ? UnitSystem::getInstance(*forcedSystemOfMeasurement, physicalQuantity) :
                                  Measurement::getDisplayUnitSystem(physicalQuantity);

   // If we cannot find a general unit system (which I think is actually a coding error), we'll just show in Metric/SI
/*   if (nullptr == displayUnitSystem) {
      Measurement::PhysicalQuantity physicalQuantity = units->getPhysicalQuantity();
      Measurement::Unit const & siUnit = Measurement::getUnitForInternalStorage(physicalQuantity);
      double siAmount = units->toSI(amount);
      return QString("%L1 %2").arg(siAmount, fieldWidth, format, precision).arg(siUnit.name);
   }*/

//      qDebug() << Q_FUNC_INFO << "Display" << amount << units->getUnitName() << "in" << temp->unitType();
   return displayUnitSystem.displayAmount(amount, units, precision, forcedScale);
}

QString Measurement::displayAmount(NamedEntity * namedEntity,
                                   QObject * guiObject,
                                   BtStringConst const & propertyName,
                                   Measurement::Unit const * units,
                                   int precision ) {

   if (namedEntity->property(*propertyName).canConvert(QVariant::Double)) {
      // Get the amount
      QString value = namedEntity->property(*propertyName).toString();
      bool ok = false;
      double amount = Localization::toDouble(value, &ok);
      if (!ok) {
         qWarning() << Q_FUNC_INFO << "Could not convert " << value << " to double";
      }

      return Measurement::displayAmount(
         amount,
         units,
         precision,
         Measurement::getForcedSystemOfMeasurementForField(*propertyName, guiObject->objectName()),
         Measurement::getForcedRelativeScaleForField(*propertyName, guiObject->objectName())
      );
   }

   return "?";
}

QString Measurement::displayAmount(double amount,
                                   BtStringConst const & section,
                                   BtStringConst const & propertyName,
                                   Measurement::Unit const * units,
                                   int precision) {
   return Measurement::displayAmount(amount,
                                     units,
                                     precision,
                                     Measurement::getForcedSystemOfMeasurementForField(*propertyName, *section),
                                     Measurement::getForcedRelativeScaleForField(*propertyName, *section));

}

double Measurement::amountDisplay(double amount,
                                  Measurement::Unit const * units,
                                  std::optional<Measurement::SystemOfMeasurement> forcedSystemOfMeasurement,
                                  std::optional<Measurement::UnitSystem::RelativeScale> forcedScale) {

   // Check for insane values.
   if (Algorithms::isNan(amount) || Algorithms::isInf(amount)) {
      return -1.0;
   }

   // Special case: we don't know the units of the supplied amount, so just return it as is
   if (units == nullptr) {
      return amount;
   }

   // If the caller told us (via forced system of measurement) what UnitSystem to use, use that, otherwise get whatever
   // one we're using generally for related physical property.
   PhysicalQuantity const physicalQuantity = units->getPhysicalQuantity();
   Measurement::UnitSystem const & displayUnitSystem =
      forcedSystemOfMeasurement ? UnitSystem::getInstance(*forcedSystemOfMeasurement, physicalQuantity) :
                                  Measurement::getDisplayUnitSystem(physicalQuantity);

   return displayUnitSystem.amountDisplay(amount, units, forcedScale);
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

      return Measurement::amountDisplay(
         amount,
         units,
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
   range.first  = Measurement::amountDisplay(min, &units, forcedSystemOfMeasurement, forcedRelativeScale);
   range.second = Measurement::amountDisplay(max, &units, forcedSystemOfMeasurement, forcedRelativeScale);
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

   double num = volUnit->fromSI(thick_lkg);
   double den = weightUnit->fromSI(1.0);

   if (showUnits) {
      return QString("%L1 %2/%3").arg(num/den, fieldWidth, format, precision).arg(volUnit->name).arg(weightUnit->name);
   }

   return QString("%L1").arg(num/den, fieldWidth, format, precision).arg(volUnit->name).arg(weightUnit->name);
}

double Measurement::qStringToSI(QString qstr,
                                Measurement::PhysicalQuantity const physicalQuantity,
                                std::optional<Measurement::SystemOfMeasurement> forcedSystemOfMeasurement,
                                std::optional<Measurement::UnitSystem::RelativeScale> forcedScale) {

   // If the caller told us (via forced system of measurement) what UnitSystem to use, use that, otherwise get whatever
   // one we're using generally for related physical property.
   Measurement::UnitSystem const & displayUnitSystem =
      forcedSystemOfMeasurement ? UnitSystem::getInstance(*forcedSystemOfMeasurement, physicalQuantity) :
                                  Measurement::getDisplayUnitSystem(physicalQuantity);

   Measurement::Unit const * unit = &Measurement::getUnitForInternalStorage(physicalQuantity);
   return displayUnitSystem.qstringToSI(qstr, unit, forcedScale);
}

std::optional<Measurement::SystemOfMeasurement> Measurement::getForcedSystemOfMeasurementForField(QString const & field,
                                                                                                  QString const & section) {
   return Measurement::getFromUniqueName(
      PersistentSettings::value(field,
                                "None", // This, or any invalid name, will give "no value" return from getFromUniqueName()
                                section,
                                PersistentSettings::Extension::UNIT).toString()
   );
}

std::optional<Measurement::UnitSystem::RelativeScale> Measurement::getForcedRelativeScaleForField(QString const & field,
                                                                                                  QString const & section) {
   return Measurement::UnitSystem::getScaleFromUniqueName(
      PersistentSettings::value(field,
                                "None", // This, or any invalid name, will give "no value" return from getFromUniqueName()
                                section,
                                PersistentSettings::Extension::SCALE).toString()
   );
}

void Measurement::setForcedSystemOfMeasurementForField(QString const & field,
                                                       QString const & section,
                                                       Measurement::SystemOfMeasurement systemOfMeasurement) {
   PersistentSettings::insert(field,
                              Measurement::getUniqueName(systemOfMeasurement),
                              section,
                              PersistentSettings::Extension::UNIT);
   return;
}

void Measurement::setForcedRelativeScaleForField(QString const & field,
                                                 QString const & section,
                                                 Measurement::UnitSystem::RelativeScale relativeScale) {
   PersistentSettings::insert(field,
                              Measurement::UnitSystem::getUniqueName(relativeScale),
                              section,
                              PersistentSettings::Extension::SCALE);
   return;
}

void Measurement::unsetForcedSystemOfMeasurementForField(QString const & field, QString const & section) {
   PersistentSettings::remove(field,
                              section,
                              PersistentSettings::Extension::UNIT);
   return;
}

void Measurement::unsetForcedRelativeScaleForField(QString const & field, QString const & section) {
   PersistentSettings::remove(field,
                              section,
                              PersistentSettings::Extension::SCALE);
   return;
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
