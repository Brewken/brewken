/**
 * unitSystems/USWeightUnitSystem.cpp is part of Brewken, and is copyright the following authors 2009-2015:
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
#include "unitSystems/USWeightUnitSystem.h"

#include <cmath>

#include <QDebug>
#include <QStringList>

#include "unit.h"

USWeightUnitSystem::USWeightUnitSystem()
{
   _type = Unit::Mass;
}

QMap<Unit::unitScale, Unit const *> const& USWeightUnitSystem::scaleToUnit()
{
   static QMap<Unit::unitScale, Unit const *> _scaleToUnit;
   if( _scaleToUnit.empty() )
   {
      _scaleToUnit.insert(Unit::scaleExtraSmall,&Units::ounces);
      _scaleToUnit.insert(Unit::scaleSmall,&Units::pounds);
   }

   return _scaleToUnit;
}

QMap<QString, Unit const *> const& USWeightUnitSystem::qstringToUnit()
{
   static QMap<QString, Unit const *> _qstringToUnit;
   if( _qstringToUnit.empty() )
   {
      _qstringToUnit.insert("oz",&Units::ounces);
      _qstringToUnit.insert("lb",&Units::pounds);
   }

   return _qstringToUnit;
}

Unit const * USWeightUnitSystem::thicknessUnit()
{
   return &Units::pounds;
}

Unit const * USWeightUnitSystem::unit() { return &Units::pounds; }
QString USWeightUnitSystem::unitType() { return "USCustomary"; }
*/
