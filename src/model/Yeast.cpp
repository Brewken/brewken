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

///namespace {
///   QStringList const types{"Ale", "Lager", "Wheat", "Wine", "Champagne"};
///   QStringList const forms{"Liquid", "Dry", "Slant", "Culture"};
///   QStringList const flocculations{"Low", "Medium", "High", "Very High"};
///   QStringList const typesTr{Yeast::tr("Ale"),
///                             Yeast::tr("Lager"),
///                             Yeast::tr("Wheat"),
///                             Yeast::tr("Wine"),
///                             Yeast::tr("Champagne")};
///   QStringList const formsTr {Yeast::tr("Liquid"),
///                              Yeast::tr("Dry"),
///                              Yeast::tr("Slant"),
///                              Yeast::tr("Culture")};
///   QStringList const flocculationsTr{Yeast::tr("Low"),
///                                     Yeast::tr("Medium"),
///                                     Yeast::tr("High"),
///                                     Yeast::tr("Very High")};
///}

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
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::type              , Yeast::m_type              ,           NonPhysicalQuantity::Enum         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::form              , Yeast::m_form              ,           NonPhysicalQuantity::Enum         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::flocculation      , Yeast::m_flocculation      ,           NonPhysicalQuantity::Enum         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::amount            , Yeast::m_amount            ,           NonPhysicalQuantity::Dimensionless), // Not really Dimensionless
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::amountIsWeight    , Yeast::m_amountIsWeight    ,           NonPhysicalQuantity::Bool         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::laboratory        , Yeast::m_laboratory        ,           NonPhysicalQuantity::String       ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::productID         , Yeast::m_productID         ,           NonPhysicalQuantity::String       ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::minTemperature_c  , Yeast::m_minTemperature_c  , Measurement::PhysicalQuantity::Temperature  ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::maxTemperature_c  , Yeast::m_maxTemperature_c  , Measurement::PhysicalQuantity::Temperature  ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::attenuation_pct   , Yeast::m_attenuation_pct   ,           NonPhysicalQuantity::Percentage   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::notes             , Yeast::m_notes             ,           NonPhysicalQuantity::String       ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::bestFor           , Yeast::m_bestFor           ,           NonPhysicalQuantity::String       ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::timesCultured     , Yeast::m_timesCultured     ,           NonPhysicalQuantity::Count        ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::maxReuse          , Yeast::m_maxReuse          ,           NonPhysicalQuantity::Count        ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::addToSecondary    , Yeast::m_addToSecondary    ,           NonPhysicalQuantity::Bool         ),
      // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞

      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::Yeast::amountWithUnits    , Yeast, amountWithUnits            , Measurement::PqEitherMassOrVolume             ),

//      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::flocculationString, Yeast::m_flocculationString),
//      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::formString        , Yeast::m_formString        ),
//      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Yeast::typeString        , Yeast::m_typeString        ),
   },
   // Parent class lookup.  NB: NamedEntityWithInventory not NamedEntity!
   &NamedEntityWithInventory::typeLookup
};
static_assert(std::is_base_of<NamedEntityWithInventory, Yeast>::value);


//============================CONSTRUCTORS======================================

Yeast::Yeast(QString name) :
   NamedEntityWithInventory{name, true},
   m_type                  {Yeast::Type::Ale},
   m_form                  {Yeast::Form::Liquid},
   m_flocculation          {Yeast::Flocculation::Low},
   m_amount                {0.0},
   m_amountIsWeight        {false},
   m_laboratory            {""},
   m_productID             {""},
   m_minTemperature_c      {0.0},
   m_maxTemperature_c      {0.0},
   m_attenuation_pct       {0.0},
   m_notes                 {""},
   m_bestFor               {""},
   m_timesCultured         {0},
   m_maxReuse              {0},
   m_addToSecondary        {false} {
   return;
}

