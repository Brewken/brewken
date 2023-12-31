/*======================================================================================================================
 * model/Inventory.h is part of Brewken, and is copyright the following authors 2021-2023:
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
#ifndef MODEL_INVENTORY_H
#define MODEL_INVENTORY_H
#pragma once

#include <memory>

#include <QObject>

#include "database/ObjectStoreWrapper.h"
#include "model/NamedParameterBundle.h"
#include "model/Ingredient.h"
#include "model/NamedEntity.h"
#include "utils/MetaTypes.h"

class Hop;
class ObjectStore;
class TypeLookup;

//======================================================================================================================
//========================================== Start of property name constants ==========================================
// See comment in model/NamedEntity.h
#define AddPropertyName(property) namespace PropertyNames::Inventory { BtStringConst const property{#property}; }
AddPropertyName(id)      // Deprecated.  Use PropertyNames::NamedEntity::key instead
AddPropertyName(amount)  // Deprecated.  Use PropertyNames::IngredientAmount::quantity instead
AddPropertyName(ingredientId)
#undef AddPropertyName
//=========================================== End of property name constants ===========================================
//======================================================================================================================


/**
 * \brief Class representing an inventory entry for Hop/Fermentable/Yeast/Misc
 *
 *        Initial version of this class holds rather minimal data, but we envisage expanding it in future
 *
 *        NB: When we add, eg, a Hop to a Recipe, we make a copy for various reasons (including that the amount of Hop
 *            used in the Recipe is stored in the Hop, not the Recipe).  Each such copy _shares_ its Inventory with the
 *            Hop from which it was copied (aka its parent).  Thus all the Hops with the same parent will have the
 *            same Inventory object as that parent (because they are not really different Hops, merely different usages
 *            of that parent hop).
 *
 *            We want each type of inventory to be a different class so that it works with \c ObjectStoreTyped
 *
 *            It would be tempting to make Inventory a templated class (for \c Inventory<Hop>,
 *            \c Inventory<Fermentable>, etc), however we need Inventory to inherit from QObject so we can use Qt
 *            Properties in the \c ObjectStore layer, and this precludes the use of templates.  (The Qt meta-object
 *            compiler, aka moc, does not understand C++ templates.)
 *
 *            Instead we use inheritance to get \c InventoryHop, \c InventoryFermentable, etc, which is a bit more
 *            clunky, but not a lot.  And there are a few tricks in the cpp file that still allow us to do some
 *            templating.
 */
class Inventory : public NamedEntity {
   Q_OBJECT

public:
   /**
    * \brief See comment in model/NamedEntity.h
    */
   static QString const LocalisedName;

   /**
    * \brief Mapping of names to types for the Qt properties of this class.  See \c NamedEntity::typeLookup for more
    *        info.
    */
   static TypeLookup const typeLookup;

   Inventory();
   Inventory(NamedParameterBundle const & namedParameterBundle);
   Inventory(Inventory const & other);

   virtual ~Inventory();

   //=================================================== PROPERTIES ====================================================
   Q_PROPERTY(int    ingredientId     READ ingredientId     WRITE setIngredientId    )
   /**
    * These properties are defined here with virtual accessors.  Child classes actually get the implementations by
    * inheriting from \c IngredientAmount.
    */
   Q_PROPERTY(Measurement::Amount           amount    READ amount     WRITE setAmount  )
   Q_PROPERTY(double                        quantity  READ quantity   WRITE setQuantity)
   Q_PROPERTY(Measurement::Unit const *     unit      READ unit       WRITE setUnit    )
   Q_PROPERTY(Measurement::PhysicalQuantity measure   READ measure    WRITE setMeasure )
   Q_PROPERTY(bool                          isWeight  READ isWeight   WRITE setIsWeight)

   //============================================ "GETTER" MEMBER FUNCTIONS ============================================
   int ingredientId() const;

   virtual Measurement::Amount           amount  () const = 0;
   virtual double                        quantity() const = 0;
   virtual Measurement::Unit const *     unit    () const = 0;
   virtual Measurement::PhysicalQuantity measure () const = 0;
   virtual bool                          isWeight() const = 0;

   //============================================ "SETTER" MEMBER FUNCTIONS ============================================
   void setIngredientId(int const val);

   virtual void setAmount  (Measurement::Amount           const & val) = 0;
   virtual void setQuantity(double                        const   val) = 0;
   virtual void setUnit    (Measurement::Unit const *     const   val) = 0;
   virtual void setMeasure (Measurement::PhysicalQuantity const   val) = 0;
   virtual void setIsWeight(bool                          const   val) = 0;


   //============================================= OTHER MEMBER FUNCTIONS ==============================================

   /**
    * \brief This doesn't actually do anything, but using ObjectStoreTyped means we have to provide an implementation,
    *        as it's needed for \c ObjectStoreTyped::softDelete().
    */
   void setDeleted(bool var);

   /**
    * \brief This doesn't actually do anything, but using ObjectStoreTyped means we have to provide an implementation,
    *        as it's needed for \c ObjectStoreTyped::softDelete().
    */
   void setDisplay(bool var);

   /**
    * \brief Returns the name of the ingredient class (eg Hop, Fermentable, Misc, Yeast) to which this Inventory class
    *        relates.  Subclasses need to provide the (trivial) implementation of this.  Primarily useful for logging
    *        and debugging.
    */
   virtual char const * getIngredientClass() const = 0;

