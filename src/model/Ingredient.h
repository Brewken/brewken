/*======================================================================================================================
 * model/Ingredient.h is part of Brewken, and is copyright the following authors 2023:
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
#ifndef MODEL_INGREDIENT_H
#define MODEL_INGREDIENT_H
#pragma once

#include "model/NamedEntity.h"
#include "utils/EnumStringMapping.h"

class NamedParameterBundle;

/**
 * \brief Subclasses of this class are actual ingredients in a recipe (eg \c Hop, \c Fermentable).
 *
 *        Ingredients are the objects for which we keep inventory.
 *
 * TODO: This should replace \c NamedEntityWithInventory
 */
class Ingredient : public NamedEntity {
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

   Ingredient(QString name = "");
   Ingredient(NamedParameterBundle const & namedParameterBundle);
   Ingredient(Ingredient const & other);

   virtual ~Ingredient();

};

#endif
