/*======================================================================================================================
 * measurement/PhysicalQuantity.cpp is part of Brewken, and is copyright the following authors 2021:
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

#include <QDebug>
#include <QString>

#include "utils/EnumStringMapping.h"

namespace {
   EnumStringMapping const physicalQuantityToName {
      {"Mass"          , Measurement::PhysicalQuantity::Mass          },
      {"Volume"        , Measurement::PhysicalQuantity::Volume        },
      {"Time"          , Measurement::PhysicalQuantity::Time          },
      {"Temperature"   , Measurement::PhysicalQuantity::Temperature   },
      {"Color"         , Measurement::PhysicalQuantity::Color         },
      {"Density"       , Measurement::PhysicalQuantity::Density       },
//      {"String"        , Measurement::PhysicalQuantity::String        },
      {"Mixed"         , Measurement::PhysicalQuantity::Mixed         },
      {"DiastaticPower", Measurement::PhysicalQuantity::DiastaticPower},
      {"None"          , Measurement::PhysicalQuantity::None          }
   };
}

QString Measurement::getDisplayName(Measurement::PhysicalQuantity physicalQuantity) {
   auto returnValue = physicalQuantityToName.enumToString(physicalQuantity);
   // It's a coding error if we don't find a result!
   if (!returnValue) {
      qCritical() << Q_FUNC_INFO << "No mapping defined for PhysicalQuantity #" << physicalQuantity;
      Q_ASSERT(false); // Stop here on debug builds
   }
   return *returnValue;
}
