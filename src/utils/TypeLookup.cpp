/*======================================================================================================================
 * utils/TypeLookup.cpp is part of Brewken, and is copyright the following authors 2023:
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
#include "utils/TypeLookup.h"

#include <optional>
#include <typeinfo>

#include <QDate>
#include <QDebug>
#include <QString>

#include "Logging.h"
#include "measurement/Amount.h"

namespace {
   //
   // There's probably some wildly clever way to do this with template meta-programming, but I think this more
   // pedestrian approach is sufficient for our needs
   //
   // The Q_DECLARE_METATYPE declarations in model/NamedParameterBundle.h should give all the std::optional types we
   // use and, by extension, all the non-optional types we use.
   //
   std::map<std::type_index, bool> typeIsOptional {
      {std::type_index(typeid(bool           )), false}, {std::type_index(typeid(std::optional<bool           >)), true},
      {std::type_index(typeid(double         )), false}, {std::type_index(typeid(std::optional<double         >)), true},
      {std::type_index(typeid(int            )), false}, {std::type_index(typeid(std::optional<int            >)), true},
      {std::type_index(typeid(QDate          )), false}, {std::type_index(typeid(std::optional<QDate          >)), true},
      {std::type_index(typeid(QString        )), false}, {std::type_index(typeid(std::optional<QString        >)), true},
      {std::type_index(typeid(unsigned int   )), false}, {std::type_index(typeid(std::optional<unsigned int   >)), true},
      {std::type_index(typeid(MassOrVolumeAmt)), false}, {std::type_index(typeid(std::optional<MassOrVolumeAmt>)), true},
   };

}

TypeLookup::TypeLookup(char       const * const                                 className,
                       std::initializer_list<TypeLookup::LookupMap::value_type> initializerList,
                       TypeLookup const * const                                 parentClassLookup) :
   className{className},
   lookupMap{initializerList},
   parentClassLookup{parentClassLookup} {
   return;
}


TypeLookup::LookupMapEntry TypeLookup::getType(BtStringConst const & propertyName) const {
   auto match = std::find_if(
      this->lookupMap.begin(),
      this->lookupMap.end(),
      [& propertyName](auto const & record) { return propertyName == *record.first; }
   );

   if (match != this->lookupMap.end()) {
      return match->second;
   }

   if (this->parentClassLookup) {
      return this->parentClassLookup->getType(propertyName);
   }

   // It's a coding error if we tried to look up a property that we don't know about
   qCritical() << Q_FUNC_INFO << "Can't find type info for property" << *propertyName << "of class" << this->className;
   qDebug().noquote() << Q_FUNC_INFO << Logging::getStackTrace();
   Q_ASSERT(false);
   throw std::bad_typeid();

}


bool TypeLookup::isOptional(BtStringConst const & propertyName) const {
   TypeLookup::LookupMapEntry const lookupMapEntry = this->getType(propertyName);
   // If the type is an enum (second part of the returned pair is true) then we need to pretend it is an int (ie
   // override what's in the first part of the returned pair).  Otherwise this file would need to know about all the
   // strongly-typed enums in the code, which would be cumbersome to say the least.
   std::type_index const propertyType = lookupMapEntry.second ? std::type_index(typeid(int)) : lookupMapEntry.first;

   //
   // Even though type_index is, essentially, a pointer to a std::type_info object (and normally you can't rely on two
   // equal std::type_info objects necessarily having the same address in memory), type_index is designed, amongst other
   // things, to "be used as index in associative and unordered associative containers".  This means it's safe to use
   // the "efficient" search, because comparisons do the right thing (ie they don't just compare pointer values).
   //
   // TLDR, we can use find() here where we had to use find_if when searching for BtStringConst above.
   //
   auto match = typeIsOptional.find(propertyType);
   if (match != typeIsOptional.end()) {
      return match->second;
   }

   // It's a coding error if any property has a type we don't know about
   qCritical() <<
      Q_FUNC_INFO << "Don't know about type" << propertyType.name() << "of property" << *propertyName << "of class" <<
      this->className;
   qDebug().noquote() << Q_FUNC_INFO << Logging::getStackTrace();
   Q_ASSERT(false);
   throw std::bad_typeid();
}
