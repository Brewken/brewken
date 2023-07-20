/*======================================================================================================================
 * model/PropertiesForInventory.h is part of Brewken, and is copyright the following authors 2021-2023:
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
#ifndef MODEL_PROPERTIESFORINVENTORY_H
#define MODEL_PROPERTIESFORINVENTORY_H
#pragma once

#include <QDebug>

#include "model/Inventory.h" // For InventoryUtils
#include "utils/BtStringConst.h"
#include "utils/CuriouslyRecurringTemplateBase.h"

//======================================================================================================================
//========================================== Start of property name constants ==========================================
// See comment in model/NamedEntity.h
#define AddPropertyName(property) namespace PropertyNames::PropertiesForInventory { BtStringConst const property{#property}; }
AddPropertyName(inventory         )
AddPropertyName(inventoryId       )
AddPropertyName(inventoryWithUnits)
#undef AddPropertyName
//=========================================== End of property name constants ===========================================
//======================================================================================================================

/**
 * \brief This is a Curiously Recurring Template Pattern base to add functionality for storing in Inventory.
 *
 *        Derived classes need to do several things besides publicly inheriting from this class and calling its
 *        constructor (in all cases below, making the relevant substitution for `Derived`!):
 *
 *           1) In their .h file, derived classes must include the following line inside their class definition:
 *                 PROPERTIES_FOR_INVENTORY_DECL(Derived)
 *
 *           2) In their .cpp file, derived classes must include the following line inside their \c typeLookup member
 *              definition:
 *                 PROPERTIES_FOR_INVENTORY_TYPE_LOOKUP_DEFNS(Derived)
 *
 *           3) Also in their .cpp file, derived classes must include the following line:
 *                 PROPERTIES_FOR_INVENTORY_COMMON_CODE(Derived)
 *
 */
template<class Derived>
class PropertiesForInventory : public CuriouslyRecurringTemplateBase<Derived> {
public:
   PropertiesForInventory() : m_inventory_id {-1} {
      return;
   }
   ~PropertiesForInventory() = default;

   int             doInventoryId       () const { return this->m_inventory_id; }
   double          doInventory         () const { return InventoryUtils::getAmount(this->derived()); }
   MassOrVolumeAmt doInventoryWithUnits() const { return MassOrVolumeAmt{InventoryUtils::getAmount(this->derived()), this->derived().amountIsWeight() ? Measurement::Units::kilograms : Measurement::Units::liters};}

   void doSetInventoryId       (int             const key) {
      if (key < 1) {
         // This really shouldn't happen
         qCritical() << Q_FUNC_INFO << this->derived().metaObject()->className() << "Bad inventory id:" << key;
         Q_ASSERT(false); // Bail on debug build
         return;          // Continue (without setting invalid ID) otherwise
      }
      this->derived().setAndNotify(PropertyNames::PropertiesForInventory::inventoryId, this->m_inventory_id, key);
      return;
   }
   void doSetInventoryAmount   (double          const val) { InventoryUtils::setAmount(this->derived(), val); return; }
   void doSetInventoryWithUnits(MassOrVolumeAmt const val) { this->doSetInventoryAmount(val.quantity()); return; }

protected:
   int m_inventory_id;
};

/**
 * \brief Derived classes should include this in their header file, right after Q_OBJECT
 *
 *        Note we have to be careful about comment formats in macro definitions
 */
#define PROPERTIES_FOR_INVENTORY_DECL(NeName) \
   /* This allows PropertiesForInventory to call protected and private members of Derived */                         \
   friend class PropertiesForInventory<NeName>;                                                                      \
public:                                                                                                              \
   /* =============================================== PROPERTIES ================================================ */ \
   /** \brief The inventory table id, needed for signals */                                                          \
   Q_PROPERTY(int               inventoryId          READ inventoryId          WRITE setInventoryId    )             \
   /** \brief The amount in inventory (usually in kg) */                                                             \
   Q_PROPERTY(double            inventory            READ inventory            WRITE setInventoryAmount)             \
   /** \brief Amounts of some things can be measured by mass or by volume (depending usually on what it is). */      \
   /*         NOTE: This property \b cannot be used to change between mass and volume. */                            \
   Q_PROPERTY(MassOrVolumeAmt   inventoryWithUnits   READ inventoryWithUnits   WRITE setInventoryWithUnits)          \
                                                                                                                     \
   /* ================================================= GETTERS ================================================= */ \
   int             inventoryId       () const;                                                                       \
   double          inventory         () const;                                                                       \
   MassOrVolumeAmt inventoryWithUnits() const;                                                                       \
                                                                                                                     \
   /* ================================================= SETTERS ================================================= */ \
   void setInventoryId       (int             const val);                                                            \
   void setInventoryAmount   (double          const val);                                                            \
   void setInventoryWithUnits(MassOrVolumeAmt const val);                                                            \


/**
 * \brief Derived classes should include this in their implementation (.cpp) file, inside their \c typeLookup member
 *        definition.
 *
 *        Note we have to be careful about comment formats in macro definitions
 */
#define PROPERTIES_FOR_INVENTORY_TYPE_LOOKUP_DEFNS(NeName) \
   PROPERTY_TYPE_LOOKUP_ENTRY      (PropertyNames::PropertiesForInventory::inventoryId,        NeName::m_inventory_id    ), \
   PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::PropertiesForInventory::inventoryWithUnits, NeName::inventoryWithUnits, Measurement::PqEitherMassOrVolume),

/**
 * \brief Derived classes should include this at the end of their implementation (.cpp) file
 *
 *        Note we have to be careful about comment formats in macro definitions
 */
#define PROPERTIES_FOR_INVENTORY_COMMON_CODE(NeName) \
   /* ================================================= GETTERS ================================================= */ \
   int             NeName::inventoryId       () const { return this->doInventoryId       (); }                       \
   double          NeName::inventory         () const { return this->doInventory         (); }                       \
   MassOrVolumeAmt NeName::inventoryWithUnits() const { return this->doInventoryWithUnits(); }                       \
                                                                                                                     \
   /* ================================================= SETTERS ================================================= */ \
   void NeName::setInventoryId       (int             const val) { this->doSetInventoryId       (val); return; }     \
   void NeName::setInventoryAmount   (double          const val) { this->doSetInventoryAmount   (val); return; }     \
   void NeName::setInventoryWithUnits(MassOrVolumeAmt const val) { this->doSetInventoryWithUnits(val); return; }     \

#endif
