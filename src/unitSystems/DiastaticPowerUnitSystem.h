/**
 * DiastaticPowerUnitSystem.h is part of Brewken, and is copyright the following authors 2016:
 *   • Mark de Wever <koraq@xs4all.nl>
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

#ifndef DIASTATICPOWERUNITSYSTEM_H
#define DIASTATICPOWERUNITSYSTEM_H

#include <QMap>
#include "unitSystems/UnitSystem.h"

class LintnerDiastaticPowerUnitSystem : public UnitSystem
{
public:
   LintnerDiastaticPowerUnitSystem();
   Unit const * thicknessUnit() { return 0; }
   QString unitType();

   QMap<Unit::unitScale, Unit const *> const& scaleToUnit();
   QMap<QString, Unit const *> const& qstringToUnit();
   Unit const * unit();

};

class WkDiastaticPowerUnitSystem : public UnitSystem
{
public:
   WkDiastaticPowerUnitSystem();
   Unit const * thicknessUnit() { return 0; }
   QString unitType();

   QMap<Unit::unitScale, Unit const *> const& scaleToUnit();
   QMap<QString, Unit const *> const& qstringToUnit();
   Unit const * unit();

};

#endif /* DIASTATICPOWERUNITSYSTEM_H */
