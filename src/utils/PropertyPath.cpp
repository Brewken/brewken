/*======================================================================================================================
 * utils/PropertyPath.cpp is part of Brewken, and is copyright the following authors 2023:
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
#include "utils/PropertyPath.h"

#include "model/NamedEntity.h"

PropertyPath::PropertyPath(BtStringConst const & singleProperty) :
   m_properties{1, &singleProperty}, m_path{*singleProperty} {
   return;
}

PropertyPath::PropertyPath(std::initializer_list<std::reference_wrapper<BtStringConst const>> listOfProperties) :
   m_properties{}, m_path{} {
   bool first = true;
   for (auto const & ii : listOfProperties) {
      m_properties.append(&ii.get());
      if (!first) {
         m_path.append("/");
      }
      first = false;
      m_path.append(*ii.get());
   }
   return;
}

PropertyPath::PropertyPath(PropertyPath const & other) :
   m_properties{other.m_properties},
   m_path      {other.m_path      } {
   return;
}

PropertyPath::~PropertyPath() = default;

QString PropertyPath::asXPath() const {
   return this->m_path;
}

QVector<BtStringConst const *> const & PropertyPath::properties() const {
   return this->m_properties;
}

bool PropertyPath::isNull() const {
   if (this->m_properties.size() == 1) {
      return BtString::NULL_STR == *this->m_properties[0];
   }
   return false;
}

TypeInfo const & PropertyPath::getTypeInfo(TypeLookup const & baseTypeLookup) const {
   TypeInfo const * returnValue = nullptr;
   // Can be useful to uncomment the next line for debugging
//   qDebug() << Q_FUNC_INFO << "Applying PropertyPath" << *this << "to" << baseTypeLookup;

   TypeLookup const * typeLookup = &baseTypeLookup;
   for (auto const property : this->m_properties) {
      TypeInfo const & typeInfo = typeLookup->getType(*property);
      if (property == this->m_properties.last()) {
         returnValue = &typeInfo;
         break;
      }
      typeLookup = typeInfo.typeLookup;
      // It's a coding error if there is no TypeLookup
      if (!typeLookup) {
         qCritical() <<
            Q_FUNC_INFO << "Applying PropertyPath" << *this << "to" << baseTypeLookup << ": no TypeLookup for" <<
            *property << ". (getType returned " << typeInfo << ")";
         Q_ASSERT(false);
      }
   }

   return *returnValue;

}

bool PropertyPath::setValue(NamedEntity & obj, QVariant const & val) const {
   NamedEntity * ne = &obj;
   for (auto const property : this->m_properties) {
      if (property == this->m_properties.last()) {

         // It's a coding error if we're trying to set a non-existent property on the NamedEntity subclass for this
         // record.
         QMetaObject const * neMetaObject = ne->metaObject();
         int propertyIndex = neMetaObject->indexOfProperty(**property);
         Q_ASSERT(propertyIndex >= 0);
         QMetaProperty neMetaProperty = neMetaObject->property(propertyIndex);

         qDebug() <<
            Q_FUNC_INFO << "Request to set" << this->m_path << "on" << obj.metaObject()->className() << "(=" <<
            *property << "on" << ne->metaObject()->className() << "); type =" << neMetaProperty.typeName() <<
            "; writable =" << neMetaProperty.isWritable();

         if (neMetaProperty.isWritable()) {
            ne->setProperty(**property, val);
            return true;
         }
         break;
      }

      ne = ne->ensureExists(*property);
   }

   return false;
}

QVariant PropertyPath::getValue(NamedEntity const & obj) const {
   QVariant retVal{};
   NamedEntity const * ne = &obj;
   for (auto const property : this->m_properties) {
      // Normally keep the next line commented out otherwise it generates too many lines in the log file
//      qDebug() << Q_FUNC_INFO << "Looking at" << *property;

      if (property == this->m_properties.last()) {
         retVal = ne->property(**property);
         break;
      }

      // Note that what we are expecting to get back inside the QVariant is `NamedEntity *`.  It's OK for us to assign
      // this pointer to a variable of type `NamedEntity const *`, but we shouldn't expect
      // `containedNe.canConvert<NamedEntity const *>()` to return true (because it won't).
      QVariant containedNe = ne->property(**property);
      if (!containedNe.isValid() || !containedNe.canConvert<NamedEntity *>()) {
         break;
      }
      ne = containedNe.value<NamedEntity *>();
      if (!ne) {
         qDebug() << Q_FUNC_INFO << "Property" << *property << "returned nullptr";
         break;
      }
   }
   return retVal;
}
