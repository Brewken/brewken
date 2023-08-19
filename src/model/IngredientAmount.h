/*======================================================================================================================
 * model/IngredientAmount.h is part of Brewken, and is copyright the following authors 2023:
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
#ifndef MODEL_INGREDIENTAMOUNT_H
#define MODEL_INGREDIENTAMOUNT_H
#pragma once

#include <QFlags>

#include "model/Ingredient.h"
#include "model/NamedParameterBundle.h"
#include "utils/CuriouslyRecurringTemplateBase.h"
#include "utils/EnumStringMapping.h"
#include "utils/TypeLookup.h"

//======================================================================================================================
//========================================== Start of property name constants ==========================================
// See comment in model/NamedEntity.h
#define AddPropertyName(property) namespace PropertyNames::IngredientAmount { BtStringConst const property{#property}; }
AddPropertyName(quantity)
AddPropertyName(measure )
#undef AddPropertyName
//=========================================== End of property name constants ===========================================
//======================================================================================================================


/**
 * \brief Represents an amount of an ingredient.  These amounts are used in two places: in the \c RecipeAddition
 *        subclasses for the amount of an ingredient being added to a \c Recipe; and in the \c Inventory subclasses for
 *        the amount of an ingredient held in stock.
 *
 *        In our model, different types of ingredients are allowed to be measured in different ways:
 *
 *           \c Salt can be measured only by mass
 *           \c Water can be measured only by volume
 *           \c Fermentable and \c Hop can be measured either by mass or by volume
 *           \c Misc and \c Yeast can be measured by mass, by volume or by count
 *
 *        Typically, for things that can be measured more than one way, it is the individual instance of a class that
 *        determines the measurement.  Eg, a \c Hop would be measured by mass if it is leaves, pellets or powder, but by
 *        volume if it is an extract.  For other things, there isn't a rule, and it's a case-by-case decision for the
 *        brewer.   Eg, for dry \c Yeast it's the brewer's choice to measure by packets or mass.  For \c Misc, it might
 *        even vary by recipe as to whether you're adding, say, half an apple or 500 grams of apple.
 *
 *        So, for, ingredient types where we allow choice of how-to-measure, we pick a sensible default and let the user
 *        change it where needed.
 *
 *        Classes inheriting from this one, should place the \c INGREDIENT_AMOUNT_DECL macro in their header class
 *        declaration (eg immediately (typically on the line after after the \c Q_OBJECT macro), and the
 *        \c INGREDIENT_AMOUNT_COMMON_CODE macro in their .cpp file.  They also need the following lines in their class
 *        declaration:
 *
 *              //! \brief The amount in kilograms, liters or count (ie "number of ...") -- see \c IngredientAmount
 *              Q_PROPERTY(double quantity                READ quantity    WRITE setQuantity)
 *              //! \brief Whether we are measuring in kilograms, liters or count (ie "number of ...") -- see \c IngredientAmount
 *              Q_PROPERTY(Ingredient::Measure measure    READ measure     WRITE setMeasure )
 *
 *
 *        (We cannot insert these lines with the \c INGREDIENT_AMOUNT_DECL macro because the Qt Meta Object Compiler aka
 *        MOC does not expand macros.)
 *
 *        ADDITIONALLY, it is necessary in the class declaration of the corresponding ingredient (eg in Hop.h for hops)
 *        to add publicly accessible:
 *           static constexpr Ingredient::Measures validMeasures = ...
 *           static constexpr Ingredient::Measure defaultMeasure = ...
 *
 *        ---
 *
 *        Because this class is essentially just adding a couple of fields to its "owner" (eg \c RecipeAdditionHop,
 *        \c InventoryHop), it doesn't merit being a full-fledged \c NamedEntity with its own separate database table.
 *        Nonetheless, we do want the fields of this class to be stored in the database(!) but just as extra columns on
 *        the tables used by the "owner" classes.  And we want to be able to take advantage of utility functions such as
 *        \c NamedEntity::setAndNotify().  Using the Curiously Recurring Template (CRTP) pattern allows us to piggy-back
 *        the fields of this class onto the "owner" class at the cost of some slight ugliness/complexity in the code,
 *        which we mostly hide from the "owner" class with macros.
 *
 *
 * TBD: With a bit more refactoring, we could perhaps align this class more closely with \c Measurement::Amount
 */
template<class Derived> class IngredientAmountPhantom;
template<class Derived, class IngredientClass>
class IngredientAmount : public CuriouslyRecurringTemplateBase<IngredientAmountPhantom, Derived> {

public:

   // Note that, because this is static, it cannot be initialised inside the class definition
   static TypeLookup const typeLookup;

   Measurement::Amount amount() const {
      switch (this->m_measure) {
         case Ingredient::Measure::Mass_Kilograms:
            return Measurement::Amount{this->m_quantity, Measurement::Units::kilograms};

         case Ingredient::Measure::Volume_Liters:
            return Measurement::Amount{this->m_quantity, Measurement::Units::liters};

         case Ingredient::Measure::Count:
            qCritical() <<
               Q_FUNC_INFO << "Cannot return amount of count for" << this->derived().metaObject()->className() <<
               "#" << this->derived().key();
            Q_ASSERT(false);
            break;
      }
      // All possibilities should be handled above, so this should be unreachable
      Q_ASSERT(false);
      return Measurement::Amount{};
   }

