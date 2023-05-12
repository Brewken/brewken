/*======================================================================================================================
 * widgets/SmartField.cpp is part of Brewken, and is copyright the following authors 2009-2023:
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
#include "widgets/SmartField.h"

#include <tuple>

#include <QDebug>
#include <QVariant>
#include <QWidget>

#include "Localization.h"
#include "Logging.h"
#include "measurement/Measurement.h"
#include "utils/OptionalHelpers.h"
#include "utils/TypeLookup.h"
#include "widgets/SmartAmountSettings.h"
#include "widgets/SmartLabel.h"

namespace {
   /**
    * \return \c true if \c input is empty or blank (ie contains only whitespace), \c false otherwise
    */
   [[nodiscard]] bool isEmptyOrBlank(QString const & input) {
      if (input.isEmpty()) {
         return true;
      }
      if (input.trimmed().isEmpty()) {
         return true;
      }
      return false;
   }
}

// This private implementation class holds all private non-virtual members of SmartField
class SmartField::impl {
public:
   impl(SmartField & self) :
      m_self                   {self},
      m_initialised            {false},
      m_fieldFqName            {"Uninitialised m_fieldFqName!"},
      m_settings               {nullptr},
      m_smartBuddyLabel        {nullptr},
      m_precision              {3},
      m_maximalDisplayString   {"100.000 srm"} {
      return;
   }

   ~impl() = default;

   /**
    * \brief We want to have two different signatures of \c SmartField::init so we can catch missing parameters at
    *        compile time.  Ultimately they both do pretty much the same work, by calling this function.
    */
   void init(char const *                const   fieldFqName,
             SmartLabel                        * smartBuddyLabel,
             std::unique_ptr<SmartAmountSettings> settings,
             std::optional<unsigned int> const   precision,
             QString                     const & maximalDisplayString) {
      // It's a coding error to call this function twice on the same object, ie we should only initialise something
      // once!
      Q_ASSERT(!this->m_initialised);

      this->m_fieldFqName     = fieldFqName;
      this->m_settings        = std::move(settings);
      this->m_smartBuddyLabel = smartBuddyLabel;

      // It's a coding error to have both a SmartBuddyLabel and a SmartAmountSettings (because the former, if present,
      // owns the latter and we only own it as a fallback if there is no SmartBuddyLabel).
      Q_ASSERT(!this->m_settings || !this->m_smartBuddyLabel);

      // Similarly, it's a coding error to have neither SmartBuddyLabel nor SmartAmountSettings
      Q_ASSERT(this->m_settings || this->m_smartBuddyLabel);

      if (precision) {
         // It's a coding error to specify precision for a field that's not a (possibly optional) double (or a float,
         // but we don't use float).  However, we allow precision of 0 for a type that is stored as an int or unsigned
         // int, because that's what we're going to set it to anyway.
         Q_ASSERT(this->m_self.getTypeInfo().typeIndex == typeid(double) ||
                  this->m_self.getTypeInfo().typeIndex == typeid(std::optional<double>) ||
                  (0 == *precision && this->m_self.getTypeInfo().typeIndex == typeid(int         )) ||
                  (0 == *precision && this->m_self.getTypeInfo().typeIndex == typeid(unsigned int)) );

         if (this->m_self.getTypeInfo().typeIndex == typeid(double) ||
             this->m_self.getTypeInfo().typeIndex == typeid(std::optional<double>)) {
            // It's a coding error if precision is not some plausible value.  For the moment at least, we assert there
            // are no envisageable circumstances where we need to show more than 3 decimal places
            Q_ASSERT(*precision <= 3);
            this->m_precision = *precision;
         }
      }
      // For integers, there are no decimal places to show
      if (this->m_self.getTypeInfo().typeIndex == typeid(int) ||
          this->m_self.getTypeInfo().typeIndex == typeid(unsigned int)) {
         this->m_precision = 0;
      }
      this->m_maximalDisplayString = maximalDisplayString;

      if (std::holds_alternative<NonPhysicalQuantity>(*this->m_self.getTypeInfo().fieldType)) {
         // It's a coding error to have a smartBuddyLabel for a NonPhysicalQuantity
         Q_ASSERT(!this->m_smartBuddyLabel);
      } else {
         // It's only meaningful to have a SmartBuddyLabel if we are dealing with a PhysicalQuantity, but it's not
         // required to have one if the scale and units are not changeable by the user.
         if (this->m_smartBuddyLabel) {
            this->m_self.connectSmartLabelSignal(*this->m_smartBuddyLabel);
         }

      }

      this->m_initialised = true;

      // Now let our subclass (SmartLineEdit, SmartDigitWidget, etc) do any of its own initialisation
      this->m_self.doPostInitWork();

      return;
   }

