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

#include "utils/EnumStringMapping.h"

//
// Settings we only use in this file.  Strictly we could put these as literals in Measurement::getSettingsName, but
// doing it this way is consistent with how we define other persistent setting name constants.
//
#define AddSettingName(name) namespace { BtStringConst const name{#name}; }
AddSettingName(unitSystem_acidity             )
AddSettingName(unitSystem_bitterness          )
AddSettingName(unitSystem_carbonation         )
AddSettingName(unitSystem_color               )
AddSettingName(unitSystem_count               )
AddSettingName(unitSystem_density             )
AddSettingName(unitSystem_diastaticPower      )
AddSettingName(unitSystem_massConcentration   )
AddSettingName(unitSystem_specificHeatCapacity)
AddSettingName(unitSystem_specificVolume      )
AddSettingName(unitSystem_temperature         )
AddSettingName(unitSystem_time                )
AddSettingName(unitSystem_viscosity           )
AddSettingName(unitSystem_volume              )
AddSettingName(unitSystem_volumeConcentration )
AddSettingName(unitSystem_weight              )
#undef AddSettingName

namespace {
   std::vector<Measurement::PhysicalQuantity> const allOf_Mass_Volume        {Measurement::PhysicalQuantity::Mass             , Measurement::PhysicalQuantity::Volume             };
   std::vector<Measurement::PhysicalQuantity> const allOf_Mass_Volume_Count  {Measurement::PhysicalQuantity::Mass             , Measurement::PhysicalQuantity::Volume             , Measurement::PhysicalQuantity::Count};
   std::vector<Measurement::PhysicalQuantity> const allOf_MassConc_VolumeConc{Measurement::PhysicalQuantity::MassConcentration, Measurement::PhysicalQuantity::VolumeConcentration};
}

EnumStringMapping const Measurement::physicalQuantityStringMapping {
   {Measurement::PhysicalQuantity::Mass                , "Mass"                },
   {Measurement::PhysicalQuantity::Volume              , "Volume"              },
   {Measurement::PhysicalQuantity::Count               , "Count"               },
   {Measurement::PhysicalQuantity::Temperature         , "Temperature"         },
   {Measurement::PhysicalQuantity::Time                , "Time"                },
   {Measurement::PhysicalQuantity::Color               , "Color"               },
   {Measurement::PhysicalQuantity::Density             , "Density"             },
   {Measurement::PhysicalQuantity::DiastaticPower      , "DiastaticPower"      },
   {Measurement::PhysicalQuantity::Acidity             , "Acidity"             },
   {Measurement::PhysicalQuantity::Bitterness          , "Bitterness"          },
   {Measurement::PhysicalQuantity::Carbonation         , "Carbonation"         },
   {Measurement::PhysicalQuantity::MassConcentration   , "MassConcentration"   },
   {Measurement::PhysicalQuantity::VolumeConcentration , "VolumeConcentration" },
   {Measurement::PhysicalQuantity::Viscosity           , "Viscosity"           },
   {Measurement::PhysicalQuantity::SpecificHeatCapacity, "SpecificHeatCapacity"},
   {Measurement::PhysicalQuantity::SpecificVolume      , "SpecificVolume"      },
};

EnumStringMapping const Measurement::physicalQuantityDisplayNames {
   {Measurement::PhysicalQuantity::Mass                , QObject::tr("Mass"                  )},
   {Measurement::PhysicalQuantity::Volume              , QObject::tr("Volume"                )},
   {Measurement::PhysicalQuantity::Count               , QObject::tr("Count"                 )},
   {Measurement::PhysicalQuantity::Temperature         , QObject::tr("Temperature"           )},
   {Measurement::PhysicalQuantity::Time                , QObject::tr("Time"                  )},
   {Measurement::PhysicalQuantity::Color               , QObject::tr("Color"                 )},
   {Measurement::PhysicalQuantity::Density             , QObject::tr("Density"               )},
   {Measurement::PhysicalQuantity::DiastaticPower      , QObject::tr("Diastatic Power"       )},
   {Measurement::PhysicalQuantity::Acidity             , QObject::tr("Acidity"               )},
   {Measurement::PhysicalQuantity::Bitterness          , QObject::tr("Bitterness"            )},
   {Measurement::PhysicalQuantity::Carbonation         , QObject::tr("Carbonation"           )},
   {Measurement::PhysicalQuantity::MassConcentration   , QObject::tr("Mass Concentration"    )},
   {Measurement::PhysicalQuantity::VolumeConcentration , QObject::tr("Volume Concentration"  )},
   {Measurement::PhysicalQuantity::Viscosity           , QObject::tr("Viscosity"             )},
   {Measurement::PhysicalQuantity::SpecificHeatCapacity, QObject::tr("Specific Heat Capacity")},
   {Measurement::PhysicalQuantity::SpecificVolume      , QObject::tr("Specific Volume"       )},
};


