/*======================================================================================================================
 * widgets/NumberWithUnits.h is part of Brewken, and is copyright the following authors 2009-2022:
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
#ifndef WIDGETS_NUMBERWITHUNITS_H
#define WIDGETS_NUMBERWITHUNITS_H
#pragma once

#include <QString>

#include "measurement/PhysicalQuantity.h"
#include "measurement/Unit.h"
#include "measurement/UnitSystem.h"

class QWidget;

/**
    * .:TODO:. Rename this UiAmountWithUnits and move it out of widgets folder
    *
 * \class NumberWithUnits Not strictly a widget, but a base class, suitable for combining with \c QLabel, \c QLineEdit,
 *                        etc, that handles all the unit transformation such a widget would need to do.
 */
class NumberWithUnits {
public:
   /**
    *
    */
   NumberWithUnits(QWidget * parent = nullptr,
                   Measurement::PhysicalQuantity physicalQuantity = Measurement::PhysicalQuantity::None,
                   Measurement::Unit const * units = nullptr);
   virtual ~NumberWithUnits();

   /**
    * \brief A class inheriting from this class is also expected to also inherit from a \c QWidget such as \c QLabel or
    *        \c QLineEdit.  We would like to be able to access the text() member function of that parent class in parts
    *        of our own implementation.  This is a bit tricky as \c QLabel::text() and \c QLineEdit::text() are actually
    *        unrelated, despite both having the same signature.  We therefore require child classes to implement this
    *        wrapper function that returns the value of \c text() from their other superclass.
    */
   virtual QString getWidgetText() const = 0;

   /**
    * \brief Similar to \c getText(), this allows this base class to access \c QLabel::setText() or
    *        \c QLineEdit::setText() in the subclass that also inherits from \c QLabel or \c QLineEdit.
    */
   virtual void setWidgetText(QString text) = 0;

   void setForcedUnitSystem(Measurement::UnitSystem const * forcedUnitSystem);
   Measurement::UnitSystem const * getForcedUnitSystem() const;

   /**
    * \brief QString version of \c setForcedUnitSystem to work with code generated from .ui files (via Q_PROPERTY
    *        declared in subclass of this class)
    */
   void setForcedUnitSystemViaString(QString forcedUnitSystemAsString);

   /**
    * \brief QString version of \c getForcedUnitSystem to work with code generated from .ui files (via Q_PROPERTY
    *        declared in subclass of this class)
    */
   QString getForcedUnitSystemViaString() const;

   void setForcedRelativeScale(Measurement::UnitSystem::RelativeScale forcedRelativeScale);
   Measurement::UnitSystem::RelativeScale getForcedRelativeScale() const;

   /**
    * \brief QString version of \c setForcedRelativeScale to work with code generated from .ui files (via Q_PROPERTY
    *        declared in subclass of this class)
    */
   void setForcedRelativeScaleViaString(QString forcedRelativeScaleAsString);

   /**
    * \brief QString version of \c getForcedRelativeScale to work with code generated from .ui files (via Q_PROPERTY
    *        declared in subclass of this class)
    */
   QString getForcedRelativeScaleViaString() const;

   // By defining the setters/getters, we can remove the need for
   // initializeProperties.
   void    setEditField(QString editField);
   QString getEditField() const;

   void    setConfigSection(QString configSection);
   QString getConfigSection();

   void setType(int type);
   int type() const;

   // Too many places still use getDouble, which just hoses me down. We're
   // gonna fix this.
   double  toDouble(bool* ok) const;

   /**
    * \brief Returns the contents of the field converted, if necessary, to SI units
    *
    * .:TBD:. Some overlap with \c convertToSI
    */
   double toSI();

   /**
    * \brief Use this when you want to do something with the returned QString
    */
   QString displayAmount(double amount, int precision = 3);

protected:
   /**
    * \brief
    */
   void textOrUnitsChanged(Measurement::UnitSystem const * oldUnitSystem,
                           Measurement::UnitSystem::RelativeScale oldScale);

   /**
    * \brief Returns the contents of the field converted, if necessary, to SI units
    *
    * \param oldUnitSystem (optional)
    * \param oldScale (optional)
    */
   double convertToSI(Measurement::UnitSystem const * oldUnitSystem = nullptr,
                      Measurement::UnitSystem::RelativeScale oldScale = Measurement::UnitSystem::noScale);

private:
   QWidget * parent;
protected:
   Measurement::PhysicalQuantity physicalQuantity;
   Measurement::Unit const * units;
   Measurement::UnitSystem const * forcedUnitSystem;
   Measurement::UnitSystem::RelativeScale forcedRelativeScale;
   QString editField;
   QString configSection;
};
#endif
