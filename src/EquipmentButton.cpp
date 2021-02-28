/**
 * EquipmentButton.cpp is part of Brewken, and is copyright the following authors 2009-2014:
 *   • Brian Rower <brian.rower@gmail.com>
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
 */

#include "EquipmentButton.h"
#include "equipment.h"
#include "recipe.h"
#include <QWidget>

EquipmentButton::EquipmentButton(QWidget* parent)
   : QPushButton(parent),
     _rec(nullptr),
     _equip(nullptr)
{
}

void EquipmentButton::setRecipe(Recipe* rec)
{
   if(_rec)
      disconnect( _rec, nullptr, this, nullptr );

   _rec = rec;
   if( _rec )
   {
      connect( _rec, &NamedEntity::changed, this, &EquipmentButton::recChanged );
      setEquipment( _rec->equipment() );
   }
   else
      setEquipment(nullptr);
}

void EquipmentButton::setEquipment(Equipment* equip)
{
   if( _equip )
      disconnect( _equip, nullptr, this, nullptr );

   _equip = equip;
   if( _equip )
   {
      connect( _equip, &NamedEntity::changed, this, &EquipmentButton::equipChanged );
      setText( _equip->name() );
   }
   else
      setText("");
}

void EquipmentButton::equipChanged(QMetaProperty prop, QVariant val)
{
   QString propName(prop.name());
   if( propName == PropertyNames::NamedEntity::name )
      setText( val.toString() );
}

void EquipmentButton::recChanged(QMetaProperty prop, QVariant val)
{
   QString propName(prop.name());

   if( propName == "equipment" )
      setEquipment( qobject_cast<Equipment*>(NamedEntity::extractPtr(val)) );
}
