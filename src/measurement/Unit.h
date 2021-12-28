/*======================================================================================================================
 * measurement/Unit.h is part of Brewken, and is copyright the following authors 2009-2021:
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
#ifndef MEASUREMENT_UNIT_H
#define MEASUREMENT_UNIT_H
#pragma once

#include <functional>
#include <memory> // For PImpl

#include <QMultiMap>
#include <QObject>
#include <QString>

#include "measurement/PhysicalQuantity.h"

// TODO: implement ppm, percent, ibuGalPerLb,

/*!
 * \class Unit
 *
 * \brief Interface for arbitrary physical units and their formatting.
 *
 * .:TBD:. Does this really need to inherit from QObject?
 */
namespace Measurement {
   class UnitSystem;


   class Unit /*: public QObject*/ {
   //Q_OBJECT

   //Q_ENUMS(unitDisplay)
   //Q_ENUMS(RelativeScale)
   //Q_ENUMS(PhysicalQuantity)

   public:
      // Did you know you need these various enums to be *INSIDE* the class definition for Qt to see them? TBD DO WE CARE ABOUT THIS ANY MORE?

      /**
       * \brief Construct a type of unit.  Note that it is \b not intended that users of this class construct their own
       *        \c Unit objects.  Rather they should use pointers or references to the constants defined in the \c Units
       *        namespace.
       *
       * \param unitSystem The \c UnitSystem to which this \c Unit belongs.  Amongst other things, this tells us which
       *                   \c PhysicalQuantity this \c Unit relates to.  (See comment in
       *                   \c measurement/PhysicalQuantity.h for more details on the relationship between classes in the
       *                   \c Measurement namespace.)
       * \param unitName This is singular of the commonly used abbreviation for this unit, eg, in English, "kg" for
       *                 kilograms, "tsp" for teaspoons. Note that this needs to be unique within the \c UnitSystem to
       *                 which this \c Unit belongs but is \c not necessarily globally unique, eg "qt" refers to both
       *                 Imperial quarts and US Customary quarts; "L" refers to liters and Lintner.
       * \param convertToCanonical
       * \param convertFromCanonical
       * \param boundaryValue
       */
      Unit(UnitSystem const & unitSystem,
           QString const unitName,
           std::function<double(double)> convertToCanonical,
           std::function<double(double)> convertFromCanonical,
           double boundaryValue = 1.0);

      ~Unit();

      /**
       * \brief The unit name will be the singular of the commonly used abbreviation.
       */
      QString const name;

      /**
       * \brief Convert an amount of this unit to its canonical system of measurement (usually, but not always, an SI measure)
       */
      double toSI(double amt) const;

      /**
       * \brief Convert an amount of this unit from its canonical system of measurement (usually, but not always, an SI measure)
       */
      double fromSI(double amt) const;

      /**
       * \brief Returns the \c Measurement::PhysicalQuantity that this \c Measurement::Unit measures.  This is a
       *        convenience function to save you having to first get the \c Measurement::UnitSystem.
       */
      Measurement::PhysicalQuantity getPhysicalQuantity() const;


      /**
       * \brief Returns the \c Measurement::UnitSystem to which this \c Measurement::Unit belongs.
       */
      Measurement::UnitSystem const & getUnitSystem() const;

      /**
       * \brief Used by \c UnitSystem
       *
       *        Returns the threshold below which a smaller unit (of the same type) should be used.  Normally it's 1, eg a
       *        length of time less than a minute should be shown in seconds.  But it can be larger, eg we show minutes for
       *        any length of time below 2 hours.  And it can be smaller, eg a US/imperial volume measure can be as small
       *        as a quarter of cup before we drop down to showing tablespoons.
       */
      double boundary() const;

      /**
       * \brief This mostly gets called when the unit entered in the field does not match what the field has been set
       *        to.  For example, if you displaying in Liters, but enter "20 qt". Since the SIVolumeUnitSystem doesn't
       *        know what "qt" is, we go searching for it.
       *
       * \param name
       * \param physicalQuantity If the caller knows what \c PhysicalQuantity the name relates to, this will help with
       *                         disambiguation (eg between Liters and Lintner, both of which have name/abbreviation
       *                         "L").  Otherwise specify \c Measurement::PhysicalQuantity::None here.
       *
       * \return \c nullptr if no sane match could be found
       */
      static Unit const * getUnit(QString const & name,
                                  Measurement::PhysicalQuantity physicalQuantity = Measurement::PhysicalQuantity::None);

      /**
       * \brief Used by \c ConverterTool to do contextless conversions - ie where we don't know what \c PhysicalQuantity
       *        we are dealing with because it's a generic tool to allow the user to convert "3 qt" to liters or "5lb"
       *        to kilograms etc.
       */
      static QString convert(QString qstr, QString toUnit);

   private:
      // Private implementation details - see https://herbsutter.com/gotw/_100/
      class impl;
      std::unique_ptr<impl> pimpl;

      //! No copy constructor, as never want anyone, not even our friends, to make copies of a singleton
      Unit(Unit const&) = delete;
      //! No assignment operator , as never want anyone, not even our friends, to make copies of a singleton.
      Unit& operator=(Unit const&) = delete;
      //! No move constructor
      Unit(Unit &&) = delete;
      //! No move assignment
      Unit & operator=(Unit &&) = delete;
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
}

#endif
