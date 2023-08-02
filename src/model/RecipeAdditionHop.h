/*======================================================================================================================
 * model/RecipeAdditionHop.h is part of Brewken, and is copyright the following authors 2023:
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
#ifndef MODEL_RECIPEADDITIONHOP_H
#define MODEL_RECIPEADDITIONHOP_H
#pragma once

#include <memory>

#include "model/Hop.h"
#include "model/RecipeAdditionBase.h"
#include "model/RecipeAdditionMassOrVolume.h"
#include "model/Recipe.h"

//======================================================================================================================
//========================================== Start of property name constants ==========================================
// See comment in model/NamedEntity.h
#define AddPropertyName(property) namespace PropertyNames::RecipeAdditionHop { BtStringConst const property{#property}; }
AddPropertyName(hop)
#undef AddPropertyName
//=========================================== End of property name constants ===========================================
//======================================================================================================================

class RecipeAdditionHop : public RecipeAdditionMassOrVolume, public RecipeAdditionBase<RecipeAdditionHop, Hop> {
   Q_OBJECT

   RECIPE_ADDITION_DECL(Hop)

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

   RecipeAdditionHop(QString name = "", int const recipeId = -1, int const hopId = -1);
   RecipeAdditionHop(NamedParameterBundle const & namedParameterBundle);
   RecipeAdditionHop(RecipeAdditionHop const & other);

   virtual ~RecipeAdditionHop();

   //=================================================== PROPERTIES ====================================================
   Q_PROPERTY(Hop * hop READ hop WRITE setHop)

   //============================================ "GETTER" MEMBER FUNCTIONS ============================================
   Hop * hop () const;

   //============================================ "SETTER" MEMBER FUNCTIONS ============================================
   void setHop(Hop * const val);

   /**
    * \brief With BeerJSON changes, there is no longer an explicit flag for a first wort hop addition.  You have to
    *        jump through a couple of hoops to work it out, which is what this function does for you.
    */
   bool isFirstWort() const;

   /**
    * \brief Similarly, what used to be Hop::Use::Aroma (ie hops added at the end of the boil) is now something we need
    *        to work out.
    */
   bool isAroma() const;

   virtual Recipe * getOwningRecipe() const;


protected:
   // Note that we don't override isEqualTo, as we don't have any non-inherited member variables
   virtual ObjectStore & getObjectStoreTypedInstance() const;

};

#endif