Yeast::Yeast(NamedParameterBundle const & namedParameterBundle) :
   NamedEntityWithInventory{namedParameterBundle},
   m_type                  {namedParameterBundle.val<Yeast::Type        >(PropertyNames::Yeast::type            )},
   m_form                  {namedParameterBundle.val<Yeast::Form        >(PropertyNames::Yeast::form            )},
   m_flocculation          {namedParameterBundle.val<Yeast::Flocculation>(PropertyNames::Yeast::flocculation    )},
   m_amount                {namedParameterBundle.val<double             >(PropertyNames::Yeast::amount          )},
   m_amountIsWeight        {namedParameterBundle.val<bool               >(PropertyNames::Yeast::amountIsWeight  )},
   m_laboratory            {namedParameterBundle.val<QString            >(PropertyNames::Yeast::laboratory      )},
   m_productID             {namedParameterBundle.val<QString            >(PropertyNames::Yeast::productID       )},
   m_minTemperature_c      {namedParameterBundle.val<double             >(PropertyNames::Yeast::minTemperature_c)},
   m_maxTemperature_c      {namedParameterBundle.val<double             >(PropertyNames::Yeast::maxTemperature_c)},
   m_attenuation_pct       {namedParameterBundle.val<double             >(PropertyNames::Yeast::attenuation_pct )},
   m_notes                 {namedParameterBundle.val<QString            >(PropertyNames::Yeast::notes           )},
   m_bestFor               {namedParameterBundle.val<QString            >(PropertyNames::Yeast::bestFor         )},
   m_timesCultured         {namedParameterBundle.val<int                >(PropertyNames::Yeast::timesCultured   )},
   m_maxReuse              {namedParameterBundle.val<int                >(PropertyNames::Yeast::maxReuse        )},
   m_addToSecondary        {namedParameterBundle.val<bool               >(PropertyNames::Yeast::addToSecondary  )} {
   return;
}

Yeast::Yeast(Yeast const & other) :
   NamedEntityWithInventory{other                     },
   m_type                  {other.m_type              },
   m_form                  {other.m_form              },
   m_flocculation          {other.m_flocculation      },
   m_amount                {other.m_amount            },
   m_amountIsWeight        {other.m_amountIsWeight    },
   m_laboratory            {other.m_laboratory        },
   m_productID             {other.m_productID         },
   m_minTemperature_c      {other.m_minTemperature_c  },
   m_maxTemperature_c      {other.m_maxTemperature_c  },
   m_attenuation_pct       {other.m_attenuation_pct   },
   m_notes                 {other.m_notes             },
   m_bestFor               {other.m_bestFor           },
   m_timesCultured         {other.m_timesCultured     },
   m_maxReuse              {other.m_maxReuse          },
   m_addToSecondary        {other.m_addToSecondary    } {
   return;
}

Yeast::~Yeast() = default;

//============================================= "GETTER" MEMBER FUNCTIONS ==============================================
Yeast::Type         Yeast::type            () const { return m_type            ; }
Yeast::Form         Yeast::form            () const { return m_form            ; }
Yeast::Flocculation Yeast::flocculation    () const { return m_flocculation    ; }
double              Yeast::amount          () const { return m_amount          ; }
bool                Yeast::amountIsWeight  () const { return m_amountIsWeight  ; }
QString             Yeast::laboratory      () const { return m_laboratory      ; }
QString             Yeast::productID       () const { return m_productID       ; }
double              Yeast::minTemperature_c() const { return m_minTemperature_c; }
double              Yeast::maxTemperature_c() const { return m_maxTemperature_c; }
double              Yeast::attenuation_pct () const { return m_attenuation_pct ; }
QString             Yeast::notes           () const { return m_notes           ; }
QString             Yeast::bestFor         () const { return m_bestFor         ; }
int                 Yeast::timesCultured   () const { return m_timesCultured   ; }
int                 Yeast::maxReuse        () const { return m_maxReuse        ; }
bool                Yeast::addToSecondary  () const { return m_addToSecondary  ; }
// ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
MassOrVolumeAmt     Yeast::amountWithUnits () const { return MassOrVolumeAmt{this->m_amount, this->m_amountIsWeight ? Measurement::Units::kilograms : Measurement::Units::liters}; }

