/*======================================================================================================================
 * model/InventorySalt.h is part of Brewken, and is copyright the following authors 2024:
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
#ifndef MODEL_INVENTORYSALT_H
#define MODEL_INVENTORYSALT_H
#pragma once

#include <QObject>
#include <QString>

#include "model/Salt.h"
#include "model/Inventory.h"
#include "model/IngredientAmount.h"

/**
 * \brief Inventory of \c Salt
 */
class InventorySalt : public Inventory, public IngredientAmount<InventorySalt, Salt> {
   Q_OBJECT

   INGREDIENT_AMOUNT_DECL(InventorySalt, Salt)
   INVENTORY_DECL(Salt, salt)
};

#endif