   /**
    * TBD: This is needed because NamedEntity has it, but I'd like to refactor it out at some point.
    */
   virtual Recipe * getOwningRecipe() const;

   /**
    * \brief We need this for ObjectStoreTyped to call
    */
   void hardDeleteOwnedEntities();

protected:
   virtual bool isEqualTo(NamedEntity const & other) const;

   int m_ingredientId;
};

/**
 * \brief For templates that require a parameter to be a subclass of \c Inventory, this makes the concept requirement
 *        slightly more concise.
 *
 *        See comment in utils/TypeTraits.h for definition of CONCEPT_FIX_UP (and why, for now, we need it).
 */
template <typename T> concept CONCEPT_FIX_UP IsInventory = std::is_base_of_v<Inventory, T>;

/**
 * \return A suitable \c Inventory subclass object for the supplied \c Ingredient subclass object.  If the former does
 *         not exist, it will be created.
 */
template<IsInventory Inv, IsIngredient Ing>
Inv * getInventory(Ing const & ing) {
   auto ingredientId = ing.key();

   //
   // At the moment, we assume there is at most on Inventory object per ingredient object.  In time we would like to
   // extend this to manage, eg, different purchases/batches as separate Inventory items, but that's for another day.
   //
   auto result = ObjectStoreWrapper::findFirstMatching<Inv>(
      [ingredientId](std::shared_ptr<Inv> inventory) {
         return inventory->ingredientId() == ingredientId;
      }
   );
   if (result) {
      return result->get();
   }

   std::shared_ptr<Inv> newInventory = std::make_shared<Inv>();
   newInventory->setIngredientId(ingredientId);
   // Even though the Inventory base class does not have a setQuantity member function, we know that all its
   // subclasses will, so this line will be fine when this template function is instantiated.
   newInventory->setQuantity(0.0);
   // After this next call, the object store will have a copy of the shared pointer, so it is OK that it subsequently
   // goes out of scope here.
   ObjectStoreWrapper::insert<Inv>(newInventory);
   return newInventory.get();
}


//////////////////////////////////////////////// OLD OLD OLD OLD OLD OLD ////////////////////////////////////////////////
class OldInventory : public QObject {
   Q_OBJECT

public:
   /**
    * \brief See comment in model/NamedEntity.h
    */
   static QString const LocalisedName;

   /**
    * \brief Mapping of names to types for the Qt properties of this class.  See \c NamedEntity::typeLookup for more
    *        info.
    */
   static TypeLookup const typeLookup;

   OldInventory();
   OldInventory(NamedParameterBundle const & namedParameterBundle);
   OldInventory(OldInventory const & other);

   ~OldInventory();

   Q_PROPERTY(int    id     READ getId     WRITE setId    )
   Q_PROPERTY(double amount READ getAmount WRITE setAmount)

   /**
    * \brief Returns the ID of the Inventory object, which is unique for a given subclass of Inventory (eg InventoryHop)
    */
   int getId() const;

   /**
    * \brief Returns the amount of the ingredient in the inventory.  Note that the interpretation of this amount (eg,
    *        whether it's kilograms, liters, etc) is the responsibility of the ingredient class.
    */
   double getAmount() const;

   void setId(int id);
   void setAmount(double amount);

   /**
    * \brief Synonym for \c setId(), as it's needed for \c ObjectStoreTyped::hardDelete()
    */
   void setKey(int id);

   /**
    * \brief This doesn't actually do anything, but using ObjectStoreTyped means we have to provide an implementation,
    *        as it's needed for \c ObjectStoreTyped::softDelete().
    */
   void setDeleted(bool var);

   /**
    * \brief This doesn't actually do anything, but using ObjectStoreTyped means we have to provide an implementation,
    *        as it's needed for \c ObjectStoreTyped::softDelete().
    */
   void setDisplay(bool var);

   /**
    * \brief Returns the name of the ingredient class (eg Hop, Fermentable, Misc, Yeast) to which this Inventory class
    *        relates.  Subclasses need to provide the (trivial) implementation of this.  Primarily useful for logging
    *        and debugging.
    */
   virtual char const * getIngredientClass() const = 0;

   /**
    * \brief We need this for ObjectStoreTyped to call
    */
   void hardDeleteOwnedEntities();

protected:
   /**
    * \brief Subclasses need to override this function to return the appropriate instance of \c ObjectStoreTyped.
    */
   virtual ObjectStore & getObjectStoreTypedInstance() const = 0;

private:
   // Private implementation details - see https://herbsutter.com/gotw/_100/
   class impl;
   std::unique_ptr<impl> pimpl;
};
// Thankfully C++11 allows us to inherit constructors using "using"
///class InventoryFermentable : public OldInventory { using OldInventory::OldInventory; public: virtual char const * getIngredientClass() const; protected: virtual ObjectStore & getObjectStoreTypedInstance() const; };
class InventoryMisc        : public OldInventory { using OldInventory::OldInventory; public: virtual char const * getIngredientClass() const; protected: virtual ObjectStore & getObjectStoreTypedInstance() const; };
class InventoryYeast       : public OldInventory { using OldInventory::OldInventory; public: virtual char const * getIngredientClass() const; protected: virtual ObjectStore & getObjectStoreTypedInstance() const; };

namespace InventoryUtils {
   /**
    * \brief Helper function to set inventory amount for a given object
    */
   template<class Ing>
   void setAmount(Ing & ing, double amount);

   template<class Ing>
   double getAmount(Ing const & ing);
}

#endif
