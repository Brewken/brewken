/*======================================================================================================================
 * model/Inventory.cpp is part of Brewken, and is copyright the following authors 2021-2023:
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
#include "model/InventoryHop.h"
#include "model/Misc.h"
#include "model/NamedParameterBundle.h"
#include "model/Yeast.h"
#include "utils/TypeLookup.h"

namespace {

//////////////////////////////////////////////// OLD OLD OLD OLD OLD OLD ////////////////////////////////////////////////

   //
   // A set of template functions and specialisations that sort of allow us to treat InventoryHop as Inventory<Hop>, etc
   //

   template<class Ing> ObjectStore & getInventoryObjectStore();
   template<> ObjectStore & getInventoryObjectStore<Fermentable>() { return ObjectStoreTyped<InventoryFermentable>::getInstance(); }
///   template<> ObjectStore & getInventoryObjectStore<Hop>()         { return ObjectStoreTyped<InventoryHop>::getInstance();         }
   template<> ObjectStore & getInventoryObjectStore<Misc>()        { return ObjectStoreTyped<InventoryMisc>::getInstance();        }
   template<> ObjectStore & getInventoryObjectStore<Yeast>()       { return ObjectStoreTyped<InventoryYeast>::getInstance();       }

   template<class Ing> std::shared_ptr<OldInventory> newInventory();
///   template<> std::shared_ptr<Inventory> newInventory<Hop>()         { return std::static_pointer_cast<Inventory>(std::make_shared<InventoryHop>());         }
   template<> std::shared_ptr<OldInventory> newInventory<Fermentable>() { return std::static_pointer_cast<OldInventory>(std::make_shared<InventoryFermentable>()); }
   template<> std::shared_ptr<OldInventory> newInventory<Misc>()        { return std::static_pointer_cast<OldInventory>(std::make_shared<InventoryMisc>());        }
   template<> std::shared_ptr<OldInventory> newInventory<Yeast>()       { return std::static_pointer_cast<OldInventory>(std::make_shared<InventoryYeast>());       }

}


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
void Inventory::setIngredientId(int const val) { this->setAndNotify(PropertyNames::Inventory::ingredientId, this->m_ingredientId, val); return;}

void Inventory::setDeleted([[maybe_unused]] bool var) {
   // See comment in header.  This is currently a no-op.
   return;
}

void Inventory::setDisplay([[maybe_unused]] bool var) {
   // See comment in header.  This is currently a no-op.
   return;
}

Recipe * Inventory::getOwningRecipe() const {
   // See comment in header.  This is not currently implemented and it's therefore a coding error if it gets called
   qCritical().noquote() << Q_FUNC_INFO << "Call stack is:" << Logging::getStackTrace();
   Q_ASSERT(false);
   return nullptr;
}

void Inventory::hardDeleteOwnedEntities() {
   qDebug() << Q_FUNC_INFO << this->metaObject()->className() << "owns no other entities";
   return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Inventory sub classes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//QString const InventoryFermentable::LocalisedName = tr("Fermentable Inventory");
QString const InventoryHop        ::LocalisedName = tr("Hop Inventory");
//QString const InventoryMisc       ::LocalisedName = tr("Misc Inventory");
//QString const InventoryYeast      ::LocalisedName = tr("Yeast Inventory");

char const * InventoryFermentable::getIngredientClass() const { return "Fermentable"; }
char const * InventoryHop        ::getIngredientClass() const { return "Hop";         }
char const * InventoryMisc       ::getIngredientClass() const { return "Misc";        }
char const * InventoryYeast      ::getIngredientClass() const { return "Yeast";       }

ObjectStore & InventoryFermentable::getObjectStoreTypedInstance() const { return ObjectStoreTyped<InventoryFermentable>::getInstance(); }
ObjectStore & InventoryHop        ::getObjectStoreTypedInstance() const { return ObjectStoreTyped<InventoryHop        >::getInstance(); }
ObjectStore & InventoryMisc       ::getObjectStoreTypedInstance() const { return ObjectStoreTyped<InventoryMisc       >::getInstance(); }
ObjectStore & InventoryYeast      ::getObjectStoreTypedInstance() const { return ObjectStoreTyped<InventoryYeast      >::getInstance(); }


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// InventoryHop
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool InventoryHop::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   InventoryHop const & rhs = static_cast<InventoryHop const &>(other);
   return (
      this->m_amount == rhs.m_amount &&
      // Parent classes have to be equal too
      this->Inventory::isEqualTo(other)
   );
}

TypeLookup const InventoryHop::typeLookup {
   "InventoryHop",
   {
      // All our properties are defined in our base classes
   },
   // Parent classes lookup.  NB: Inventory not NamedEntity!
   {&Inventory::typeLookup,
    std::addressof(IngredientAmount<InventoryHop, Hop>::typeLookup)}
};
static_assert(std::is_base_of<Inventory, InventoryHop>::value);

InventoryHop::InventoryHop() :
   Inventory(),
   IngredientAmount<InventoryHop, Hop>() {
   return;
}

InventoryHop::InventoryHop(NamedParameterBundle const & namedParameterBundle) :
   Inventory                          {namedParameterBundle},
   IngredientAmount<InventoryHop, Hop>{namedParameterBundle} {
   return;
}

InventoryHop::InventoryHop(InventoryHop const & other) :
   Inventory                          {other},
   IngredientAmount<InventoryHop, Hop>{other} {
   return;
}

InventoryHop::~InventoryHop() = default;

Hop * InventoryHop::hop() const {
   return ObjectStoreWrapper::getByIdRaw<Hop>(this->m_ingredientId);
}

// Boilerplate code for IngredientAmount
INGREDIENT_AMOUNT_COMMON_CODE(InventoryHop, Hop)

//////////////////////////////////////////////// OLD OLD OLD OLD OLD OLD ////////////////////////////////////////////////


//
// This private implementation class holds all private non-virtual members of Inventory
//
class OldInventory::impl {
public:

   //
   // Constructors
   //
   impl() : id    {-1},
            amount{0.0} {
      return;
   }

   impl(NamedParameterBundle const & namedParameterBundle) :
      SET_REGULAR_FROM_NPB (id    , namedParameterBundle, PropertyNames::Inventory::id    ),
      SET_REGULAR_FROM_NPB (amount, namedParameterBundle, PropertyNames::Inventory::amount) {
      return;
   }

   impl(impl const & other) :
      id    {other.id    },
      amount{other.amount} {
      return;
   }

   /**
    * Destructor
    */
   ~impl() = default;

   int id;
   double amount;
};

