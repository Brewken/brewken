/*======================================================================================================================
 * WaterButton.cpp is part of Brewken, and is copyright the following authors 2020-2024:
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
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
#include "WaterButton.h"

#include <QWidget>

#include "model/Recipe.h"
#include "model/RecipeUseOfWater.h"
#include "model/Water.h"

WaterButton::WaterButton(QWidget * parent) : QPushButton(parent), m_rec(nullptr), m_water(nullptr) {
   return;
}

WaterButton::~WaterButton() = default;

void WaterButton::setRecipe(Recipe * rec) {
   if (m_rec) {
      disconnect( m_rec, nullptr, this, nullptr );
   }

   m_rec = rec;
   if (m_rec && m_rec->waterUses().size() > 0 ) {
      connect( m_rec, &NamedEntity::changed, this, &WaterButton::recChanged );
      this->setWater(m_rec->waterUses().at(0)->water());
   } else {
      setWater(nullptr);
   }
   return;
}

void WaterButton::setWater(Water * water) {
   if ( m_water ) {
      disconnect( m_water, nullptr, this, nullptr );
   }

   m_water = water;
   if ( m_water ) {
      connect( m_water, &NamedEntity::changed, this, &WaterButton::waterChanged );
      setText( m_water->name() );
   } else {
      setText("");
   }
   return;
}

void WaterButton::waterChanged(QMetaProperty prop, QVariant val) {
   if (prop.name() == PropertyNames::NamedEntity::name) {
      this->setText(val.toString());
   }
   return;
}

void WaterButton::recChanged(QMetaProperty prop, QVariant val) {
   if (prop.name() == PropertyNames::Recipe::waterUses) {
      if (m_rec && m_rec->waterUses().size() > 0) {
         this->setWater(m_rec->waterUses().at(0)->water());
      }
   }
   return;
}
