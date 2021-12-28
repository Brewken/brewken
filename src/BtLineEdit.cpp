/*======================================================================================================================
 * BtLineEdit.cpp is part of Brewken, and is copyright the following authors 2009-2021:
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
#include "BtLineEdit.h"

#include <QDebug>
#include <QSettings>
#include <QStyle>

#include "Algorithms.h"
#include "Brewken.h"
#include "Localization.h"
#include "measurement/Measurement.h"
#include "measurement/Unit.h"
#include "measurement/UnitSystem.h"
#include "model/NamedEntity.h"
#include "PersistentSettings.h"

namespace {
   int const min_text_size = 8;
   int const max_text_size = 50;
}

BtLineEdit::BtLineEdit(QWidget *parent,
                       Measurement::PhysicalQuantity physicalQuantity,
                       QString const & maximalDisplayString) :
   QLineEdit(parent),
   btParent(parent),
   physicalQuantity(physicalQuantity),
   forcedUnitSystem(nullptr)/*,
   forcedRelativeScale(Measurement::UnitSystem::noScale)*/ {
   this->_section = property("configSection").toString();
   connect(this, &QLineEdit::editingFinished, this, &BtLineEdit::onLineChanged);

   // We can work out (and store) our display size here, but not yet set it.  The way the Designer UI Files work is to
   // generate code that calls setters such as setMaximumWidth() etc, which would override anything we do here in the
   // constructor.  So we set our size when setText() is called.
   this->calculateDisplaySize(maximalDisplayString);

   return;
}

BtLineEdit::~BtLineEdit() = default;

void BtLineEdit::onLineChanged() {
   this->lineChanged(nullptr, Measurement::UnitSystem::noScale);
   return;
}

void BtLineEdit::lineChanged(Measurement::UnitSystem const * oldUnitSystem,
                             Measurement::UnitSystem::RelativeScale oldScale) {
   // This is where it gets hard
   double val = -1.0;
   QString amt;

   // editingFinished happens on focus being lost, regardless of anything
   // being changed. I am hoping this short circuits properly and we do
   // nothing if nothing changed.
   if ( sender() == this && ! isModified() ) {
      return;
   }

   if (text().isEmpty()) {
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
         amt = this->text();
         break;
      case Measurement::PhysicalQuantity::DiastaticPower:
         val = this->convertToSI(oldUnitSystem, oldScale);
         amt = this->displayAmount(val, 3);
         break;
      case Measurement::PhysicalQuantity::None:
      default:
         {
            bool ok = false;
            val = Localization::toDouble(text(), &ok);
            if ( ! ok )
               qWarning() << QString("%1: failed to convert %2 (%3:%4) to double").arg(Q_FUNC_INFO).arg(text()).arg(_section).arg(_editField);
            amt = displayAmount(val);
         }
   }
   QLineEdit::setText(amt);

   if (sender() == this) {
      emit textModified();
   }

   return;
}

double BtLineEdit::convertToSI(Measurement::UnitSystem const * oldUnitSystem,
                               Measurement::UnitSystem::RelativeScale oldScale) {
   Measurement::UnitSystem const * dspUnitSystem  = oldUnitSystem;
   Measurement::UnitSystem::RelativeScale   dspScale = oldScale;
   // If units are specified in the text, just use those.  Otherwise, if we are not forcing the unit & scale, we need to
   // read the configured properties
   if (!Localization::hasUnits(this->text())) {
      // If the display unit system is forced, use this as the default one.
      if (this->forcedUnitSystem != nullptr) {
         dspUnitSystem = this->forcedUnitSystem;
      } else {
         dspUnitSystem = Measurement::getUnitSystemForField(this->_editField, this->_section);
      }

      // If the display scale is forced, use this scale as the default one.
//      if (this->forcedRelativeScale != Measurement::UnitSystem::noScale) {
//         dspScale = this->forcedRelativeScale;
//      } else {
         dspScale = Measurement::getRelativeScaleForField(this->_editField, this->_section);
//      }
   }

   if (nullptr != dspUnitSystem) {
      Measurement::Unit const * works = dspUnitSystem->scaleUnit(dspScale);
      if (!works) {
         // If we didn't find the unit, default to the UnitSystem's default
         // unit
         works = dspUnitSystem->unit();
      }

      return dspUnitSystem->qstringToSI(this->text(), works, dspScale);
   }

   if (this->physicalQuantity == Measurement::PhysicalQuantity::String) {
      return 0.0;
   }

   // If all else fails, simply try to force the contents of the field to a
   // double. This doesn't seem advisable?
   bool ok = false;
   double amt = this->toDouble(&ok);
   if (!ok) {
      qWarning() <<
         Q_FUNC_INFO << "Could not convert " << text() << " (" << this->_section << ":" << this->_editField <<
         ") to double";
   }

   return amt;
}

