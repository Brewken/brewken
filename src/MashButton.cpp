/**
 * MashButton.cpp is part of Brewken, and is copyright the following authors 2009-2014:
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

#include "MashButton.h"
#include "model/Mash.h"
#include "model/Recipe.h"
#include <QWidget>
#include <QDebug>

MashButton::MashButton(QWidget* parent)
   : QPushButton(parent),
     m_rec(0),
     _mash(0)
{
}

void MashButton::setRecipe(Recipe* rec)
{

   if(m_rec)
      disconnect( m_rec, 0, this, 0 );

   m_rec = rec;
   if( m_rec )
   {
      connect( m_rec, &NamedEntity::changed, this, &MashButton::recChanged );
      setMash( m_rec->mash() );
   }
   else
      setMash(0);
}

void MashButton::setMash(Mash* mash)
{
   if( _mash )
      disconnect( _mash, 0, this, 0 );

   _mash = mash;
   if( _mash )
   {
      connect( _mash, &NamedEntity::changed, this, &MashButton::mashChanged );
      setText( _mash->name() );
   }
   else
      setText("");
}

// This is a bit different from the other buttons. I think we need this
// because the mash tab is the only tab where you can delete stuff directly.
Mash* MashButton::mash() { return _mash; }

void MashButton::mashChanged(QMetaProperty prop, QVariant val) {
   if (prop.name() == PropertyNames::NamedEntity::name) {
      this->setText(val.toString());
   }
   return;
}

void MashButton::recChanged(QMetaProperty prop, QVariant val) {
   if (prop.name() == PropertyNames::Recipe::mash) {
      this->setMash(val.value<Mash*>());
   }
   return;
}
