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

std::array<Hop::Type, 7> const Hop::allTypes {
   Hop::Type::Bittering              ,
   Hop::Type::Aroma                  ,
   Hop::Type::AromaAndBittering      ,
   Hop::Type::Flavor                 ,
   Hop::Type::BitteringAndFlavor     ,
   Hop::Type::AromaAndFlavor         ,
   Hop::Type::AromaBitteringAndFlavor,
};

// Note that Hop::typeStringMapping and Hop::FormMapping are as defined by BeerJSON, but we also use them for the DB and
// for the UI.  We can't use them for BeerXML as it only supports subsets of these types.
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

std::array<Hop::Form, 6> const Hop::allForms {
   Hop::Form::Leaf   ,
   Hop::Form::Pellet ,
   Hop::Form::Plug   ,
   Hop::Form::Extract,
   Hop::Form::WetLeaf,
   Hop::Form::Powder ,
};

EnumStringMapping const Hop::formStringMapping {
   {Hop::Form::Leaf   , "leaf"      },
   {Hop::Form::Pellet , "pellet"    },
   {Hop::Form::Plug   , "plug"      },
   {Hop::Form::Extract, "extract"   },
   {Hop::Form::WetLeaf, "leaf (wet)"},
   {Hop::Form::Powder , "powder"    },
};

EnumStringMapping const Hop::formDisplayNames {
   {Hop::Form::Leaf   , tr("Leaf"   )},
   {Hop::Form::Pellet , tr("Pellet" )},
   {Hop::Form::Plug   , tr("Plug"   )},
   {Hop::Form::Extract, tr("Extract")},
   {Hop::Form::WetLeaf, tr("WetLeaf")},
   {Hop::Form::Powder , tr("Powder" )},
};

std::array<Hop::Use, 5> const Hop::allUses {
   Hop::Use::Mash      ,
   Hop::Use::First_Wort,
   Hop::Use::Boil      ,
   Hop::Use::Aroma     ,
   Hop::Use::Dry_Hop   ,
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
      this->m_form                  == rhs.m_form                  &&
      this->m_alpha_pct             == rhs.m_alpha_pct             &&
      this->m_beta_pct              == rhs.m_beta_pct              &&
      this->m_hsi_pct               == rhs.m_hsi_pct               &&
      this->m_origin                == rhs.m_origin                &&
      this->m_humulene_pct          == rhs.m_humulene_pct          &&
      this->m_caryophyllene_pct     == rhs.m_caryophyllene_pct     &&
      this->m_cohumulone_pct        == rhs.m_cohumulone_pct        &&
      this->m_myrcene_pct           == rhs.m_myrcene_pct           &&
      this->m_producer              == rhs.m_producer              &&
      this->m_product_id            == rhs.m_product_id            &&
      this->m_year                  == rhs.m_year                  &&
      this->m_total_oil_ml_per_100g == rhs.m_total_oil_ml_per_100g &&
      this->m_farnesene_pct         == rhs.m_farnesene_pct         &&
      this->m_geraniol_pct          == rhs.m_geraniol_pct          &&
      this->m_b_pinene_pct          == rhs.m_b_pinene_pct          &&
      this->m_linalool_pct          == rhs.m_linalool_pct          &&
      this->m_limonene_pct          == rhs.m_limonene_pct          &&
      this->m_nerol_pct             == rhs.m_nerol_pct             &&
      this->m_pinene_pct            == rhs.m_pinene_pct            &&
      this->m_polyphenols_pct       == rhs.m_polyphenols_pct       &&
      this->m_xanthohumol_pct       == rhs.m_xanthohumol_pct
   );
}

ObjectStore & Hop::getObjectStoreTypedInstance() const {
   return ObjectStoreTyped<Hop>::getInstance();
}

