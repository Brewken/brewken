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

#include <QDebug>
#include <QFontMetrics>
#include <QMargins>
#include <QRect>
#include <QStyle>

#include "measurement/Measurement.h"
#include "UiAmountWithUnits.h"
#include "utils/OptionalHelpers.h"
#include "widgets/SmartLabel.h"

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
      m_typeInfo            {nullptr},
      m_buddyLabel          {nullptr},
      m_uiAmountWithUnits   {nullptr},
      m_defaultPrecision    {3},
      m_maximalDisplayString{"100.000 srm"},
      m_desiredWidthInPixels{0},
      m_editField           {""},
      m_configSection       {""} {
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

   /**
    * \brief We want to have two different signatures of \c SmartLineEdit::init so we can catch missing parameters at
    *        compile time.  Ultimately they both do pretty much the same work, by calling this function.
    */
   void init(TypeInfo    const & typeInfo,
             SmartLabel        * buddyLabel,
             int         const   defaultPrecision,
             QString     const & maximalDisplayString) {
      // It's a coding error to call this function twice on the same object, ie we should only initialise something once!
      Q_ASSERT(!this->m_initialised);

      this->m_typeInfo             = &typeInfo;
      this->m_buddyLabel           = buddyLabel;
      this->m_defaultPrecision     = defaultPrecision;
      this->m_maximalDisplayString = maximalDisplayString;
      this->m_initialised          = true;

      connect(&this->m_self, &QLineEdit::editingFinished, &this->m_self, &SmartLineEdit::onLineChanged);
      if (!std::holds_alternative<NonPhysicalQuantity>(*this->m_typeInfo->fieldType)) {
         QString configSection =
            this->m_self.property(*PropertyNames::UiAmountWithUnits::configSection).toString();
         qDebug() << Q_FUNC_INFO << "Config Section =" << configSection;
         this->m_uiAmountWithUnits->setConfigSection(configSection);

         // It's a coding error if we didn't specify the buddy for something measuring a physical quantity
         Q_ASSERT(this->m_buddyLabel);

         // It's also a coding error if we are not the buddy of the label we think we are.  However, we cannot test this
         // here as the buddy's QLabel::setBuddy() hasn't necessarily yet been called from the code generated from the
         // .ui file.  What we can do, as belt-and-braces is call it here.
         this->m_buddyLabel->setBuddy(&this->m_self);

         connect(this->m_buddyLabel, &SmartLabel::changedSystemOfMeasurementOrScale, &this->m_self, &SmartLineEdit::lineChanged);
      }

      // We can work out (and store) our display size here, but we don't yet set it.  The way the Designer UI Files work is
      // to generate code that calls setters such as setMaximumWidth() etc, which would override anything we do too early
      // on in the life of the object.  To be safe therefore, we set our size when setText() is called.
      this->calculateDisplaySize(maximalDisplayString);
      return;
   }


   SmartLineEdit &                    m_self;
   bool                               m_initialised;
   TypeInfo const *                   m_typeInfo;
   SmartLabel *                       m_buddyLabel;
   std::unique_ptr<UiAmountWithUnits> m_uiAmountWithUnits;
   int                                m_defaultPrecision;
   QString                            m_maximalDisplayString;
   int                                m_desiredWidthInPixels;
   // .:TBD:. This is a bit ugly.  We keep our own copies of fields that exist in UiAmountWithUnits because we get given
   // the values before we have created the UiAmountWithUnits object
   QString                            m_editField;
   QString                            m_configSection;

};

SmartLineEdit::SmartLineEdit(QWidget * parent) : QLineEdit(parent), pimpl{std::make_unique<impl>(*this)} {
   return;
}

SmartLineEdit::~SmartLineEdit() = default;

