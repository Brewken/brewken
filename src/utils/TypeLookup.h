/*======================================================================================================================
 * utils/TypeLookup.h is part of Brewken, and is copyright the following authors 2023:
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
#ifndef UTILS_TYPELOOKUP_H
#define UTILS_TYPELOOKUP_H
#pragma once

#include <map>
#include <typeindex>

#include "utils/BtStringConst.h"

/**
 * This implementation of \c is_instance is from
 * https://stackoverflow.com/questions/44012938/how-to-tell-if-template-type-is-an-instance-of-a-template-class
 */
namespace {
    template <typename, template <typename...> typename>
    struct is_instance_impl : public std::false_type {};

    template <template <typename...> typename U, typename...Ts>
    struct is_instance_impl<U<Ts...>, U> : public std::true_type {};
}

template <typename T, template <typename ...> typename U>
using is_instance = is_instance_impl<std::remove_cvref_t<T>, U>;

//¥¥¥¥ To do : this allows us to know if something is an instance of std::optional, but it does not give us the type inside
//that...

/**
 * \class TypeLookup allows us to get the typeId of a property that is guaranteed unique for each different type and
 *        guaranteed identical for two properties of the same type.
 *
 *        Note that we cannot use \c std::type_info::name() for this purpose as "the returned string can be identical
 *        for several types".
 *
 *        With the advent of BeerJSON, we have a lot more "optional" fields on objects.  We don't want to extend three
 *        different serialisation models (database, BeerXML and BeerJSON) with an extra flag, especially as the
 *        (subclass of) \c NamedEntity ought to know itself whether a field is optional/nullable.  This is enough for
 *        serialisation (where we just need to know eg whether we're reading/writing `double` or
 *        `std::optional<double>`).
 *
 *        In principle we might be able to avoid the need for this class and instead construct a look-up table at run
 *        time by making a bunch of calls to \c qRegisterMetaType(std::optional<T>) during start-up for all types \c T
 *        and storing the resulting IDs in a set or list that we then consult to discover whether a property is
 *        of type \c T or \c std::optional<T>.  But I _think_ the approach here is easier to debug.
 */
class TypeLookup {
public:

   /**
    *\brief The data we want to store for each property name: its type ID and whether it is an enum.  We need the latter
    *       because, for the purposes of the property system, we want to treat enums as ints.  (It makes the generic
    *       handling of enums in serialisation code a lot simpler.)
    */
   using LookupMapEntry = std::pair<std::type_index, bool>;

   /**
    * \brief If we want to change from using std::map in future, it's easier if we have a typedef alias for it
    */
   using LookupMap = std::map<BtStringConst const *, LookupMapEntry>;

   /**
    * \brief Construct a \c TypeLookup that optinoally extends an existing one (typically from the parent class)
    *
    * \param className Name of the class for which this is the property type lookup.  Eg for the \c TypeLookup for
    *                  \c Hop, this should be "Hop".  Used for error logging.
    * \param initializerList The mappings for this \c TypeLookup.  Eg for the \c TypeLookup for \c Hop, this would be
    *                        the type mappings for PropertyNames::Hop::... properties (but not the
    *                        PropertyNames::NamedEntity::... properties
    * \param parentClassLookup Pointer to the \c TypeLookup for the parent class, or \c nullptr if there is none.  Eg
    *                          for the \c TypeLookup for \c Hop,
    *                          this should point to the \c TypeLookup for \c NamedEntity because \c Hop inherits from
    *                          \c NamedEntity.
    */
   TypeLookup(char       const * const                     className,
              std::initializer_list<LookupMap::value_type> initializerList,
              TypeLookup const * const                     parentClassLookup = nullptr);

   /**
    * \brief Get the type ID (and whether it's an enum) for a given property name
    */
   LookupMapEntry getType(BtStringConst const & propertyName) const;

   /**
    * \brief Returns whether the attribute for a given property name is optional (ie std::optional<T> rather than T)
    */
   bool isOptional(BtStringConst const & propertyName) const;

private:
   char       const * const className;
   LookupMap          const lookupMap;
   TypeLookup const * const parentClassLookup;
};



/**
 * \brief This macro simplifies the entries in the \c initializerList parameter of a \c TypeLookup constructor call.  It
 *        also makes it easier for us to modify the structure of \c LookupMapEntry or \c LookupMap in future if we need
 *        to.
 */
#define PROPERTY_TYPE_LOOKUP_ENTRY(propNameConstVar, memberVar) {&propNameConstVar, {std::type_index(typeid(decltype(memberVar))), std::is_enum<decltype(memberVar)>()}}


#endif