BtStringConst const & Measurement::getSettingsName(PhysicalQuantity const physicalQuantity) {
   // Some physical quantities, such as Time, only have one UnitSystem, so we don't strictly need to store those in
   // PersistentSettings.  However, it's simpler to keep the same logic for everything.
   switch (physicalQuantity) {
      // Yes, strictly, unitSystem_weight should be unitSystem_mass, but users already have this in their settings files
      // so it would be annoying to just change it now.
      case Measurement::PhysicalQuantity::Mass                : return unitSystem_weight              ;
      case Measurement::PhysicalQuantity::Volume              : return unitSystem_volume              ;
      case Measurement::PhysicalQuantity::Time                : return unitSystem_time                ;
      case Measurement::PhysicalQuantity::Count               : return unitSystem_count               ;
      case Measurement::PhysicalQuantity::Temperature         : return unitSystem_temperature         ;
      case Measurement::PhysicalQuantity::Color               : return unitSystem_color               ;
      case Measurement::PhysicalQuantity::Density             : return unitSystem_density             ;
      case Measurement::PhysicalQuantity::DiastaticPower      : return unitSystem_diastaticPower      ;
      case Measurement::PhysicalQuantity::Acidity             : return unitSystem_acidity             ;
      case Measurement::PhysicalQuantity::Bitterness          : return unitSystem_bitterness          ;
      case Measurement::PhysicalQuantity::Carbonation         : return unitSystem_carbonation         ;
      case Measurement::PhysicalQuantity::MassConcentration   : return unitSystem_massConcentration   ;
      case Measurement::PhysicalQuantity::VolumeConcentration : return unitSystem_volumeConcentration ;
      case Measurement::PhysicalQuantity::Viscosity           : return unitSystem_viscosity           ;
      case Measurement::PhysicalQuantity::SpecificHeatCapacity: return unitSystem_specificHeatCapacity;
      case Measurement::PhysicalQuantity::SpecificVolume      : return unitSystem_specificVolume      ;
      // In C++23, we'd add:
      // default: std::unreachable();
   }
   // In C++23, we'd add:
   // std::unreachable()
   // It's a coding error if we get here
   Q_ASSERT(false);
}

EnumStringMapping const Measurement::choiceOfPhysicalQuantityStringMapping {
   {Measurement::ChoiceOfPhysicalQuantity::Mass_Volume        , "Mass_Volume"        },
   {Measurement::ChoiceOfPhysicalQuantity::Mass_Volume_Count  , "Mass_Volume_Count"  },
   {Measurement::ChoiceOfPhysicalQuantity::MassConc_VolumeConc, "MassConc_VolumeConc"},
};

EnumStringMapping const Measurement::choiceOfPhysicalQuantityDisplayNames {
   {Measurement::ChoiceOfPhysicalQuantity::Mass_Volume        , QObject::tr("Mass or Volume"              )},
   {Measurement::ChoiceOfPhysicalQuantity::Mass_Volume_Count  , QObject::tr("Mass, Volume or Count"       )},
   {Measurement::ChoiceOfPhysicalQuantity::MassConc_VolumeConc, QObject::tr("Mass or Volume Concentration")},
};

// Default case is that PhysicalQuantities holds PhysicalQuantity; specialisations are for all ChoiceOfPhysicalQuantity
// possibilities.  Note that, because this is a function template, we are not allowed _partial_ specialisations.
template<Measurement::PhysicalQuantityConstTypes PQT, PQT pqt> Measurement::PhysicalQuantity Measurement::defaultPhysicalQuantity() {
   return pqt;
}
template<> Measurement::PhysicalQuantity Measurement::defaultPhysicalQuantity<Measurement::ChoiceOfPhysicalQuantity const, Measurement::ChoiceOfPhysicalQuantity::Mass_Volume        >() { return Measurement::PhysicalQuantity::Mass             ; }
template<> Measurement::PhysicalQuantity Measurement::defaultPhysicalQuantity<Measurement::ChoiceOfPhysicalQuantity const, Measurement::ChoiceOfPhysicalQuantity::Mass_Volume_Count  >() { return Measurement::PhysicalQuantity::Mass             ; }
template<> Measurement::PhysicalQuantity Measurement::defaultPhysicalQuantity<Measurement::ChoiceOfPhysicalQuantity const, Measurement::ChoiceOfPhysicalQuantity::MassConc_VolumeConc>() { return Measurement::PhysicalQuantity::MassConcentration; }


Measurement::PhysicalQuantity Measurement::defaultPhysicalQuantity(Measurement::ChoiceOfPhysicalQuantity const val) {
   switch (val) {
      case Measurement::ChoiceOfPhysicalQuantity::Mass_Volume        : return Measurement::defaultPhysicalQuantity<Measurement::ChoiceOfPhysicalQuantity const, Measurement::ChoiceOfPhysicalQuantity::Mass_Volume        >();
      case Measurement::ChoiceOfPhysicalQuantity::Mass_Volume_Count  : return Measurement::defaultPhysicalQuantity<Measurement::ChoiceOfPhysicalQuantity const, Measurement::ChoiceOfPhysicalQuantity::Mass_Volume_Count  >();
      case Measurement::ChoiceOfPhysicalQuantity::MassConc_VolumeConc: return Measurement::defaultPhysicalQuantity<Measurement::ChoiceOfPhysicalQuantity const, Measurement::ChoiceOfPhysicalQuantity::MassConc_VolumeConc>();
   }
   // Should be unreachable
   Q_ASSERT(false);
   // Keep the compiler happy
   return PhysicalQuantity::Mass;
}


