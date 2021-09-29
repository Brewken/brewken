/*======================================================================================================================
 * Unit.h is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Jeff Bailey <skydvr38@verizon.net>
 *   • Mark de Wever <koraq@xs4all.nl>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Rob Taylor <robtaylor@floopily.org>
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
#ifndef UNIT_H
#define UNIT_H
#pragma once

#include <functional>

#include <QMultiMap>
#include <QObject>
#include <QString>

namespace SystemsOfMeasurement {
   /**
    * \enum SystemOfMeasurement tells us which sets of units to use for \c Unit::QuantityType of \c Unit::Mass or
    *       \c Unit::Volume.  These are the quantity types where we have multiple units in each system (eg milligrams,
    *       grams and kilograms in the metric aka SI system for mass), so we need a group name.  For other quantity types,
    *       such as \c Unit::Temperature, there is only one unit in each system of measurement (eg degrees Fahrenheit in US
    *       customary units) and/or we don't want to use the "standard" unit (eg technically we should use Kelvin in the
    *       metric/SI system, but outside the science lab, it's more sensible to use degrees Celsius) and/or the name of
    *       the system of measurement is the same as the unit of measurement (eg SRM and EBC for \c Unit::Color).  So in
    *       those cases, we use the unit itself rather than having a separate enum for system of measurement.
    */
   enum MassOrVolumeScales {
      SI = 0,
      USCustomary = 1,
      Imperial = 2,
   //    ImperialAndUS = 3, Not used and I'm not even sure what it means!
      Any = 4 // .:TODO:. This is a hack for the Unit class that we need to remove
   };

   /**
    * \enum TempScale tells us which units to use for for \c Unit::QuantityType of \c Unit::Temperature
    */
   enum TempScale {
      Celsius,
      Fahrenheit,
      Kelvin
   };

   //! \brief The units to display color in.
   enum ColorUnitType {SRM, EBC};

   //! \brief Units for density
   enum DensityUnitType {SG, PLATO};

   //! \brief The units for the diastatic power.
   enum DiastaticPowerUnitType {LINTNER, WK};

   // Options to be edited ONLY by the OptionDialog============================
   // Whether or not to display plato instead of SG.

   extern MassOrVolumeScales weightUnitSystem;
   extern MassOrVolumeScales volumeUnitSystem;

   // Sigh. You knew this was coming right? But I think I can clean a lot of
   // shit up with some clever work.
   extern QHash<int, UnitSystem const *> thingToUnitSystem;

   extern TempScale tempScale;
   extern ColorUnitType colorUnit;
   extern DensityUnitType densityUnit;
   extern DiastaticPowerUnitType diastaticPowerUnit;

   // .:TODO:. At the moment, this can take the following values:
   //   Unit::displayUS  = mm-dd-YYYY
   //   Unit::displayImp = dd-mm-YYYY
   //   Unit::displaySI  = YYYY-mm-dd
   // This looks like a bit of a hack to avoid creating a new enum (perhaps because Qt::DateFormat is not suitable)
   // We should fix this!
   static Unit::unitDisplay dateFormat;

}

// TODO: implement ppm, percent, ibuGalPerLb,

/*!
 * \class Unit
 *
 * \brief Interface for arbitrary physical units and their formatting.
 */
class Unit : public QObject {
Q_OBJECT

Q_ENUMS(unitDisplay)
Q_ENUMS(RelativeScale)
Q_ENUMS(QuantityType)

public:
   // Did you know you need these various enums to be *INSIDE* the class definition for Qt to see them?

   /**
    *
    */
   enum unitDisplay {
      noUnit         = -1,
      displayDef     = 0x000,
      displaySI      = 0x100,
      displayUS      = 0x101,
      displayImp     = 0x102,
      displaySrm     = 0x200,
      displayEbc     = 0x201,
      displaySg      = 0x300,
      displayPlato   = 0x301,
      displayLintner = 0x400,
      displayWK      = 0x401
   };

