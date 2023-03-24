/*======================================================================================================================
 * BtLineEdit.cpp is part of Brewken, and is copyright the following authors 2009-2023:
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
#include "Logging.h"
#include "measurement/Measurement.h"
#include "model/NamedEntity.h"
#include "PersistentSettings.h"
#include "utils/OptionalHelpers.h"

namespace {
   int const min_text_size = 8;
   int const max_text_size = 50;
}

BtLineEdit::BtLineEdit(QWidget *parent,
                       BtFieldType fieldType,
                       int const defaultPrecision,
                       QString const & maximalDisplayString) :
   QLineEdit{parent},
   fieldType{fieldType},
   defaultPrecision{defaultPrecision} {

   if (std::holds_alternative<NonPhysicalQuantity>(fieldType)) {
      connect(this, &QLineEdit::editingFinished, this, &BtLineEdit::onLineChanged);
   }

   // We can work out (and store) our display size here, but not yet set it.  The way the Designer UI Files work is to
   // generate code that calls setters such as setMaximumWidth() etc, which would override anything we do here in the
   // constructor.  So we set our size when setText() is called.
   this->calculateDisplaySize(maximalDisplayString);

   return;
}

BtLineEdit::~BtLineEdit() = default;

BtFieldType const BtLineEdit::getFieldType() const {
   return this->fieldType;
}

template<typename T> T BtLineEdit::getValueAs() const {
   qDebug() << Q_FUNC_INFO << "Converting" << this->text() << "to" << Measurement::extractRawFromString<T>(this->text());
   return Measurement::extractRawFromString<T>(this->text());
}
//
// Instantiate the above template function for the types that are going to use it
// (This is all just a trick to allow the template definition to be here in the .cpp file and not in the header, which
// saves having to put a bunch of std::string stuff there.)
//
template int          BtLineEdit::getValueAs<int         >() const;
template unsigned int BtLineEdit::getValueAs<unsigned int>() const;
template double       BtLineEdit::getValueAs<double      >() const;

void BtLineEdit::onLineChanged() {
   qDebug() << Q_FUNC_INFO;
   if (sender() == this) {
      emit textModified();
   }
   return;
}

void BtLineEdit::setText(std::optional<double> amount, std::optional<int> precision) {
   // For percentages, we'd like to show the % symbol after the number
   QString symbol{""};
   if (NonPhysicalQuantity::Percentage == std::get<NonPhysicalQuantity>(this->fieldType)) {
      symbol = " %";
   }
   if (amount) {
      this->QLineEdit::setText(
         Measurement::displayQuantity(*amount, precision.value_or(this->defaultPrecision)) + symbol
      );
   } else {
      this->QLineEdit::setText("");
   }
   this->setDisplaySize();
   return;
}

void BtLineEdit::setText(QString amount, std::optional<int> precision) {
   if (!amount.isEmpty() && NonPhysicalQuantity::String != std::get<NonPhysicalQuantity>(this->fieldType)) {
      bool ok = false;
      double amt = Measurement::extractRawFromString<double>(amount, &ok);
      if (!ok) {
         qWarning() << Q_FUNC_INFO << "Could not convert" << amount << "to double";
      }
      this->setText(amt, precision.value_or(this->defaultPrecision));
      return;
   }

   this->QLineEdit::setText(amount);
   this->setDisplaySize(true);
   return;
}

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


BtGenericEdit      ::BtGenericEdit      (QWidget *parent) : BtLineEdit(parent, NonPhysicalQuantity::String          ) { return; }
BtStringEdit       ::BtStringEdit       (QWidget *parent) : BtLineEdit(parent, NonPhysicalQuantity::String          ) { return; }
BtPercentageEdit   ::BtPercentageEdit   (QWidget *parent) : BtLineEdit(parent, NonPhysicalQuantity::Percentage   , 0) { return; }
BtDimensionlessEdit::BtDimensionlessEdit(QWidget *parent) : BtLineEdit(parent, NonPhysicalQuantity::Dimensionless, 3) { return; }
