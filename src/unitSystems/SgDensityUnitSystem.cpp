/**
 * SgDensityUnitSystem.cpp is part of Brewken, and is copyright the following authors 2015-2021:
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
 */
/*
#include "unitSystems/SgDensityUnitSystem.h"
#include <QStringList>
#include "unit.h"
#include "model/BrewNote.h"

SgDensityUnitSystem::SgDensityUnitSystem()
   : UnitSystem()
{
   _type = Unit::Density;
}

QMap<Unit::unitScale, Unit const *> const& SgDensityUnitSystem::scaleToUnit()
{
   static QMap<Unit::unitScale, Unit const *> _scaleToUnit;
   if( _scaleToUnit.empty() )
   {
      _scaleToUnit.insert(Unit::scaleWithout,&Units::sp_grav);
   }

   return _scaleToUnit;
}

QMap<QString, Unit const *> const& SgDensityUnitSystem::qstringToUnit()
{
   static QMap<QString, Unit const *> _qstringToUnit;
   if( _qstringToUnit.empty() )
   {
      _qstringToUnit.insert(PropertyNames::BrewNote::sg,&Units::sp_grav);
   }

   return _qstringToUnit;
}

QString SgDensityUnitSystem::unitType() { return "Density"; }
Unit const * SgDensityUnitSystem::unit() { return &Units::sp_grav; }
*/