double BtLineEdit::toSI() {
   bool ok = false;
   double amt = this->toDouble(&ok);
   if (!ok) {
      qWarning() <<
         Q_FUNC_INFO << "Could not convert " << text() << " (" << this->_section << ":" << this->_editField <<
         ") to double";
   }
   return this->_units->toSI(amt);
}

QString BtLineEdit::displayAmount( double amount, int precision) {
   Measurement::UnitSystem const * displayUnitSystem;
   if (this->forcedUnitSystem != nullptr) {
      displayUnitSystem = this->forcedUnitSystem;
   } else {
      displayUnitSystem = Measurement::getUnitSystemForField(this->_editField, this->_section);
   }

   Measurement::UnitSystem::RelativeScale relativeScale = Measurement::getRelativeScaleForField(this->_editField,
                                                                                                this->_section);

   // I find this a nice level of abstraction. This lets all of the setText()
   // methods make a single call w/o having to do the logic for finding the
   // unit and scale.
   return Measurement::displayAmount(amount, _units, precision, displayUnitSystem, relativeScale);
}

double BtLineEdit::toDouble(bool* ok) {
   QRegExp amtUnit;

   if ( ok ) {
      *ok = true;
   }

   // Make sure we get the right decimal point (. or ,) and the right grouping
   // separator (, or .). Some locales write 1.000,10 and other write
   // 1,000.10. We need to catch both
   QString decimal = QRegExp::escape( QLocale::system().decimalPoint());
   QString grouping = QRegExp::escape(QLocale::system().groupSeparator());

   amtUnit.setPattern("((?:\\d+" + grouping + ")?\\d+(?:" + decimal + "\\d+)?|" + decimal + "\\d+)\\s*(\\w+)?");
   amtUnit.setCaseSensitivity(Qt::CaseInsensitive);

   // if the regex dies, return 0.0
   if (amtUnit.indexIn(text()) == -1) {
      if ( ok ) {
         *ok = false;
      }
      return 0.0;
   }

   return Localization::toDouble(amtUnit.cap(1), Q_FUNC_INFO);
}

void BtLineEdit::setText( double amount, int precision) {
   QLineEdit::setText( displayAmount(amount,precision) );
   this->setDisplaySize();
   return;
}

void BtLineEdit::setText(NamedEntity * element, int precision) {
   double amount = 0.0;
   QString display;

   if (physicalQuantity == Measurement::PhysicalQuantity::String ) {
      display = element->property(_editField.toLatin1().constData()).toString();
   } else if ( element->property(_editField.toLatin1().constData()).canConvert(QVariant::Double) ) {
      bool ok = false;
      // Get the value from the element, and put it in a QVariant
      QVariant tmp = element->property(_editField.toLatin1().constData());
      // It is important here to use QVariant::toDouble() instead of going
      // through toString() and then Localization::toDouble().
      amount = tmp.toDouble(&ok);
      if ( !ok ) {
         qWarning() << QString("%1 could not convert %2 (%3:%4) to double")
                              .arg(Q_FUNC_INFO)
                              .arg(tmp.toString())
                              .arg(_section)
                              .arg(_editField);
      }

      display = displayAmount(amount, precision);
   } else {
      display = "?";
   }

   QLineEdit::setText(display);
   this->setDisplaySize(physicalQuantity == Measurement::PhysicalQuantity::String);
   return;
}

void BtLineEdit::setText( QString amount, int precision) {
   bool ok = false;
   bool force = false;

   if (this->physicalQuantity == Measurement::PhysicalQuantity::String) {
      QLineEdit::setText(amount);
      force = true;
   } else {
      double amt = Localization::toDouble(amount, &ok);
      if (!ok) {
         qWarning() <<
            Q_FUNC_INFO << "Could not convert" << amount << "(" << _section << ":" << _editField << ") to double";
      }
      QLineEdit::setText(displayAmount(amt, precision));
   }

   this->setDisplaySize(force);
   return;
}