   /**
    * \brief Returns the contents of the field converted, if necessary, to SI units
    *
    * \param enteredText
    * \param previousScaleInfo
    */
   Measurement::Amount toCanonical(QString const & enteredText,
                                   SmartAmounts::ScaleInfo previousScaleInfo,
                                   bool * const ok = nullptr) {
      Q_ASSERT(this->m_initialised);

      // It's a coding error to call this for a NonPhysicalQuantity
      Q_ASSERT(!std::holds_alternative<NonPhysicalQuantity>(*this->m_self.getTypeInfo().fieldType));

      qDebug() <<
         Q_FUNC_INFO << "enteredText:" << enteredText <<  ", old SystemOfMeasurement:" <<
         previousScaleInfo.systemOfMeasurement << ", old RelativeScale: " << previousScaleInfo.relativeScale;

      Measurement::UnitSystem const & oldUnitSystem = this->m_self.getUnitSystem(previousScaleInfo);

      Measurement::Unit const * defaultUnit{
         previousScaleInfo.relativeScale ? oldUnitSystem.scaleUnit(*previousScaleInfo.relativeScale) : oldUnitSystem.unit()
      };

      // It's a coding error if defaultUnit is null, because it means previousScaleInfo.relativeScale was not valid for
      // oldUnitSystem.  However, we can recover.
      if (!defaultUnit) {
         qWarning() << Q_FUNC_INFO << "previousScaleInfo.relativeScale invalid?" << previousScaleInfo.relativeScale;
         defaultUnit = oldUnitSystem.unit();
         if (ok) {
            *ok = false;
         }
      }

      //
      // Normally, we display units with the text.  If the user just edits the number, then the units will still be there.
      // Alternatively, if the user specifies different units in the text, we should try to honour those.  Otherwise, if,
      // no units are specified in the text, we need to go to defaults.  Defaults are either what is "forced" for this
      // specific field or, failing that, what is configured globally.
      //
      // Measurement::UnitSystem::qStringToSI will handle all the logic to deal with any units specified by the user in the
      // string.  (In theory, we just grab the units that the user has specified in the input text.  In reality, it's not
      // that easy as we sometimes need to disambiguate - eg between Imperial gallons and US customary ones.  So, if we
      // have old or current units then that helps with this - eg, if current units are US customary cups and user enters
      // gallons, then we'll go with US customary gallons over Imperial ones.)
      //
      auto amount = oldUnitSystem.qstringToSI(enteredText, *defaultUnit);
      qDebug() << Q_FUNC_INFO << "Converted to" << amount;
      if (ok) {
         *ok = true;
      }
      return amount;
   }

   SmartField &                         m_self;
   bool                                 m_initialised;
   char const *                         m_fieldFqName;
   std::unique_ptr<SmartAmountSettings> m_settings;
   SmartLabel *                         m_smartBuddyLabel;
   // "Precision" (ie number of decimal places to show) is used if and only the field is numeric.  For int and unsigned
   // int, it must always be 0.
   unsigned int              m_precision;
   QString                   m_maximalDisplayString;
};


SmartField::SmartField() :
   SmartBase<SmartField>(),
   pimpl{std::make_unique<impl>(*this)} {
   return;
}

SmartField::~SmartField() = default;

template<> void SmartField::init<SmartLabel>([[maybe_unused]] char const * const   editorName,
                                             [[maybe_unused]] char const * const   fieldName,
                                             char const *                  const   fieldFqName,
                                             SmartLabel                          & smartBuddyLabel,
                                             TypeInfo                      const & typeInfo,
                                             std::optional<unsigned int>   const   precision,
                                             QString                       const & maximalDisplayString) {
   qDebug() << Q_FUNC_INFO << fieldFqName << ":" << typeInfo;

   // It's a coding error to call this version of init with a NonPhysicalQuantity
   Q_ASSERT(typeInfo.fieldType && !std::holds_alternative<NonPhysicalQuantity>(*typeInfo.fieldType));

   // It's a coding error if SmartLabel not initialised first
   Q_ASSERT(smartBuddyLabel.isInitialised());
   Q_ASSERT(&smartBuddyLabel.getTypeInfo() == &typeInfo);

   this->pimpl->init(fieldFqName,
                     &smartBuddyLabel,
                     nullptr,   // Where there is a SmartLabel, it holds the SmartAmountSettings
                     precision,
                     maximalDisplayString);
   return;
}