void SmartLineEdit::init(TypeInfo                        const & typeInfo,
                         SmartLabel                            & buddyLabel,
                         int                             const   defaultPrecision,
                         QString                         const & maximalDisplayString) {
   // It's a coding error to call this version of init with a NonPhysicalQuantity
   Q_ASSERT(typeInfo.fieldType && !std::holds_alternative<NonPhysicalQuantity>(*typeInfo.fieldType));

   // It's only meaningful to have a UiAmountWithUnits if we are dealing with a PhysicalQuantity, hence why we do it
   // here and not in SmartLineEdit::impl::init().
   // It's a coding error if we already created a UiAmountWithUnits
   Q_ASSERT(!this->pimpl->m_uiAmountWithUnits);
   this->pimpl->m_uiAmountWithUnits = std::make_unique<UiAmountWithUnits>(this->parentWidget(), ConvertToPhysicalQuantities(*typeInfo.fieldType));
   // See above for why we need to pass these in to UiAmountWithUnits
   if (!this->pimpl->m_editField    .isEmpty()) { this->pimpl->m_uiAmountWithUnits->setEditField    (this->pimpl->m_editField    ); }
   if (!this->pimpl->m_configSection.isEmpty()) { this->pimpl->m_uiAmountWithUnits->setConfigSection(this->pimpl->m_configSection); }

   this->pimpl->init(typeInfo, &buddyLabel, defaultPrecision, maximalDisplayString);
   return;
}

void SmartLineEdit::init(TypeInfo            const & typeInfo,
                         int                 const   defaultPrecision,
                         QString             const & maximalDisplayString) {
   // It's a coding error to call this version of init with anything other than a NonPhysicalQuantity
   Q_ASSERT(typeInfo.fieldType && std::holds_alternative<NonPhysicalQuantity>(*typeInfo.fieldType));

   this->pimpl->init(typeInfo, nullptr, defaultPrecision, maximalDisplayString);
   return;
}

BtFieldType const SmartLineEdit::getFieldType() const {
   Q_ASSERT(this->pimpl->m_initialised);
   return *this->pimpl->m_typeInfo->fieldType;
}

TypeInfo const & SmartLineEdit::getTypeInfo() const {
   Q_ASSERT(this->pimpl->m_initialised);
   return *this->pimpl->m_typeInfo;
}

UiAmountWithUnits & SmartLineEdit::getUiAmountWithUnits() const {
   Q_ASSERT(this->pimpl->m_initialised);
   Q_ASSERT(this->pimpl->m_uiAmountWithUnits);
   return *this->pimpl->m_uiAmountWithUnits;
}

Measurement::Amount SmartLineEdit::toCanonical() const {
   Q_ASSERT(this->pimpl->m_initialised);
   Q_ASSERT(this->pimpl->m_uiAmountWithUnits);
   return this->pimpl->m_uiAmountWithUnits->rawToCanonical(this->text());
}

void SmartLineEdit::setAmount(std::optional<double> amount, std::optional<int> precision) {
   Q_ASSERT(this->pimpl->m_initialised);

   if (!amount) {
      // What the field is measuring doesn't matter as it's not set
      this->QLineEdit::setText("");
   } else if (std::holds_alternative<NonPhysicalQuantity>(*this->pimpl->m_typeInfo->fieldType)) {
      // The field is not measuring a physical quantity so there are no units or unit conversions to handle

      NonPhysicalQuantity const nonPhysicalQuantity = std::get<NonPhysicalQuantity>(*this->pimpl->m_typeInfo->fieldType);
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
      this->QLineEdit::setText(
         this->pimpl->m_uiAmountWithUnits->displayAmount(*amount, precision.value_or(this->pimpl->m_defaultPrecision))
      );
   }

   this->pimpl->setDisplaySize();
   return;
}

//============================================ Property Getters and Setters ============================================
// Note that we cannot assume init() has yet been run when these are called from (code generated from) a .ui file

void    SmartLineEdit::setEditField                         (QString val) { this->pimpl->m_editField     = val; if (this->pimpl->m_initialised) { this->pimpl->m_uiAmountWithUnits->setEditField     (val); } return; }
void    SmartLineEdit::setConfigSection                     (QString val) { this->pimpl->m_configSection = val; if (this->pimpl->m_initialised) { this->pimpl->m_uiAmountWithUnits->setConfigSection (val); } return; }
 // TODO Check where these two are used and whether we can eliminate them