void BtLineEdit::setText( QVariant amount, int precision) {
   setText(amount.toString(), precision);
   return;
}

int BtLineEdit::type() const {
   // .:TBD:. Why can't we just return PhysicalQuantity?
   return static_cast<int>(this->physicalQuantity);
}

QString BtLineEdit::editField() const {
   return this->_editField;
}

QString BtLineEdit::configSection() {
   if (this->_section.isEmpty() ) {
      setConfigSection("");
   }

   return this->_section;
}

/*
// Once we require >qt5.5, we can replace this noise with
// QMetaEnum::fromType()
QString BtLineEdit::forcedUnit() const
{
   const QMetaObject &mo = Measurement::Unit::staticMetaObject;
   int index = mo.indexOfEnumerator("unitDisplay");
   QMetaEnum unitEnum = mo.enumerator(index);

   return QString( unitEnum.valueToKey(this->forcedUnitSystem) );
}

QString BtLineEdit::forcedScale() const
{
   const QMetaObject &mo = Measurement::Unit::staticMetaObject;
   int index = mo.indexOfEnumerator("RelativeScale");
   QMetaEnum scaleEnum = mo.enumerator(index);

   return QString( scaleEnum.valueToKey(this->_forceScale) );
}
*/

void BtLineEdit::setType(int type) {
   // .:TBD:. Why do we need to pass in int and then cast?  Why not pass PhysicalQuantity?
   this->physicalQuantity = static_cast<Measurement::PhysicalQuantity>(type);
   return;
}

void BtLineEdit::setEditField( QString editField) {
   this->_editField = editField;
   return;
}

// The cascade looks a little odd, but it is intentional.
void BtLineEdit::setConfigSection( QString configSection) {
   this->_section = configSection;

   if (this->_section.isEmpty()) {
      this->_section = btParent->property("configSection").toString();
   }

   if (this->_section.isEmpty()) {
      this->_section = btParent->objectName();
   }
   return;
}

/*
// previous comment about qt5.5 applies
void BtLineEdit::setForcedUnit( QString forcedUnit ) {
   const QMetaObject &mo = Measurement::Unit::staticMetaObject;
   int index = mo.indexOfEnumerator("unitDisplay");
   QMetaEnum unitEnum = mo.enumerator(index);

   this->_forceUnit = (Measurement::Unit::unitDisplay)unitEnum.keyToValue(forcedUnit.toStdString().c_str());
   return;
}

void BtLineEdit::setForcedScale( QString forcedScale ) {
   const QMetaObject &mo = Measurement::Unit::staticMetaObject;
   int index = mo.indexOfEnumerator("RelativeScale");
   QMetaEnum unitEnum = mo.enumerator(index);

   this->_forceScale = (Measurement::UnitSystem::RelativeScale)unitEnum.keyToValue(forcedScale.toStdString().c_str());
   return;
}
*/

Measurement::UnitSystem const * BtLineEdit::getForcedUnitSystem() const {
   return this->forcedUnitSystem;
}

void BtLineEdit::setForcedUnitSystem(Measurement::UnitSystem const * forcedUnitSystem) {
   this->forcedUnitSystem = forcedUnitSystem;
   return;
}

/*
Measurement::UnitSystem::RelativeScale BtLineEdit::getForcedRelativeScale() const {
   return this->forcedRelativeScale;
}

void BtLineEdit::setForcedRelativeScale(Measurement::UnitSystem::RelativeScale forcedRelativeScale) {
   this->forcedRelativeScale = forcedRelativeScale;
   return;
}
*/

