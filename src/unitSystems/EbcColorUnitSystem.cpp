/**
 * EbcColorUnitSystem.cpp is part of Brewken, and is copyright the following authors 2015:
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
#include "unitSystems/EbcColorUnitSystem.h"
#include <QStringList>
#include "unit.h"

EbcColorUnitSystem::EbcColorUnitSystem()
   : UnitSystem()
{
   _type = Unit::Color;
}

QMap<Unit::unitScale, Unit const *> const& EbcColorUnitSystem::scaleToUnit()
{
   static QMap<Unit::unitScale, Unit const *> _scaleToUnit;
   if( _scaleToUnit.empty() )
   {
      _scaleToUnit.insert(Unit::scaleWithout, &Units::ebc);
   }

   return _scaleToUnit;
}

QMap<QString, Unit const *> const& EbcColorUnitSystem::qstringToUnit()
{
   static QMap<QString, Unit const *> _qstringToUnit;
   if( _qstringToUnit.empty() )
   {
      _qstringToUnit.insert("ebc", &Units::ebc);
   }

   return _qstringToUnit;
}

QString EbcColorUnitSystem::unitType() { return "Color"; }
Unit const * EbcColorUnitSystem::unit() { return &Units::ebc; }
*/
