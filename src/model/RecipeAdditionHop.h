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

#include "model/Hop.h"
#include "model/InRecipeBase.h"
#include "model/RecipeAdditionMassOrVolume.h"
#include "model/Recipe.h"

class RecipeAdditionHop : public RecipeAdditionMassOrVolume, public InRecipeBase<RecipeAdditionHop, Hop> {
   Q_OBJECT
public:
   RecipeAdditionHop(QString name = "");
   RecipeAdditionHop(NamedParameterBundle const & namedParameterBundle);
   RecipeAdditionHop(RecipeAdditionHop const & other);

   virtual ~RecipeAdditionHop();

   virtual Recipe * getOwningRecipe() const;

   // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
// We need to add in the properties from HopVarietyBase, which also exist in Hop.  It would be a bit painful to copy-and-paste here
// Currently thinking that RecipeAddition should _contain_ a base object.  This then requires xpaths in our property names to handle setting.
// The contained object always exists but is returned by pointer.

   // utils/PropertyXPath.h

   // Base class could be HopFundamentals or HopBase or HopVarietyBase


   //                                HopVarietyBase - - - RecipeHopAddition --- RecipeAddition
   //                                  /
   //                                 /
   //                               Hop (= VarietyInformation)


   // ⮜⮜⮜ The following properties are only for BeerXML support ⮞⮞⮞
   // When writing out to BeerXML, they pull information from the underlying hop we're adding.


protected:
   // Note that we don't override isEqualTo, as we don't have any non-inherited member variables
   virtual ObjectStore & getObjectStoreTypedInstance() const;
};

#endif