QString const OldInventory::LocalisedName = tr("Inventory");

TypeLookup const OldInventory::typeLookup {
   "Inventory",
   {
      // Note that we need Enums to be treated as ints for the purposes of type lookup
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Inventory::amount               , OldInventory::impl::amount, Measurement::ChoiceOfPhysicalQuantity::Mass_Volume),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Inventory::id                   , OldInventory::impl::id    ),
   },
   // Parent class lookup
   // Note that OldInventory does _not_ inherit from NamedEntity, so this is intentionally empty list
   {}
};

OldInventory::OldInventory() : pimpl{std::make_unique<impl>()} {
   return;
}

OldInventory::OldInventory(NamedParameterBundle const & namedParameterBundle) :
   pimpl{std::make_unique<impl>(namedParameterBundle)} {
   return;
}

// Strictly speaking a QObject is not allowed to be copied, which would mean that since we do not use any state in the
// QObject from which we inherit, we allow Inventory to be copied and just default-initialise the QObject base class in
// the copy.  Hopefully this will never come back to bite us...
OldInventory::OldInventory(OldInventory const & other) :
   QObject{},
   pimpl{std::make_unique<impl>(*other.pimpl)} {
   return;
}

OldInventory::~OldInventory() = default;

int    OldInventory::getId() const      { return this->pimpl->id; }
double OldInventory::getAmount() const  { return this->pimpl->amount; }

void OldInventory::setId(int id) {
   this->pimpl->id = id;
   return;
}

void OldInventory::setKey(int id) {
   this->setId(id);
   return;
}

void OldInventory::setAmount(double amount) {
   this->pimpl->amount = amount;
   // If we're already stored in the object store, tell it about the property change so that it can write it to the
   // database.  (We don't pass the new value as it will get read out of the object via propertyName.)
   if (this->pimpl->id > 0) {
      this->getObjectStoreTypedInstance().updateProperty(*this, PropertyNames::Inventory::amount);
   }
   // .:TBD:. Do we need to send any signals here?  Or should we do that in updateProperty?
   return;
}

