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
//
// It would be nice to find a way to reduce the amount of copy-and-paste here
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QString const InventoryFermentable::LocalisedName = tr("Fermentable Inventory");
QString const InventoryHop        ::LocalisedName = tr("Hop Inventory");
QString const InventoryMisc       ::LocalisedName = tr("Misc Inventory");
QString const InventoryYeast      ::LocalisedName = tr("Yeast Inventory");

char const * InventoryFermentable::getIngredientClass() const { return "Fermentable"; }
char const * InventoryHop        ::getIngredientClass() const { return "Hop"        ; }
char const * InventoryMisc       ::getIngredientClass() const { return "Misc"       ; }
char const * InventoryYeast      ::getIngredientClass() const { return "Yeast"      ; }

ObjectStore & InventoryFermentable::getObjectStoreTypedInstance() const { return ObjectStoreTyped<InventoryFermentable>::getInstance(); }
ObjectStore & InventoryHop        ::getObjectStoreTypedInstance() const { return ObjectStoreTyped<InventoryHop        >::getInstance(); }
ObjectStore & InventoryMisc       ::getObjectStoreTypedInstance() const { return ObjectStoreTyped<InventoryMisc       >::getInstance(); }
ObjectStore & InventoryYeast      ::getObjectStoreTypedInstance() const { return ObjectStoreTyped<InventoryYeast      >::getInstance(); }

bool InventoryFermentable::isEqualTo(NamedEntity const & other) const {
   InventoryFermentable const & rhs = static_cast<InventoryFermentable const &>(other);
   return (this->m_amount == rhs.m_amount && this->Inventory::isEqualTo(other));
}
bool InventoryHop::isEqualTo(NamedEntity const & other) const {
   InventoryHop const & rhs = static_cast<InventoryHop const &>(other);
   return (this->m_amount == rhs.m_amount && this->Inventory::isEqualTo(other));
}
bool InventoryMisc::isEqualTo(NamedEntity const & other) const {
   InventoryMisc const & rhs = static_cast<InventoryMisc const &>(other);
   return (this->m_amount == rhs.m_amount && this->Inventory::isEqualTo(other));
}
bool InventoryYeast::isEqualTo(NamedEntity const & other) const {
   InventoryYeast const & rhs = static_cast<InventoryYeast const &>(other);
   return (this->m_amount == rhs.m_amount && this->Inventory::isEqualTo(other));
}

// All properties are defined in base classes
TypeLookup const InventoryFermentable::typeLookup {
   "InventoryFermentable", { },
   {&Inventory::typeLookup, std::addressof(IngredientAmount<InventoryFermentable, Fermentable>::typeLookup)}
};
static_assert(std::is_base_of<Inventory, InventoryFermentable>::value);
TypeLookup const InventoryHop::typeLookup {
   "InventoryHop", { },
   {&Inventory::typeLookup, std::addressof(IngredientAmount<InventoryHop, Hop>::typeLookup)}
};
static_assert(std::is_base_of<Inventory, InventoryHop>::value);
TypeLookup const InventoryMisc::typeLookup {
   "InventoryMisc", { },
   {&Inventory::typeLookup, std::addressof(IngredientAmount<InventoryMisc, Misc>::typeLookup)}
};
static_assert(std::is_base_of<Inventory, InventoryMisc>::value);
TypeLookup const InventoryYeast::typeLookup {
   "InventoryYeast", { },
   {&Inventory::typeLookup, std::addressof(IngredientAmount<InventoryYeast, Yeast>::typeLookup)}
};
static_assert(std::is_base_of<Inventory, InventoryYeast>::value);

InventoryFermentable::InventoryFermentable() : Inventory(), IngredientAmount<InventoryFermentable, Fermentable>() { return; }
InventoryHop        ::InventoryHop        () : Inventory(), IngredientAmount<InventoryHop        , Hop        >() { return; }
InventoryMisc       ::InventoryMisc       () : Inventory(), IngredientAmount<InventoryMisc       , Misc       >() { return; }
InventoryYeast      ::InventoryYeast      () : Inventory(), IngredientAmount<InventoryYeast      , Yeast      >() { return; }

InventoryFermentable::InventoryFermentable(NamedParameterBundle const & npb) : Inventory {npb}, IngredientAmount<InventoryFermentable, Fermentable>{npb} { return; }
InventoryHop        ::InventoryHop        (NamedParameterBundle const & npb) : Inventory {npb}, IngredientAmount<InventoryHop        , Hop        >{npb} { return; }
InventoryMisc       ::InventoryMisc       (NamedParameterBundle const & npb) : Inventory {npb}, IngredientAmount<InventoryMisc       , Misc       >{npb} { return; }
InventoryYeast      ::InventoryYeast      (NamedParameterBundle const & npb) : Inventory {npb}, IngredientAmount<InventoryYeast      , Yeast      >{npb} { return; }

InventoryFermentable::InventoryFermentable(InventoryFermentable const & other) : Inventory {other}, IngredientAmount<InventoryFermentable, Fermentable>{other} { return; }
InventoryHop        ::InventoryHop        (InventoryHop         const & other) : Inventory {other}, IngredientAmount<InventoryHop        , Hop        >{other} { return; }
InventoryMisc       ::InventoryMisc       (InventoryMisc        const & other) : Inventory {other}, IngredientAmount<InventoryMisc       , Misc       >{other} { return; }
InventoryYeast      ::InventoryYeast      (InventoryYeast       const & other) : Inventory {other}, IngredientAmount<InventoryYeast      , Yeast      >{other} { return; }

InventoryFermentable::~InventoryFermentable() = default;
InventoryHop        ::~InventoryHop        () = default;
InventoryMisc       ::~InventoryMisc       () = default;
InventoryYeast      ::~InventoryYeast      () = default;

Fermentable * InventoryFermentable::fermentable() const { return ObjectStoreWrapper::getByIdRaw<Fermentable>(this->m_ingredientId); }
Hop         * InventoryHop        ::hop        () const { return ObjectStoreWrapper::getByIdRaw<Hop        >(this->m_ingredientId); }
Misc        * InventoryMisc       ::misc       () const { return ObjectStoreWrapper::getByIdRaw<Misc       >(this->m_ingredientId); }
Yeast       * InventoryYeast      ::yeast      () const { return ObjectStoreWrapper::getByIdRaw<Yeast      >(this->m_ingredientId); }

// Boilerplate code for IngredientAmount
INGREDIENT_AMOUNT_COMMON_CODE(InventoryFermentable, Fermentable)
INGREDIENT_AMOUNT_COMMON_CODE(InventoryHop        , Hop        )
INGREDIENT_AMOUNT_COMMON_CODE(InventoryMisc       , Misc       )
INGREDIENT_AMOUNT_COMMON_CODE(InventoryYeast      , Yeast      )
