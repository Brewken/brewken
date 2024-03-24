/*======================================================================================================================
 * widgets/SmartAmountSettings.h is part of Brewken, and is copyright the following authors 2023:
 *   • Matt Young <mfsy@yahoo.com>
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
#ifndef WIDGETS_SMARTAMOUNTSETTINGS_H
#define WIDGETS_SMARTAMOUNTSETTINGS_H
#pragma once

#include <memory> // For PImpl
#include <optional>

#include "measurement/PhysicalQuantity.h"
#include "measurement/SystemOfMeasurement.h"
#include "measurement/Unit.h"
#include "measurement/UnitSystem.h"
#include "utils/TypeLookup.h"
#include "widgets/SmartAmounts.h"

/**
 * \class SmartAmountSettings
 *
 * \brief Holds the settings chosen by the user for a given field.  Per comments in \c widgets/SmartLabel.h, depending
 *        on the circumstances, it is sometimes \c SmartLabel and sometimes \c SmartField that needs to hold this info.
 */
class SmartAmountSettings {
public:
   SmartAmountSettings(char const * const        editorName,
                       char const * const        labelOrFieldName,
                       TypeInfo     const &      typeInfo,
                       Measurement::Unit const * fixedDisplayUnit);
   ~SmartAmountSettings();

   TypeInfo const & getTypeInfo() const;

   void setForcedSystemOfMeasurement(std::optional<Measurement::SystemOfMeasurement> systemOfMeasurement);
   void setForcedRelativeScale(std::optional<Measurement::UnitSystem::RelativeScale> relativeScale);
   std::optional<Measurement::SystemOfMeasurement> getForcedSystemOfMeasurement() const;
   std::optional<Measurement::UnitSystem::RelativeScale> getForcedRelativeScale() const;

   /**
    * \brief Get the current settings (which may come from system-wide defaults) for \c SystemOfMeasurement and
    *        \c RelativeScale.
    */
   SmartAmounts::ScaleInfo getScaleInfo() const;

   Measurement::UnitSystem const & getUnitSystem(SmartAmounts::ScaleInfo const & scaleInfo) const;

   /**
    * \brief Returns the \c UnitSystem that should be used to display this field, based on the forced
    *        \c SystemOfMeasurement for the field if there is one or otherwise on the the system-wide default
    *        \c UnitSystem for the field's \c PhysicalQuantity.
    */
   Measurement::UnitSystem const & getDisplayUnitSystem() const;

   /**
    * \brief Returns what type of field this is - except that, if it is \c Mixed2PhysicalQuantities, will one of the two
    *        possible \c Measurement::PhysicalQuantity values depending on the value of \c this->units.
    *
    *        It is a coding error to call this function if our field type \c is \c NonPhysicalQuantity.)
    */
   Measurement::PhysicalQuantity getPhysicalQuantity() const;

   /**
    * \brief If the \c Measurement::PhysicalQuantities supplied in the \c init call was not a single
    *        \c Measurement::PhysicalQuantity, then this member function permits selecting the current
    *        \c Measurement::PhysicalQuantity from two in the \c Measurement::Mixed2PhysicalQuantities supplied in the
    *        constructor.
    *
    *        NB: Caller's responsibility to ensure the display gets updated.  (SmartBase handles this.)
    */
   void selectPhysicalQuantity(Measurement::PhysicalQuantity const physicalQuantity);

   /**
    * \brief Alternative version of \c selectPhysicalQuantity for generic usage.  By convention, whenever we have a
    *        checkbox for "Amount is weight?" or "Amount is mass concentration?", \c true (ie box checked) is selecting
    *        the first of the two values in the \c Mixed2PhysicalQuantities pair (eg \c Mass in \c PqEitherMassOrVolume
    *        or \c MassConcentration in \c PqEitherMassOrVolumeConcentration).  So, passing in the boolean state of the
    *        checkbox to this function selects the correct option.
    *
    *        NB: Caller's responsibility to ensure the display gets updated.  (SmartBase handles this.)
    */
   void selectPhysicalQuantity(bool const isFirst);

   /**
    * \brief Use this when you want to do something with the returned QString
    *
    * \param quantity Must be in canonical units eg kilograms for mass, liters for volume
    */
   [[nodiscard]] QString displayAmount(double quantity, unsigned int precision) const;

   /**
    * \brief This version is not const as we'll set the field's \c PhysicalQuantity based on the \c Unit in \c amount.
    */
   [[nodiscard]] QString displayAmount(Measurement::Amount const & amount, unsigned int precision);

private:
   // Private implementation details - see https://herbsutter.com/gotw/_100/
   class impl;
   std::unique_ptr<impl> pimpl;
};

#endif
