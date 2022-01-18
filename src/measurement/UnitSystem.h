/*======================================================================================================================
 * measurement/UnitSystem.h is part of Brewken, and is copyright the following authors 2009-2022:
 *   • Jeff Bailey <skydvr38@verizon.net>
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
#ifndef MEASUREMENT_UNITSYSTEM_H
#define MEASUREMENT_UNITSYSTEM_H
#pragma once

#include <memory> // For PImpl
#include <optional>

#include <QMap>
#include <QString>

#include "measurement/PhysicalQuantity.h"
#include "measurement/SystemOfMeasurement.h"

namespace Measurement {
   class Unit;

   /*!
    * \class UnitSystem
    *
    * \brief A unit system is a collection of related \c Units for a particular \c PhysicalQuantity.  It is (often) the
    *        subset of a system of measurement that relates to an individual physical quantity.
    *
    *        From Wikipedia: "A system of measurement is a collection of units of measurement and rules relating them to
    *        each other. ... Systems of measurement in use include the International System of Units (SI), the modern form
    *        of the metric system, the British imperial system, and the United States customary system."
    *
    *        We don't use systems of measurement directly for two reasons:
    *         - Some things we need to measure, such as diastatic power, color, or density aren't covered by some or all of
    *           the common systems of measurement
    *         - Users want to be able to mix-and-match (eg using US customary for volume but metric for temperature)
    *
    *        For each physical quantity (mass, volume, temperature) where we support more than one way of measuring it, we
    *        allow the user to choose a \c UnitSystem that corresponds to (and is named after) the system of measurement or
    *        scale they want to see things displayed in.  This will also determine the units we will assume for user input
    *        if none are specified.  (Thus if the user has chosen to show temperatures in Fahrenheit then we assume any
    *        temperature they input is in Fahrenheit unless they specify that it's in Celsius.)
    *
    *        Internally we store things in metric / SI units, and do the appropriate conversions for input/display on
    *        other scales.  Conversion is handled by \c Unit objects.
    *
    *        See also \c Measurement.
    */
   class UnitSystem {
   public:
      /**
       * \enum RelativeScale
       *
       * \brief For some types of quantity, a given system of measurement will have multiple units, so we need to be able
       *        to order these units by relative size, eg, for fluid volume:
       *           fluid teaspoon < tablespoon < cup < pint < quart < gallon   (in both Imperial and US Customary systems)
       *           milliliters < liters                                        (in Metric/SI system)
       *        We only worry about units we actually use/permit, thus we don't, for example, care about where minims,
       *        fluid drams, gills etc fit in on the imperial / US customary volume scales, as we don't support them.
       *
       *        The \c scaleWithout value is used when a \c UnitSystem only has one \c Unit (eg as is typically the case
       *        with temperature, color and density).
       *
       *        The \c noScale value does not correspond to a \c Unit and is used as "not specified" or "not known"
       *        value.  (.:TODO:. At some point we should replace this usage with std::optional)
       */
      enum RelativeScale {
         noScale         = -1,
         scaleExtraSmall = 0,
         scaleSmall      = 1,
         scaleMedium     = 2,
         scaleLarge      = 3,
         scaleExtraLarge = 4,
         scaleHuge       = 5,
         scaleWithout    = 1000
      };

      /*!
       * \brief Constructor
       *
       * \param type
       * \param thickness
       * \param defaultUnit
       * \param scaleToUnitEntries
       * \param qstringToUnitEntries
       * \param uniqueName
       * \param systemOfMeasurementName
       */
      UnitSystem(Measurement::PhysicalQuantity const physicalQuantity,
                 Measurement::Unit const * const thickness,
                 Measurement::Unit const * const defaultUnit,
                 std::initializer_list<std::pair<Measurement::UnitSystem::RelativeScale const,
                                                 Measurement::Unit const *> > scaleToUnit,
                 char const * const uniqueName,
                 SystemOfMeasurement const systemOfMeasurement);

      ~UnitSystem();

      /**
       * \brief Test whether two \c UnitSystem references are the same.  (This is by no means a full test for equality,
       *        since we assume there is only one, constant, instance of each different \c UnitSystem.
       */
      bool operator==(UnitSystem const & other) const;

      /**
       * \brief The name that uniquely identifies this unit system.  This is not for display to the user, but rather so
       *        that we can save preferences via \c PersistentSettings.  It must be the \b same as the global variable
       *        in the \c UnitSystems namespace (because we rely on this in some places).
       */
      QString const uniqueName;

      /**
       * \brief The system of measurement to which this \c UnitSystem relates
       */
      SystemOfMeasurement const systemOfMeasurement;

      /*!
       * \brief Returns a string appropriately displaying 'amount' of type 'units' in this \c UnitSystem.  This string
       *        should also be recognised by \c qstringToSI()
       *
       * \param amount
       * \param units
       * \param precision
       * \param scale
       *
       * \return
       */
      QString displayAmount(double amount,
                            Unit const * units,
                            int precision = -1,
                            std::optional<Measurement::UnitSystem::RelativeScale> forcedScale = std::nullopt) const;

      /*!
       * \brief Converts the supplied the appropriate unit and scale to an amount in this \c UnitSystem. Similar in
       *        nature to \c displayAmount(), but just returning raw doubles.
       *
       * \param amount The amount to convert
       * \param units  The units of the amount to convert
       * \param scale  Optional: the scale of this \c UnitSystem to use for the output
       *
       * \return
       */
      double amountDisplay(double amount,
                           Unit const * units,
                           std::optional<Measurement::UnitSystem::RelativeScale> forcedScale = std::nullopt) const;

      /*!
       * \brief Converts 'qstr' (consisting of a decimal amount, optionally followed by a unit string) to the
       *        appropriate Metric/SI amount under this UnitSystem.  This is typically for parsing user input.  Eg, if
       *        the user is entering an amount into a volume field that is configured for Imperial units and a default
       *        scale of \c UnitSystem::scaleExtraLarge, we want:
       *         • "3" to be interpreted as 3 imperial gallons, and converted to litres
       *         • "3 qt" to be interpreted as 3 imperial quarts, and converted to litres
       *         • "3l" or "3 l" to be interpreted as 3 litres
       *        This gives the user a lot of flexibility on inputting amounts, within the limitations ambiguity between
       *        US Customary and Imperial volumes (eg if the user enters "3 pints", then it's reasonable to assume US
       *        pints if the field is configured for US Customary volumes and Imperial pints if it's configured for
       *        Imperial volumes, but if the user enters "3 pints" in a field that's configured for metric/SI volumes
       *        then we can't know for certain whether Imperial or US pints were meant).
       *
       * .:TBD:. Feels like we have one too many parameters on this function!
       *
       * \param qstr    The string to convert
       * \param defUnit The units to use if none are specified in the string
       * \param scale
       *
       * \return
       */
      double qstringToSI(QString qstr,
                         Unit const * defUnit = nullptr,
                         std::optional<Measurement::UnitSystem::RelativeScale> forcedScale = std::nullopt) const;

      /**
       * \brief returns all the \c UnitSystem::RelativeScale for this \c UnitSystem
       */
      QList<UnitSystem::RelativeScale> getRelativeScales() const;

      /*!
       * \brief Returns the \c Unit corresponding to \c scale in this \c UnitSystem
       */
      Unit const * scaleUnit(UnitSystem::RelativeScale scale) const;

      /*!
       * \brief Returns the unit associated with thickness. If this unit system is US weight, it would return lb. If it
       *        were US volume, it would return quarts.
       *
       * \return \c nullptr if thickness does not apply to this unit system (eg a temperature system)
       */
      Unit const * thicknessUnit() const;

      /*!
       * \brief Returns the default unit to use in this system - eg minutes for time, pounds for US weight
       */
      Unit const * unit() const;

      /**
       * \brief Return the \c PhysicalQuantity which this \c UnitSystem measures
       */
      Measurement::PhysicalQuantity getPhysicalQuantity() const;

      /**
       * \brief Returns a pointer to the named \c UnitSystem.  This make it easy to store in \c PersistentSettings the
       *        user's choices about which \c UnitSystem to use for each \c PhysicalQuantity
       * \param name
       * \return \c null if no \c UnitSystem with the supplied name exists
       */
      static UnitSystem const * getInstanceByUniqueName(QString const & name);

      static UnitSystem const & getInstance(SystemOfMeasurement const systemOfMeasurement,
                                            PhysicalQuantity const physicalQuantity);

      /**
       * \brief Returns a list of all the \c UnitSystem instances that relate to a particular \c PhysicalQuantity
       */
      static QList<UnitSystem const *> getUnitSystems(Measurement::PhysicalQuantity const physicalQuantity);

      static QString getUniqueName(Measurement::UnitSystem::RelativeScale relativeScale);

      /**
       * \brief Returns a \c RelativeScale from its unique name.  Useful for serialising.
       *        Returns \c std::nullopt if no \c RelativeScale exists for the supplied name
       */
      static std::optional<Measurement::UnitSystem::RelativeScale> getScaleFromUniqueName(QString relativeScaleAsString);

   private:
      // Private implementation details - see https://herbsutter.com/gotw/_100/
      class impl;
      std::unique_ptr<impl> pimpl;

      //! No copy constructor, as never want anyone, not even our friends, to make copies of a singleton
      UnitSystem(UnitSystem const&) = delete;
      //! No assignment operator , as never want anyone, not even our friends, to make copies of a singleton.
      UnitSystem& operator=(UnitSystem const&) = delete;
      //! No move constructor
      UnitSystem(UnitSystem &&) = delete;
      //! No move assignment
      UnitSystem & operator=(UnitSystem &&) = delete;
   };

   namespace UnitSystems {
      // Note, per https://en.wikipedia.org/wiki/United_States_customary_units#Mass_and_weight, that "For the pound and
      // smaller units, the US customary system and the British imperial system are identical.  However, they differ
      // when dealing with units larger than the pound."
      extern UnitSystem const mass_Imperial;
      extern UnitSystem const mass_UsCustomary;
      extern UnitSystem const mass_Metric;

      extern UnitSystem const volume_Imperial;
      extern UnitSystem const volume_UsCustomary;
      extern UnitSystem const volume_Metric;

      extern UnitSystem const temperature_MetricIsCelsius;
      extern UnitSystem const temperature_UsCustomaryIsFahrenheit;

      extern UnitSystem const time_CoordinatedUniversalTime;

      extern UnitSystem const color_StandardReferenceMethod;
      extern UnitSystem const color_EuropeanBreweryConvention;

      extern UnitSystem const density_SpecificGravity;
      extern UnitSystem const density_Plato;

      extern UnitSystem const diastaticPower_Lintner;
      extern UnitSystem const diastaticPower_WindischKolbach;

   }
}
#endif
