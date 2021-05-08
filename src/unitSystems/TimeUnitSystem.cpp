/**
 * unitSystems/TimeUnitSystem.cpp is part of Brewken, and is copyright the following authors 2009-2015:
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
#include "unitSystems/TimeUnitSystem.h"

#include <cmath>
#include <QStringList>

#include "unit.h"

TimeUnitSystem::TimeUnitSystem()
{
   _type = Unit::Time;
}

QMap<Unit::unitScale, Unit const *> const& TimeUnitSystem::scaleToUnit()
{
   static QMap<Unit::unitScale, Unit const *> _scaleToUnit;
   if( _scaleToUnit.empty() )
   {
      _scaleToUnit.insert(Unit::scaleExtraSmall,&Units::seconds);
      _scaleToUnit.insert(Unit::scaleSmall,&Units::minutes);
      _scaleToUnit.insert(Unit::scaleMedium,&Units::hours);
      _scaleToUnit.insert(Unit::scaleLarge,&Units::days);
   }

   return _scaleToUnit;
}

QMap<QString, Unit const *> const& TimeUnitSystem::qstringToUnit()
{
   static QMap<QString, Unit const *> _qstringToUnit;
   if( _qstringToUnit.empty() )
   {
      _qstringToUnit.insert("s", &Units::seconds);
      _qstringToUnit.insert("m", &Units::minutes);
      _qstringToUnit.insert("h", &Units::hours);
      _qstringToUnit.insert("d", &Units::days);
   }

   return _qstringToUnit;
}

Unit const * TimeUnitSystem::unit() { return &Units::minutes; }
QString TimeUnitSystem::unitType() { return "entropy"; }
*/
