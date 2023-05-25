/*======================================================================================================================
 * model/NamedParameterBundle.h is part of Brewken, and is copyright the following authors 2021-2023:
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
#ifndef MODEL_NAMEDPARAMETERBUNDLE_H
#define MODEL_NAMEDPARAMETERBUNDLE_H
#pragma once

#include <optional>

#include <QDate>
#include <QHash>
#include <QString>
#include <QVariant>

#include "measurement/Amount.h"
#include "measurement/ConstrainedAmount.h"
#include "utils/BtStringConst.h"
#include "utils/MetaTypes.h"

/**
 * \brief This allows constructors to be called without a long list of positional parameters and, more importantly, for
 *        those parameters to be data-driven, eg from a mapping of database column names to property names.
 */
class NamedParameterBundle : public QHash<QString, QVariant> {
public:
   enum OperationMode {
      Strict,
      NotStrict
   };

   NamedParameterBundle(OperationMode mode = Strict);
   ~NamedParameterBundle();

   /**
    * \brief Override of \c insert to support \c BtStringConst
    */
   QHash::iterator insert(BtStringConst const & parameterName, QVariant const & value);

   using QHash<QString, QVariant>::contains;

   /**
    * \brief Overload of QHash<QString, QVariant>::contains to support passing \c BtStringConst
    */
   bool contains(BtStringConst const & parameterName) const;

   /**
    * \brief Get the value of a parameter that is required to be present in the DB.  In "strict" mode, throw an
    *        exception if it is not present.  Otherwise, return whatever default value QVariant gives us.
    *        This is a convenience function to make the call to extract parameters concise.  (We don't want to use the
    *        operator[] of QHash because we want "parameter not found" to be an error.)
    *
    * \throw std::invalid_argument if the parameter is not present or does not have a valid \c QVariant value
    */
   QVariant get(BtStringConst const & parameterName) const;

   /**
    * \brief Templated version of above
    *
    * \throw std::invalid_argument if the parameter is not present or does not have a valid \c QVariant value
    */
   template <class T> T val(BtStringConst const & parameterName) const {
      return this->get(parameterName).value<T>();
   }

   /**
    * \brief Special case for optional enums which are always stored as std::optional<int> inside the QVariant.
    *        Obviously by definition there's always a default value and it's always std::nullopt
    *
    * \throw std::invalid_argument if the parameter is not present or does not have a valid \c QVariant value
    */
   template <class T> std::optional<T> optEnumVal(BtStringConst const & parameterName) const {
      // Of course it's a coding error to request a parameter without a name!
      Q_ASSERT(!parameterName.isNull());
      if (!this->contains(*parameterName)) {
         return std::nullopt;
      }
      auto value = this->value(*parameterName).value< std::optional<int> >();
      if (value.has_value()) {
         return std::optional<T>(static_cast<T>(value.value()));
      }
      return std::nullopt;
   }

   /**
    * \brief Get the value of a parameter that is not required to be present
    *
    * \param parameterName
    * \param defaultValue  What to return if the parameter is not present in the bundle
    */
   template <class T> T val(BtStringConst const & parameterName, T const & defaultValue) const {
      // Of course it's a coding error to request a parameter without a name!
      Q_ASSERT(!parameterName.isNull());
      // In expression below, first value() is QHash::value(), second is templated QVariant::value()
      return this->contains(*parameterName) ? this->value(*parameterName).value<T>() : defaultValue;
   }
private:
   OperationMode mode;
};

/**
 * \brief Convenience function to allow output of \c NamedParameterBundle to \c QDebug or \c QTextStream stream etc
 */
template<class S>
S & operator<<(S & stream, NamedParameterBundle const & namedParameterBundle);

/**
 * \brief Convenience function to allow output of \c NamedParameterBundle to \c QDebug or \c QTextStream stream etc
 */
template<class S>
S & operator<<(S & stream, NamedParameterBundle const * namedParameterBundle);

#endif
