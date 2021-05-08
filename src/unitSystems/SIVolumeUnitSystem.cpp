/**
 * SIVolumeUnitSystem.cpp is part of Brewken, and is copyright the following authors 2009-2015:
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
#include "unitSystems/SIVolumeUnitSystem.h"
#include <QStringList>
#include <cmath>

SIVolumeUnitSystem::SIVolumeUnitSystem()
   : UnitSystem()
{
   _type = Unit::Volume;
}

Unit const * SIVolumeUnitSystem::thicknessUnit()
{
   return &Units::liters;
}

QMap<Unit::unitScale, Unit const *> const& SIVolumeUnitSystem::scaleToUnit()
{
   static QMap<Unit::unitScale, Unit const *> _scaleToUnit;
   if( _scaleToUnit.empty() )
   {
      _scaleToUnit.insert(Unit::scaleExtraSmall, &Units::milliliters);
      _scaleToUnit.insert(Unit::scaleSmall, &Units::liters);
   }

   return _scaleToUnit;
}

QMap<QString, Unit const *> const& SIVolumeUnitSystem::qstringToUnit()
{
   static QMap<QString, Unit const *> _qstringToUnit;
   if( _qstringToUnit.empty() )
   {
      _qstringToUnit.insert("mL", &Units::milliliters);
      _qstringToUnit.insert("ml", &Units::milliliters);
      _qstringToUnit.insert("L", &Units::liters);
      _qstringToUnit.insert("l", &Units::liters);
   }

   return _qstringToUnit;
}

Unit const * SIVolumeUnitSystem::unit() { return &Units::liters; }
QString SIVolumeUnitSystem::unitType() { return "SI"; }
*/