void    SmartLineEdit::setForcedSystemOfMeasurementViaString(QString val) { Q_ASSERT(this->pimpl->m_initialised); this->pimpl->m_uiAmountWithUnits->setForcedSystemOfMeasurementViaString(val); return; }
void    SmartLineEdit::setForcedRelativeScaleViaString      (QString val) { Q_ASSERT(this->pimpl->m_initialised); this->pimpl->m_uiAmountWithUnits->setForcedRelativeScaleViaString      (val); return; }

QString SmartLineEdit::getEditField()                          const { if (this->pimpl->m_initialised) { return this->pimpl->m_uiAmountWithUnits->getEditField()                     ; } return this->pimpl->m_editField    ; }
QString SmartLineEdit::getConfigSection() /* not const */            { if (this->pimpl->m_initialised) { return this->pimpl->m_uiAmountWithUnits->getConfigSection() /* not const */ ; } return this->pimpl->m_configSection; }
QString SmartLineEdit::getForcedSystemOfMeasurementViaString() const { Q_ASSERT(this->pimpl->m_initialised); return this->pimpl->m_uiAmountWithUnits->getForcedSystemOfMeasurementViaString(); }
QString SmartLineEdit::getForcedRelativeScaleViaString()       const { Q_ASSERT(this->pimpl->m_initialised); return this->pimpl->m_uiAmountWithUnits->getForcedRelativeScaleViaString()      ; }

//======================================================================================================================

void SmartLineEdit::onLineChanged() {
   Q_ASSERT(this->pimpl->m_initialised);

   if (std::holds_alternative<NonPhysicalQuantity>(*this->pimpl->m_typeInfo->fieldType)) {
      // The field is not measuring a physical quantity so there are no units or unit conversions to handle
      qDebug() << Q_FUNC_INFO;
      if (sender() == this) {
         emit textModified();
      }
      return;
   }

   // The field is measuring a physical quantity
   Q_ASSERT(this->pimpl->m_uiAmountWithUnits);
   qDebug() <<
      Q_FUNC_INFO << "Field Type:" << *this->pimpl->m_typeInfo->fieldType << ", forcedSystemOfMeasurement=" <<
      this->pimpl->m_uiAmountWithUnits->getForcedSystemOfMeasurement() << ", forcedRelativeScale=" <<
      this->pimpl->m_uiAmountWithUnits->getForcedRelativeScale() << ", value=" << this->text();

   Measurement::PhysicalQuantities const physicalQuantities = ConvertToPhysicalQuantities(*this->pimpl->m_typeInfo->fieldType);

   QString const propertyName = this->pimpl->m_uiAmountWithUnits->getEditField();
   QString const configSection = this->pimpl->m_uiAmountWithUnits->getConfigSection();
   Measurement::SystemOfMeasurement const oldSystemOfMeasurement =
      Measurement::getSystemOfMeasurementForField(propertyName, configSection, physicalQuantities);
   auto oldForcedRelativeScale = Measurement::getForcedRelativeScaleForField(propertyName, configSection);
   PreviousScaleInfo previousScaleInfo{
      oldSystemOfMeasurement,
      oldForcedRelativeScale
   };

   qDebug() <<
      Q_FUNC_INFO << "propertyName=" << propertyName << ", configSection=" << configSection <<
      ", oldSystemOfMeasurement=" << oldSystemOfMeasurement << ", oldForcedRelativeScale=" << oldForcedRelativeScale;

   this->lineChanged(previousScaleInfo);
   return;
}

void SmartLineEdit::lineChanged(PreviousScaleInfo previousScaleInfo) {
   Q_ASSERT(this->pimpl->m_initialised);
   Q_ASSERT(this->pimpl->m_uiAmountWithUnits);

   // editingFinished happens on focus being lost, regardless of anything
   // being changed. I am hoping this short circuits properly and we do
   // nothing if nothing changed.
   if (this->sender() == this && !this->isModified()) {
      qDebug() << Q_FUNC_INFO << "Nothing changed; field holds" << this->text();
      return;
   }

   this->QLineEdit::setText(this->pimpl->m_uiAmountWithUnits->correctEnteredText(this->text(), previousScaleInfo));

   if (sender() == this) {
      emit textModified();
   }

   return;
}
