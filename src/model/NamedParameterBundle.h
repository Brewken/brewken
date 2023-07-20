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

#include <cstddef> // for std::size_t
#include <optional>
#include <map>

#include <QDate>
#include <QString>
#include <QVariant>

#include "measurement/Amount.h"
#include "measurement/ConstrainedAmount.h"
#include "utils/BtStringConst.h"
#include "utils/MetaTypes.h"
#include "utils/PropertyPath.h"

class NamedParameterBundle;

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


/**
 * \brief This allows constructors to be called without a long list of positional parameters and, more importantly, for
 *        those parameters to be data-driven, eg from a mapping of database column names to property names.
 *
 *        In certain circumstances, it is useful for one \c NamedParameterBundle to be able to contain another.  This is
 *        when we are mapping between a serialisation format that has a different structure from our model.  Eg, BeerXML
 *        does not have a separate record for a \c Boil; some parameters we store in a \c Boil owned by a \c Recipe are,
 *        in BeerXML, direct properties of the \c Recipe.
 */
class NamedParameterBundle {
public:
   enum OperationMode {
      Strict,
      NotStrict
   };

   template<class S> friend S & operator<<(S & stream, NamedParameterBundle const & namedParameterBundle);

   NamedParameterBundle(OperationMode mode = Strict);
   ~NamedParameterBundle();

///   /**
///    * \brief Override of \c insert to support \c BtStringConst
///    */
   void insert(BtStringConst const & propertyName, QVariant const & value);

   void insert(PropertyPath  const & propertyPath, QVariant const & value);

///   using QHash<QString, QVariant>::contains;

///   /**
///    * \brief Overload of QHash<QString, QVariant>::contains to support passing \c BtStringConst
///    */
   bool contains(BtStringConst const & propertyName) const;

   bool contains(PropertyPath  const & propertyPath) const;

   std::size_t size() const noexcept;

   bool isEmpty() const;

   /**
    * \brief Get the value of a parameter that is required to be present in the DB.  In "strict" mode, throw an
    *        exception if it is not present.  Otherwise, return whatever default value QVariant gives us.
    *        This is a convenience function to make the call to extract parameters concise.  (We don't want to use the
    *        operator[] of QHash because we want "parameter not found" to be an error.)
    *
    * \throw std::invalid_argument if the parameter is not present or does not have a valid \c QVariant value
    */
   QVariant get(BtStringConst const & propertyName) const;

   /**
    * \brief Templated version of above
    *
    * \throw std::invalid_argument if the parameter is not present or does not have a valid \c QVariant value
    */
   template <class T> T val(BtStringConst const & propertyName) const {
      return this->get(propertyName).value<T>();
   }

   /**
    * \brief Special case for optional enums which are always stored as std::optional<int> inside the QVariant.
    *        Obviously by definition there's always a default value and it's always std::nullopt
    *
    * \throw std::invalid_argument if the parameter is not present or does not have a valid \c QVariant value
    */
   template <class T> std::optional<T> optEnumVal(BtStringConst const & propertyName) const {
      // Of course it's a coding error to request a parameter without a name!
      Q_ASSERT(!propertyName.isNull());
      if (!this->m_parameters.contains(*propertyName)) {
         return std::nullopt;
      }
      auto value = this->m_parameters.at(*propertyName).value< std::optional<int> >();
      if (value.has_value()) {
         return std::optional<T>(static_cast<T>(value.value()));
      }
      return std::nullopt;
   }

   /**
    * \brief Get the value of a parameter that is not required to be present
    *
    * \param propertyName
    * \param defaultValue  What to return if the parameter is not present in the bundle
    */
   template <class T> T val(BtStringConst const & propertyName, T const & defaultValue) const {
      // Of course it's a coding error to request a parameter without a name!
      Q_ASSERT(!propertyName.isNull());
      if (!this->m_parameters.contains(*propertyName)) {
         return defaultValue;
      }
      return this->m_parameters.at(*propertyName).value<T>();
   }

   bool containsBundle(BtStringConst const & propertyName) const;

   NamedParameterBundle const & getBundle(BtStringConst const & propertyName) const;

private:
   //
   // The default choice here for look-ups would be QMap or QHash.  However, these have the undesirable attribute that
   // they always return a copy of the contained value, which we especially don't want to do for m_containedBundles.
   // The C++ standard library counterparts, std::map and std::unordered_map, are preferable for our use here as they
   // return objects by reference (and the references stay valid provided the entries to which they refer are not
   // removed from the map.
   //
   // I haven't done the profiling, but, for the relatively small number of entries we're storing (a few dozen at most),
   // I'd imagine there's not a huge overall performance difference between std::map and std::unordered_map.  We can
   // always change it later if I'm wrong.
   //
   std::map<QString, QVariant> m_parameters;
   OperationMode m_mode;
   std::map<QString, NamedParameterBundle> m_containedBundles;
};

#endif