   Measurement::PhysicalQuantity physicalQuantity() const {
      switch (this->m_measure) {
         case Ingredient::Measure::Mass_Kilograms:
            return Measurement::PhysicalQuantity::Mass;

         case Ingredient::Measure::Volume_Liters:
            return Measurement::PhysicalQuantity::Volume;

         case Ingredient::Measure::Count:
            qCritical() <<
               Q_FUNC_INFO << "Cannot return physical quantity for count for" <<
               this->derived().metaObject()->className() << "#" << this->derived().key();
            Q_ASSERT(false);
            break;
      }
      // All possibilities should be handled above, so this should be unreachable
      Q_ASSERT(false);
      return Measurement::PhysicalQuantity::Mass;
   }

protected:

   /**
    * NB: Since this is the constructor that will be called in the absence of any other being specified, it is
    * not necessary for subclass constructors to explicitly invoke this (because the compiler will automatically
    * ensure it is called before the subclass constructor).  This saves us having to re-specify
    * \c validMeasures in subclass constructors.
    */
   IngredientAmount() :
      m_quantity{0.0},
      m_measure{IngredientClass::defaultMeasure} {
      return;
   }

   IngredientAmount(NamedParameterBundle const & namedParameterBundle) :
      SET_REGULAR_FROM_NPB (m_quantity, namedParameterBundle, PropertyNames::IngredientAmount::quantity),
      SET_REGULAR_FROM_NPB (m_measure , namedParameterBundle, PropertyNames::IngredientAmount::measure ) {
      return;
   }

   ~IngredientAmount() = default;

   void doSetQuantity(double                    const val) {
      this->derived().setAndNotify(PropertyNames::IngredientAmount::quantity, this->m_quantity, val);
      return;
   }

   void doSetMeasure (Ingredient::Measure const val) {
      Q_ASSERT(val & IngredientClass::validMeasures);
      this->derived().setAndNotify(PropertyNames::IngredientAmount::measure, this->m_measure, val);
      return;
   }


   double              m_quantity;
   Ingredient::Measure m_measure ;
};

template<class Derived, class IngredientClass>
TypeLookup const IngredientAmount<Derived, IngredientClass>::typeLookup {
   "IngredientAmount",
   {
      // We can't use the PROPERTY_TYPE_LOOKUP_ENTRY macro here, because it doesn't know how to handle templated
      // references such as `IngredientAmount<Derived, IngredientClass>::m_quantity` (which would get treated as two
      // macro parameters).  So we have to put the raw code instead.
      {&PropertyNames::IngredientAmount::quantity,
       TypeInfo::construct<decltype(IngredientAmount<Derived, IngredientClass>::m_quantity)>(
          PropertyNames::IngredientAmount::quantity,
          TypeLookupOf<decltype(IngredientAmount<Derived, IngredientClass>::m_quantity)>::value
       )},
      {&PropertyNames::IngredientAmount::measure,
       TypeInfo::construct<decltype(IngredientAmount<Derived, IngredientClass>::m_measure)>(
          PropertyNames::IngredientAmount::measure,
          TypeLookupOf<decltype(IngredientAmount<Derived, IngredientClass>::m_measure)>::value
       )},
   },
   // Parent class lookup: none as we are at the top of this arm of the inheritance tree
   {}
};


/**
 * \brief Derived classes should include this in their header file, right after Q_OBJECT
 *
 *        Note we have to be careful about comment formats in macro definitions
 */
#define INGREDIENT_AMOUNT_DECL(Derived, IngredientClass) \
   /* This allows IngredientAmount to call protected and private members of Derived. */ \
   /* Note that a friend statement can either apply to all instances of              */ \
   /* IngredientAmount or to one specialisation.  It cannot apply to a partial       */ \
   /* specialisation.  Hence why we need to specify IngredientClass here.            */ \
   friend class IngredientAmount<Derived, IngredientClass>; \
                                                                                            \
   public:                                                                                  \
   /*=========================== IA "GETTER" MEMBER FUNCTIONS ===========================*/ \
   double              quantity() const;                                                    \
   Ingredient::Measure measure () const;                                                    \
   /*=========================== IA "SETTER" MEMBER FUNCTIONS ===========================*/ \
   void setQuantity(double              const val);                                         \
   void setMeasure (Ingredient::Measure const val);                                         \


/**
 * \brief Derived classes should include this in their .cpp file
 *
 *        Note we have to be careful about comment formats in macro definitions.
 *
 *        We implement the "getter" functions inline in the macros because they are trivial, but do the setters in
 *        the CRTP base above as there's a bit more to them.
 */
#define INGREDIENT_AMOUNT_COMMON_CODE(Derived) \
   /*============================ "GETTER" MEMBER FUNCTIONS ============================*/ \
   double              Derived::quantity() const { return this->m_quantity; }              \
   Ingredient::Measure Derived::measure () const { return this->m_measure ; }              \
   /*============================ "SETTER" MEMBER FUNCTIONS ============================*/ \
   void Derived::setQuantity(double              const val) { this->doSetQuantity(val); return; } \
   void Derived::setMeasure (Ingredient::Measure const val) { this->doSetMeasure (val); return; } \


#endif
