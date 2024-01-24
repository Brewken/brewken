/*======================================================================================================================
 * model/Ingredient.cpp is part of Brewken, and is copyright the following authors 2023-2024:
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
#include "model/Ingredient.h"

#include "model/NamedParameterBundle.h"

QString const Ingredient::LocalisedName = tr("Ingredient");

TypeLookup const Ingredient::typeLookup {
   "Ingredient",
   {
      // Empty list - for now at least.  (We can't do PropertyNames::Ingredient::totalInventory here because the
      // BtFieldType value for it depends on the Ingredient subclass.  Hence, it is instead done in IngredientBase.
   },
   // Parent class lookup
   {&NamedEntityWithFolder::typeLookup}
};

Ingredient::Ingredient(QString name) :
   NamedEntityWithFolder{name, true} {
   return;
}

Ingredient::Ingredient(NamedParameterBundle const & namedParameterBundle) :
   NamedEntityWithFolder{namedParameterBundle} {
   return;
}

Ingredient::Ingredient(Ingredient const & other) :
   NamedEntityWithFolder{other} {
   return;
}

Ingredient::~Ingredient() = default;