TypeLookup const Hop::typeLookup {
   "Hop",
   {
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::use                  , Hop::m_use                  ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::type                 , Hop::m_type                 ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::form                 , Hop::m_form                 ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::alpha_pct            , Hop::m_alpha_pct            ,           NonPhysicalQuantity::Percentage   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::amount_kg            , Hop::m_amount_kg            , Measurement::PhysicalQuantity::Mass         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::time_min             , Hop::m_time_min             , Measurement::PhysicalQuantity::Time         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::notes                , Hop::m_notes                ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::beta_pct             , Hop::m_beta_pct             ,           NonPhysicalQuantity::Percentage   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::hsi_pct              , Hop::m_hsi_pct              ,           NonPhysicalQuantity::Percentage   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::origin               , Hop::m_origin               ,           NonPhysicalQuantity::String       ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::substitutes          , Hop::m_substitutes          ,           NonPhysicalQuantity::String       ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::humulene_pct         , Hop::m_humulene_pct         ,           NonPhysicalQuantity::Percentage   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::caryophyllene_pct    , Hop::m_caryophyllene_pct    ,           NonPhysicalQuantity::Percentage   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::cohumulone_pct       , Hop::m_cohumulone_pct       ,           NonPhysicalQuantity::Percentage   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::myrcene_pct          , Hop::m_myrcene_pct          ,           NonPhysicalQuantity::Percentage   ),
      // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::producer             , Hop::m_producer             ,           NonPhysicalQuantity::String       ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::product_id           , Hop::m_product_id           ,           NonPhysicalQuantity::String       ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Hop::year                 , Hop::m_year                 ,           NonPhysicalQuantity::String       ),
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
   },
   // Parent class lookup.  NB: NamedEntityWithInventory not NamedEntity!
   &NamedEntityWithInventory::typeLookup
};
static_assert(std::is_base_of<NamedEntityWithInventory, Hop>::value);

Hop::Hop(QString name) :
   NamedEntityWithInventory{name, true},
   m_use                  {Hop::Use::Mash},
   m_type                 {Hop::Type::Bittering},
   m_form                 {Hop::Form::Leaf},
   m_alpha_pct            {0.0},
   m_amount_kg            {0.0},
   m_time_min             {0.0},
   m_notes                {"" },
   m_beta_pct             {0.0},
   m_hsi_pct              {0.0},
   m_origin               {"" },
   m_substitutes          {"" },
   m_humulene_pct         {0.0},
   m_caryophyllene_pct    {0.0},
   m_cohumulone_pct       {0.0},
   m_myrcene_pct          {0.0},
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   m_producer             {"" },
   m_product_id           {"" },
   m_year                 {"" },
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
   NamedEntityWithInventory{namedParameterBundle},
   m_use                  {namedParameterBundle.val<Hop::Use             >(PropertyNames::Hop::use                  )},
   m_type                 {namedParameterBundle.val<Hop::Type            >(PropertyNames::Hop::type                 )},
   m_form                 {namedParameterBundle.val<Hop::Form            >(PropertyNames::Hop::form                 )},
   m_alpha_pct            {namedParameterBundle.val<double               >(PropertyNames::Hop::alpha_pct            )},
   m_amount_kg            {namedParameterBundle.val<double               >(PropertyNames::Hop::amount_kg            )},
   m_time_min             {namedParameterBundle.val<double               >(PropertyNames::Hop::time_min             )},
   m_notes                {namedParameterBundle.val<QString              >(PropertyNames::Hop::notes                )},
   m_beta_pct             {namedParameterBundle.val<double               >(PropertyNames::Hop::beta_pct             )},
   m_hsi_pct              {namedParameterBundle.val<double               >(PropertyNames::Hop::hsi_pct              )},
   m_origin               {namedParameterBundle.val<QString              >(PropertyNames::Hop::origin               )},
   m_substitutes          {namedParameterBundle.val<QString              >(PropertyNames::Hop::substitutes          )},
   m_humulene_pct         {namedParameterBundle.val<double               >(PropertyNames::Hop::humulene_pct         )},
   m_caryophyllene_pct    {namedParameterBundle.val<double               >(PropertyNames::Hop::caryophyllene_pct    )},
   m_cohumulone_pct       {namedParameterBundle.val<double               >(PropertyNames::Hop::cohumulone_pct       )},
   m_myrcene_pct          {namedParameterBundle.val<double               >(PropertyNames::Hop::myrcene_pct          )},
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   m_producer             {namedParameterBundle.val<QString              >(PropertyNames::Hop::producer             )},
   m_product_id           {namedParameterBundle.val<QString              >(PropertyNames::Hop::product_id           )},
   m_year                 {namedParameterBundle.val<QString              >(PropertyNames::Hop::year                 )},
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
   return;
}

