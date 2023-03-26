/*======================================================================================================================
 * widgets/SmartLineEdit.cpp is part of Brewken, and is copyright the following authors 2009-2023:
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
#include "widgets/SmartLineEdit.h"

#include <QFontMetrics>
#include <QMargins>
#include <QRect>
#include <QStyle>

#include "measurement/Measurement.h"
#include "UiAmountWithUnits.h"

namespace {
   int const min_text_size = 8;
   int const max_text_size = 50;
}

// This private implementation class holds all private non-virtual members of SmartLineEdit
class SmartLineEdit::impl {
public:
   impl(SmartLineEdit & self) :
      m_self                {self},
      m_initialised         {false},
      m_fieldType           {NonPhysicalQuantity::String},
      m_typeInfo            {nullptr},
      m_uiAmountWithUnits   {nullptr},
      m_defaultPrecision    {3},
      m_maximalDisplayString{"100.000 srm"} {
      return;
   }

   ~impl() = default;

   void calculateDisplaySize(QString const & maximalDisplayString) {
      //
      // By default, some, but not all, boxes have a min and max width of 100 pixels, but this is not wide enough on a
      // high DPI display.  We instead calculate width here based on font-size - but without reducing any existing
      // minimum width.
      //
      // Unfortunately, for a QLineEdit object, calculating the width is hard because, besides the text, we need to
      // allow for the width of padding and frame, which is non-trivial to discover.  Eg, typically:
      //   marginsAroundText() and contentsMargins() both return 0 for left and right margins
      //   contentsRect() and frameSize() both give the same width as width()
      // AFAICT, the best option is to query via pixelMetric() calls to the widget's style, but we need to check this
      // works in practice on a variety of different systems.
      //
      QFontMetrics displayFontMetrics(this->m_self.font());
      QRect minimumTextRect = displayFontMetrics.boundingRect(maximalDisplayString);
      QMargins marginsAroundText = this->m_self.textMargins();
      auto myStyle = this->m_self.style();
      // NB: 2× frame width as on left and right; same for horizontal spacing
      int totalWidgetWidthForMaximalDisplayString = minimumTextRect.width() +
                                                   marginsAroundText.left() +
                                                   marginsAroundText.right() +
                                                   (2 * myStyle->pixelMetric(QStyle::PM_DefaultFrameWidth)) +
                                                   (2 * myStyle->pixelMetric(QStyle::PM_LayoutHorizontalSpacing));

      this->m_desiredWidthInPixels = qMax(this->m_self.minimumWidth(), totalWidgetWidthForMaximalDisplayString);
      return;
   }

   void setDisplaySize(bool recalculate = false) {
      if (recalculate) {
         QString sizingString = this->m_self.text();

         // this is a dirty bit of cheating. If we do not reset the minimum
         // width, the field only ever gets bigger. This forces the resize I
         // want, but only when we are instructed to force it
         this->m_self.setMinimumWidth(0);
         if (sizingString.length() < min_text_size) {
            sizingString = QString(min_text_size,'a');
         } else if (sizingString.length() > max_text_size) {
            sizingString = QString(max_text_size,'a');
         }
         this->calculateDisplaySize(sizingString);
      }
      this->m_self.setFixedWidth(this->m_desiredWidthInPixels);
      return;
   }


   SmartLineEdit &                    m_self;
   bool                               m_initialised;
   BtFieldType                        m_fieldType;
   TypeInfo const *                   m_typeInfo;
   std::unique_ptr<UiAmountWithUnits> m_uiAmountWithUnits;
   int                                m_defaultPrecision;
   QString                            m_maximalDisplayString;
   int                                m_desiredWidthInPixels;
};

SmartLineEdit::SmartLineEdit(QWidget * parent) : QLineEdit(parent), pimpl{std::make_unique<impl>(*this)} {
   return;
}

SmartLineEdit::~SmartLineEdit() = default;

void SmartLineEdit::init(BtFieldType const   fieldType,
                         TypeInfo    const & typeInfo,
                         int         const   defaultPrecision,
                         QString     const & maximalDisplayString) {
   // It's a coding error to call this function twice on the same object, ie we should only initialise something once!
   Q_ASSERT(!this->pimpl->m_initialised);

   this->pimpl->m_fieldType            = fieldType;
   // It's only meaningful to have a UiAmountWithUnits if we are dealing with a PhysicalQuantity
   if (!std::holds_alternative<NonPhysicalQuantity>(this->pimpl->m_fieldType)) {
      // It's a coding error if we already created a UiAmountWithUnits
      Q_ASSERT(!this->pimpl->m_uiAmountWithUnits);

      this->pimpl->m_uiAmountWithUnits =
         std::make_unique<UiAmountWithUnits>(this->parentWidget(),
                                             ConvertToPhysicalQuantities(this->pimpl->m_fieldType));
   }
   this->pimpl->m_typeInfo             = &typeInfo;
   this->pimpl->m_defaultPrecision     = defaultPrecision;
   this->pimpl->m_maximalDisplayString = maximalDisplayString;
   this->pimpl->m_initialised          = true;
   return;
}

BtFieldType const SmartLineEdit::getFieldType() const {
   Q_ASSERT(this->pimpl->m_initialised);
   return this->pimpl->m_fieldType;
}

TypeInfo const & SmartLineEdit::getTypeInfo() const {
   Q_ASSERT(this->pimpl->m_initialised);
   return *this->pimpl->m_typeInfo;
}

UiAmountWithUnits const & SmartLineEdit::getUiAmountWithUnits() const {
   Q_ASSERT(this->pimpl->m_initialised);
   Q_ASSERT(this->pimpl->m_uiAmountWithUnits);
   return *this->pimpl->m_uiAmountWithUnits;
}

Measurement::Amount SmartLineEdit::toCanonical() const {
   Q_ASSERT(this->pimpl->m_initialised);
   Q_ASSERT(this->pimpl->m_uiAmountWithUnits);
   return this->pimpl->m_uiAmountWithUnits->rawToCanonical(this->text());
}

void SmartLineEdit::setText(std::optional<double> amount, std::optional<int> precision) {
   Q_ASSERT(this->pimpl->m_initialised);

   if (!amount) {
      // What the field is measuring doesn't matter as it's not set
      this->QLineEdit::setText("");
   } else if (std::holds_alternative<NonPhysicalQuantity>(this->pimpl->m_fieldType)) {
      // The field is not measuring a physical quantity so there are no units or unit conversions to handle

      NonPhysicalQuantity const nonPhysicalQuantity = std::get<NonPhysicalQuantity>(this->pimpl->m_fieldType);
      // It's a coding error if we're trying to pass a number in to a string field
      Q_ASSERT(nonPhysicalQuantity != NonPhysicalQuantity::String);

      // For percentages, we'd like to show the % symbol after the number
      QString symbol{""};
      if (NonPhysicalQuantity::Percentage == nonPhysicalQuantity) {
         symbol = " %";
      }

      this->QLineEdit::setText(
         Measurement::displayQuantity(*amount, precision.value_or(this->pimpl->m_defaultPrecision)) + symbol
      );
   } else {
      // The field is measuring a physical quantity
      this->pimpl->m_uiAmountWithUnits->displayAmount(*amount, precision.value_or(this->pimpl->m_defaultPrecision));
   }

   this->pimpl->setDisplaySize();
   return;
}
