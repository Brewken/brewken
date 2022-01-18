/*======================================================================================================================
 * BtLineEdit.cpp is part of Brewken, and is copyright the following authors 2009-2022:
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
#include "Localization.h"
#include "measurement/Measurement.h"
#include "measurement/Unit.h"
#include "measurement/UnitSystem.h"
#include "model/NamedEntity.h"
#include "PersistentSettings.h"
#include "utils/OptionalToStream.h"

namespace {
   int const min_text_size = 8;
   int const max_text_size = 50;
}

BtLineEdit::BtLineEdit(QWidget *parent,
                       BtFieldType fieldType,
                       Measurement::Unit const * units,
                       QString const & maximalDisplayString) :
   QLineEdit(parent),
   UiAmountWithUnits(parent, fieldType, units) {
   this->configSection = property("configSection").toString();
   connect(this, &QLineEdit::editingFinished, this, &BtLineEdit::onLineChanged);

   // We can work out (and store) our display size here, but not yet set it.  The way the Designer UI Files work is to
   // generate code that calls setters such as setMaximumWidth() etc, which would override anything we do here in the
   // constructor.  So we set our size when setText() is called.
   this->calculateDisplaySize(maximalDisplayString);

   return;
}

BtLineEdit::~BtLineEdit() = default;

QString BtLineEdit::getWidgetText() const {
   return this->text();
}

void BtLineEdit::setWidgetText(QString text) {
   this->QLineEdit::setText(text);
   return;
}

void BtLineEdit::onLineChanged() {
   qDebug() <<
      Q_FUNC_INFO << "this->fieldType=" << this->fieldType << "this->units=" << (nullptr == this->units ? "NULL" : this->units->name) <<
      ", this->forcedSystemOfMeasurement=" << this->forcedSystemOfMeasurement;

   if (!std::holds_alternative<Measurement::PhysicalQuantity>(this->fieldType) ||
       Measurement::PhysicalQuantity::None == std::get<Measurement::PhysicalQuantity>(this->fieldType)) {
      return;
   }

   Measurement::UnitSystem const & oldUnitSystem =
      Measurement::getUnitSystemForField(this->editField,
                                         this->configSection,
                                         std::get<Measurement::PhysicalQuantity>(this->fieldType));
   PreviousScaleInfo previousScaleInfo{
      oldUnitSystem.systemOfMeasurement,
      Measurement::getForcedRelativeScaleForField(this->editField, this->configSection)
   };

   this->lineChanged(previousScaleInfo);
   return;
}

void BtLineEdit::lineChanged(PreviousScaleInfo previousScaleInfo) {
   // editingFinished happens on focus being lost, regardless of anything
   // being changed. I am hoping this short circuits properly and we do
   // nothing if nothing changed.
   if (this->sender() == this && !isModified()) {
      return;
   }

   this->textOrUnitsChanged(previousScaleInfo);

   if (sender() == this) {
      emit textModified();
   }

   return;
}

void BtLineEdit::setText(double amount, int precision) {
   this->setWidgetText(this->displayAmount(amount, precision));
   this->setDisplaySize();
   return;
}

void BtLineEdit::setText(NamedEntity * element, int precision) {
   QString display;

   bool force = false;
   if (std::holds_alternative<NonPhysicalQuantity>(this->fieldType) &&
       NonPhysicalQuantity::String == std::get<NonPhysicalQuantity>(this->fieldType)) {
      display = element->property(editField.toLatin1().constData()).toString();
      force = true;
   } else if ( element->property(editField.toLatin1().constData()).canConvert(QVariant::Double) ) {
      bool ok = false;
      // Get the value from the element, and put it in a QVariant
      QVariant tmp = element->property(editField.toLatin1().constData());
      // It is important here to use QVariant::toDouble() instead of going
      // through toString() and then Localization::toDouble().
      double amount = tmp.toDouble(&ok);
      if (!ok) {
         qWarning() <<
            Q_FUNC_INFO << "Could not convert " << tmp.toString() << " (" << this->configSection << ":" <<
            this->editField << ") to double";
      }

      display = this->displayAmount(amount, precision);
   } else {
      display = "?";
   }

   this->setWidgetText(display);
   this->setDisplaySize(force);
   return;
}

void BtLineEdit::setText(QString amount, int precision) {
   bool force = false;

   if (std::holds_alternative<NonPhysicalQuantity>(this->fieldType) &&
       NonPhysicalQuantity::String == std::get<NonPhysicalQuantity>(this->fieldType)) {
      this->setWidgetText(amount);
      force = true;
   } else {
      bool ok = false;
      double amt = Localization::toDouble(amount, &ok);
      if (!ok) {
         qWarning() <<
            Q_FUNC_INFO << "Could not convert" << amount << "(" << this->configSection << ":" << this->editField <<
            ") to double";
      }
      this->setWidgetText(displayAmount(amt, precision));
   }

   this->setDisplaySize(force);
   return;
}

void BtLineEdit::setText(QVariant amount, int precision) {
   this->setText(amount.toString(), precision);
   return;
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

BtGenericEdit::BtGenericEdit(QWidget *parent) : BtLineEdit(parent, Measurement::PhysicalQuantity::None, nullptr) {
   return;
}

BtMassEdit::BtMassEdit(QWidget *parent) : BtLineEdit(parent, Measurement::PhysicalQuantity::Mass, &Measurement::Units::kilograms) {
   return;
}

BtVolumeEdit::BtVolumeEdit(QWidget *parent) : BtLineEdit(parent, Measurement::PhysicalQuantity::Volume, &Measurement::Units::liters) {
   return;
}

BtTemperatureEdit::BtTemperatureEdit(QWidget *parent) : BtLineEdit(parent, Measurement::PhysicalQuantity::Temperature, &Measurement::Units::celsius) {
   return;
}

BtTimeEdit::BtTimeEdit(QWidget *parent) : BtLineEdit(parent, Measurement::PhysicalQuantity::Time, &Measurement::Units::minutes) {
   return;
}

BtDensityEdit::BtDensityEdit(QWidget *parent) : BtLineEdit(parent, Measurement::PhysicalQuantity::Density, &Measurement::Units::sp_grav) {
   return;
}

BtColorEdit::BtColorEdit(QWidget *parent) : BtLineEdit(parent, Measurement::PhysicalQuantity::Color, &Measurement::Units::srm) {
   return;
}

BtStringEdit::BtStringEdit(QWidget *parent) : BtLineEdit(parent, NonPhysicalQuantity::String, nullptr) {
   return;
}

BtMixedEdit::BtMixedEdit(QWidget *parent) : BtLineEdit(parent, Measurement::PhysicalQuantity::Mixed) {
   // This is probably pure evil I will later regret
   this->fieldType = Measurement::PhysicalQuantity::Volume;
   this->units = &Measurement::Units::liters;
   return;
}

void BtMixedEdit::setIsWeight(bool state) {
   // But you have to admit, this is clever
   if (state) {
      this->fieldType = Measurement::PhysicalQuantity::Mass;
      this->units = &Measurement::Units::kilograms;
   } else {
      this->fieldType = Measurement::PhysicalQuantity::Volume;
      this->units = &Measurement::Units::liters;
   }

   // maybe? My head hurts now
   this->onLineChanged();
   return;
}

BtDiastaticPowerEdit::BtDiastaticPowerEdit(QWidget *parent) :
   BtLineEdit(parent, Measurement::PhysicalQuantity::DiastaticPower, &Measurement::Units::lintner) {
   return;
}
