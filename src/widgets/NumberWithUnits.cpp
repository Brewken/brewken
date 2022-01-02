/*======================================================================================================================
 * widgets/NumberWithUnits.cpp is part of Brewken, and is copyright the following authors 2009-2021:
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
#include "widgets/NumberWithUnits.h"

#include <QDebug>
#include <QVariant>
#include <QWidget>

#include "Localization.h"
#include "measurement/Measurement.h"

NumberWithUnits::NumberWithUnits(QWidget * parent,
                                 Measurement::PhysicalQuantity physicalQuantity,
                                 Measurement::Unit const * units) :
   parent{parent},
   physicalQuantity{physicalQuantity},
   units{units},
   forcedUnitSystem{nullptr},
   forcedRelativeScale(Measurement::UnitSystem::noScale) {
   return;
}

NumberWithUnits::~NumberWithUnits() = default;

Measurement::UnitSystem const * NumberWithUnits::getForcedUnitSystem() const {
   return this->forcedUnitSystem;
}

void NumberWithUnits::setForcedUnitSystem(Measurement::UnitSystem const * forcedUnitSystem) {
   this->forcedUnitSystem = forcedUnitSystem;
   return;
}

void NumberWithUnits::setForcedUnitSystemViaString(QString forcedUnitSystemAsString) {
   this->forcedUnitSystem = Measurement::UnitSystem::getInstanceByUniqueName(forcedUnitSystemAsString);
   if (nullptr == this->forcedUnitSystem && forcedUnitSystemAsString!= "") {
      // It's a coding error if someone sent us an invalid name for a UnitSystem.  (Note that the variable names of the
      // global constants in the Measurement::UnitSystems namespace are supposed to match the corresponding uniqueName
      // member variables, per the comment in measurement/UnitSystem.h header file.)
      qCritical() << Q_FUNC_INFO << "Unable to find UnitSystem called" << forcedUnitSystemAsString;
      Q_ASSERT(false); // Stop here on a debug build
   }
   return;
}

QString NumberWithUnits::getForcedUnitSystemViaString() const {
   return (nullptr == this->forcedUnitSystem) ? "" : this->forcedUnitSystem->uniqueName;
}

void NumberWithUnits::setForcedRelativeScale(Measurement::UnitSystem::RelativeScale forcedRelativeScale) {
   this->forcedRelativeScale = forcedRelativeScale;
   return;
}

Measurement::UnitSystem::RelativeScale NumberWithUnits::getForcedRelativeScale() const {
   return this->forcedRelativeScale;
}

/**
   * \brief QString version of \c setForcedRelativeScale to work with code generated from .ui files (via Q_PROPERTY
   *        declared in subclass of this class)
   */
void NumberWithUnits::setForcedRelativeScaleViaString(QString forcedRelativeScaleAsString) {
   this->forcedRelativeScale = Measurement::UnitSystem::relativeScaleFromString(forcedRelativeScaleAsString);
   return;
}

/**
   * \brief QString version of \c getForcedRelativeScale to work with code generated from .ui files (via Q_PROPERTY
   *        declared in subclass of this class)
   */
QString NumberWithUnits::getForcedRelativeScaleViaString() const {
   return Measurement::UnitSystem::relativeScaleToString(this->forcedRelativeScale);
}

void NumberWithUnits::setEditField(QString editField) {
   this->editField = editField;
   return;
}

QString NumberWithUnits::getEditField() const {
   return this->editField;
}

// The cascade looks a little odd, but it is intentional.
void NumberWithUnits::setConfigSection(QString configSection) {
   this->configSection = configSection;

   if (this->configSection.isEmpty()) {
      this->configSection = this->parent->property("configSection").toString();
   }

   if (this->configSection.isEmpty()) {
      this->configSection = this->parent->objectName();
   }
   return;
}

QString NumberWithUnits::getConfigSection() {
   if (this->configSection.isEmpty()) {
      this->setConfigSection("");
   }

   return this->configSection;
}

void NumberWithUnits::setType(int type) {
   // .:TBD:. Why do we need to pass in int and then cast?  Why not pass PhysicalQuantity?
   this->physicalQuantity = static_cast<Measurement::PhysicalQuantity>(type);
   return;
}

int NumberWithUnits::type() const {
   // .:TBD:. Why can't we just return PhysicalQuantity?
   return static_cast<int>(this->physicalQuantity);
}

