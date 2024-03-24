/*======================================================================================================================
 * buttons/EquipmentButton.h is part of Brewken, and is copyright the following authors 2009-2024:
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
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
#ifndef BUTTONS_EQUIPMENTBUTTON_H
#define BUTTONS_EQUIPMENTBUTTON_H
#pragma once

#include "buttons/RecipeAttributeButton.h"
#include "model/Equipment.h"

/*!
 * \class EquipmentButton
 *
 * \brief This is a view class that displays the name of an equipment.
 */
class EquipmentButton : public RecipeAttributeButton,
                        public RecipeAttributeButtonBase<EquipmentButton, Equipment> {
   Q_OBJECT
   RECIPE_ATTRIBUTE_BUTTON_BASE_DECL(Equipment)
};

#endif