void OldInventory::setDeleted([[maybe_unused]] bool var) {
   // See comment in header.  This is not currently implemented and it's therefore a coding error if it gets called
   Q_ASSERT(false);
   return;
}

void OldInventory::setDisplay([[maybe_unused]] bool var) {
   // See comment in header.  This is not currently implemented and it's therefore a coding error if it gets called
   Q_ASSERT(false);
   return;
}

void OldInventory::hardDeleteOwnedEntities() {
   qDebug() << Q_FUNC_INFO << this->metaObject()->className() << "owns no other entities";
   return;
}



template<class Ing>
void InventoryUtils::setAmount(Ing & ing, double amount) {
   // Callers shouldn't try to set negative amounts, but filter here just in case
   if (amount < 0.0) {
      qWarning() << Q_FUNC_INFO << ing.metaObject()->className() << ": negative inventory: " << amount;
      return;
   }

   ObjectStore & inventoryObjectStore = getInventoryObjectStore<Ing>();

   int invId = ing.inventoryId();
   if (invId > 0) {
      // The easy case: set an amount in an existing inventory entry
      auto inventory = std::static_pointer_cast<OldInventory>(inventoryObjectStore.getById(invId));
      inventory->setAmount(amount);
      return;
   }

   // There isn't an inventory entry so (a) create a new one and set the amount...
   auto inventory = newInventory<Ing>();
   inventory->setAmount(amount);
   // ...(b) store it...
   inventoryObjectStore.insert(std::static_pointer_cast<QObject>(inventory));
   // ...(c) tell the ingredient (and its parent, children, siblings) that it now has an inventory
   if (ing.key() > 0) {
      // The ingredient has a valid ID, so it's meaningful to look for its parent, children, siblings
      QVector<int> idsOfParentIngredientAndItsChildren = ing.getParentAndChildrenIds();
      qDebug() <<
         Q_FUNC_INFO << ing.metaObject()->className() << "#" << ing.key() << "has" <<
         idsOfParentIngredientAndItsChildren.size() - 1 << "parents, children and siblings : " <<
         idsOfParentIngredientAndItsChildren;
      auto parentIngredientAndItsChildren = ObjectStoreWrapper::getByIds<Ing>(idsOfParentIngredientAndItsChildren);
      for (auto ii : parentIngredientAndItsChildren) {
         qDebug() <<
            Q_FUNC_INFO << "Assigning new" << inventory->metaObject()->className() << "#" << inventory->getId() <<
            "to" << ing.metaObject()->className() << "#" << ii->key();
         ii->setInventoryId(inventory->getId());
      }
   } else {
      // The ingredient does not have a valid ID, which means it's not yet stored in the database.  We don't normally do
      // things this way around, because it's harder to undo/clean-up, but it should work if the ingredient is about to
      // be stored.
      qWarning() <<
         Q_FUNC_INFO << "Setting inventory amount (" << amount << ") for" << ing.metaObject()->className() <<
         "before it is stored in the database, so inventory #" << inventory->getId() << "does not yet have an owner";
      ing.setInventoryId(inventory->getId());
   }
   return;
}
// Instantiate the above template for all the classes we care about.
// This is just a trick to avoid having the template definition in the header file.
template void InventoryUtils::setAmount(Fermentable & ing, double amount);
///template void InventoryUtils::setAmount(Hop & ing,         double amount);
template void InventoryUtils::setAmount(Misc & ing,        double amount);
template void InventoryUtils::setAmount(Yeast & ing,       double amount);

template<class Ing>
double InventoryUtils::getAmount(Ing const & ing) {
   ObjectStore & inventoryObjectStore = getInventoryObjectStore<Ing>();
   int invId = ing.inventoryId();
   if (invId > 0) {
      auto inventory = std::static_pointer_cast<OldInventory>(inventoryObjectStore.getById(invId));
      return inventory->getAmount();
   }

   // There isn't an inventory for this object, so we don't have any stock of it
   return 0.0;
}
template double InventoryUtils::getAmount(Fermentable const & ing);
///template double InventoryUtils::getAmount(Hop const & ing        );
template double InventoryUtils::getAmount(Misc const & ing       );
template double InventoryUtils::getAmount(Yeast const & ing      );
