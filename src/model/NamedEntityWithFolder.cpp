/*======================================================================================================================
 * model/NamedEntityWithFolder.cpp is part of Brewken, and is copyright the following authors 2024:
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
#include "model/NamedEntityWithFolder.h"

#include "model/NamedParameterBundle.h"

QString const NamedEntityWithFolder::LocalisedName = tr("Named Entity With Folder");

TypeLookup const NamedEntityWithFolder::typeLookup {
   "NamedEntityWithFolder",
   {
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::NamedEntityWithFolder::folder, NamedEntityWithFolder::m_folder),
   },
   // Parent class lookup
   {&NamedEntity::typeLookup}
};

NamedEntityWithFolder::NamedEntityWithFolder(QString name, bool display, QString folder) :
   NamedEntity{name, display},
   m_folder   {folder       } {
   return;
}

NamedEntityWithFolder::NamedEntityWithFolder(NamedParameterBundle const & namedParameterBundle) :
   NamedEntity        {namedParameterBundle},
   SET_REGULAR_FROM_NPB (m_folder , namedParameterBundle, PropertyNames::NamedEntityWithFolder::folder, QString{}) {
   return;
}

NamedEntityWithFolder::NamedEntityWithFolder(NamedEntityWithFolder const & other) :
   NamedEntity{other},
   m_folder       {other.m_folder } {
   return;
}

NamedEntityWithFolder::~NamedEntityWithFolder() = default;

void NamedEntityWithFolder::swap(NamedEntityWithFolder & other) noexcept {
   this->NamedEntity::swap(other);
   std::swap(this->m_folder , other.m_folder );
   return;
}

QString NamedEntityWithFolder::folder() const {
   return this->m_folder;
}

void NamedEntityWithFolder::setFolder(QString const & var) {
   if (this->newValueMatchesExisting(PropertyNames::NamedEntityWithFolder::folder, this->m_folder, var)) {
      return;
   }
   this->m_folder = var;
   this->propagatePropertyChange(PropertyNames::NamedEntityWithFolder::folder);
   return;
}
