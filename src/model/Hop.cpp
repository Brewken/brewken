/*======================================================================================================================
 * model/Hop.cpp is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Kregg Kemper <gigatropolis@yahoo.com>
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
#include "model/Hop.h"

#include <QDebug>
#include <QObject>

#include "database/ObjectStoreWrapper.h"
#include "model/Inventory.h"
#include "model/NamedParameterBundle.h"
#include "model/Recipe.h"

QString const Hop::LocalisedName = tr("Hop");

// Note that Hop::typeStringMapping and Hop::formStringMapping are as defined by BeerJSON, but we also use them for the
// DB and for the UI.  We can't use them for BeerXML as it only supports subsets of these types.
EnumStringMapping const Hop::typeStringMapping {
   {Hop::Type::Bittering              , "bittering"              },
   {Hop::Type::Aroma                  , "aroma"                  },
   {Hop::Type::AromaAndBittering      , "aroma/bittering"        }, // Previous seralisation (still used for BeerXML) was "Both"
   {Hop::Type::Flavor                 , "flavor"                 },
   {Hop::Type::BitteringAndFlavor     , "bittering/flavor"       },
   {Hop::Type::AromaAndFlavor         , "aroma/flavor"           },
   {Hop::Type::AromaBitteringAndFlavor, "aroma/bittering/flavor" },
};

EnumStringMapping const Hop::typeDisplayNames {
   {Hop::Type::Bittering              , tr("Bittering"                )},
   {Hop::Type::Aroma                  , tr("Aroma"                    )},
   {Hop::Type::AromaAndBittering      , tr("Aroma & Bittering"        )},
   {Hop::Type::Flavor                 , tr("Flavor"                   )},
   {Hop::Type::BitteringAndFlavor     , tr("Bittering & Flavor"       )},
   {Hop::Type::AromaAndFlavor         , tr("Aroma & Flavor"           )},
   {Hop::Type::AromaBitteringAndFlavor, tr("Aroma, Bittering & Flavor")},
};


EnumStringMapping const Hop::useStringMapping {
   {Hop::Use::Mash      , "Mash"      },
   {Hop::Use::First_Wort, "First Wort"},
   {Hop::Use::Boil      , "Boil"      },
   {Hop::Use::Aroma     , "Aroma"     },
   {Hop::Use::Dry_Hop   , "Dry Hop"   },
};

EnumStringMapping const Hop::useDisplayNames {
   {Hop::Use::Mash      , tr("Mash"      )},
   {Hop::Use::First_Wort, tr("First Wort")},
   {Hop::Use::Boil      , tr("Boil"      )},
   {Hop::Use::Aroma     , tr("Post-Boil" )},
   {Hop::Use::Dry_Hop   , tr("Dry Hop"   )},
};

bool Hop::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   Hop const & rhs = static_cast<Hop const &>(other);
   // Base class will already have ensured names are equal
   return (
      this->m_use                   == rhs.m_use                   &&
      this->m_type                  == rhs.m_type                  &&
      this->m_hsi_pct               == rhs.m_hsi_pct               &&
      this->m_humulene_pct          == rhs.m_humulene_pct          &&
      this->m_caryophyllene_pct     == rhs.m_caryophyllene_pct     &&
      this->m_cohumulone_pct        == rhs.m_cohumulone_pct        &&
      this->m_myrcene_pct           == rhs.m_myrcene_pct           &&
      // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
      this->m_total_oil_ml_per_100g == rhs.m_total_oil_ml_per_100g &&
      this->m_farnesene_pct         == rhs.m_farnesene_pct         &&
      this->m_geraniol_pct          == rhs.m_geraniol_pct          &&
      this->m_b_pinene_pct          == rhs.m_b_pinene_pct          &&
      this->m_linalool_pct          == rhs.m_linalool_pct          &&
      this->m_limonene_pct          == rhs.m_limonene_pct          &&
      this->m_nerol_pct             == rhs.m_nerol_pct             &&
      this->m_pinene_pct            == rhs.m_pinene_pct            &&
      this->m_polyphenols_pct       == rhs.m_polyphenols_pct       &&
      this->m_xanthohumol_pct       == rhs.m_xanthohumol_pct       &&
      // Parent classes have to be equal too
      this->HopBase::isEqualTo(other)
   );
}

ObjectStore & Hop::getObjectStoreTypedInstance() const {
   return ObjectStoreTyped<Hop>::getInstance();
}

TypeLookup const Hop::typeLookup {
   "Hop",
   {
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::use                  , Hop::m_use                  ,           NonPhysicalQuantity::Enum         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::type                 , Hop::m_type                 ,           NonPhysicalQuantity::Enum         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::amount               , Hop::m_amount               , Measurement::PqEitherMassOrVolume           ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::amountIsWeight       , Hop::m_amountIsWeight       ,           NonPhysicalQuantity::Bool         ), // ⮜⮜⮜ Added for BeerJSON support ⮞⮞⮞
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::time_min             , Hop::m_time_min             , Measurement::PhysicalQuantity::Time         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::notes                , Hop::m_notes                ,           NonPhysicalQuantity::String       ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::hsi_pct              , Hop::m_hsi_pct              ,           NonPhysicalQuantity::Percentage   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::substitutes          , Hop::m_substitutes          ,           NonPhysicalQuantity::String       ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::humulene_pct         , Hop::m_humulene_pct         ,           NonPhysicalQuantity::Percentage   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::caryophyllene_pct    , Hop::m_caryophyllene_pct    ,           NonPhysicalQuantity::Percentage   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::cohumulone_pct       , Hop::m_cohumulone_pct       ,           NonPhysicalQuantity::Percentage   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::myrcene_pct          , Hop::m_myrcene_pct          ,           NonPhysicalQuantity::Percentage   ),
      // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::total_oil_ml_per_100g, Hop::m_total_oil_ml_per_100g,           NonPhysicalQuantity::Dimensionless), // Not really dimensionless...
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::farnesene_pct        , Hop::m_farnesene_pct        ,           NonPhysicalQuantity::Percentage   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::geraniol_pct         , Hop::m_geraniol_pct         ,           NonPhysicalQuantity::Percentage   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::b_pinene_pct         , Hop::m_b_pinene_pct         ,           NonPhysicalQuantity::Percentage   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::linalool_pct         , Hop::m_linalool_pct         ,           NonPhysicalQuantity::Percentage   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::limonene_pct         , Hop::m_limonene_pct         ,           NonPhysicalQuantity::Percentage   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::nerol_pct            , Hop::m_nerol_pct            ,           NonPhysicalQuantity::Percentage   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::pinene_pct           , Hop::m_pinene_pct           ,           NonPhysicalQuantity::Percentage   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::polyphenols_pct      , Hop::m_polyphenols_pct      ,           NonPhysicalQuantity::Percentage   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::xanthohumol_pct      , Hop::m_xanthohumol_pct      ,           NonPhysicalQuantity::Percentage   ),
      PROPERTIES_FOR_INVENTORY_TYPE_LOOKUP_DEFNS(Hop)
   },
   // Parent class lookup.  NB: HopBase not NamedEntity!
   &HopBase::typeLookup
///   // Parent class lookup.  NB: NamedEntityWithInventory not NamedEntity!
///   &NamedEntityWithInventory::typeLookup
};
///static_assert(std::is_base_of<NamedEntityWithInventory, Hop>::value);
static_assert(std::is_base_of<HopBase, Hop>::value);

Hop::Hop(QString name) :
///   NamedEntityWithInventory{name, true},
   HopBase                {name},
   PropertiesForInventory<Hop>{},
   m_use                  {std::nullopt},
   m_type                 {std::nullopt},
   m_amount               {0.0         },
   m_amountIsWeight       {true        }, // ⮜⮜⮜ Added for BeerJSON support ⮞⮞⮞
   m_time_min             {0.0         },
   m_notes                {""          },
   m_hsi_pct              {0.0         },
   m_substitutes          {""          },
   m_humulene_pct         {0.0         },
   m_caryophyllene_pct    {0.0         },
   m_cohumulone_pct       {0.0         },
   m_myrcene_pct          {0.0         },
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   m_total_oil_ml_per_100g{std::nullopt},
   m_farnesene_pct        {std::nullopt},
   m_geraniol_pct         {std::nullopt},
   m_b_pinene_pct         {std::nullopt},
   m_linalool_pct         {std::nullopt},
   m_limonene_pct         {std::nullopt},
   m_nerol_pct            {std::nullopt},
   m_pinene_pct           {std::nullopt},
   m_polyphenols_pct      {std::nullopt},
   m_xanthohumol_pct      {std::nullopt} {
   return;
}

Hop::Hop(NamedParameterBundle const & namedParameterBundle) :
///   NamedEntityWithInventory{namedParameterBundle},
   HopBase                {namedParameterBundle},
   PropertiesForInventory<Hop>{},
   m_use                  {namedParameterBundle.optEnumVal<Hop::Use      >(PropertyNames::Hop::use                  )},
   m_type                 {namedParameterBundle.optEnumVal<Hop::Type     >(PropertyNames::Hop::type                 )},
   m_time_min             {namedParameterBundle.val<double               >(PropertyNames::Hop::time_min             )},
   m_notes                {namedParameterBundle.val<QString              >(PropertyNames::Hop::notes                )},
   m_hsi_pct              {namedParameterBundle.val<double               >(PropertyNames::Hop::hsi_pct              )},
   m_substitutes          {namedParameterBundle.val<QString              >(PropertyNames::Hop::substitutes          )},
   m_humulene_pct         {namedParameterBundle.val<double               >(PropertyNames::Hop::humulene_pct         )},
   m_caryophyllene_pct    {namedParameterBundle.val<double               >(PropertyNames::Hop::caryophyllene_pct    )},
   m_cohumulone_pct       {namedParameterBundle.val<double               >(PropertyNames::Hop::cohumulone_pct       )},
   m_myrcene_pct          {namedParameterBundle.val<double               >(PropertyNames::Hop::myrcene_pct          )},
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   m_total_oil_ml_per_100g{namedParameterBundle.val<std::optional<double>>(PropertyNames::Hop::total_oil_ml_per_100g)},
   m_farnesene_pct        {namedParameterBundle.val<std::optional<double>>(PropertyNames::Hop::farnesene_pct        )},
   m_geraniol_pct         {namedParameterBundle.val<std::optional<double>>(PropertyNames::Hop::geraniol_pct         )},
   m_b_pinene_pct         {namedParameterBundle.val<std::optional<double>>(PropertyNames::Hop::b_pinene_pct         )},
   m_linalool_pct         {namedParameterBundle.val<std::optional<double>>(PropertyNames::Hop::linalool_pct         )},
   m_limonene_pct         {namedParameterBundle.val<std::optional<double>>(PropertyNames::Hop::limonene_pct         )},
   m_nerol_pct            {namedParameterBundle.val<std::optional<double>>(PropertyNames::Hop::nerol_pct            )},
   m_pinene_pct           {namedParameterBundle.val<std::optional<double>>(PropertyNames::Hop::pinene_pct           )},
   m_polyphenols_pct      {namedParameterBundle.val<std::optional<double>>(PropertyNames::Hop::polyphenols_pct      )},
   m_xanthohumol_pct      {namedParameterBundle.val<std::optional<double>>(PropertyNames::Hop::xanthohumol_pct      )} {
   this->setEitherOrReqParams<MassOrVolumeAmt             >(namedParameterBundle, PropertyNames::Hop::amount    , PropertyNames::Hop::amountIsWeight           , PropertyNames::Hop::amountWithUnits    , this->m_amount    , this->m_amountIsWeight           );
   return;
}

Hop::Hop(Hop const & other) :
///   NamedEntityWithInventory{other                        },
   HopBase                 {other                        },
   PropertiesForInventory<Hop>{},
   m_use                   {other.m_use                  },
   m_type                  {other.m_type                 },
   m_amount                {other.m_amount               },
   m_amountIsWeight        {other.m_amountIsWeight       }, // ⮜⮜⮜ Added for BeerJSON support ⮞⮞⮞
   m_time_min              {other.m_time_min             },
   m_notes                 {other.m_notes                },
   m_hsi_pct               {other.m_hsi_pct              },
   m_substitutes           {other.m_substitutes          },
   m_humulene_pct          {other.m_humulene_pct         },
   m_caryophyllene_pct     {other.m_caryophyllene_pct    },
   m_cohumulone_pct        {other.m_cohumulone_pct       },
   m_myrcene_pct           {other.m_myrcene_pct          },
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   m_total_oil_ml_per_100g {other.m_total_oil_ml_per_100g},
   m_farnesene_pct         {other.m_farnesene_pct        },
   m_geraniol_pct          {other.m_geraniol_pct         },
   m_b_pinene_pct          {other.m_b_pinene_pct         },
   m_linalool_pct          {other.m_linalool_pct         },
   m_limonene_pct          {other.m_limonene_pct         },
   m_nerol_pct             {other.m_nerol_pct            },
   m_pinene_pct            {other.m_pinene_pct           },
   m_polyphenols_pct       {other.m_polyphenols_pct      },
   m_xanthohumol_pct       {other.m_xanthohumol_pct      } {
   return;
}

Hop::~Hop() = default;

//============================================= "GETTER" MEMBER FUNCTIONS ==============================================
double                   Hop::amount               () const { return this->m_amount               ; }
bool                     Hop::amountIsWeight       () const { return this->m_amountIsWeight       ; } // ⮜⮜⮜ Added for BeerJSON support ⮞⮞⮞
std::optional<Hop::Use>  Hop::use                  () const { return this->m_use                  ; }
std::optional<int>       Hop::useAsInt             () const { return Optional::toOptInt(m_use)    ; }
double                   Hop::time_min             () const { return this->m_time_min             ; }
QString                  Hop::notes                () const { return this->m_notes                ; }
std::optional<Hop::Type> Hop::type                 () const { return this->m_type                 ; }
std::optional<int>       Hop::typeAsInt            () const { return Optional::toOptInt(m_type)   ; }
std::optional<double>    Hop::hsi_pct              () const { return this->m_hsi_pct              ; }
QString                  Hop::substitutes          () const { return this->m_substitutes          ; }
std::optional<double>    Hop::humulene_pct         () const { return this->m_humulene_pct         ; }
std::optional<double>    Hop::caryophyllene_pct    () const { return this->m_caryophyllene_pct    ; }
std::optional<double>    Hop::cohumulone_pct       () const { return this->m_cohumulone_pct       ; }
std::optional<double>    Hop::myrcene_pct          () const { return this->m_myrcene_pct          ; }
// ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
std::optional<double>   Hop::total_oil_ml_per_100g() const { return this->m_total_oil_ml_per_100g; }
std::optional<double>   Hop::farnesene_pct        () const { return this->m_farnesene_pct        ; }
std::optional<double>   Hop::geraniol_pct         () const { return this->m_geraniol_pct         ; }
std::optional<double>   Hop::b_pinene_pct         () const { return this->m_b_pinene_pct         ; }
std::optional<double>   Hop::linalool_pct         () const { return this->m_linalool_pct         ; }
std::optional<double>   Hop::limonene_pct         () const { return this->m_limonene_pct         ; }
std::optional<double>   Hop::nerol_pct            () const { return this->m_nerol_pct            ; }
std::optional<double>   Hop::pinene_pct           () const { return this->m_pinene_pct           ; }
std::optional<double>   Hop::polyphenols_pct      () const { return this->m_polyphenols_pct      ; }
std::optional<double>   Hop::xanthohumol_pct      () const { return this->m_xanthohumol_pct      ; }

// Combined getters (all added for BeerJSON support)
MassOrVolumeAmt         Hop::amountWithUnits      () const { return MassOrVolumeAmt{this->m_amount, this->m_amountIsWeight ? Measurement::Units::kilograms : Measurement::Units::liters}; }

//============================================= "SETTER" MEMBER FUNCTIONS ==============================================
void Hop::setAmount               (double                   const   val) { this->setAndNotify(PropertyNames::Hop::amount               , this->m_amount               , this->enforceMin      (val, "amount"));                     return; }
void Hop::setAmountIsWeight       (bool                     const   val) { this->setAndNotify(PropertyNames::Hop::amountIsWeight       , this->m_amountIsWeight       , val); return; } // ⮜⮜⮜ Added for BeerJSON support ⮞⮞⮞

void Hop::setUse                  (std::optional<Hop::Use>  const   val) { this->setAndNotify(PropertyNames::Hop::use                  , this->m_use                  , val                                                             ); return; }
void Hop::setUseAsInt             (std::optional<int>       const   val) { this->setAndNotify(PropertyNames::Hop::use                  , this->m_use                  , Optional::fromOptInt<Use>(val));                                   return; }
void Hop::setTime_min             (double                   const   val) { this->setAndNotify(PropertyNames::Hop::time_min             , this->m_time_min             , this->enforceMin      (val, "time")                             ); return; }
void Hop::setNotes                (QString                  const & val) { this->setAndNotify(PropertyNames::Hop::notes                , this->m_notes                , val                                                             ); return; }
void Hop::setType                 (std::optional<Hop::Type> const   val) { this->setAndNotify(PropertyNames::Hop::type                 , this->m_type                 , val                                                             ); return; }
void Hop::setTypeAsInt            (std::optional<int>       const   val) { this->setAndNotify(PropertyNames::Hop::type                 , this->m_type                 , Optional::fromOptInt<Type>(val));                                  return; }
void Hop::setHsi_pct              (std::optional<double>    const   val) { this->setAndNotify(PropertyNames::Hop::hsi_pct              , this->m_hsi_pct              , this->enforceMinAndMax(val, "hsi",                   0.0, 100.0)); return; }
void Hop::setSubstitutes          (QString                  const & val) { this->setAndNotify(PropertyNames::Hop::substitutes          , this->m_substitutes          , val                                                             ); return; }
void Hop::setHumulene_pct         (std::optional<double>    const   val) { this->setAndNotify(PropertyNames::Hop::humulene_pct         , this->m_humulene_pct         , this->enforceMinAndMax(val, "humulene",              0.0, 100.0)); return; }
void Hop::setCaryophyllene_pct    (std::optional<double>    const   val) { this->setAndNotify(PropertyNames::Hop::caryophyllene_pct    , this->m_caryophyllene_pct    , this->enforceMinAndMax(val, "caryophyllene",         0.0, 100.0)); return; }
void Hop::setCohumulone_pct       (std::optional<double>    const   val) { this->setAndNotify(PropertyNames::Hop::cohumulone_pct       , this->m_cohumulone_pct       , this->enforceMinAndMax(val, "cohumulone",            0.0, 100.0)); return; }
void Hop::setMyrcene_pct          (std::optional<double>    const   val) { this->setAndNotify(PropertyNames::Hop::myrcene_pct          , this->m_myrcene_pct          , this->enforceMinAndMax(val, "myrcene",               0.0, 100.0)); return; }
// ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
void Hop::setTotal_oil_ml_per_100g(std::optional<double>    const   val) { this->setAndNotify(PropertyNames::Hop::total_oil_ml_per_100g, this->m_total_oil_ml_per_100g, this->enforceMinAndMax(val, "total_oil_ml_per_100g", 0.0, 100.0)); return; }
void Hop::setFarnesene_pct        (std::optional<double>    const   val) { this->setAndNotify(PropertyNames::Hop::farnesene_pct        , this->m_farnesene_pct        , this->enforceMinAndMax(val, "farnesene_pct",         0.0, 100.0)); return; }
void Hop::setGeraniol_pct         (std::optional<double>    const   val) { this->setAndNotify(PropertyNames::Hop::geraniol_pct         , this->m_geraniol_pct         , this->enforceMinAndMax(val, "geraniol_pct",          0.0, 100.0)); return; }
void Hop::setB_pinene_pct         (std::optional<double>    const   val) { this->setAndNotify(PropertyNames::Hop::b_pinene_pct         , this->m_b_pinene_pct         , this->enforceMinAndMax(val, "b_pinene_pct",          0.0, 100.0)); return; }
void Hop::setLinalool_pct         (std::optional<double>    const   val) { this->setAndNotify(PropertyNames::Hop::linalool_pct         , this->m_linalool_pct         , this->enforceMinAndMax(val, "linalool_pct",          0.0, 100.0)); return; }
void Hop::setLimonene_pct         (std::optional<double>    const   val) { this->setAndNotify(PropertyNames::Hop::limonene_pct         , this->m_limonene_pct         , this->enforceMinAndMax(val, "limonene_pct",          0.0, 100.0)); return; }
void Hop::setNerol_pct            (std::optional<double>    const   val) { this->setAndNotify(PropertyNames::Hop::nerol_pct            , this->m_nerol_pct            , this->enforceMinAndMax(val, "nerol_pct",             0.0, 100.0)); return; }
void Hop::setPinene_pct           (std::optional<double>    const   val) { this->setAndNotify(PropertyNames::Hop::pinene_pct           , this->m_pinene_pct           , this->enforceMinAndMax(val, "pinene_pct",            0.0, 100.0)); return; }
void Hop::setPolyphenols_pct      (std::optional<double>    const   val) { this->setAndNotify(PropertyNames::Hop::polyphenols_pct      , this->m_polyphenols_pct      , this->enforceMinAndMax(val, "polyphenols_pct",       0.0, 100.0)); return; }
void Hop::setXanthohumol_pct      (std::optional<double>    const   val) { this->setAndNotify(PropertyNames::Hop::xanthohumol_pct      , this->m_xanthohumol_pct      , this->enforceMinAndMax(val, "xanthohumol_pct",       0.0, 100.0)); return; }

Recipe * Hop::getOwningRecipe() const {
   return ObjectStoreWrapper::findFirstMatching<Recipe>( [this](Recipe * rec) {return rec->uses(*this);} );
}

bool hopLessThanByTime(Hop const * const lhs, Hop const * const rhs) {
   if (lhs->use() == rhs->use())    {
      if (lhs->time_min() == rhs->time_min()) {
         return lhs->name() < rhs->name();
      }
      return lhs->time_min() > rhs->time_min();
   }
   return lhs->use() < rhs->use();
}

// Insert the boiler-plate stuff for inventory
///INVENTORY_COMMON_CODE_MO(Hop)
PROPERTIES_FOR_INVENTORY_COMMON_CODE(Hop)
