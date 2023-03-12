/*======================================================================================================================
 * measurement/PhysicalQuantity.cpp is part of Brewken, and is copyright the following authors 2021-2023:
 *   • Matt Young <mfsy@yahoo.com>
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
#include "measurement/PhysicalQuantity.h"

#include <utility>
#include <QDebug>

QString Measurement::getDisplayName(Measurement::PhysicalQuantity physicalQuantity) {
   //
   // We could use an EnumStringMapping object to hold all the data and then call its enumToString member function.
   // However, the advantage of using a switch statement is that the compiler will warn us if we have missed one of the
   // enum values (because it's a strongly-typed enum).  This is better than waiting until run time for
   // EnumStringMapping::enumToString to log an error and throw an exception).
   //
   switch (physicalQuantity) {
      case Measurement::PhysicalQuantity::Mass                : return QObject::tr("Mass"                  );
      case Measurement::PhysicalQuantity::Volume              : return QObject::tr("Volume"                );
      case Measurement::PhysicalQuantity::Time                : return QObject::tr("Time"                  );
      case Measurement::PhysicalQuantity::Temperature         : return QObject::tr("Temperature"           );
      case Measurement::PhysicalQuantity::Color               : return QObject::tr("Color"                 );
      case Measurement::PhysicalQuantity::Density             : return QObject::tr("Density"               );
      case Measurement::PhysicalQuantity::DiastaticPower      : return QObject::tr("Diastatic Power"       );
      case Measurement::PhysicalQuantity::Acidity             : return QObject::tr("Acidity"               );
      case Measurement::PhysicalQuantity::Bitterness          : return QObject::tr("Bitterness"            );
      case Measurement::PhysicalQuantity::Carbonation         : return QObject::tr("Carbonation"           );
      case Measurement::PhysicalQuantity::MassConcentration   : return QObject::tr("Mass Concentration"    );
      case Measurement::PhysicalQuantity::VolumeConcentration : return QObject::tr("Volume Concentration"  );
      case Measurement::PhysicalQuantity::Viscosity           : return QObject::tr("Viscosity"             );
      case Measurement::PhysicalQuantity::SpecificHeatCapacity: return QObject::tr("Specific Heat Capacity");
      // In C++23, we'd add:
      // default: std::unreachable();
   }
   // In C++23, we'd add:
   // std::unreachable()
   // It's a coding error if we get here
   Q_ASSERT(false);
}

namespace Measurement {
   Mixed2PhysicalQuantities const PqEitherMassOrVolume              {std::make_tuple(PhysicalQuantity::Mass,              PhysicalQuantity::Volume             )};
   Mixed2PhysicalQuantities const PqEitherMassOrVolumeConcentration {std::make_tuple(PhysicalQuantity::MassConcentration, PhysicalQuantity::VolumeConcentration)};
}
