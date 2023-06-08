/*======================================================================================================================
 * model/Yeast.cpp is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Samuel Östling <MrOstling@gmail.com>
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
#include "model/Yeast.h"

#include <QDebug>

#include "database/ObjectStoreWrapper.h"
#include "model/Inventory.h"
#include "model/NamedParameterBundle.h"
#include "model/Recipe.h"
#include "PhysicalConstants.h"

QString const Yeast::LocalisedName = tr("Yeast");

EnumStringMapping const Yeast::typeStringMapping {
   {Yeast::Type::Ale         , "ale"          },
   {Yeast::Type::Lager       , "lager"        },
   {Yeast::Type::Other       , "other"        },  // Was Wheat / wheat
   {Yeast::Type::Wine        , "wine"         },
   {Yeast::Type::Champagne   , "champagne"    },
   {Yeast::Type::Bacteria    , "bacteria"     },
   {Yeast::Type::Brett       , "brett"        },
   {Yeast::Type::Kveik       , "kveik"        },
   {Yeast::Type::Lacto       , "lacto"        },
   {Yeast::Type::Malolactic  , "malolactic"   },
   {Yeast::Type::MixedCulture, "mixed-culture"},
   {Yeast::Type::Pedio       , "pedio"        },
   {Yeast::Type::Spontaneous , "spontaneous"  },
};

EnumStringMapping const Yeast::typeDisplayNames {
   {Yeast::Type::Ale         , tr("Ale"          )},
   {Yeast::Type::Lager       , tr("Lager"        )},
   {Yeast::Type::Other       , tr("Other"        )},
   {Yeast::Type::Wine        , tr("Wine"         )},
   {Yeast::Type::Champagne   , tr("Champagne"    )},
   {Yeast::Type::Bacteria    , tr("Bacteria"     )},
   {Yeast::Type::Brett       , tr("Brett"        )},
   {Yeast::Type::Kveik       , tr("Kveik"        )},
   {Yeast::Type::Lacto       , tr("Lacto"        )},
   {Yeast::Type::Malolactic  , tr("Malolactic"   )},
   {Yeast::Type::MixedCulture, tr("Mixed-culture")},
   {Yeast::Type::Pedio       , tr("Pedio"        )},
   {Yeast::Type::Spontaneous , tr("Spontaneous"  )},
};

EnumStringMapping const Yeast::formStringMapping {
   {Yeast::Form::Liquid , "liquid" },
   {Yeast::Form::Dry    , "dry"    },
   {Yeast::Form::Slant  , "slant"  },
   {Yeast::Form::Culture, "culture"},
   {Yeast::Form::Dregs  , "dregs"  },
};

EnumStringMapping const Yeast::formDisplayNames  {
   {Yeast::Form::Liquid , tr("Liquid" )},
   {Yeast::Form::Dry    , tr("Dry"    )},
   {Yeast::Form::Slant  , tr("Slant"  )},
   {Yeast::Form::Culture, tr("Culture")},
   {Yeast::Form::Dregs  , tr("Dregs"  )},
};

EnumStringMapping const Yeast::flocculationStringMapping {
   {Yeast::Flocculation::VeryLow   , "very low"   },
   {Yeast::Flocculation::Low       , "low"        },
   {Yeast::Flocculation::MediumLow , "medium low" },
   {Yeast::Flocculation::Medium    , "medium"     },
   {Yeast::Flocculation::MediumHigh, "medium high"},
   {Yeast::Flocculation::High      , "high"       },
   {Yeast::Flocculation::VeryHigh  , "very high"  },
};

EnumStringMapping const Yeast::flocculationDisplayNames {
   {Yeast::Flocculation::VeryLow   , tr("Very Low"   )},
   {Yeast::Flocculation::Low       , tr("Low"        )},
   {Yeast::Flocculation::MediumLow , tr("Medium Low" )},
   {Yeast::Flocculation::Medium    , tr("Medium"     )},
   {Yeast::Flocculation::MediumHigh, tr("Medium High")},
   {Yeast::Flocculation::High      , tr("High"       )},
   {Yeast::Flocculation::VeryHigh  , tr("Very High"  )},
};


bool Yeast::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   Yeast const & rhs = static_cast<Yeast const &>(other);
   // Base class will already have ensured names are equal
   return (
      this->m_type         == rhs.m_type         &&
      this->m_form         == rhs.m_form         &&
      this->m_laboratory   == rhs.m_laboratory   &&
      this->m_productID    == rhs.m_productID    &&
      this->m_flocculation == rhs.m_flocculation
   );
}

ObjectStore & Yeast::getObjectStoreTypedInstance() const {
   return ObjectStoreTyped<Yeast>::getInstance();
}

TypeLookup const Yeast::typeLookup {
   "Yeast",
   {
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::type                     , Yeast::m_type                     ,           NonPhysicalQuantity::Enum         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::form                     , Yeast::m_form                     ,           NonPhysicalQuantity::Enum         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::amount                   , Yeast::m_amount                   , Measurement::PqEitherMassOrVolume           ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::amountIsWeight           , Yeast::m_amountIsWeight           ,           NonPhysicalQuantity::Bool         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::laboratory               , Yeast::m_laboratory               ,           NonPhysicalQuantity::String       ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::productID                , Yeast::m_productID                ,           NonPhysicalQuantity::String       ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::minTemperature_c         , Yeast::m_minTemperature_c         , Measurement::PhysicalQuantity::Temperature  ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::maxTemperature_c         , Yeast::m_maxTemperature_c         , Measurement::PhysicalQuantity::Temperature  ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::flocculation             , Yeast::m_flocculation             ,           NonPhysicalQuantity::Enum         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::attenuation_pct          , Yeast::m_attenuation_pct          ,           NonPhysicalQuantity::Percentage   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::notes                    , Yeast::m_notes                    ,           NonPhysicalQuantity::String       ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::bestFor                  , Yeast::m_bestFor                  ,           NonPhysicalQuantity::String       ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::timesCultured            , Yeast::m_timesCultured            ,           NonPhysicalQuantity::Count        ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::maxReuse                 , Yeast::m_maxReuse                 ,           NonPhysicalQuantity::Count        ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::addToSecondary           , Yeast::m_addToSecondary           ,           NonPhysicalQuantity::Bool         ),
      // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::alcoholTolerance_pct     , Yeast::m_alcoholTolerance_pct     ,           NonPhysicalQuantity::Percentage   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::attenuationMin_pct       , Yeast::m_attenuationMin_pct       ,           NonPhysicalQuantity::Percentage   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::attenuationMax_pct       , Yeast::m_attenuationMax_pct       ,           NonPhysicalQuantity::Percentage   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::phenolicOffFlavorPositive, Yeast::m_phenolicOffFlavorPositive,           NonPhysicalQuantity::Bool         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::glucoamylasePositive     , Yeast::m_glucoamylasePositive     ,           NonPhysicalQuantity::Bool         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::killerProducingK1Toxin   , Yeast::m_killerProducingK1Toxin   ,           NonPhysicalQuantity::Bool         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::killerProducingK2Toxin   , Yeast::m_killerProducingK2Toxin   ,           NonPhysicalQuantity::Bool         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::killerProducingK28Toxin  , Yeast::m_killerProducingK28Toxin  ,           NonPhysicalQuantity::Bool         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::killerProducingKlusToxin , Yeast::m_killerProducingKlusToxin ,           NonPhysicalQuantity::Bool         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::killerNeutral            , Yeast::m_killerNeutral            ,           NonPhysicalQuantity::Bool         ),

      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::Yeast::amountWithUnits    , Yeast, amountWithUnits            , Measurement::PqEitherMassOrVolume           ),
   },
   // Parent class lookup.  NB: NamedEntityWithInventory not NamedEntity!
   &NamedEntityWithInventory::typeLookup
};
static_assert(std::is_base_of<NamedEntityWithInventory, Yeast>::value);


//============================CONSTRUCTORS======================================

Yeast::Yeast(QString name) :
   NamedEntityWithInventory   {name, true},
   m_type                     {Yeast::Type::Ale},
   m_form                     {Yeast::Form::Liquid},
   m_amount                   {0.0},
   m_amountIsWeight           {false},
   m_laboratory               {""},
   m_productID                {""},
   m_minTemperature_c         {std::nullopt},
   m_maxTemperature_c         {std::nullopt},
   m_flocculation             {std::nullopt},
   m_attenuation_pct          {std::nullopt},
   m_notes                    {""},
   m_bestFor                  {""},
   m_timesCultured            {std::nullopt},
   m_maxReuse                 {std::nullopt},
   m_addToSecondary           {std::nullopt},
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   m_alcoholTolerance_pct     {std::nullopt},
   m_attenuationMin_pct       {std::nullopt},
   m_attenuationMax_pct       {std::nullopt},
   m_phenolicOffFlavorPositive{std::nullopt},
   m_glucoamylasePositive     {std::nullopt},
   m_killerProducingK1Toxin   {std::nullopt},
   m_killerProducingK2Toxin   {std::nullopt},
   m_killerProducingK28Toxin  {std::nullopt},
   m_killerProducingKlusToxin {std::nullopt},
   m_killerNeutral            {std::nullopt} {
   return;
}

Yeast::Yeast(NamedParameterBundle const & namedParameterBundle) :
   NamedEntityWithInventory   {namedParameterBundle},
   m_type                     {namedParameterBundle.val<Yeast::Type               >(PropertyNames::Yeast::type                     )},
   m_form                     {namedParameterBundle.val<Yeast::Form               >(PropertyNames::Yeast::form                     )},
   m_amount                   {namedParameterBundle.val<double                    >(PropertyNames::Yeast::amount                   )},
   m_amountIsWeight           {namedParameterBundle.val<bool                      >(PropertyNames::Yeast::amountIsWeight           )},
   m_laboratory               {namedParameterBundle.val<QString                   >(PropertyNames::Yeast::laboratory               )},
   m_productID                {namedParameterBundle.val<QString                   >(PropertyNames::Yeast::productID                )},
   m_minTemperature_c         {namedParameterBundle.val<std::optional<double>     >(PropertyNames::Yeast::minTemperature_c         )},
   m_maxTemperature_c         {namedParameterBundle.val<std::optional<double>     >(PropertyNames::Yeast::maxTemperature_c         )},
   m_flocculation             {namedParameterBundle.optEnumVal<Yeast::Flocculation>(PropertyNames::Yeast::flocculation             )},
   m_attenuation_pct          {namedParameterBundle.val<std::optional<double>     >(PropertyNames::Yeast::attenuation_pct          )},
   m_notes                    {namedParameterBundle.val<QString                   >(PropertyNames::Yeast::notes                    )},
   m_bestFor                  {namedParameterBundle.val<QString                   >(PropertyNames::Yeast::bestFor                  )},
   m_timesCultured            {namedParameterBundle.val<std::optional<int>        >(PropertyNames::Yeast::timesCultured            )},
   m_maxReuse                 {namedParameterBundle.val<std::optional<int>        >(PropertyNames::Yeast::maxReuse                 )},
   m_addToSecondary           {namedParameterBundle.val<std::optional<bool>       >(PropertyNames::Yeast::addToSecondary           )},
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   m_alcoholTolerance_pct     {namedParameterBundle.val<std::optional<double>     >(PropertyNames::Yeast::alcoholTolerance_pct     )},
   m_attenuationMin_pct       {namedParameterBundle.val<std::optional<double>     >(PropertyNames::Yeast::attenuationMin_pct       )},
   m_attenuationMax_pct       {namedParameterBundle.val<std::optional<double>     >(PropertyNames::Yeast::attenuationMax_pct       )},
   m_phenolicOffFlavorPositive{namedParameterBundle.val<std::optional<bool>       >(PropertyNames::Yeast::phenolicOffFlavorPositive)},
   m_glucoamylasePositive     {namedParameterBundle.val<std::optional<bool>       >(PropertyNames::Yeast::glucoamylasePositive     )},
   m_killerProducingK1Toxin   {namedParameterBundle.val<std::optional<bool>       >(PropertyNames::Yeast::killerProducingK1Toxin   )},
   m_killerProducingK2Toxin   {namedParameterBundle.val<std::optional<bool>       >(PropertyNames::Yeast::killerProducingK2Toxin   )},
   m_killerProducingK28Toxin  {namedParameterBundle.val<std::optional<bool>       >(PropertyNames::Yeast::killerProducingK28Toxin  )},
   m_killerProducingKlusToxin {namedParameterBundle.val<std::optional<bool>       >(PropertyNames::Yeast::killerProducingKlusToxin )},
   m_killerNeutral            {namedParameterBundle.val<std::optional<bool>       >(PropertyNames::Yeast::killerNeutral            )} {
   return;
}

Yeast::Yeast(Yeast const & other) :
   NamedEntityWithInventory   {other                            },
   m_type                     {other.m_type                     },
   m_form                     {other.m_form                     },
   m_amount                   {other.m_amount                   },
   m_amountIsWeight           {other.m_amountIsWeight           },
   m_laboratory               {other.m_laboratory               },
   m_productID                {other.m_productID                },
   m_minTemperature_c         {other.m_minTemperature_c         },
   m_maxTemperature_c         {other.m_maxTemperature_c         },
   m_flocculation             {other.m_flocculation             },
   m_attenuation_pct          {other.m_attenuation_pct          },
   m_notes                    {other.m_notes                    },
   m_bestFor                  {other.m_bestFor                  },
   m_timesCultured            {other.m_timesCultured            },
   m_maxReuse                 {other.m_maxReuse                 },
   m_addToSecondary           {other.m_addToSecondary           },
   m_alcoholTolerance_pct     {other.m_alcoholTolerance_pct     },
   m_attenuationMin_pct       {other.m_attenuationMin_pct       },
   m_attenuationMax_pct       {other.m_attenuationMax_pct       },
   m_phenolicOffFlavorPositive{other.m_phenolicOffFlavorPositive},
   m_glucoamylasePositive     {other.m_glucoamylasePositive     },
   m_killerProducingK1Toxin   {other.m_killerProducingK1Toxin   },
   m_killerProducingK2Toxin   {other.m_killerProducingK2Toxin   },
   m_killerProducingK28Toxin  {other.m_killerProducingK28Toxin  },
   m_killerProducingKlusToxin {other.m_killerProducingKlusToxin },
   m_killerNeutral            {other.m_killerNeutral            } {
   return;
}

Yeast::~Yeast() = default;

//============================================= "GETTER" MEMBER FUNCTIONS ==============================================
Yeast::Type                        Yeast::type                     () const { return                    m_type                     ; }
Yeast::Form                        Yeast::form                     () const { return                    m_form                     ; }
double                             Yeast::amount                   () const { return                    m_amount                   ; }
bool                               Yeast::amountIsWeight           () const { return                    m_amountIsWeight           ; }
QString                            Yeast::laboratory               () const { return                    m_laboratory               ; }
QString                            Yeast::productID                () const { return                    m_productID                ; }
std::optional<double>              Yeast::minTemperature_c         () const { return                    m_minTemperature_c         ; } // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
std::optional<double>              Yeast::maxTemperature_c         () const { return                    m_maxTemperature_c         ; } // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
std::optional<Yeast::Flocculation> Yeast::flocculation             () const { return                    m_flocculation             ; } // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
std::optional<int>                 Yeast::flocculationAsInt        () const { return Optional::toOptInt(m_flocculation)            ; } // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
std::optional<double>              Yeast::attenuation_pct          () const { return                    m_attenuation_pct          ; } // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
QString                            Yeast::notes                    () const { return                    m_notes                    ; }
QString                            Yeast::bestFor                  () const { return                    m_bestFor                  ; }
std::optional<int>                 Yeast::timesCultured            () const { return                    m_timesCultured            ; } // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
std::optional<int>                 Yeast::maxReuse                 () const { return                    m_maxReuse                 ; } // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
std::optional<bool>                Yeast::addToSecondary           () const { return                    m_addToSecondary           ; } // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
// ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
std::optional<double>              Yeast::alcoholTolerance_pct     () const { return                    m_alcoholTolerance_pct     ; }
std::optional<double>              Yeast::attenuationMin_pct       () const { return                    m_attenuationMin_pct       ; }
std::optional<double>              Yeast::attenuationMax_pct       () const { return                    m_attenuationMax_pct       ; }
std::optional<bool>                Yeast::phenolicOffFlavorPositive() const { return                    m_phenolicOffFlavorPositive; }
std::optional<bool>                Yeast::glucoamylasePositive     () const { return                    m_glucoamylasePositive     ; }
std::optional<bool>                Yeast::killerProducingK1Toxin   () const { return                    m_killerProducingK1Toxin   ; }
std::optional<bool>                Yeast::killerProducingK2Toxin   () const { return                    m_killerProducingK2Toxin   ; }
std::optional<bool>                Yeast::killerProducingK28Toxin  () const { return                    m_killerProducingK28Toxin  ; }
std::optional<bool>                Yeast::killerProducingKlusToxin () const { return                    m_killerProducingKlusToxin ; }
std::optional<bool>                Yeast::killerNeutral            () const { return                    m_killerNeutral            ; }

MassOrVolumeAmt     Yeast::amountWithUnits () const { return MassOrVolumeAmt{this->m_amount, this->m_amountIsWeight ? Measurement::Units::kilograms : Measurement::Units::liters}; }

//============================================= "SETTER" MEMBER FUNCTIONS ==============================================
// It seems a bit of overkill to enforce absolute zero as the lowest allowable temperature, but we do
void Yeast::setType                     (Yeast::Type                 const   val) { this->setAndNotify(PropertyNames::Yeast::type                     , m_type            , val); return; }
void Yeast::setForm                     (Yeast::Form                 const   val) { this->setAndNotify(PropertyNames::Yeast::form                     , m_form            , val); return; }
void Yeast::setAmount                   (double                      const   val) { this->setAndNotify(PropertyNames::Yeast::amount                   , m_amount          , this->enforceMin      (val, "amount"         , 0.0)); return; }
void Yeast::setAmountIsWeight           (bool                        const   val) { this->setAndNotify(PropertyNames::Yeast::amountIsWeight           , m_amountIsWeight  , val); return; }
void Yeast::setLaboratory               (QString                     const & val) { this->setAndNotify(PropertyNames::Yeast::laboratory               , m_laboratory      , val); return; }
void Yeast::setProductID                (QString                     const & val) { this->setAndNotify(PropertyNames::Yeast::productID                , m_productID       , val); return; }
void Yeast::setMinTemperature_c         (std::optional<double>       const   val) { this->setAndNotify(PropertyNames::Yeast::minTemperature_c         , m_minTemperature_c, this->enforceMin      (val, "max temp"       , PhysicalConstants::absoluteZero, 0.0  )); return; }
void Yeast::setMaxTemperature_c         (std::optional<double>       const   val) { this->setAndNotify(PropertyNames::Yeast::maxTemperature_c         , m_maxTemperature_c, this->enforceMin      (val, "max temp"       , PhysicalConstants::absoluteZero, 0.0  )); return; }
void Yeast::setFlocculation             (std::optional<Flocculation> const   val) { this->setAndNotify(PropertyNames::Yeast::flocculation             , m_flocculation    , val); return; }
void Yeast::setFlocculationAsInt        (std::optional<int>          const   val) { this->setAndNotify(PropertyNames::Yeast::flocculation             , m_flocculation    , Optional::fromOptInt<Flocculation>(val)); return; }
void Yeast::setAttenuation_pct          (std::optional<double>       const   val) { this->setAndNotify(PropertyNames::Yeast::attenuation_pct          , m_attenuation_pct , this->enforceMinAndMax(val, "pct attenuation", 0.0                            , 100.0, 0.0)); return; }
void Yeast::setNotes                    (QString                     const & val) { this->setAndNotify(PropertyNames::Yeast::notes                    , m_notes           , val); return; }
void Yeast::setBestFor                  (QString                     const & val) { this->setAndNotify(PropertyNames::Yeast::bestFor                  , m_bestFor         , val); return; }
void Yeast::setTimesCultured            (std::optional<int>          const   val) { this->setAndNotify(PropertyNames::Yeast::timesCultured            , m_timesCultured   , this->enforceMin      (val, "times cultured" )); return; }
void Yeast::setMaxReuse                 (std::optional<int>          const   val) { this->setAndNotify(PropertyNames::Yeast::maxReuse                 , m_maxReuse        , this->enforceMin      (val, "max reuse"      )); return; }
void Yeast::setAddToSecondary           (std::optional<bool>         const   val) { this->setAndNotify(PropertyNames::Yeast::addToSecondary           , m_addToSecondary  , val); return; }
// ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
void Yeast::setAlcoholTolerance_pct     (std::optional<double>       const   val) { this->setAndNotify(PropertyNames::Yeast::alcoholTolerance_pct     , m_alcoholTolerance_pct     , val); return; }
void Yeast::setAttenuationMin_pct       (std::optional<double>       const   val) { this->setAndNotify(PropertyNames::Yeast::attenuationMin_pct       , m_attenuationMin_pct       , val); return; }
void Yeast::setAttenuationMax_pct       (std::optional<double>       const   val) { this->setAndNotify(PropertyNames::Yeast::attenuationMax_pct       , m_attenuationMax_pct       , val); return; }
void Yeast::setPhenolicOffFlavorPositive(std::optional<bool>         const   val) { this->setAndNotify(PropertyNames::Yeast::phenolicOffFlavorPositive, m_phenolicOffFlavorPositive, val); return; }
void Yeast::setGlucoamylasePositive     (std::optional<bool>         const   val) { this->setAndNotify(PropertyNames::Yeast::glucoamylasePositive     , m_glucoamylasePositive     , val); return; }
void Yeast::setKillerProducingK1Toxin   (std::optional<bool>         const   val) { this->setAndNotify(PropertyNames::Yeast::killerProducingK1Toxin   , m_killerProducingK1Toxin   , val); return; }
void Yeast::setKillerProducingK2Toxin   (std::optional<bool>         const   val) { this->setAndNotify(PropertyNames::Yeast::killerProducingK2Toxin   , m_killerProducingK2Toxin   , val); return; }
void Yeast::setKillerProducingK28Toxin  (std::optional<bool>         const   val) { this->setAndNotify(PropertyNames::Yeast::killerProducingK28Toxin  , m_killerProducingK28Toxin  , val); return; }
void Yeast::setKillerProducingKlusToxin (std::optional<bool>         const   val) { this->setAndNotify(PropertyNames::Yeast::killerProducingKlusToxin , m_killerProducingKlusToxin , val); return; }
void Yeast::setKillerNeutral            (std::optional<bool>         const   val) { this->setAndNotify(PropertyNames::Yeast::killerNeutral            , m_killerNeutral            , val); return; }

void Yeast::setAmountWithUnits (MassOrVolumeAmt     const   val) {
   this->setAndNotify(PropertyNames::Yeast::amount        , this->m_amount        , val.quantity());
   this->setAndNotify(PropertyNames::Yeast::amountIsWeight, this->m_amountIsWeight, val.isMass()  );
   return;
}


double Yeast::getTypicalAttenuation_pct() const {
   if (m_attenuation_pct) {
      return *m_attenuation_pct;
   }
   if (m_attenuationMin_pct && m_attenuationMax_pct) {
      return *m_attenuationMin_pct + *m_attenuationMax_pct / 2.0;
   }
   return Yeast::DefaultAttenuation_pct;
}

Recipe * Yeast::getOwningRecipe() {
   return ObjectStoreWrapper::findFirstMatching<Recipe>( [this](Recipe * rec) {return rec->uses(*this);} );
}

void Yeast::setInventoryQuanta(int var) {
   this->setInventoryAmount(var);
   return;
}

// Insert the boiler-plate stuff for inventory
INVENTORY_COMMON_CODE(Yeast)
