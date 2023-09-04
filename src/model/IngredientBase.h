/*======================================================================================================================
 * model/IngredientBase.h is part of Brewken, and is copyright the following authors 2023:
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
#ifndef MODEL_INGREDIENTBASE_H
#define MODEL_INGREDIENTBASE_H
#pragma once

#include "model/Inventory.h"
#include "utils/CuriouslyRecurringTemplateBase.h"

/**
 * \brief
 */
template<class Derived> class IngredientBasePhantom;
template<class Derived>
class IngredientBase : public CuriouslyRecurringTemplateBase<IngredientBasePhantom, Derived> {

protected:
   // Note that, because this is static, it cannot be initialised inside the class definition
   static TypeLookup const typeLookup;

   /**
    * \brief
    */
   Measurement::Amount getTotalInventory() const {
      return getInventory<typename Derived::InventoryClass>(this->derived())->amount();
   }

   /**
    * \brief
    */
   void doSetTotalInventory(Measurement::Amount const val) {
      getInventory<typename Derived::InventoryClass>(this->derived())->setAmount(val);
      return;
   }

};

template<class Derived>
TypeLookup const IngredientBase<Derived>::typeLookup {
   "IngredientBase",
   {
      //
      // See comment in model/IngredientAmount.h for why we can't use the PROPERTY_TYPE_LOOKUP_ENTRY or
      // PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV macros here.
      //
      // See comment in model/Ingredient.cpp for why we can't do PropertyNames::Ingredient::totalInventory there.
      //
      {&PropertyNames::Ingredient::totalInventory,
       TypeInfo::construct<MemberFunctionReturnType_t<&IngredientBase::getTotalInventory>>(
          PropertyNames::Ingredient::totalInventory,
          TypeLookupOf<MemberFunctionReturnType_t<&IngredientBase::getTotalInventory>>::value,
          Derived::validMeasures
       )}
   },
   // Parent class lookup: none as we are at the top of this arm of the inheritance tree
   {}
};


/**
 * \brief Derived classes should include this in their header file, right after Q_OBJECT
 *
 *        Note we have to be careful about comment formats in macro definitions
 */
#define INGREDIENT_BASE_DECL(Derived) \
   /* This allows IngredientBase to call protected and private members of Derived. */ \
   friend class IngredientBase<Derived>;                                              \
                                                                                            \
   public:                                                                                  \
   /*=========================== IB "GETTER" MEMBER FUNCTIONS ===========================*/ \
   virtual Measurement::Amount totalInventory  () const;                                    \
   /*=========================== IB "SETTER" MEMBER FUNCTIONS ===========================*/ \
   virtual void setTotalInventory(Measurement::Amount const & val);                         \

/**
 * \brief Derived classes should include this in their .cpp file
 *
 *        Note we have to be careful about comment formats in macro definitions.
 */
#define INGREDIENT_BASE_COMMON_CODE(Derived) \
   /*====================================== IB "GETTER" MEMBER FUNCTIONS ======================================*/ \
   Measurement::Amount Derived::totalInventory() const { return this->getTotalInventory(); }                      \
   /*====================================== IB "SETTER" MEMBER FUNCTIONS ======================================*/ \
   void Derived::setTotalInventory  (Measurement::Amount const & val) { this->doSetTotalInventory(val); return; } \

#endif
