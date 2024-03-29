/*======================================================================================================================
 * model/NamedEntityWithInventory.h is part of Brewken, and is copyright the following authors 2021-2023:
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
// See comment in model/NamedEntity.h
#define AddPropertyName(property) namespace PropertyNames::NamedEntityWithInventory { BtStringConst const property{#property}; }
AddPropertyName(inventory  )
AddPropertyName(inventoryId)
AddPropertyName(inventoryWithUnits)
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
   /**
    * \brief Mapping of names to types for the Qt properties of this class.  See \c NamedEntity::typeLookup for more
    *        info.
    */
   static TypeLookup const typeLookup;

   NamedEntityWithInventory(QString t_name, bool t_display = false, QString folder = QString());
   NamedEntityWithInventory(NamedEntityWithInventory const & other);
   NamedEntityWithInventory(NamedParameterBundle const & namedParameterBundle);

   virtual ~NamedEntityWithInventory();

   //! \brief The amount in inventory (usually in kg)
   Q_PROPERTY(double inventory    READ inventory    WRITE setInventoryAmount)
   //! \brief The inventory table id, needed for signals
   Q_PROPERTY(int    inventoryId  READ inventoryId  WRITE setInventoryId    )

   /**
    * \brief Amounts of \c Fermentable and \c Misc can be measured by mass or by volume (depending usually on what it
    *        is).
    *
    *        NOTE: This property \b cannot be used to change between mass and volume.
    */
   Q_PROPERTY(MassOrVolumeAmt    inventoryWithUnits   READ inventoryWithUnits   WRITE setInventoryWithUnits)

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
   virtual MassOrVolumeAmt inventoryWithUnits() const = 0;

   virtual void setInventoryAmount(double amount) = 0;
   void setInventoryId(int key);
   virtual void setInventoryWithUnits(MassOrVolumeAmt const val) = 0;

protected:
   int m_inventory_id;
};

/**
 * \brief Derived classes should include this in their header file
 */
#define    INVENTORY_COMMON_HEADER_DEFNS \
   virtual double inventory() const; \
   virtual MassOrVolumeAmt inventoryWithUnits() const; \
   virtual void setInventoryAmount(double amount); \
   virtual void setInventoryWithUnits(MassOrVolumeAmt const val);

/**
 * \brief Derived classes should include this in their implementation file if they support measuring by volume and by
 *        mass
 */
#define INVENTORY_COMMON_CODE(NeName) \
double NeName::inventory() const { return InventoryUtils::getAmount(*this); } \
MassOrVolumeAmt NeName::inventoryWithUnits() const { return MassOrVolumeAmt{InventoryUtils::getAmount(*this), this->amountIsWeight() ? Measurement::Units::kilograms : Measurement::Units::liters}; } \
void NeName::setInventoryAmount(double num) { InventoryUtils::setAmount(*this, num); return; } \
void NeName::setInventoryWithUnits(MassOrVolumeAmt const  val) { this->setInventoryAmount(val.quantity()); return; }

/**
 * \brief Derived classes should include this in their implementation file if they support measuring by mass only
 */
#define INVENTORY_COMMON_CODE_MO(NeName) \
double NeName::inventory() const { return InventoryUtils::getAmount(*this); } \
MassOrVolumeAmt NeName::inventoryWithUnits() const { return MassOrVolumeAmt{InventoryUtils::getAmount(*this), Measurement::Units::kilograms}; } \
void NeName::setInventoryAmount(double num) { InventoryUtils::setAmount(*this, num); return; } \
void NeName::setInventoryWithUnits(MassOrVolumeAmt const  val) { this->setInventoryAmount(val.quantity()); return; }

#endif
