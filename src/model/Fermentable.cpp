/*======================================================================================================================
 * model/Fermentable.cpp is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Blair Bonnett <blair.bonnett@gmail.com>
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Kregg Kemper <gigatropolis@yahoo.com>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Samuel Östling <MrOstling@gmail.com>
 *   • Théophane Martin <theophane.m@gmail.com>
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
#include "model/Fermentable.h"

#include <QDebug>
#include <QObject>
#include <QVariant>

#include "database/ObjectStoreWrapper.h"
#include "model/Inventory.h"
#include "model/NamedParameterBundle.h"
#include "model/Recipe.h"

std::array<Fermentable::Type, 8> const Fermentable::allTypes {
   Fermentable::Type::Dry_Extract,
   Fermentable::Type::Extract,
   Fermentable::Type::Grain,
   Fermentable::Type::Sugar,
   Fermentable::Type::Fruit,
   Fermentable::Type::Juice,
   Fermentable::Type::Honey,
   Fermentable::Type::Other_Adjunct
};

// Note that Fermentable::typeStringMapping and Fermentable::FormMapping are as defined by BeerJSON, but we also use
// them for the DB and for the UI.  We can't use them for BeerXML as it only supports subsets of these types.
EnumStringMapping const Fermentable::typeStringMapping {
   {"dry extract", Fermentable::Type::Dry_Extract},
   {"extract",     Fermentable::Type::Extract},
   {"grain",       Fermentable::Type::Grain},
   {"sugar",       Fermentable::Type::Sugar},
   {"fruit",       Fermentable::Type::Fruit},
   {"juice",       Fermentable::Type::Juice},
   {"honey",       Fermentable::Type::Honey},
   {"other",       Fermentable::Type::Other_Adjunct}
};

QMap<Fermentable::Type, QString> const Fermentable::typeDisplayNames {
   {Fermentable::Type::Dry_Extract,   tr("Dry Extract"  )},
   {Fermentable::Type::Extract,       tr("Extract"      )},
   {Fermentable::Type::Grain,         tr("Grain"        )},
   {Fermentable::Type::Sugar,         tr("Sugar"        )},
   {Fermentable::Type::Fruit,         tr("Fruit"        )},
   {Fermentable::Type::Juice,         tr("Juice"        )},
   {Fermentable::Type::Honey,         tr("Honey"        )},
   {Fermentable::Type::Other_Adjunct, tr("Other Adjunct")},
};

std::array<Fermentable::GrainGroup, 7> const Fermentable::allGrainGroups {
   Fermentable::GrainGroup::Base,
   Fermentable::GrainGroup::Caramel,
   Fermentable::GrainGroup::Flaked,
   Fermentable::GrainGroup::Roasted,
   Fermentable::GrainGroup::Specialty,
   Fermentable::GrainGroup::Smoked,
   Fermentable::GrainGroup::Adjunct
};

// This is based on the BeerJSON encoding
EnumStringMapping const Fermentable::grainGroupStringMapping {
   {"base",       Fermentable::GrainGroup::Base     },
   {"caramel",    Fermentable::GrainGroup::Caramel  },
   {"flaked",     Fermentable::GrainGroup::Flaked   },
   {"roasted",    Fermentable::GrainGroup::Roasted  },
   {"specialty",  Fermentable::GrainGroup::Specialty},
   {"smoked",     Fermentable::GrainGroup::Smoked   },
   {"adjunct",    Fermentable::GrainGroup::Adjunct  }
};

bool Fermentable::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   Fermentable const & rhs = static_cast<Fermentable const &>(other);
   // Base class will already have ensured names are equal
   return (
      this->m_type                   == rhs.m_type                   &&
      this->m_yield_pct              == rhs.m_yield_pct              &&
      this->m_color_srm              == rhs.m_color_srm              &&
      this->m_origin                 == rhs.m_origin                 &&
      this->m_supplier               == rhs.m_supplier               &&
      this->m_coarseFineDiff_pct     == rhs.m_coarseFineDiff_pct     &&
      this->m_moisture_pct           == rhs.m_moisture_pct           &&
      this->m_diastaticPower_lintner == rhs.m_diastaticPower_lintner &&
      this->m_protein_pct            == rhs.m_protein_pct            &&
      this->m_maxInBatch_pct         == rhs.m_maxInBatch_pct         &&
      this->m_grainGroup             == rhs.m_grainGroup
   );
}

ObjectStore & Fermentable::getObjectStoreTypedInstance() const {
   return ObjectStoreTyped<Fermentable>::getInstance();
}

TypeLookup const Fermentable::typeLookup {
   "Fermentable",
   {
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::type                     , Fermentable::m_type                     ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::amount                   , Fermentable::m_amount                   , Measurement::PqEitherMassOrVolume             ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::amountIsWeight           , Fermentable::m_amountIsWeight           ,           NonPhysicalQuantity::Bool           ), // ⮜⮜⮜ Added for BeerJSON support ⮞⮞⮞
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::yield_pct                , Fermentable::m_yield_pct                ,           NonPhysicalQuantity::Percentage     ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::color_srm                , Fermentable::m_color_srm                , Measurement::PhysicalQuantity::Color          ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::addAfterBoil             , Fermentable::m_addAfterBoil             ,           NonPhysicalQuantity::Bool           ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::origin                   , Fermentable::m_origin                   ,           NonPhysicalQuantity::String         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::supplier                 , Fermentable::m_supplier                 ,           NonPhysicalQuantity::String         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::notes                    , Fermentable::m_notes                    ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::coarseFineDiff_pct       , Fermentable::m_coarseFineDiff_pct       ,           NonPhysicalQuantity::Percentage     ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::moisture_pct             , Fermentable::m_moisture_pct             ,           NonPhysicalQuantity::Percentage     ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::diastaticPower_lintner   , Fermentable::m_diastaticPower_lintner   , Measurement::PhysicalQuantity::DiastaticPower ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::protein_pct              , Fermentable::m_protein_pct              ,           NonPhysicalQuantity::Percentage     ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::maxInBatch_pct           , Fermentable::m_maxInBatch_pct           ,           NonPhysicalQuantity::Percentage     ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::recommendMash            , Fermentable::m_recommendMash            ,           NonPhysicalQuantity::Bool           ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::ibuGalPerLb              , Fermentable::m_ibuGalPerLb              ,           NonPhysicalQuantity::Dimensionless  ), // Not really dimensionless...
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::isMashed                 , Fermentable::m_isMashed                 ,           NonPhysicalQuantity::Bool           ),
      // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::grainGroup               , Fermentable::m_grainGroup               ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::producer                 , Fermentable::m_producer                 ,           NonPhysicalQuantity::String         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::productId                , Fermentable::m_productId                ,           NonPhysicalQuantity::String         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::fineGrindYield_pct       , Fermentable::m_fineGrindYield_pct       ,           NonPhysicalQuantity::Percentage     ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::coarseGrindYield_pct     , Fermentable::m_coarseGrindYield_pct     ,           NonPhysicalQuantity::Percentage     ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::potentialYield_sg        , Fermentable::m_potentialYield_sg        , Measurement::PhysicalQuantity::Density        ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::alphaAmylase_dextUnits   , Fermentable::m_alphaAmylase_dextUnits   ,           NonPhysicalQuantity::Dimensionless  ), // Not really dimensionless...
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::kolbachIndex_pct         , Fermentable::m_kolbachIndex_pct         ,           NonPhysicalQuantity::Percentage     ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::hardnessPrpGlassy_pct    , Fermentable::m_hardnessPrpGlassy_pct    ,           NonPhysicalQuantity::Percentage     ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::hardnessPrpHalf_pct      , Fermentable::m_hardnessPrpHalf_pct      ,           NonPhysicalQuantity::Percentage     ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::hardnessPrpMealy_pct     , Fermentable::m_hardnessPrpMealy_pct     ,           NonPhysicalQuantity::Percentage     ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::kernelSizePrpPlump_pct   , Fermentable::m_kernelSizePrpPlump_pct   ,           NonPhysicalQuantity::Percentage     ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::kernelSizePrpThin_pct    , Fermentable::m_kernelSizePrpThin_pct    ,           NonPhysicalQuantity::Percentage     ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::friability_pct           , Fermentable::m_friability_pct           ,           NonPhysicalQuantity::Percentage     ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::di_ph                    , Fermentable::m_di_ph                    , Measurement::PhysicalQuantity::Acidity        ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::viscosity_cP             , Fermentable::m_viscosity_cP             , Measurement::PhysicalQuantity::Viscosity      ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::dmsP                     , Fermentable::m_dmsP                     , Measurement::PqEitherMassOrVolumeConcentration),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::dmsPIsMassPerVolume      , Fermentable::m_dmsPIsMassPerVolume      ,           NonPhysicalQuantity::Bool           ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::fan                      , Fermentable::m_fan                      , Measurement::PqEitherMassOrVolumeConcentration),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::fanIsMassPerVolume       , Fermentable::m_fanIsMassPerVolume       ,           NonPhysicalQuantity::Bool           ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::fermentability_pct       , Fermentable::m_fermentability_pct       ,           NonPhysicalQuantity::Percentage     ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::betaGlucan               , Fermentable::m_betaGlucan               , Measurement::PqEitherMassOrVolumeConcentration),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentable::betaGlucanIsMassPerVolume, Fermentable::m_betaGlucanIsMassPerVolume,           NonPhysicalQuantity::Bool           ),

      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::Fermentable::amountWithUnits    , Fermentable::amountWithUnits            , Measurement::PqEitherMassOrVolume             ),
      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::Fermentable::dmsPWithUnits      , Fermentable::dmsPWithUnits              , Measurement::PqEitherMassOrVolumeConcentration),
      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::Fermentable::fanWithUnits       , Fermentable::fanWithUnits               , Measurement::PqEitherMassOrVolumeConcentration),
      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::Fermentable::betaGlucanWithUnits, Fermentable::betaGlucanWithUnits        , Measurement::PqEitherMassOrVolumeConcentration),
   },
   // Parent class lookup.  NB: NamedEntityWithInventory not NamedEntity!
   &NamedEntityWithInventory::typeLookup
};
static_assert(std::is_base_of<NamedEntityWithInventory, Fermentable>::value);

Fermentable::Fermentable(QString name) :
   NamedEntityWithInventory   {name, true},
   m_type                     {Fermentable::Type::Grain},
   m_amount                   {0.0                     },
   m_amountIsWeight           {true                    }, // ⮜⮜⮜ Added for BeerJSON support ⮞⮞⮞
   m_yield_pct                {0.0                     },
   m_color_srm                {0.0                     },
   m_addAfterBoil             {false                   },
   m_origin                   {""                      },
   m_supplier                 {""                      },
   m_notes                    {""                      },
   m_coarseFineDiff_pct       {0.0                     },
   m_moisture_pct             {0.0                     },
   m_diastaticPower_lintner   {0.0                     },
   m_protein_pct              {0.0                     },
   m_maxInBatch_pct           {100.0                   },
   m_recommendMash            {false                   },
   m_ibuGalPerLb              {0.0                     },
   m_isMashed                 {false                   },
   // ⮜⮜⮜ All below added f   or BeerJSON support ⮞⮞⮞
   m_grainGroup               {std::nullopt            },
   m_producer                 {""                      },
   m_productId                {""                      },
   m_fineGrindYield_pct       {std::nullopt            },
   m_coarseGrindYield_pct     {std::nullopt            },
   m_potentialYield_sg        {std::nullopt            },
   m_alphaAmylase_dextUnits   {std::nullopt            },
   m_kolbachIndex_pct         {std::nullopt            },
   m_hardnessPrpGlassy_pct    {std::nullopt            },
   m_hardnessPrpHalf_pct      {std::nullopt            },
   m_hardnessPrpMealy_pct     {std::nullopt            },
   m_kernelSizePrpPlump_pct   {std::nullopt            },
   m_kernelSizePrpThin_pct    {std::nullopt            },
   m_friability_pct           {std::nullopt            },
   m_di_ph                    {std::nullopt            },
   m_viscosity_cP             {std::nullopt            },
   m_dmsP                     {std::nullopt            },
   m_dmsPIsMassPerVolume      {true                    },
   m_fan                      {std::nullopt            },
   m_fanIsMassPerVolume       {true                    },
   m_fermentability_pct       {std::nullopt            },
   m_betaGlucan               {std::nullopt            },
   m_betaGlucanIsMassPerVolume{true                    } {
   return;
}

Fermentable::Fermentable(NamedParameterBundle const & namedParameterBundle) :
   NamedEntityWithInventory   {namedParameterBundle},
   m_type                     {namedParameterBundle.val<Fermentable::Type             >(PropertyNames::Fermentable::type                             )},
   m_yield_pct                {namedParameterBundle.val<double                        >(PropertyNames::Fermentable::yield_pct                        )},
   m_color_srm                {namedParameterBundle.val<double                        >(PropertyNames::Fermentable::color_srm                        )},
   m_addAfterBoil             {namedParameterBundle.val<bool                          >(PropertyNames::Fermentable::addAfterBoil                     )},
   m_origin                   {namedParameterBundle.val<QString                       >(PropertyNames::Fermentable::origin                , QString())},
   m_supplier                 {namedParameterBundle.val<QString                       >(PropertyNames::Fermentable::supplier              , QString())},
   m_notes                    {namedParameterBundle.val<QString                       >(PropertyNames::Fermentable::notes                 , QString())},
   m_coarseFineDiff_pct       {namedParameterBundle.val<double                        >(PropertyNames::Fermentable::coarseFineDiff_pct               )},
   m_moisture_pct             {namedParameterBundle.val<double                        >(PropertyNames::Fermentable::moisture_pct                     )},
   m_diastaticPower_lintner   {namedParameterBundle.val<double                        >(PropertyNames::Fermentable::diastaticPower_lintner           )},
   m_protein_pct              {namedParameterBundle.val<double                        >(PropertyNames::Fermentable::protein_pct                      )},
   m_maxInBatch_pct           {namedParameterBundle.val<double                        >(PropertyNames::Fermentable::maxInBatch_pct                   )},
   m_recommendMash            {namedParameterBundle.val<bool                          >(PropertyNames::Fermentable::recommendMash                    )},
   m_ibuGalPerLb              {namedParameterBundle.val<double                        >(PropertyNames::Fermentable::ibuGalPerLb                      )},
   m_isMashed                 {namedParameterBundle.val<bool                          >(PropertyNames::Fermentable::isMashed              , false    )},
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   m_grainGroup               {namedParameterBundle.optEnumVal<Fermentable::GrainGroup>(PropertyNames::Fermentable::grainGroup                       )},
   m_producer                 {namedParameterBundle.val<QString                       >(PropertyNames::Fermentable::producer                         )},
   m_productId                {namedParameterBundle.val<QString                       >(PropertyNames::Fermentable::productId                        )},
   m_fineGrindYield_pct       {namedParameterBundle.val<std::optional<double>         >(PropertyNames::Fermentable::fineGrindYield_pct               )},
   m_coarseGrindYield_pct     {namedParameterBundle.val<std::optional<double>         >(PropertyNames::Fermentable::coarseGrindYield_pct             )},
   m_potentialYield_sg        {namedParameterBundle.val<std::optional<double>         >(PropertyNames::Fermentable::potentialYield_sg                )},
   m_alphaAmylase_dextUnits   {namedParameterBundle.val<std::optional<double>         >(PropertyNames::Fermentable::alphaAmylase_dextUnits           )},
   m_kolbachIndex_pct         {namedParameterBundle.val<std::optional<double>         >(PropertyNames::Fermentable::kolbachIndex_pct                 )},
   m_hardnessPrpGlassy_pct    {namedParameterBundle.val<std::optional<double>         >(PropertyNames::Fermentable::hardnessPrpGlassy_pct            )},
   m_hardnessPrpHalf_pct      {namedParameterBundle.val<std::optional<double>         >(PropertyNames::Fermentable::hardnessPrpHalf_pct              )},
   m_hardnessPrpMealy_pct     {namedParameterBundle.val<std::optional<double>         >(PropertyNames::Fermentable::hardnessPrpMealy_pct             )},
   m_kernelSizePrpPlump_pct   {namedParameterBundle.val<std::optional<double>         >(PropertyNames::Fermentable::kernelSizePrpPlump_pct           )},
   m_kernelSizePrpThin_pct    {namedParameterBundle.val<std::optional<double>         >(PropertyNames::Fermentable::kernelSizePrpThin_pct            )},
   m_friability_pct           {namedParameterBundle.val<std::optional<double>         >(PropertyNames::Fermentable::friability_pct                   )},
   m_di_ph                    {namedParameterBundle.val<std::optional<double>         >(PropertyNames::Fermentable::di_ph                            )},
   m_viscosity_cP             {namedParameterBundle.val<std::optional<double>         >(PropertyNames::Fermentable::viscosity_cP                     )},
   m_fermentability_pct       {namedParameterBundle.val<std::optional<double>         >(PropertyNames::Fermentable::fermentability_pct               )} {

   this->setEitherOrReqParams<MassOrVolumeAmt             >(namedParameterBundle, PropertyNames::Fermentable::amount    , PropertyNames::Fermentable::amountIsWeight           , PropertyNames::Fermentable::amountWithUnits    , this->m_amount    , this->m_amountIsWeight           );
   this->setEitherOrOptParams<MassOrVolumeConcentrationAmt>(namedParameterBundle, PropertyNames::Fermentable::dmsP      , PropertyNames::Fermentable::dmsPIsMassPerVolume      , PropertyNames::Fermentable::dmsPWithUnits      , this->m_dmsP      , this->m_dmsPIsMassPerVolume      );
   this->setEitherOrOptParams<MassOrVolumeConcentrationAmt>(namedParameterBundle, PropertyNames::Fermentable::fan       , PropertyNames::Fermentable::fanIsMassPerVolume       , PropertyNames::Fermentable::fanWithUnits       , this->m_fan       , this->m_fanIsMassPerVolume       );
   this->setEitherOrOptParams<MassOrVolumeConcentrationAmt>(namedParameterBundle, PropertyNames::Fermentable::betaGlucan, PropertyNames::Fermentable::betaGlucanIsMassPerVolume, PropertyNames::Fermentable::betaGlucanWithUnits, this->m_betaGlucan, this->m_betaGlucanIsMassPerVolume);
   return;
}

Fermentable::Fermentable(Fermentable const & other) :
   NamedEntityWithInventory   {other                         },
   m_type                     {other.m_type                  },
   m_amount                   {other.m_amount                },
   m_amountIsWeight           {other.m_amountIsWeight        }, // ⮜⮜⮜ Added for BeerJSON support ⮞⮞⮞
   m_yield_pct                {other.m_yield_pct             },
   m_color_srm                {other.m_color_srm             },
   m_addAfterBoil             {other.m_addAfterBoil          },
   m_origin                   {other.m_origin                },
   m_supplier                 {other.m_supplier              },
   m_notes                    {other.m_notes                 },
   m_coarseFineDiff_pct       {other.m_coarseFineDiff_pct    },
   m_moisture_pct             {other.m_moisture_pct          },
   m_diastaticPower_lintner   {other.m_diastaticPower_lintner},
   m_protein_pct              {other.m_protein_pct           },
   m_maxInBatch_pct           {other.m_maxInBatch_pct        },
   m_recommendMash            {other.m_recommendMash         },
   m_ibuGalPerLb              {other.m_ibuGalPerLb           },
   m_isMashed                 {other.m_isMashed              },
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   m_grainGroup               {other.m_grainGroup            },
   m_producer                 {other.m_producer              },
   m_productId                {other.m_productId             },
   m_fineGrindYield_pct       {other.m_fineGrindYield_pct    },
   m_coarseGrindYield_pct     {other.m_coarseGrindYield_pct  },
   m_potentialYield_sg        {other.m_potentialYield_sg     },
   m_alphaAmylase_dextUnits   {other.m_alphaAmylase_dextUnits},
   m_kolbachIndex_pct         {other.m_kolbachIndex_pct      },
   m_hardnessPrpGlassy_pct    {other.m_hardnessPrpGlassy_pct },
   m_hardnessPrpHalf_pct      {other.m_hardnessPrpHalf_pct   },
   m_hardnessPrpMealy_pct     {other.m_hardnessPrpMealy_pct  },
   m_kernelSizePrpPlump_pct   {other.m_kernelSizePrpPlump_pct},
   m_kernelSizePrpThin_pct    {other.m_kernelSizePrpThin_pct },
   m_friability_pct           {other.m_friability_pct        },
   m_di_ph                    {other.m_di_ph                 },
   m_viscosity_cP             {other.m_viscosity_cP          },
   m_dmsP                     {other.m_dmsP                     },
   m_dmsPIsMassPerVolume      {other.m_dmsPIsMassPerVolume      },
   m_fan                      {other.m_fan                      },
   m_fanIsMassPerVolume       {other.m_fanIsMassPerVolume       },
   m_fermentability_pct       {other.m_fermentability_pct       },
   m_betaGlucan               {other.m_betaGlucan               },
   m_betaGlucanIsMassPerVolume{other.m_betaGlucanIsMassPerVolume} {
   return;
}

Fermentable::~Fermentable() = default;

//============================================= "GETTER" MEMBER FUNCTIONS ==============================================
Fermentable::Type                      Fermentable::type                     () const { return              this->m_type                     ; }
double                                 Fermentable::amount                   () const { return              this->m_amount                   ; }
bool                                   Fermentable::amountIsWeight           () const { return              this->m_amountIsWeight           ; } // ⮜⮜⮜ Added for BeerJSON support ⮞⮞⮞
double                                 Fermentable::yield_pct                () const { return              this->m_yield_pct                ; }
double                                 Fermentable::color_srm                () const { return              this->m_color_srm                ; }
bool                                   Fermentable::addAfterBoil             () const { return              this->m_addAfterBoil             ; }
QString                                Fermentable::origin                   () const { return              this->m_origin                   ; }
QString                                Fermentable::supplier                 () const { return              this->m_supplier                 ; }
QString                                Fermentable::notes                    () const { return              this->m_notes                    ; }
double                                 Fermentable::coarseFineDiff_pct       () const { return              this->m_coarseFineDiff_pct       ; }
double                                 Fermentable::moisture_pct             () const { return              this->m_moisture_pct             ; }
double                                 Fermentable::diastaticPower_lintner   () const { return              this->m_diastaticPower_lintner   ; }
double                                 Fermentable::protein_pct              () const { return              this->m_protein_pct              ; }
double                                 Fermentable::maxInBatch_pct           () const { return              this->m_maxInBatch_pct           ; }
bool                                   Fermentable::recommendMash            () const { return              this->m_recommendMash            ; }
double                                 Fermentable::ibuGalPerLb              () const { return              this->m_ibuGalPerLb              ; }
bool                                   Fermentable::isMashed                 () const { return              this->m_isMashed                 ; }
// ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
std::optional<Fermentable::GrainGroup> Fermentable::grainGroup               () const { return              this->m_grainGroup               ; }
std::optional<int>                     Fermentable::grainGroupAsInt          () const { return castToOptInt(this->m_grainGroup)              ; }
QString                                Fermentable::producer                 () const { return              this->m_producer                 ; }
QString                                Fermentable::productId                () const { return              this->m_productId                ; }
std::optional<double>                  Fermentable::fineGrindYield_pct       () const { return              this->m_fineGrindYield_pct       ; }
std::optional<double>                  Fermentable::coarseGrindYield_pct     () const { return              this->m_coarseGrindYield_pct     ; }
std::optional<double>                  Fermentable::potentialYield_sg        () const { return              this->m_potentialYield_sg        ; }
std::optional<double>                  Fermentable::alphaAmylase_dextUnits   () const { return              this->m_alphaAmylase_dextUnits   ; }
std::optional<double>                  Fermentable::kolbachIndex_pct         () const { return              this->m_kolbachIndex_pct         ; }
std::optional<double>                  Fermentable::hardnessPrpGlassy_pct    () const { return              this->m_hardnessPrpGlassy_pct    ; }
std::optional<double>                  Fermentable::hardnessPrpHalf_pct      () const { return              this->m_hardnessPrpHalf_pct      ; }
std::optional<double>                  Fermentable::hardnessPrpMealy_pct     () const { return              this->m_hardnessPrpMealy_pct     ; }
std::optional<double>                  Fermentable::kernelSizePrpPlump_pct   () const { return              this->m_kernelSizePrpPlump_pct   ; }
std::optional<double>                  Fermentable::kernelSizePrpThin_pct    () const { return              this->m_kernelSizePrpThin_pct    ; }
std::optional<double>                  Fermentable::friability_pct           () const { return              this->m_friability_pct           ; }
std::optional<double>                  Fermentable::di_ph                    () const { return              this->m_di_ph                    ; }
std::optional<double>                  Fermentable::viscosity_cP             () const { return              this->m_viscosity_cP             ; }
std::optional<double>                  Fermentable::dmsP                     () const { return              this->m_dmsP                     ; }
bool                                   Fermentable::dmsPIsMassPerVolume      () const { return              this->m_dmsPIsMassPerVolume      ; }
std::optional<double>                  Fermentable::fan                      () const { return              this->m_fan                      ; }
bool                                   Fermentable::fanIsMassPerVolume       () const { return              this->m_fanIsMassPerVolume       ; }
std::optional<double>                  Fermentable::fermentability_pct       () const { return              this->m_fermentability_pct       ; }
std::optional<double>                  Fermentable::betaGlucan               () const { return              this->m_betaGlucan               ; }
bool                                   Fermentable::betaGlucanIsMassPerVolume() const { return              this->m_betaGlucanIsMassPerVolume; }

// Combined getters (all added for BeerJSON support)
MassOrVolumeAmt                             Fermentable::amountWithUnits    () const { return MassOrVolumeAmt{this->m_amount, this->m_amountIsWeight ? Measurement::Units::kilograms : Measurement::Units::liters}; }
std::optional<MassOrVolumeConcentrationAmt> Fermentable::dmsPWithUnits      () const { return Optional::eitherOr<MassOrVolumeConcentrationAmt>(this->m_dmsP      , this->m_dmsPIsMassPerVolume      , Measurement::Units::milligramsPerLiter, Measurement::Units::partsPerMillion); }
std::optional<MassOrVolumeConcentrationAmt> Fermentable::fanWithUnits       () const { return Optional::eitherOr<MassOrVolumeConcentrationAmt>(this->m_fan       , this->m_fanIsMassPerVolume       , Measurement::Units::milligramsPerLiter, Measurement::Units::partsPerMillion); }
std::optional<MassOrVolumeConcentrationAmt> Fermentable::betaGlucanWithUnits() const { return Optional::eitherOr<MassOrVolumeConcentrationAmt>(this->m_betaGlucan, this->m_betaGlucanIsMassPerVolume, Measurement::Units::milligramsPerLiter, Measurement::Units::partsPerMillion); }


bool Fermentable::isExtract() const {
   return ((type() == Fermentable::Type::Extract) || (type() == Fermentable::Type::Dry_Extract));
}

bool Fermentable::isSugar() const {
   return (type() == Fermentable::Type::Sugar);
}

double Fermentable::equivSucrose_kg() const {
   // .:TBD:. Not clear what we should return (or whether we should even be called) if amount is a volume
   if (!this->m_amountIsWeight) {
      qWarning() << Q_FUNC_INFO << "Trying to calculate equivSucrose_kg for Fermantable measured by volume";
   }
   double const ret = this->amount() * this->yield_pct() * (1.0 - this->moisture_pct() / 100.0) / 100.0;

   // If this is a steeped grain...
   if (this->type() == Fermentable::Type::Grain && !this->isMashed()) {
      return 0.60 * ret; // Reduce the yield by 60%.
   }
   return ret;
}

//============================================= "SETTER" MEMBER FUNCTIONS ==============================================
void Fermentable::setType                     (Type                      const   val) { this->setAndNotify(PropertyNames::Fermentable::type                     , this->m_type                     , val); }
void Fermentable::setAddAfterBoil             (bool                      const   val) { this->setAndNotify(PropertyNames::Fermentable::addAfterBoil             , this->m_addAfterBoil             , val); }
void Fermentable::setOrigin                   (QString                   const & val) { this->setAndNotify(PropertyNames::Fermentable::origin                   , this->m_origin                   , val); }
void Fermentable::setSupplier                 (QString                   const & val) { this->setAndNotify(PropertyNames::Fermentable::supplier                 , this->m_supplier                 , val); }
void Fermentable::setNotes                    (QString                   const & val) { this->setAndNotify(PropertyNames::Fermentable::notes                    , this->m_notes                    , val); }
void Fermentable::setRecommendMash            (bool                      const   val) { this->setAndNotify(PropertyNames::Fermentable::recommendMash            , this->m_recommendMash            , val); }
void Fermentable::setIsMashed                 (bool                      const   val) { this->setAndNotify(PropertyNames::Fermentable::isMashed                 , this->m_isMashed                 , val); }
void Fermentable::setIbuGalPerLb              (double                    const   val) { this->setAndNotify(PropertyNames::Fermentable::ibuGalPerLb              , this->m_ibuGalPerLb              , val); }
void Fermentable::setAmount                   (double                    const   val) { this->setAndNotify(PropertyNames::Fermentable::amount                   , this->m_amount                   , this->enforceMin      (val, "amount"));                     }
void Fermentable::setAmountIsWeight           (bool                      const   val) { this->setAndNotify(PropertyNames::Fermentable::amountIsWeight           , this->m_amountIsWeight           , val); } // ⮜⮜⮜ Added for BeerJSON support ⮞⮞⮞
void Fermentable::setYield_pct                (double                    const   val) { this->setAndNotify(PropertyNames::Fermentable::yield_pct                , this->m_yield_pct                , this->enforceMinAndMax(val, "amount",         0.0, 100.0)); }
void Fermentable::setColor_srm                (double                    const   val) { this->setAndNotify(PropertyNames::Fermentable::color_srm                , this->m_color_srm                , this->enforceMin      (val, "color"));                      }
void Fermentable::setCoarseFineDiff_pct       (double                    const   val) { this->setAndNotify(PropertyNames::Fermentable::coarseFineDiff_pct       , this->m_coarseFineDiff_pct       , this->enforceMinAndMax(val, "coarseFineDiff", 0.0, 100.0)); }
void Fermentable::setMoisture_pct             (double                    const   val) { this->setAndNotify(PropertyNames::Fermentable::moisture_pct             , this->m_moisture_pct             , this->enforceMinAndMax(val, "moisture",       0.0, 100.0)); }
void Fermentable::setDiastaticPower_lintner   (double                    const   val) { this->setAndNotify(PropertyNames::Fermentable::diastaticPower_lintner   , this->m_diastaticPower_lintner   , this->enforceMin      (val, "diastatic power"));            }
void Fermentable::setProtein_pct              (double                    const   val) { this->setAndNotify(PropertyNames::Fermentable::protein_pct              , this->m_protein_pct              , this->enforceMinAndMax(val, "protein",        0.0, 100.0)); }
void Fermentable::setMaxInBatch_pct           (double                    const   val) { this->setAndNotify(PropertyNames::Fermentable::maxInBatch_pct           , this->m_maxInBatch_pct           , this->enforceMinAndMax(val, "max in batch",   0.0, 100.0)); }
// ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
void Fermentable::setGrainGroup               (std::optional<GrainGroup> const   val) { this->setAndNotify(PropertyNames::Fermentable::grainGroup               , this->m_grainGroup               , val                            ); }
void Fermentable::setGrainGroupAsInt          (std::optional<int>        const   val) { this->setAndNotify(PropertyNames::Fermentable::grainGroup               , this->m_grainGroup               , castFromOptInt<GrainGroup>(val)); }
void Fermentable::setProducer                 (QString                   const & val) { this->setAndNotify(PropertyNames::Fermentable::producer                 , this->m_producer                 , val                            ); }
void Fermentable::setProductId                (QString                   const & val) { this->setAndNotify(PropertyNames::Fermentable::productId                , this->m_productId                , val                            ); }
void Fermentable::setFineGrindYield_pct       (std::optional<double>     const   val) { this->setAndNotify(PropertyNames::Fermentable::fineGrindYield_pct       , this->m_fineGrindYield_pct       , val                            ); }
void Fermentable::setCoarseGrindYield_pct     (std::optional<double>     const   val) { this->setAndNotify(PropertyNames::Fermentable::coarseGrindYield_pct     , this->m_coarseGrindYield_pct     , val                            ); }
void Fermentable::setPotentialYield_sg        (std::optional<double>     const   val) { this->setAndNotify(PropertyNames::Fermentable::potentialYield_sg        , this->m_potentialYield_sg        , val                            ); }
void Fermentable::setAlphaAmylase_dextUnits   (std::optional<double>     const   val) { this->setAndNotify(PropertyNames::Fermentable::alphaAmylase_dextUnits   , this->m_alphaAmylase_dextUnits   , val                            ); }
void Fermentable::setKolbachIndex_pct         (std::optional<double>     const   val) { this->setAndNotify(PropertyNames::Fermentable::kolbachIndex_pct         , this->m_kolbachIndex_pct         , val                            ); }
void Fermentable::setHardnessPrpGlassy_pct    (std::optional<double>     const   val) { this->setAndNotify(PropertyNames::Fermentable::hardnessPrpGlassy_pct    , this->m_hardnessPrpGlassy_pct    , val                            ); }
void Fermentable::setHardnessPrpHalf_pct      (std::optional<double>     const   val) { this->setAndNotify(PropertyNames::Fermentable::hardnessPrpHalf_pct      , this->m_hardnessPrpHalf_pct      , val                            ); }
void Fermentable::setHardnessPrpMealy_pct     (std::optional<double>     const   val) { this->setAndNotify(PropertyNames::Fermentable::hardnessPrpMealy_pct     , this->m_hardnessPrpMealy_pct     , val                            ); }
void Fermentable::setKernelSizePrpPlump_pct   (std::optional<double>     const   val) { this->setAndNotify(PropertyNames::Fermentable::kernelSizePrpPlump_pct   , this->m_kernelSizePrpPlump_pct   , val                            ); }
void Fermentable::setKernelSizePrpThin_pct    (std::optional<double>     const   val) { this->setAndNotify(PropertyNames::Fermentable::kernelSizePrpThin_pct    , this->m_kernelSizePrpThin_pct    , val                            ); }
void Fermentable::setFriability_pct           (std::optional<double>     const   val) { this->setAndNotify(PropertyNames::Fermentable::friability_pct           , this->m_friability_pct           , val                            ); }
void Fermentable::setDi_ph                    (std::optional<double>     const   val) { this->setAndNotify(PropertyNames::Fermentable::di_ph                    , this->m_di_ph                    , val                            ); }
void Fermentable::setViscosity_cP             (std::optional<double>     const   val) { this->setAndNotify(PropertyNames::Fermentable::viscosity_cP             , this->m_viscosity_cP             , val                            ); }
void Fermentable::setDmsP                     (std::optional<double>     const   val) { this->setAndNotify(PropertyNames::Fermentable::dmsP                     , this->m_dmsP                     , val                            ); }
void Fermentable::setDmsPIsMassPerVolume      (bool                      const   val) { this->setAndNotify(PropertyNames::Fermentable::dmsPIsMassPerVolume      , this->m_dmsPIsMassPerVolume      , val                            ); }
void Fermentable::setFan                      (std::optional<double>     const   val) { this->setAndNotify(PropertyNames::Fermentable::fan                      , this->m_fan                      , val                            ); }
void Fermentable::setFanIsMassPerVolume       (bool                      const   val) { this->setAndNotify(PropertyNames::Fermentable::fanIsMassPerVolume       , this->m_fanIsMassPerVolume       , val                            ); }
void Fermentable::setFermentability_pct       (std::optional<double>     const   val) { this->setAndNotify(PropertyNames::Fermentable::fermentability_pct       , this->m_fermentability_pct       , val                            ); }
void Fermentable::setBetaGlucan               (std::optional<double>     const   val) { this->setAndNotify(PropertyNames::Fermentable::betaGlucan               , this->m_betaGlucan               , val                            ); }
void Fermentable::setBetaGlucanIsMassPerVolume(bool                      const   val) { this->setAndNotify(PropertyNames::Fermentable::betaGlucanIsMassPerVolume, this->m_betaGlucanIsMassPerVolume, val                            ); }

void Fermentable::setAmountWithUnits          (MassOrVolumeAmt           const   val) {
   this->setAndNotify(PropertyNames::Fermentable::amount        , this->m_amount        , val.quantity());
   this->setAndNotify(PropertyNames::Fermentable::amountIsWeight, this->m_amountIsWeight, val.isMass()  );
   return;
}
void Fermentable::setDmsPWithUnits      (std::optional<MassOrVolumeConcentrationAmt> const   val) {
   std::optional<double> quantity = std::nullopt; // Gets set by Optional::eitherOr
   bool const isMassPerVolume = Optional::eitherOr(val, quantity);
   this->setAndNotify(PropertyNames::Fermentable::dmsP               , this->m_dmsP               , quantity       );
   this->setAndNotify(PropertyNames::Fermentable::dmsPIsMassPerVolume, this->m_dmsPIsMassPerVolume, isMassPerVolume);
   return;
}
void Fermentable::setFanWithUnits       (std::optional<MassOrVolumeConcentrationAmt> const   val) {
   std::optional<double> quantity = std::nullopt; // Gets set by Optional::eitherOr
   bool const isMassPerVolume = Optional::eitherOr(val, quantity);
   this->setAndNotify(PropertyNames::Fermentable::fan               , this->m_fan               , quantity       );
   this->setAndNotify(PropertyNames::Fermentable::fanIsMassPerVolume, this->m_fanIsMassPerVolume, isMassPerVolume);
   return;
}
void Fermentable::setBetaGlucanWithUnits(std::optional<MassOrVolumeConcentrationAmt> const   val) {
   std::optional<double> quantity = std::nullopt; // Gets set by Optional::eitherOr
   bool const isMassPerVolume = Optional::eitherOr(val, quantity);
   this->setAndNotify(PropertyNames::Fermentable::betaGlucan               , this->m_betaGlucan               , quantity       );
   this->setAndNotify(PropertyNames::Fermentable::betaGlucanIsMassPerVolume, this->m_betaGlucanIsMassPerVolume, isMassPerVolume);
   return;
}

void Fermentable::setInventoryAmount(double num) {
   InventoryUtils::setAmount(*this, num);
   return;
}

double Fermentable::inventory() const {
   return InventoryUtils::getAmount(*this);
}

Recipe * Fermentable::getOwningRecipe() {
   return ObjectStoreWrapper::findFirstMatching<Recipe>( [this](Recipe * rec) {return rec->uses(*this);} );
}

bool fermentablesLessThanByWeight(Fermentable const * const lhs, Fermentable const * const rhs) {
   // Sort by name if the two fermentables are of equal weight or volume
   if (lhs->amountIsWeight() == rhs->amountIsWeight() && qFuzzyCompare(lhs->amount(), rhs->amount())) {
      return lhs->name() < rhs->name();
   }

   // .:TBD:. Do we want to separate out liquids and solids?

   // Yes. I know. This seems silly, but I want the returned list in descending not ascending order.
   return lhs->amount() > rhs->amount();
}