double NumberWithUnits::toDouble(bool * ok) const {
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

double NumberWithUnits::toSI() {
   bool ok = false;
   double amt = this->toDouble(&ok);
   if (!ok) {
      qWarning() <<
         Q_FUNC_INFO << "Could not convert " << this->getWidgetText() << " (" << this->configSection << ":" <<
         this->editField << ") to double";
   }
   return this->units->toSI(amt);
}


QString NumberWithUnits::displayAmount( double amount, int precision) {
   Measurement::UnitSystem const * displayUnitSystem;
   if (this->forcedUnitSystem != nullptr) {
      displayUnitSystem = this->forcedUnitSystem;
   } else {
      displayUnitSystem = Measurement::getUnitSystemForField(this->editField, this->configSection);
   }

   Measurement::UnitSystem::RelativeScale relativeScale = Measurement::getRelativeScaleForField(this->editField,
                                                                                                this->configSection);

   // I find this a nice level of abstraction. This lets all of the setText()
   // methods make a single call w/o having to do the logic for finding the
   // unit and scale.
   return Measurement::displayAmount(amount, this->units, precision, displayUnitSystem, relativeScale);
}

void NumberWithUnits::textOrUnitsChanged(Measurement::UnitSystem const * oldUnitSystem,
                                         Measurement::UnitSystem::RelativeScale oldScale) {
   // This is where it gets hard
   double val = -1.0;
   QString amt;

   if (this->getWidgetText().isEmpty()) {
      return;
   }

   // The idea here is we need to first translate the field into a known
   // amount (aka to SI) and then into the unit we want.
   switch(this->physicalQuantity) {
      case Measurement::PhysicalQuantity::Mass:
      case Measurement::PhysicalQuantity::Volume:
      case Measurement::PhysicalQuantity::Temperature:
      case Measurement::PhysicalQuantity::Time:
      case Measurement::PhysicalQuantity::Density:
         val = this->convertToSI(oldUnitSystem, oldScale);
         amt = this->displayAmount(val, 3);
         break;
      case Measurement::PhysicalQuantity::Color:
         val = this->convertToSI(oldUnitSystem, oldScale);
         amt = this->displayAmount(val, 0);
         break;
      case Measurement::PhysicalQuantity::String:
         amt = this->getWidgetText();
         break;
      case Measurement::PhysicalQuantity::DiastaticPower:
         val = this->convertToSI(oldUnitSystem, oldScale);
         amt = this->displayAmount(val, 3);
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

double NumberWithUnits::convertToSI(Measurement::UnitSystem const * oldUnitSystem,
                                    Measurement::UnitSystem::RelativeScale oldScale) {
   qDebug() <<
      Q_FUNC_INFO << "Old Unit system:" << (nullptr == oldUnitSystem ? "NULL" : oldUnitSystem->uniqueName) <<
      ", oldScale: " << oldScale;
   // .:TBD:. 2021-12-31 MY: My gut instinct is that the logic here is more complicated than it needs to be.  It would
   //                        be nice to see if we can add some unit tests for all the edge cases and then simplify.
   Measurement::UnitSystem const * dspUnitSystem  = oldUnitSystem;
   Measurement::UnitSystem::RelativeScale   dspScale = oldScale;
   // If units are specified in the text, try to use those.  Otherwise, if we are not forcing the unit & scale, we need
   // to read the configured properties
   if (Localization::hasUnits(this->getWidgetText())) {
      // In theory, we just grab the units that the user has specified in the input text.  In reality, it's not that
      // easy as we sometimes need to disambiguate - eg between Imperial gallons and US customary ones.  So, if we have
      // old or current units then that helps with this - eg, if current units are US customary cups and user enters
      // gallons, then we'll go with US customary gallons over Imperial ones.
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

      return dspUnitSystem->qstringToSI(this->getWidgetText(), works, dspScale);
   }

   if (this->physicalQuantity == Measurement::PhysicalQuantity::String) {
      return 0.0;
   }

//      return Measurement::qStringToSI(this->text(), this->physicalQuantity, dspUnitSystem, dspScale)

   // If all else fails, simply try to force the contents of the field to a
   // double. This doesn't seem advisable?
   bool ok = false;
   double amt = this->toDouble(&ok);
   if (!ok) {
      qWarning() <<
         Q_FUNC_INFO << "Could not convert " << this->getWidgetText() << " (" << this->configSection << ":" << this->editField <<
         ") to double";
   }

   return amt;
}