   /**
    * For some types of quantity, a given system of measurement will have multiple units, so we need to be able to order
    * these units by relative size, eg, for fluid volume:
    *   fluid teaspoon < tablespoon < cup < pint < quart < gallon   (in both imperial units and US customary units)
    *   milliliters < liters                                        (in metric system)
    * We only worry about units we actually use/permit, thus we don't, for example, care about where minims, fluid
    * drams, gills etc fit in on the imperial / US customary volume scales, as we don't support them.
    */
   enum RelativeScale {
      noScale = -1,
      scaleExtraSmall = 0,
      scaleSmall = 1,
      scaleMedium = 2,
      scaleLarge = 3,
      scaleExtraLarge = 4,
      scaleHuge = 5,
      scaleWithout=1000
   };

   /**
    * \brief The various types of quantity (https://en.wikipedia.org/wiki/Quantity) that we need to be able to measure
    */
   enum QuantityType {
      Mass           = 0x100000,
      Volume         = 0x200000,
      Time           = 0x300000,
      Temperature    = 0x400000,
      Color          = 0x500000,
      Density        = 0x600000,
      String         = 0x700000,
      Mixed          = 0x800000,
      DiastaticPower = 0x900000,
      None           = 0x000000
   };

   /**
    * \brief Construct a type of unit
    */
   Unit(QuantityType const unitType,
        SystemsOfMeasurement::MassOrVolumeScales const systemOfMeasurement,
        QString const unitName,
        QString const siUnitName,
        std::function<double(double)> convertToCanonical,
        std::function<double(double)> convertFromCanonical,
        double boundaryValue = 1.0);

   ~Unit();

   /**
    * \brief Convert an amount of this unit to its canonical system of measurement (usually, but not always, an SI measure)
    */
   double toSI(double amt) const;

   /**
    * \brief Convert an amount of this unit from its canonical system of measurement (usually, but not always, an SI measure)
    */
   double fromSI(double amt) const;

   // The unit name will be the singular of the commonly used abbreviation.
   QString const & getUnitName() const;
   QString const & getSIUnitName() const;

   Unit::QuantityType getUnitType() const;

   /**
    * \brief Used by \c UnitSystem
    *
    *        Returns the threshold below which a smaller unit (of the same type) should be used.  Normally it's 1, eg a
    *        length of time less than a minute should be shown in seconds.  But it can be larger, eg we show minutes for
    *        any length of time below 2 hours.  And it can be smaller, eg a US/imperial volume measure can be as small
    *        as a quarter of cup before we drop down to showing tablespoons.
    */
   double boundary() const;

   static Unit const * getUnit(QString& name, bool matchCurrentSystem = true);
   static QString convert(QString qstr, QString toUnit);

private:
   QuantityType const unitType;
   SystemsOfMeasurement::MassOrVolumeScales const systemOfMeasurement;
   QString const unitName;
   QString const siUnitName;
   std::function<double(double)> convertToCanonical;
   std::function<double(double)> convertFromCanonical;
   double boundaryValue;

   static QMultiMap<QString, Unit const *> const nameToUnit;
};


namespace Units {
   // === Mass ===
   extern Unit const kilograms;
   extern Unit const grams;
   extern Unit const milligrams;
   extern Unit const pounds;
   extern Unit const ounces;
   // === Volume ===
   extern Unit const liters;
   extern Unit const milliliters;
   extern Unit const us_barrels;
   extern Unit const us_gallons;
   extern Unit const us_quarts;
   extern Unit const us_cups;
   extern Unit const us_tablespoons;
   extern Unit const us_teaspoons;
   extern Unit const imperial_barrels;
   extern Unit const imperial_gallons;
   extern Unit const imperial_quarts;
   extern Unit const imperial_cups;
   extern Unit const imperial_tablespoons;
   extern Unit const imperial_teaspoons;
   // === Time ===
   extern Unit const seconds;
   extern Unit const minutes;
   extern Unit const hours;
   extern Unit const days;
   // === Temperature ===
   extern Unit const celsius;
   extern Unit const fahrenheit;
   extern Unit const kelvin; // .:TBD:. MY 2021-05-07 Does anyone really use this in brewing?
   // === Color ===
   extern Unit const srm;
   extern Unit const ebc;
   // == Density ===
   extern Unit const sp_grav;
   extern Unit const plato;
   // == Diastatic power ==
   extern Unit const lintner;
   extern Unit const wk;
}

#endif
