/*======================================================================================================================
 * UiAmountWithUnits.h is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Mark de Wever <koraq@xs4all.nl>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mike Evans <mikee@saxicola.co.uk>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Scott Peshak <scott@peshak.net>
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
#ifndef UIAMOUNTWITHUNITS_H
#define UIAMOUNTWITHUNITS_H
#pragma once

#include <memory> // For PImpl
#include <optional>

#include <QString>

#include "measurement/PhysicalQuantity.h"
#include "measurement/Unit.h"
#include "measurement/UnitSystem.h"
#include "utils/BtStringConst.h"

//======================================================================================================================
//========================================== Start of property name constants ==========================================
#define AddPropertyName(property) namespace PropertyNames::UiAmountWithUnits { BtStringConst const property{#property}; }
AddPropertyName(configSection)
#undef AddPropertyName
//=========================================== End of property name constants ===========================================
//======================================================================================================================

class QWidget;

struct PreviousScaleInfo {
   Measurement::SystemOfMeasurement oldSystemOfMeasurement;
   std::optional<Measurement::UnitSystem::RelativeScale> oldForcedScale = std::nullopt;
};

/**
 * \class UiAmountWithUnits A class, suitable for combining with \c QLabel, \c QLineEdit, etc, that handles all the unit
 *                          transformation such a widget would need to do.  It is inherited by \c BtDigitWidget and
 *                          \c BtAmountEdit.
 */
class UiAmountWithUnits {
public:
   /**
    * \param parent The \c QWidget that "owns" us.  Used for looking up config section names for retrieving forced
    *               scales etc for this individual field
    * \param physicalQuantities the \c PhysicalQuantity or \c Mixed2PhysicalQuantities to which this amount relates
    */
   UiAmountWithUnits(QWidget const * const parent,
                     Measurement::PhysicalQuantities const physicalQuantities);
   virtual ~UiAmountWithUnits();

   /**
    * \brief Returns what type of field this is - except that, if it is \c Mixed2PhysicalQuantities, will one of the two
    *        possible \c Measurement::PhysicalQuantity values depending on the value of \c this->units.
    */
   Measurement::PhysicalQuantity getPhysicalQuantity() const;

   /**
    * \brief If the \c Measurement::PhysicalQuantities supplied in the constructor was not a single
    *        \c Measurement::PhysicalQuantity, then this member function permits selecting the current
    *        \c Measurement::PhysicalQuantity from two in the \c Measurement::Mixed2PhysicalQuantities supplied in the
    *        constructor.
    */
   void selectPhysicalQuantity(Measurement::PhysicalQuantity const physicalQuantity);

   void setForcedSystemOfMeasurement(std::optional<Measurement::SystemOfMeasurement> systemOfMeasurement);
   void setForcedRelativeScale(std::optional<Measurement::UnitSystem::RelativeScale> relativeScale);

   std::optional<Measurement::SystemOfMeasurement> getForcedSystemOfMeasurement() const;
   std::optional<Measurement::UnitSystem::RelativeScale> getForcedRelativeScale() const;

   /**
    * \brief QString version of \c setForcedSystemOfMeasurement to work with code generated from .ui files (via
    *        Q_PROPERTY declared in subclass of this class)
    */
   void setForcedSystemOfMeasurementViaString(QString systemOfMeasurementAsString);

   /**
    * \brief QString version of \c getForcedSystemOfMeasurement to work with code generated from .ui files (via
    *        Q_PROPERTY declared in subclass of this class)
    */
   QString getForcedSystemOfMeasurementViaString() const;

   /**
    * \brief QString version of \c setForcedRelativeScale to work with code generated from .ui files (via Q_PROPERTY
    *        declared in subclass of this class)
    */
   void setForcedRelativeScaleViaString(QString relativeScaleAsString);

   /**
    * \brief QString version of \c getForcedRelativeScale to work with code generated from .ui files (via Q_PROPERTY
    *        declared in subclass of this class)
    */
   QString getForcedRelativeScaleViaString() const;

   // By defining the setters/getters, we can remove the need for initializeProperties.
   void    setEditField(QString editField);
   QString getEditField() const;

   void    setConfigSection(QString configSection);
   QString getConfigSection();

///   /**
///    * \brief Converts the numeric part of the input field to a double, ignoring any string suffix.  So "5.5 gal" will
///    *        give 5.5, "20L" will return 20.0, and so on.
///    */
///   double toDoubleRaw(bool * ok = nullptr) const;

   /**
    * \brief Returns the field converted to canonical units for the relevant \c Measurement::PhysicalQuantity
    *
    * \param rawValue field text to process
    * \return
    */
   Measurement::Amount rawToCanonical(QString const & rawValue) const;

   /**
    * \brief Use this when you want to do something with the returned QString
    *
    * \param amount Must be in canonical units eg kilograms for mass, liters for volume
    * \param precision Number of decimals to show .:TBD:. Remove default value here?
    */
   [[nodiscard]] QString displayAmount(double amount, int precision = 3) const;

   /**
    * \brief When the user has finished entering some text, this function does the corrections, eg if the field is set
    *        to show US Customary volumes and user enters an amount in liters (aka litres) then we need to convert it to
    *        display in pints or quarts etc.
    * \param enteredText Typically retrieved by caller from \c QLabel::text() or \c QLineEdit::text()
    * \param precision Number of decimals to show
    * \param previousScaleInfo
    *
    * \return Corrected text that caller should typically pass back to \c QLabel::setText() or \c QLineEdit::setText()
    */
   [[nodiscard]] QString correctEnteredText(QString const & enteredText,
                                            int precision,
                                            PreviousScaleInfo previousScaleInfo);

protected:
///   /**
///    * \brief
///    */
///   void textOrUnitsChanged(PreviousScaleInfo previousScaleInfo);
private:
///   /**
///    * \brief Returns the contents of the field converted, if necessary, to SI units
///    *
///    * \param oldSystemOfMeasurement
///    * \param oldScale (optional)
///    */
///   Measurement::Amount convertToSI(PreviousScaleInfo previousScaleInfo);

protected:
   /**
    * \brief If \c physicalQuantities is a \c Measurement::PhysicalQuantity, this is the \c Measurement::Unit that
    *        should be used to store the amount of this field.  This is normally fixed as our "standard" (normally
    *        metric) unit for the \c Measurement::PhysicalQuantity of the field -- eg kilograms for Mass, liters for
    *        Volume, celsius for Temperature, minutes for Time, etc.  However, for \c physicalQuantities of
    *        \c Mixed2PhysicalQuantities, this will need to vary between two different \c Measurement::Units values
    *        depending on which \c Measurement::PhysicalQuantity the field is currently set to measure.
    */
   Measurement::Unit const * m_canonicalUnits;

private:
   // Private implementation details - see https://herbsutter.com/gotw/_100/
   class impl;
   std::unique_ptr<impl> pimpl;
};

#endif
