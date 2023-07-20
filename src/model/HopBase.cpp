/*======================================================================================================================
 * model/HopBase.cpp is part of Brewken, and is copyright the following authors 2023:
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
#include "model/HopBase.h"

#include "model/NamedParameterBundle.h"

QString const HopBase::LocalisedName = tr("HopBase Base");

EnumStringMapping const HopBase::formStringMapping {
   {HopBase::Form::Leaf   , "leaf"      },
   {HopBase::Form::Pellet , "pellet"    },
   {HopBase::Form::Plug   , "plug"      },
   {HopBase::Form::Extract, "extract"   },
   {HopBase::Form::WetLeaf, "leaf (wet)"},
   {HopBase::Form::Powder , "powder"    },
};

EnumStringMapping const HopBase::formDisplayNames {
   {HopBase::Form::Leaf   , tr("Leaf"   )},
   {HopBase::Form::Pellet , tr("Pellet" )},
   {HopBase::Form::Plug   , tr("Plug"   )},
   {HopBase::Form::Extract, tr("Extract")},
   {HopBase::Form::WetLeaf, tr("WetLeaf")},
   {HopBase::Form::Powder , tr("Powder" )},
};

bool HopBase::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   HopBase const & rhs = static_cast<HopBase const &>(other);
   // Base class will already have ensured names are equal
   return (
      this->m_alpha_pct  == rhs.m_alpha_pct  &&
      this->m_form       == rhs.m_form       &&
      this->m_beta_pct   == rhs.m_beta_pct   &&
      this->m_origin     == rhs.m_origin     &&
      // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
      this->m_producer   == rhs.m_producer   &&
      this->m_product_id == rhs.m_product_id &&
      this->m_year       == rhs.m_year
   );
}

TypeLookup const HopBase::typeLookup {
   "HopBase",
   {
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::HopBase::alpha_pct , HopBase::m_alpha_pct , NonPhysicalQuantity::Percentage),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::HopBase::form      , HopBase::m_form      , NonPhysicalQuantity::Enum      ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::HopBase::beta_pct  , HopBase::m_beta_pct  , NonPhysicalQuantity::Percentage),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::HopBase::origin    , HopBase::m_origin    , NonPhysicalQuantity::String    ),
      // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::HopBase::producer  , HopBase::m_producer  , NonPhysicalQuantity::String    ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::HopBase::product_id, HopBase::m_product_id, NonPhysicalQuantity::String    ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::HopBase::year      , HopBase::m_year      , NonPhysicalQuantity::String    ),
   },
   // Parent class lookup
   &NamedEntity::typeLookup
};

HopBase::HopBase(QString name) :
   NamedEntity {name, true},
   m_alpha_pct {0.0},
   m_form      {std::nullopt},
   m_beta_pct  {std::nullopt},
   m_origin    {"" },
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   m_producer  {"" },
   m_product_id{"" },
   m_year      {"" } {
   return;
}

HopBase::HopBase(NamedParameterBundle const & namedParameterBundle) :
   NamedEntity{namedParameterBundle},
   m_alpha_pct {namedParameterBundle.val<double               >(PropertyNames::HopBase::alpha_pct )},
   m_form      {namedParameterBundle.optEnumVal<HopBase::Form >(PropertyNames::HopBase::form      )},
   m_beta_pct  {namedParameterBundle.val<std::optional<double>>(PropertyNames::HopBase::beta_pct  )},
   m_origin    {namedParameterBundle.val<QString              >(PropertyNames::HopBase::origin    )},
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   m_producer  {namedParameterBundle.val<QString              >(PropertyNames::HopBase::producer  )},
   m_product_id{namedParameterBundle.val<QString              >(PropertyNames::HopBase::product_id)},
   m_year      {namedParameterBundle.val<QString              >(PropertyNames::HopBase::year      )} {
   return;
}

HopBase::HopBase(HopBase const & other) :
   NamedEntity {other             },
   m_alpha_pct {other.m_alpha_pct },
   m_form      {other.m_form      },
   m_beta_pct  {other.m_beta_pct  },
   m_origin    {other.m_origin    },
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   m_producer  {other.m_producer  },
   m_product_id{other.m_product_id},
   m_year      {other.m_year      } {
   return;
}

HopBase::~HopBase() = default;

//============================================= "GETTER" MEMBER FUNCTIONS ==============================================
double                       HopBase::alpha_pct () const { return this->m_alpha_pct         ; }
std::optional<HopBase::Form> HopBase::form      () const { return this->m_form              ; }
std::optional<int>           HopBase::formAsInt () const { return Optional::toOptInt(m_form); }
std::optional<double>        HopBase::beta_pct  () const { return this->m_beta_pct          ; }
QString                      HopBase::origin    () const { return this->m_origin            ; }
// ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
QString                      HopBase::producer  () const { return this->m_producer          ; }
QString                      HopBase::product_id() const { return this->m_product_id        ; }
QString                      HopBase::year      () const { return this->m_year              ; }

//============================================= "SETTER" MEMBER FUNCTIONS ==============================================
void HopBase::setAlpha_pct (double                       const   val) { this->setAndNotify(PropertyNames::HopBase::alpha_pct , this->m_alpha_pct , this->enforceMinAndMax(val, "alpha", 0.0, 100.0)); }
void HopBase::setForm      (std::optional<HopBase::Form> const   val) { this->setAndNotify(PropertyNames::HopBase::form      , this->m_form      , val                                             ); }
void HopBase::setFormAsInt (std::optional<int>           const   val) { this->setAndNotify(PropertyNames::HopBase::form      , this->m_form      , Optional::fromOptInt<Form>(val)); }
void HopBase::setBeta_pct  (std::optional<double>        const   val) { this->setAndNotify(PropertyNames::HopBase::beta_pct  , this->m_beta_pct  , this->enforceMinAndMax(val, "beta",  0.0, 100.0)); }
void HopBase::setOrigin    (QString                      const & val) { this->setAndNotify(PropertyNames::HopBase::origin    , this->m_origin    , val                                             ); }
// ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
void HopBase::setProducer  (QString                      const & val) { this->setAndNotify(PropertyNames::HopBase::producer  , this->m_producer  , val                                             ); }
void HopBase::setProduct_id(QString                      const & val) { this->setAndNotify(PropertyNames::HopBase::product_id, this->m_product_id, val                                             ); }
void HopBase::setYear      (QString                      const   val) { this->setAndNotify(PropertyNames::HopBase::year      , this->m_year      , val                                             ); }