Hop::Hop(Hop const & other) :
   NamedEntityWithInventory{other                        },
   m_use                   {other.m_use                  },
   m_type                  {other.m_type                 },
   m_form                  {other.m_form                 },
   m_alpha_pct             {other.m_alpha_pct            },
   m_amount_kg             {other.m_amount_kg            },
   m_time_min              {other.m_time_min             },
   m_notes                 {other.m_notes                },
   m_beta_pct              {other.m_beta_pct             },
   m_hsi_pct               {other.m_hsi_pct              },
   m_origin                {other.m_origin               },
   m_substitutes           {other.m_substitutes          },
   m_humulene_pct          {other.m_humulene_pct         },
   m_caryophyllene_pct     {other.m_caryophyllene_pct    },
   m_cohumulone_pct        {other.m_cohumulone_pct       },
   m_myrcene_pct           {other.m_myrcene_pct          },
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   m_producer              {other.m_producer             },
   m_product_id            {other.m_product_id           },
   m_year                  {other.m_year                 },
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
Hop::Use                    Hop::use()                   const { return this->m_use;                   }
QString                     Hop::notes()                 const { return this->m_notes;                 }
Hop::Type                   Hop::type()                  const { return this->m_type;                  }
Hop::Form                   Hop::form()                  const { return this->m_form;                  }
QString                     Hop::origin()                const { return this->m_origin;                }
QString                     Hop::substitutes()           const { return this->m_substitutes;           }
double                      Hop::alpha_pct()             const { return this->m_alpha_pct;             }
double                      Hop::amount_kg()             const { return this->m_amount_kg;             }
double                      Hop::time_min()              const { return this->m_time_min;              }
double                      Hop::beta_pct()              const { return this->m_beta_pct;              }
double                      Hop::hsi_pct()               const { return this->m_hsi_pct;               }
double                      Hop::humulene_pct()          const { return this->m_humulene_pct;          }
double                      Hop::caryophyllene_pct()     const { return this->m_caryophyllene_pct;     }
double                      Hop::cohumulone_pct()        const { return this->m_cohumulone_pct;        }
double                      Hop::myrcene_pct()           const { return this->m_myrcene_pct;           }
// ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
QString                     Hop::producer()              const { return this->m_producer;              }
QString                     Hop::product_id()            const { return this->m_product_id;            }
QString                     Hop::year()                  const { return this->m_year;                  }
std::optional<double>       Hop::total_oil_ml_per_100g() const { return this->m_total_oil_ml_per_100g; }
std::optional<double>       Hop::farnesene_pct()         const { return this->m_farnesene_pct;         }
std::optional<double>       Hop::geraniol_pct()          const { return this->m_geraniol_pct;          }
std::optional<double>       Hop::b_pinene_pct()          const { return this->m_b_pinene_pct;          }
std::optional<double>       Hop::linalool_pct()          const { return this->m_linalool_pct;          }
std::optional<double>       Hop::limonene_pct()          const { return this->m_limonene_pct;          }
std::optional<double>       Hop::nerol_pct()             const { return this->m_nerol_pct;             }
std::optional<double>       Hop::pinene_pct()            const { return this->m_pinene_pct;            }
std::optional<double>       Hop::polyphenols_pct()       const { return this->m_polyphenols_pct;       }
std::optional<double>       Hop::xanthohumol_pct()       const { return this->m_xanthohumol_pct;       }

double Hop::inventory() const {
   return InventoryUtils::getAmount(*this);
}

