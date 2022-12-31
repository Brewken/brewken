/*======================================================================================================================
 * model/NamedEntityWithInventory.h is part of Brewken, and is copyright the following authors 2021-2022:
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
#ifndef MODEL_NAMEDENTITYWITHINVENTORY_H
#define MODEL_NAMEDENTITYWITHINVENTORY_H
#pragma once

#include "model/NamedEntity.h"

//======================================================================================================================
//========================================== Start of property name constants ==========================================
#define AddPropertyName(property) namespace PropertyNames::NamedEntityWithInventory { BtStringConst const property{#property}; }
AddPropertyName(inventory)
AddPropertyName(inventoryId)
#undef AddPropertyName
//=========================================== End of property name constants ===========================================
//======================================================================================================================


/**
 * \class NamedEntityWithInventory
 *
 * \brief Extends \c NamedEntity to provide functionality for storing in Inventory
 */
class NamedEntityWithInventory : public NamedEntity {
   Q_OBJECT
public:
   NamedEntityWithInventory(QString t_name, bool t_display = false, QString folder = QString());
   NamedEntityWithInventory(NamedEntityWithInventory const & other);
   NamedEntityWithInventory(NamedParameterBundle const & namedParameterBundle);

   virtual ~NamedEntityWithInventory() = default;

   //! \brief The amount in inventory (usually in kg)
   Q_PROPERTY( double inventory              READ inventory              WRITE setInventoryAmount        /*NOTIFY changed*/ /*changedInventory*/ )
   //! \brief The inventory table id, needed for signals
   Q_PROPERTY( double inventoryId            READ inventoryId            WRITE setInventoryId            /*NOTIFY changed*/ /*changedInventoryId*/ )

   /**
    * \brief Override \c NamedEntity::makeChild() as we have additional work to do for objects with inventory.
    *        Specifically, a child object needs to have the same inventory as its parent.
    *
    * \param copiedFrom Note that this must stay as a reference to \c NamedEntity because we need to have the same
    *                   signature as the base class member function that we're overriding.
    */
   virtual void makeChild(NamedEntity const & copiedFrom);

   virtual double inventory() const = 0;
   int inventoryId() const;

   virtual void setInventoryAmount(double amount) = 0;
   void setInventoryId(int key);

protected:
   int m_inventory_id;
};

#endif