template<> void SmartField::init<QLabel>(char const *                const   editorName,
                                         char const *                const   fieldName,
                                         char const *                const   fieldFqName,
                                         [[maybe_unused]] QLabel           & regularBuddyLabel,
                                         TypeInfo                    const & typeInfo,
                                         std::optional<unsigned int> const   precision,
                                         QString                     const & maximalDisplayString) {
   qDebug() << Q_FUNC_INFO << fieldFqName << ":" << typeInfo;

   // It's a coding error to call this version of init with a PhysicalQuantity
   Q_ASSERT(typeInfo.fieldType && std::holds_alternative<NonPhysicalQuantity>(*typeInfo.fieldType));
   this->pimpl->init(fieldFqName,
                     nullptr,
                     std::make_unique<SmartAmountSettings>(editorName, fieldName, typeInfo, nullptr),
                     precision,
                     maximalDisplayString);
   return;
}

void SmartField::initFixed(char const *                const   editorName,
                           char const *                const   fieldName,
                           char const *                const   fieldFqName,
                           [[maybe_unused]] QLabel           & buddyLabel,
                           TypeInfo                    const & typeInfo,
                           Measurement::Unit           const & fixedDisplayUnit,
                           std::optional<unsigned int> const   precision,
                           QString                     const & maximalDisplayString) {
   qDebug() << Q_FUNC_INFO << fieldFqName << ":" << typeInfo;

   // It's a coding error to call this version of init with a NonPhysicalQuantity
   Q_ASSERT(typeInfo.fieldType && !std::holds_alternative<NonPhysicalQuantity>(*typeInfo.fieldType));

   this->pimpl->init(fieldFqName,
                     nullptr,
                     std::make_unique<SmartAmountSettings>(editorName, fieldName, typeInfo, &fixedDisplayUnit),
                     precision,
                     maximalDisplayString);
   return;
}

[[nodiscard]] bool SmartField::isInitialised() const {
  return this->pimpl->m_initialised;
}

//BtFieldType const SmartField::getFieldType() const {
//   Q_ASSERT(this->pimpl->m_initialised);
//   return *this->getTypeInfo().fieldType;
//}
//
//TypeInfo const & SmartField::getTypeInfo() const {
//   Q_ASSERT(this->pimpl->m_initialised);
//   return *this->pimpl->m_typeInfo;
//}

[[nodiscard]] SmartAmountSettings & SmartField::settings() {
   // Note that this can be called from within this class before we have set the this->pimpl->m_initialised flag
   if (this->pimpl->m_smartBuddyLabel) {
      return this->pimpl->m_smartBuddyLabel->settings();
   }
   Q_ASSERT(this->pimpl->m_settings);
   return *this->pimpl->m_settings.get();
}

QString const & SmartField::getMaximalDisplayString() const {
   return this->pimpl->m_maximalDisplayString;
}

char const * SmartField::getFqFieldName() const {
   return this->pimpl->m_fieldFqName;
}

// Note that, because partial specialisation of _functions_ is not allowed, we actually have two overloads of setAmount
// This shouldn't make any difference to callers.
template<typename T, typename> void SmartField::setAmount(std::optional<T> amount) {
   Q_ASSERT(this->pimpl->m_initialised);

   if (this->getTypeInfo().typeIndex != typeid(T)) {
      // This is a coding error
      qCritical() <<
         Q_FUNC_INFO << this->pimpl->m_fieldFqName << ": Trying to set wrong type; m_typeInfo=" <<
         this->getTypeInfo() << ", typeid(T)=" << typeid(T).name();
      Q_ASSERT(false);
   }

   if (!amount) {
      this->setRawText("");
      return;
   }

   this->setAmount<T>(*amount);
   return;
}