//============================================= "SETTER" MEMBER FUNCTIONS ==============================================
void Hop::setAlpha_pct            (double                      const   val) { this->setAndNotify(PropertyNames::Hop::alpha_pct,             this->m_alpha_pct,             this->enforceMinAndMax(val, "alpha",                 0.0, 100.0)); }
void Hop::setAmount_kg            (double                      const   val) { this->setAndNotify(PropertyNames::Hop::amount_kg,             this->m_amount_kg,             this->enforceMin      (val, "amount")                           ); }
void Hop::setUse                  (Hop::Use                    const   val) { this->setAndNotify(PropertyNames::Hop::use,                   this->m_use,                   val                                                             ); }
void Hop::setTime_min             (double                      const   val) { this->setAndNotify(PropertyNames::Hop::time_min,              this->m_time_min,              this->enforceMin      (val, "time")                             ); }
void Hop::setNotes                (QString                     const & val) { this->setAndNotify(PropertyNames::Hop::notes,                 this->m_notes,                 val                                                             ); }
void Hop::setType                 (Hop::Type                   const   val) { this->setAndNotify(PropertyNames::Hop::type,                  this->m_type,                  val                                                             ); }
void Hop::setForm                 (Hop::Form                   const   val) { this->setAndNotify(PropertyNames::Hop::form,                  this->m_form,                  val                                                             ); }
void Hop::setBeta_pct             (double                      const   val) { this->setAndNotify(PropertyNames::Hop::beta_pct,              this->m_beta_pct,              this->enforceMinAndMax(val, "beta",                  0.0, 100.0)); }
void Hop::setHsi_pct              (double                      const   val) { this->setAndNotify(PropertyNames::Hop::hsi_pct,               this->m_hsi_pct,               this->enforceMinAndMax(val, "hsi",                   0.0, 100.0)); }
void Hop::setOrigin               (QString                     const & val) { this->setAndNotify(PropertyNames::Hop::origin,                this->m_origin,                val                                                             ); }
void Hop::setSubstitutes          (QString                     const & val) { this->setAndNotify(PropertyNames::Hop::substitutes,           this->m_substitutes,           val                                                             ); }
void Hop::setHumulene_pct         (double                      const   val) { this->setAndNotify(PropertyNames::Hop::humulene_pct,          this->m_humulene_pct,          this->enforceMinAndMax(val, "humulene",              0.0, 100.0)); }
void Hop::setCaryophyllene_pct    (double                      const   val) { this->setAndNotify(PropertyNames::Hop::caryophyllene_pct,     this->m_caryophyllene_pct,     this->enforceMinAndMax(val, "caryophyllene",         0.0, 100.0)); }
void Hop::setCohumulone_pct       (double                      const   val) { this->setAndNotify(PropertyNames::Hop::cohumulone_pct,        this->m_cohumulone_pct,        this->enforceMinAndMax(val, "cohumulone",            0.0, 100.0)); }
void Hop::setMyrcene_pct          (double                      const   val) { this->setAndNotify(PropertyNames::Hop::myrcene_pct,           this->m_myrcene_pct,           this->enforceMinAndMax(val, "myrcene",               0.0, 100.0)); }
// ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
void Hop::setProducer             (QString                     const & val) { this->setAndNotify(PropertyNames::Hop::producer,              this->m_producer,              val                                                             ); }
void Hop::setProduct_id           (QString                     const & val) { this->setAndNotify(PropertyNames::Hop::product_id,            this->m_product_id,            val                                                             ); }
void Hop::setYear                 (QString                     const   val) { this->setAndNotify(PropertyNames::Hop::year,                  this->m_year,                  val                                                             ); }
void Hop::setTotal_oil_ml_per_100g(std::optional<double      > const   val) { this->setAndNotify(PropertyNames::Hop::total_oil_ml_per_100g, this->m_total_oil_ml_per_100g, this->enforceMinAndMax(val, "total_oil_ml_per_100g", 0.0, 100.0)); }
void Hop::setFarnesene_pct        (std::optional<double      > const   val) { this->setAndNotify(PropertyNames::Hop::farnesene_pct,         this->m_farnesene_pct,         this->enforceMinAndMax(val, "farnesene_pct",         0.0, 100.0)); }
void Hop::setGeraniol_pct         (std::optional<double      > const   val) { this->setAndNotify(PropertyNames::Hop::geraniol_pct,          this->m_geraniol_pct,          this->enforceMinAndMax(val, "geraniol_pct",          0.0, 100.0)); }
void Hop::setB_pinene_pct         (std::optional<double      > const   val) { this->setAndNotify(PropertyNames::Hop::b_pinene_pct,          this->m_b_pinene_pct,          this->enforceMinAndMax(val, "b_pinene_pct",          0.0, 100.0)); }
void Hop::setLinalool_pct         (std::optional<double      > const   val) { this->setAndNotify(PropertyNames::Hop::linalool_pct,          this->m_linalool_pct,          this->enforceMinAndMax(val, "linalool_pct",          0.0, 100.0)); }
void Hop::setLimonene_pct         (std::optional<double      > const   val) { this->setAndNotify(PropertyNames::Hop::limonene_pct,          this->m_limonene_pct,          this->enforceMinAndMax(val, "limonene_pct",          0.0, 100.0)); }
void Hop::setNerol_pct            (std::optional<double      > const   val) { this->setAndNotify(PropertyNames::Hop::nerol_pct,             this->m_nerol_pct,             this->enforceMinAndMax(val, "nerol_pct",             0.0, 100.0)); }
void Hop::setPinene_pct           (std::optional<double      > const   val) { this->setAndNotify(PropertyNames::Hop::pinene_pct,            this->m_pinene_pct,            this->enforceMinAndMax(val, "pinene_pct",            0.0, 100.0)); }
void Hop::setPolyphenols_pct      (std::optional<double      > const   val) { this->setAndNotify(PropertyNames::Hop::polyphenols_pct,       this->m_polyphenols_pct,       this->enforceMinAndMax(val, "polyphenols_pct",       0.0, 100.0)); }
void Hop::setXanthohumol_pct      (std::optional<double      > const   val) { this->setAndNotify(PropertyNames::Hop::xanthohumol_pct,       this->m_xanthohumol_pct,       this->enforceMinAndMax(val, "xanthohumol_pct",       0.0, 100.0)); }

void Hop::setInventoryAmount(double num) { InventoryUtils::setAmount(*this, num); }


Recipe * Hop::getOwningRecipe() {
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