///const QString Yeast::typeString() const { return types.at(static_cast<int>(this->m_type)); }
///
///const QString Yeast::formString() const { return forms.at(static_cast<int>(this->m_form)); }
///
///const QString Yeast::flocculationString() const { return flocculations.at(static_cast<int>(this->m_flocculation)); }
///const QString Yeast::typeStringTr() const {
///   int myType = static_cast<int>(this->m_type);
///   Q_ASSERT(myType >= 0);
///   Q_ASSERT(myType < typesTr.size());
///   return typesTr.at(myType);
///}
///
///const QString Yeast::formStringTr() const {
///   int myForm = static_cast<int>(this->m_form);
///   Q_ASSERT(myForm >= 0);
///   Q_ASSERT(myForm < formsTr.size());
///   return formsTr.at(myForm);
///}
///
///const QString Yeast::flocculationStringTr() const {
///   int myFlocculation = static_cast<int>(this->m_flocculation);
///   Q_ASSERT(myFlocculation >= 0);
///   Q_ASSERT(myFlocculation < flocculationsTr.size());
///   return flocculationsTr.at(myFlocculation);
///}

//============================================= "SETTER" MEMBER FUNCTIONS ==============================================
// It seems a bit of overkill to enforce absolute zero as the lowest allowable temperature, but we do
void Yeast::setType            (Yeast::Type         const   val) { this->setAndNotify(PropertyNames::Yeast::type            , this->m_type            , val); return; }
void Yeast::setForm            (Yeast::Form         const   val) { this->setAndNotify(PropertyNames::Yeast::form            , this->m_form            , val); return; }
void Yeast::setAmount          (double              const   val) { this->setAndNotify(PropertyNames::Yeast::amount          , this->m_amount          , this->enforceMin      (val, "amount"         , 0.0)); return; }
void Yeast::setAmountIsWeight  (bool                const   val) { this->setAndNotify(PropertyNames::Yeast::amountIsWeight  , this->m_amountIsWeight  , val); return; }
void Yeast::setLaboratory      (QString             const & val) { this->setAndNotify(PropertyNames::Yeast::laboratory      , this->m_laboratory      , val); return; }
void Yeast::setProductID       (QString             const & val) { this->setAndNotify(PropertyNames::Yeast::productID       , this->m_productID       , val); return; }
void Yeast::setMinTemperature_c(double              const   val) { this->setAndNotify(PropertyNames::Yeast::minTemperature_c, this->m_minTemperature_c, this->enforceMin      (val, "max temp"       , PhysicalConstants::absoluteZero, 0.0  )); return; }
void Yeast::setMaxTemperature_c(double              const   val) { this->setAndNotify(PropertyNames::Yeast::maxTemperature_c, this->m_maxTemperature_c, this->enforceMin      (val, "max temp"       , PhysicalConstants::absoluteZero, 0.0  )); return; }
void Yeast::setFlocculation    (Yeast::Flocculation const   val) { this->setAndNotify(PropertyNames::Yeast::flocculation    , this->m_flocculation    , val); return; }
void Yeast::setAttenuation_pct (double              const   val) { this->setAndNotify(PropertyNames::Yeast::attenuation_pct , this->m_attenuation_pct , this->enforceMinAndMax(val, "pct attenuation", 0.0                            , 100.0, 0.0)); return; }
void Yeast::setNotes           (QString             const & val) { this->setAndNotify(PropertyNames::Yeast::notes           , this->m_notes           , val); return; }
void Yeast::setBestFor         (QString             const & val) { this->setAndNotify(PropertyNames::Yeast::bestFor         , this->m_bestFor         , val); return; }
void Yeast::setTimesCultured   (int                 const   val) { this->setAndNotify(PropertyNames::Yeast::timesCultured   , this->m_timesCultured   , this->enforceMin      (val, "times cultured" )); return; }
void Yeast::setMaxReuse        (int                 const   val) { this->setAndNotify(PropertyNames::Yeast::maxReuse        , this->m_maxReuse        , this->enforceMin      (val, "max reuse"      )); return; }
void Yeast::setAddToSecondary  (bool                const   val) { this->setAndNotify(PropertyNames::Yeast::addToSecondary  , this->m_addToSecondary  , val); return; }
// ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
void Yeast::setAmountWithUnits (MassOrVolumeAmt     const   val) {
   this->setAndNotify(PropertyNames::Yeast::amount        , this->m_amount        , val.quantity());
   this->setAndNotify(PropertyNames::Yeast::amountIsWeight, this->m_amountIsWeight, val.isMass()  );
   return;
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
