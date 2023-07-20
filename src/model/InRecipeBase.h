/*======================================================================================================================
 * model/InRecipeBase.h is part of Brewken, and is copyright the following authors 2023:
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
#ifndef MODEL_INRECIPEBASE_H
#define MODEL_INRECIPEBASE_H
#pragma once

#include "utils/CuriouslyRecurringTemplateBase.h"

/**
 * \brief Small template base class to provide common functionaility for recipe addition classes: \c RecipeAdditionHop,
 *        \c RecipeAdditionFermentable, \c RecipeAdditionMisc, \c RecipeAdditionYeast.
 *
 *        This follows the corresponding BeerJSON \c HopAdditionType, \c FermentableAdditionType, etc types.  (However,
 *        note that we do \b not have a class corresponding with BeerJSON's \c WaterAdditionType as it's simpler just to
 *        include the two component fields directly in \c Recipe.)
 *
 *        As elsewhere we use the Curiously Recurring Template Pattern (CRTP) to allow us to template as much as
 *        possible given the constraints on templating in the Qt meta-object compiler (moc).
 *
 * TBD: Do we still need this class?!?
 *
 * \param Derived = the derived class, eg \c RecipeAdditionHop
 * \param Ingredient = the ingredient class, eg \c Hop
 */
template<class Derived, class Ingredient>
class InRecipeBase : public CuriouslyRecurringTemplateBase<Derived> {
public:

protected:

   InRecipeBase() {
      return;
   }

};

/**
 * \brief Derived classes should include this in their header file, right after Q_OBJECT
 *
 *        We use NeName here rather than Ingredient or IngredientName for consistency with all our other CRTP macros.
 *
 *        Note we have to be careful about comment formats in macro definitions
 */
#define IN_RECIPE_COMMON_DECL(NeName) \
   /* This allows InRecipeBase to call protected and private members of Derived */  \
   friend class InRecipeBase<NeName##InRecipe,                                      \
                             NeName>;                                               \
                                                                                    \
   public:                                                                          \


#endif