void BtLineEdit::calculateDisplaySize(QString const & maximalDisplayString) {
   //
   // By default, some, but not all, boxes have a min and max width of 100 pixels, but this is not wide enough on a
   // high DPI display.  We instead calculate width here based on font-size - but without reducing any existing minimum
   // width.
   //
   // Unfortunately, for a QLineEdit object, calculating the width is hard because, besides the text, we need to allow
   // for the width of padding and frame, which is non-trivial to discover.  Eg, typically:
   //   marginsAroundText() and contentsMargins() both return 0 for left and right margins
   //   contentsRect() and frameSize() both give the same width as width()
   // AFAICT, the best option is to query via pixelMetric() calls to the widget's style, but we need to check this works
   // in practice on a variety of different systems.
   //
   QFontMetrics displayFontMetrics(this->font());
   QRect minimumTextRect = displayFontMetrics.boundingRect(maximalDisplayString);
   QMargins marginsAroundText = this->textMargins();
   auto myStyle = this->style();
   // NB: 2× frame width as on left and right; same for horizontal spacing
   int totalWidgetWidthForMaximalDisplayString = minimumTextRect.width() +
                                                 marginsAroundText.left() +
                                                 marginsAroundText.right() +
                                                 (2 * myStyle->pixelMetric(QStyle::PM_DefaultFrameWidth)) +
                                                 (2 * myStyle->pixelMetric(QStyle::PM_LayoutHorizontalSpacing));

   this->desiredWidthInPixels = qMax(this->minimumWidth(), totalWidgetWidthForMaximalDisplayString);
   return;
}

void BtLineEdit::setDisplaySize(bool recalculate) {
   if ( recalculate ) {
      QString sizing_string = text();

      // this is a dirty bit of cheating. If we do not reset the minimum
      // width, the field only ever gets bigger. This forces the resize I
      // want, but only when we are instructed to force it
      setMinimumWidth(0);
      if ( sizing_string.length() < min_text_size ) {
         sizing_string = QString(min_text_size,'a');
      } else if ( sizing_string.length() > max_text_size ) {
         sizing_string = QString(max_text_size,'a');
      }
      calculateDisplaySize(sizing_string);
   }
   this->setFixedWidth(this->desiredWidthInPixels);
   return;
}

BtGenericEdit::BtGenericEdit(QWidget *parent) : BtLineEdit(parent, Measurement::PhysicalQuantity::None) {
   this->_units = 0;
   return;
}

BtMassEdit::BtMassEdit(QWidget *parent) : BtLineEdit(parent, Measurement::PhysicalQuantity::Mass) {
   this->_units = &Measurement::Units::kilograms;
   return;
}

BtVolumeEdit::BtVolumeEdit(QWidget *parent) : BtLineEdit(parent, Measurement::PhysicalQuantity::Volume) {
   this->_units = &Measurement::Units::liters;
   return;
}

BtTemperatureEdit::BtTemperatureEdit(QWidget *parent) : BtLineEdit(parent, Measurement::PhysicalQuantity::Temperature) {
   this->_units = &Measurement::Units::celsius;
   return;
}

BtTimeEdit::BtTimeEdit(QWidget *parent) : BtLineEdit(parent, Measurement::PhysicalQuantity::Time) {
   this->_units = &Measurement::Units::minutes;
   return;
}

BtDensityEdit::BtDensityEdit(QWidget *parent) : BtLineEdit(parent, Measurement::PhysicalQuantity::Density) {
   this->_units = &Measurement::Units::sp_grav;
   return;
}

BtColorEdit::BtColorEdit(QWidget *parent) : BtLineEdit(parent, Measurement::PhysicalQuantity::Color) {
   this->_units = &Measurement::Units::srm;
   return;
}

BtStringEdit::BtStringEdit(QWidget *parent) : BtLineEdit(parent, Measurement::PhysicalQuantity::String) {
   this->_units = 0;
   return;
}

BtMixedEdit::BtMixedEdit(QWidget *parent) : BtLineEdit(parent, Measurement::PhysicalQuantity::Mixed) {
   // This is probably pure evil I will later regret
   this->physicalQuantity = Measurement::PhysicalQuantity::Volume;
   this->_units = &Measurement::Units::liters;
   return;
}

void BtMixedEdit::setIsWeight(bool state) {
   // But you have to admit, this is clever
   if (state) {
      this->physicalQuantity = Measurement::PhysicalQuantity::Mass;
      this->_units = &Measurement::Units::kilograms;
   } else {
      this->physicalQuantity = Measurement::PhysicalQuantity::Volume;
      this->_units = &Measurement::Units::liters;
   }

   // maybe? My head hurts now
   this->onLineChanged();
   return;
}

BtDiastaticPowerEdit::BtDiastaticPowerEdit(QWidget *parent) :
   BtLineEdit(parent, Measurement::PhysicalQuantity::DiastaticPower) {
   this->_units = &Measurement::Units::lintner;
   return;
}
