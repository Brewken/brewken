/**
 * model/NamedEntityWithInventory.cpp is part of Brewken, and is copyright the following authors 2021:
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
 */
#include "model/NamedEntityWithInventory.h"

#include "model/Inventory.h"

NamedEntityWithInventory::NamedEntityWithInventory(int key, bool cache, QString t_name, bool t_display, QString folder) :
   NamedEntity   {key, cache, t_name, t_display, folder},
   m_inventory_id{-1} {
   return;
}

NamedEntityWithInventory::NamedEntityWithInventory(NamedParameterBundle const & namedParameterBundle) :
   NamedEntity   {namedParameterBundle},
   m_inventory_id{namedParameterBundle(PropertyNames::NamedEntityWithInventory::inventoryId).toInt()} {
   return;
}

NamedEntityWithInventory::NamedEntityWithInventory(NamedEntityWithInventory const & other) :
   NamedEntity     {other                 },
   // Don't copy Inventory ID as new Fermentable should have its own inventory - unless it's a child, but that case is
   // handled in makeChild() below
   m_inventory_id {-1} {
   return;
}

void NamedEntityWithInventory::makeChild(NamedEntity const & copiedFrom) {
   // First do the base class work
   this->NamedEntity::makeChild(copiedFrom);

   // Now we want the child to share the same inventory item as its parent
   this->m_inventory_id = static_cast<NamedEntityWithInventory const &>(copiedFrom).m_inventory_id;
   return;
}

void NamedEntityWithInventory::setInventoryId(int key) {
   if( key < 1 ) {
      qWarning() << QString("Fermentable: bad inventory id: %1").arg(key);
      return;
   }
   m_inventory_id = key;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::NamedEntityWithInventory::inventoryId, key);
   }
   return;
}

int NamedEntityWithInventory::inventoryId() const {
   return m_inventory_id;
}
