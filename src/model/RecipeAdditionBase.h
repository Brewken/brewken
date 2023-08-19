/*======================================================================================================================
 * model/RecipeAdditionBase.h is part of Brewken, and is copyright the following authors 2023:
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
#ifndef MODEL_RECIPEADDITIONBASE_H
#define MODEL_RECIPEADDITIONBASE_H
#pragma once

#include "database/ObjectStoreWrapper.h"
#include "utils/CuriouslyRecurringTemplateBase.h"

/**
 * \brief Small template base class to provide templated code for recipe addition classes: \c RecipeAdditionHop,
 *        \c RecipeAdditionFermentable, \c RecipeAdditionMisc, \c RecipeAdditionYeast.
 *
 * \param Derived = the derived class, eg \c RecipeAdditionHop
 * \param Ingredient = the ingredient class, eg \c Hop
 */
template<class Derived> class RecipeAdditionPhantom;
template<class Derived, class Ingredient>
class RecipeAdditionBase : public CuriouslyRecurringTemplateBase<RecipeAdditionPhantom, Derived> {

protected:
   RecipeAdditionBase() {
      return;
   }

   ~RecipeAdditionBase() = default;

public:
   /**
    * \brief Create \c RecipeAddition##NeName objects for a given \c Recipe from \c NeName objects
    */
   static QList<std::shared_ptr<Derived>> create(Recipe & recipe, QList<Ingredient *> ingredients) {
      QList<std::shared_ptr<Derived>> listOfAdditions;
      for (auto ingredient : ingredients) {
         auto addition = std::make_shared<Derived>(recipe, *ingredient);
         listOfAdditions.append(addition);
      }
      return listOfAdditions;
   }

   std::shared_ptr<Derived> ingredient() {
      return ObjectStoreWrapper::getById<Derived>(this->derived().m_ingredientId);
   }

};

/**
 * \brief Derived classes should include this in their header file, right after Q_OBJECT
 *
 *        We use NeName here rather than Ingredient or IngredientName for consistency with all our other CRTP macros.
 *
 *        Note we have to be careful about comment formats in macro definitions
 */
#define RECIPE_ADDITION_DECL(NeName) \
   /* This allows RecipeAdditionBase to call protected and private members of Derived */     \
   friend class RecipeAdditionBase<RecipeAddition##NeName, NeName>;                          \
                                                                                             \
   public:                                                                                   \
   RecipeAddition##NeName(Recipe & recipe, NeName & ne) :                                    \
     RecipeAddition##NeName{QString(tr("Add %1").arg(ne.name())), recipe.key(), ne.key()} {  \
     return;                                                                                 \
   }                                                                                         \

#endif