namespace Measurement {
   // TODO: Should be able to get rid of this ultimately
   std::array<QString const, 2> descAmountIsWeight {
      QObject::tr("Volume"), // amountIsWeight() == false
      QObject::tr("Weight")  // amountIsWeight() == true
   };

}

template<Measurement::PhysicalQuantity const pq> bool isValid(Measurement::PhysicalQuantity const physicalQuantity) {
   return physicalQuantity == pq;
}
template<> bool Measurement::isValid<Measurement::ChoiceOfPhysicalQuantity const,
                                     Measurement::ChoiceOfPhysicalQuantity::Mass_Volume        >(Measurement::PhysicalQuantity const physicalQuantity) {
   return (physicalQuantity == Measurement::PhysicalQuantity::Mass  ||
           physicalQuantity == Measurement::PhysicalQuantity::Volume);
}
template<> bool Measurement::isValid<Measurement::ChoiceOfPhysicalQuantity const,
                                     Measurement::ChoiceOfPhysicalQuantity::Mass_Volume_Count  >(Measurement::PhysicalQuantity const physicalQuantity) {
   return (physicalQuantity == Measurement::PhysicalQuantity::Mass   ||
           physicalQuantity == Measurement::PhysicalQuantity::Volume ||
           physicalQuantity == Measurement::PhysicalQuantity::Count  );
}
template<> bool Measurement::isValid<Measurement::ChoiceOfPhysicalQuantity const,
                                     Measurement::ChoiceOfPhysicalQuantity::MassConc_VolumeConc>(Measurement::PhysicalQuantity const physicalQuantity) {
   return (physicalQuantity == Measurement::PhysicalQuantity::MassConcentration  ||
           physicalQuantity == Measurement::PhysicalQuantity::VolumeConcentration);
}

bool Measurement::isValid(Measurement::ChoiceOfPhysicalQuantity const choiceOfPhysicalQuantity,
                          Measurement::PhysicalQuantity const physicalQuantity) {
   switch (choiceOfPhysicalQuantity) {
      case Measurement::ChoiceOfPhysicalQuantity::Mass_Volume        : return Measurement::isValid<Measurement::ChoiceOfPhysicalQuantity const, Measurement::ChoiceOfPhysicalQuantity::Mass_Volume        >(physicalQuantity);
      case Measurement::ChoiceOfPhysicalQuantity::Mass_Volume_Count  : return Measurement::isValid<Measurement::ChoiceOfPhysicalQuantity const, Measurement::ChoiceOfPhysicalQuantity::Mass_Volume_Count  >(physicalQuantity);
      case Measurement::ChoiceOfPhysicalQuantity::MassConc_VolumeConc: return Measurement::isValid<Measurement::ChoiceOfPhysicalQuantity const, Measurement::ChoiceOfPhysicalQuantity::MassConc_VolumeConc>(physicalQuantity);
   }

   // Should be unreachable
   Q_ASSERT(false);
   return false;
}

std::vector<Measurement::PhysicalQuantity> const & Measurement::allPossibilities(
   Measurement::ChoiceOfPhysicalQuantity const choiceOfPhysicalQuantity
) {
   switch (choiceOfPhysicalQuantity) {
      case Measurement::ChoiceOfPhysicalQuantity::Mass_Volume        : return allOf_Mass_Volume        ;
      case Measurement::ChoiceOfPhysicalQuantity::Mass_Volume_Count  : return allOf_Mass_Volume_Count  ;
      case Measurement::ChoiceOfPhysicalQuantity::MassConc_VolumeConc: return allOf_MassConc_VolumeConc;
   }
   // Should be unreachable
   Q_ASSERT(false);
   // But we have to return something
   return allOf_Mass_Volume;
}


template<class S>
S & operator<<(S & stream, Measurement::PhysicalQuantity const val) {
   stream <<
      "PhysicalQuantity #" << static_cast<int>(val) << ": (" <<
      Measurement::physicalQuantityStringMapping[val] << ")";
   return stream;
}
template QDebug      & operator<<(QDebug      & stream, Measurement::PhysicalQuantity const val);
template QTextStream & operator<<(QTextStream & stream, Measurement::PhysicalQuantity const val);

template<class S>
S & operator<<(S & stream, Measurement::ChoiceOfPhysicalQuantity const val) {
   stream <<
      "PhysicalQuantity #" << static_cast<int>(val) << ": (" <<
      Measurement::choiceOfPhysicalQuantityStringMapping[val] << ")";
   return stream;
}
template QDebug      & operator<<(QDebug      & stream, Measurement::ChoiceOfPhysicalQuantity const val);
template QTextStream & operator<<(QTextStream & stream, Measurement::ChoiceOfPhysicalQuantity const val);