template<typename T, typename> void SmartField::setAmount(T amount) {
   Q_ASSERT(this->pimpl->m_initialised);
   qDebug() << Q_FUNC_INFO << this->pimpl->m_fieldFqName << "amount =" << amount;

   if (this->getTypeInfo().typeIndex != typeid(T)) {
      // This is a coding error
      qCritical() <<
         Q_FUNC_INFO << this->pimpl->m_fieldFqName << ": Trying to set wrong type; m_typeInfo=" <<
         this->getTypeInfo() << ", typeid(T)=" << typeid(T).name();
      Q_ASSERT(false);
   }

   if (std::holds_alternative<NonPhysicalQuantity>(*this->getTypeInfo().fieldType)) {
      // The field is not measuring a physical quantity so there are no units or unit conversions to handle

      NonPhysicalQuantity const nonPhysicalQuantity =
         std::get<NonPhysicalQuantity>(*this->getTypeInfo().fieldType);
      // It's a coding error if we're trying to pass a number in to a string field
      Q_ASSERT(nonPhysicalQuantity != NonPhysicalQuantity::String);

      // For percentages, we'd like to show the % symbol after the number
      QString symbol{""};
      if (NonPhysicalQuantity::Percentage == nonPhysicalQuantity) {
         symbol = " %";
      }

      this->setRawText(
         Measurement::displayQuantity(amount, this->pimpl->m_precision) + symbol
      );
   } else {
      // The field is measuring a physical quantity
      qDebug() <<
         Q_FUNC_INFO << this->pimpl->m_fieldFqName << "forcedSystemOfMeasurement:" <<
         this->getForcedSystemOfMeasurement() << ", forcedRelativeScale:" <<
         this->getForcedRelativeScale();
      this->setRawText(this->displayAmount(amount, this->pimpl->m_precision));
   }

   return;
}

// Instantiate the above template functions for the types that are going to use it
// (This is all just a trick to allow the template definition to be here in the .cpp file and not in the header, which
// saves having to put a bunch of std::string stuff there.)
template void SmartField::setAmount(std::optional<int         > amount);
template void SmartField::setAmount(std::optional<unsigned int> amount);
template void SmartField::setAmount(std::optional<double      > amount);
template void SmartField::setAmount(int          amount);
template void SmartField::setAmount(unsigned int amount);
template void SmartField::setAmount(double       amount);

void SmartField::setPrecision(unsigned int const precision) {
   this->pimpl->m_precision = precision;
   return;
}

[[nodiscard]] unsigned int SmartField::getPrecision() const {
   return this->pimpl->m_precision;
}

Measurement::Amount SmartField::toCanonical() const {
   Q_ASSERT(this->pimpl->m_initialised);
   // It's a coding error to call this for a NonPhysicalQuantity
   Q_ASSERT(!std::holds_alternative<NonPhysicalQuantity>(*this->getTypeInfo().fieldType));
   // It's a coding error to call this for an optional value
   Q_ASSERT(!this->getTypeInfo().isOptional());
   return this->pimpl->toCanonical(this->getRawText(), this->getScaleInfo());
}

// We can't do the same trick on get-value-as as we do for set-amount because we can't overload base on return type,
// hence two different function names.
template<typename T> T SmartField::getNonOptValue(bool * const ok) const {
   Q_ASSERT(this->pimpl->m_initialised);

   QString const rawText = this->getRawText();
   qDebug() << Q_FUNC_INFO << this->pimpl->m_fieldFqName << ": Converting" << rawText;

   // It's a coding error to call this for ann optional value.  We put the assert after the log statement to help
   // with debugging!
   Q_ASSERT(!this->getTypeInfo().isOptional());

   if (std::holds_alternative<NonPhysicalQuantity>(*this->getTypeInfo().fieldType)) {
      // Note that Measurement::extractRawFromString returns 0 if it can't parse the text
      return Measurement::extractRawFromString<T>(rawText, ok);
   }

   return static_cast<T>(this->pimpl->toCanonical(rawText, this->getScaleInfo(), ok).quantity());
}
// Instantiate the above template function for the types that are going to use it
template int          SmartField::getNonOptValue<int         >(bool * const ok) const;
template unsigned int SmartField::getNonOptValue<unsigned int>(bool * const ok) const;
template double       SmartField::getNonOptValue<double      >(bool * const ok) const;

