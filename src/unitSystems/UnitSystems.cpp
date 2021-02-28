/**
 * UnitSystems.cpp is part of Brewken, and is copyright the following authors 2009-2014:
 *   • Mark de Wever <koraq@xs4all.nl>
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

#include "unitSystems/UnitSystems.h"
#include "unitSystems/USWeightUnitSystem.h"
#include "unitSystems/SIWeightUnitSystem.h"
#include "unitSystems/ImperialVolumeUnitSystem.h"
#include "unitSystems/USVolumeUnitSystem.h"
#include "unitSystems/SIVolumeUnitSystem.h"
#include "unitSystems/CelsiusTempUnitSystem.h"
#include "unitSystems/FahrenheitTempUnitSystem.h"
#include "unitSystems/TimeUnitSystem.h"
#include "unitSystems/EbcColorUnitSystem.h"
#include "unitSystems/SrmColorUnitSystem.h"
#include "unitSystems/SgDensityUnitSystem.h"
#include "unitSystems/PlatoDensityUnitSystem.h"
#include "unitSystems/DiastaticPowerUnitSystem.h"
#include "unit.h"
#include <QDebug>

USWeightUnitSystem* UnitSystems::usWeightUnitSystem()
{
   static USWeightUnitSystem* us = new USWeightUnitSystem();
   return us;
}

SIWeightUnitSystem* UnitSystems::siWeightUnitSystem()
{
   static SIWeightUnitSystem* si = new SIWeightUnitSystem();
   return si;
}

ImperialVolumeUnitSystem* UnitSystems::imperialVolumeUnitSystem()
{
   static ImperialVolumeUnitSystem* imp = new ImperialVolumeUnitSystem();
   return imp;
}

USVolumeUnitSystem* UnitSystems::usVolumeUnitSystem()
{
   static USVolumeUnitSystem* us = new USVolumeUnitSystem();
   return us;
}

SIVolumeUnitSystem* UnitSystems::siVolumeUnitSystem()
{
   static SIVolumeUnitSystem* si = new SIVolumeUnitSystem();
   return si;
}

CelsiusTempUnitSystem* UnitSystems::celsiusTempUnitSystem()
{
   static CelsiusTempUnitSystem* c = new CelsiusTempUnitSystem();
   return c;
}

FahrenheitTempUnitSystem* UnitSystems::fahrenheitTempUnitSystem()
{
   static FahrenheitTempUnitSystem* f = new FahrenheitTempUnitSystem();
   return f;
}

TimeUnitSystem* UnitSystems::timeUnitSystem()
{
   static TimeUnitSystem* t = new TimeUnitSystem();
   return t;
}

EbcColorUnitSystem* UnitSystems::ebcColorUnitSystem()
{
   static EbcColorUnitSystem* e = new EbcColorUnitSystem();
   return e;
}

SrmColorUnitSystem* UnitSystems::srmColorUnitSystem()
{
   static SrmColorUnitSystem* s = new SrmColorUnitSystem();
   return s;
}

SgDensityUnitSystem* UnitSystems::sgDensityUnitSystem()
{
   static SgDensityUnitSystem* sg = new SgDensityUnitSystem();
   return sg;
}

PlatoDensityUnitSystem* UnitSystems::platoDensityUnitSystem()
{
   static PlatoDensityUnitSystem* p = new PlatoDensityUnitSystem();
   return p;
}

LintnerDiastaticPowerUnitSystem* UnitSystems::lintnerDiastaticPowerUnitSystem()
{
   static LintnerDiastaticPowerUnitSystem* result = new LintnerDiastaticPowerUnitSystem();
   return result;
}

WkDiastaticPowerUnitSystem* UnitSystems::wkDiastaticPowerUnitSystem()
{
   static WkDiastaticPowerUnitSystem* result = new WkDiastaticPowerUnitSystem();
   return result;
}
