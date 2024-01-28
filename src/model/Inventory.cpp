/*======================================================================================================================
 * model/Inventory.cpp is part of Brewken, and is copyright the following authors 2021-2024:
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
#include "model/Inventory.h"

#include "database/ObjectStoreWrapper.h"
#include "model/Fermentable.h"
#include "model/Hop.h"
#include "model/InventoryFermentable.h"
#include "model/InventoryHop.h"
#include "model/Misc.h"
#include "model/NamedParameterBundle.h"
#include "model/Yeast.h"
#include "utils/TypeLookup.h"

// If we're compiling with CMake, the AUTOMOC property will run the Qt meta-object compiler (MOC) on InventoryHop.h (to
// produce moc_InventoryHop.cpp) etc but will not link the resulting code (because there is not a corresponding
// InventoryHop.cpp.  If we include the results of the MOC here, it guarantees they get linked into the final
// executable.
#include "moc_InventoryFermentable.cpp"
#include "moc_InventoryHop.cpp"
#include "moc_InventoryMisc.cpp"
#include "moc_InventorySalt.cpp"
#include "moc_InventoryYeast.cpp"

QString const Inventory::LocalisedName = tr("Inventory");

bool Inventory::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   Inventory const & rhs = static_cast<Inventory const &>(other);
   return (
      this->m_ingredientId == rhs.m_ingredientId
   );
}

TypeLookup const Inventory::typeLookup {
   "Inventory",
   {
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Inventory::ingredientId, Inventory::m_ingredientId),
   },
   {&NamedEntity::typeLookup}
};

Inventory::Inventory() :
   NamedEntity{""},
   m_ingredientId{-1} {
   return;
}

Inventory::Inventory(NamedParameterBundle const & namedParameterBundle) :
   NamedEntity{namedParameterBundle},
   SET_REGULAR_FROM_NPB (m_ingredientId, namedParameterBundle, PropertyNames::Inventory::ingredientId) {
   return;
}

Inventory::Inventory(Inventory const & other) :
   NamedEntity   {other               },
   m_ingredientId{other.m_ingredientId} {
   return;
}

Inventory::~Inventory() = default;

//============================================ "GETTER" MEMBER FUNCTIONS ============================================
int Inventory::ingredientId() const { return this->m_ingredientId; }

//============================================ "SETTER" MEMBER FUNCTIONS ============================================
void Inventory::setIngredientId(int const val) { SET_AND_NOTIFY(PropertyNames::Inventory::ingredientId, this->m_ingredientId, val); return;}

void Inventory::setDeleted([[maybe_unused]] bool var) {
   // See comment in header.  This is currently a no-op.
   return;
}

void Inventory::setDisplay([[maybe_unused]] bool var) {
   // See comment in header.  This is currently a no-op.
   return;
}

///Recipe * Inventory::getOwningRecipe() const {
///   // See comment in header.  This is not currently implemented and it's therefore a coding error if it gets called
///   qCritical().noquote() << Q_FUNC_INFO << "Call stack is:" << Logging::getStackTrace();
///   Q_ASSERT(false);
///   return nullptr;
///}

void Inventory::hardDeleteOwnedEntities() {
   qDebug() << Q_FUNC_INFO << this->metaObject()->className() << "owns no other entities";
   return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Inventory sub classes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INVENTORY_COMMON_CODE(Fermentable, fermentable)
INVENTORY_COMMON_CODE(Hop        , hop        )
INVENTORY_COMMON_CODE(Misc       , misc       )
INVENTORY_COMMON_CODE(Salt       , salt       )
INVENTORY_COMMON_CODE(Yeast      , yeast      )

// Boilerplate code for IngredientAmount
INGREDIENT_AMOUNT_COMMON_CODE(InventoryFermentable, Fermentable)
INGREDIENT_AMOUNT_COMMON_CODE(InventoryHop        , Hop        )
INGREDIENT_AMOUNT_COMMON_CODE(InventoryMisc       , Misc       )
INGREDIENT_AMOUNT_COMMON_CODE(InventorySalt       , Salt       )
INGREDIENT_AMOUNT_COMMON_CODE(InventoryYeast      , Yeast      )