template<typename T> std::optional<T> SmartField::getOptValue(bool * const ok) const {

   Q_ASSERT(this->pimpl->m_initialised);

   QString const rawText = this->getRawText();
   qDebug() << Q_FUNC_INFO << this->pimpl->m_fieldFqName << ": Converting" << rawText;

   // It's a coding error to call this for a non optional value.  We put the assert after the log statement to help
   // with debugging!
   Q_ASSERT(this->getTypeInfo().isOptional());

   // Optional values are allowed to be blank
   if (isEmptyOrBlank(rawText)) {
      if (ok) {
         *ok = true;
      }
      return std::optional<T>{std::nullopt};
   }

   if (std::holds_alternative<NonPhysicalQuantity>(*this->getTypeInfo().fieldType)) {
      bool parseOk = false;
      T amount = Measurement::extractRawFromString<T>(rawText, &parseOk);
      if (ok) {
         *ok = parseOk;
      }
      // If we couldn't parse something, return null
      if (!parseOk) {
         return std::optional<T>{std::nullopt};
      }

      return std::make_optional<T>(amount);
   }

   return std::make_optional<T>(
      static_cast<T>(
         this->pimpl->toCanonical(rawText, this->getScaleInfo(), ok).quantity()
      )
   );
}
// Instantiate the above template function for the types that are going to use it
template std::optional<int         > SmartField::getOptValue<int         >(bool * const ok) const;
template std::optional<unsigned int> SmartField::getOptValue<unsigned int>(bool * const ok) const;
template std::optional<double      > SmartField::getOptValue<double      >(bool * const ok) const;

void SmartField::correctEnteredText(SmartAmounts::ScaleInfo previousScaleInfo) {
   Q_ASSERT(this->pimpl->m_initialised);

   // It's a coding error to call this version of correctEnteredText with a NonPhysicalQuantity
   Q_ASSERT(!std::holds_alternative<NonPhysicalQuantity>(*this->getTypeInfo().fieldType));

   QString const enteredText = this->getRawText();

   qDebug() << Q_FUNC_INFO << this->pimpl->m_fieldFqName << "enteredText:" << enteredText;

   if (enteredText.isEmpty()) {
      return;
   }

   // The idea here is we need to first translate the field into a known
   // amount (aka to SI) and then into the unit we want.
   Measurement::Amount amountAsCanonical = this->pimpl->toCanonical(enteredText, previousScaleInfo);

   QString const correctedText = this->displayAmount(amountAsCanonical.quantity(), this->pimpl->m_precision);
   qDebug() <<
      Q_FUNC_INFO << this->pimpl->m_fieldFqName << "Interpreted" << enteredText << "as" << amountAsCanonical <<
      "and corrected to" << correctedText;

   this->setRawText(correctedText);
   return;
}

void SmartField::correctEnteredText() {
   Q_ASSERT(this->pimpl->m_initialised);

   // It's a coding error to call this version of correctEnteredText with anything other than NonPhysicalQuantity
   Q_ASSERT(std::holds_alternative<NonPhysicalQuantity>(*this->getTypeInfo().fieldType));

   // .:TBD:. At the moment, the special handling here for types other than double is a bit moot, but we keep it in
   // case we need to do more in future.
   NonPhysicalQuantity const nonPhysicalQuantity =
      std::get<NonPhysicalQuantity>(*this->getTypeInfo().fieldType);
   if (nonPhysicalQuantity != NonPhysicalQuantity::String) {
      QString const rawText = this->getRawText();

      auto const type = this->getTypeInfo().typeIndex;
      bool const optional = this->getTypeInfo().isOptional();
      bool ok = false;
      if (type == typeid(double      )) { if (optional) { this->setAmount(this->getOptValue<double      >(&ok)); } else { this->setAmount(this->getNonOptValue<double      >(&ok)); } } else
      if (type == typeid(int         )) { if (optional) { this->setAmount(this->getOptValue<int         >(&ok)); } else { this->setAmount(this->getNonOptValue<int         >(&ok)); } } else
      if (type == typeid(unsigned int)) { if (optional) { this->setAmount(this->getOptValue<unsigned int>(&ok)); } else { this->setAmount(this->getNonOptValue<unsigned int>(&ok)); } } else {
         // It's a coding error if we get here
         qCritical() <<
            Q_FUNC_INFO << this->getFqFieldName() << ": Don't know how to parse" << this->getTypeInfo();
         Q_ASSERT(false);
      }

      if (!ok) {
         qWarning() <<
            Q_FUNC_INFO << this->getFqFieldName() << ": Unable to extract number from" << rawText << "for" <<
            this->getTypeInfo();
         // setAmount will already have been called with 0 or std::nullopt as appropriate
      }
   }

   return;
}
