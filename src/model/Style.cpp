/*======================================================================================================================
 * model/Style.cpp is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Matt Young <mfsy@yahoo.com>
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
 =====================================================================================================================*/
#include "model/Style.h"

#include <QDebug>

#include "database/ObjectStoreWrapper.h"
#include "model/NamedParameterBundle.h"
#include "model/Recipe.h"

EnumStringMapping const Style::typeStringMapping {
   {Style::Type::Lager        , "lager"      },
   {Style::Type::Ale          , "ale"        },
   {Style::Type::Mead         , "mead"       },
   {Style::Type::Wheat        , "wheat"      },
   {Style::Type::Mixed        , "mixed"      },
   {Style::Type::Cider        , "cider"      },
};
EnumStringMapping const Style::typeDisplayNames{
   {Style::Type::Lager        , tr("Lager")      },
   {Style::Type::Ale          , tr("Ale"  )      },
   {Style::Type::Mead         , tr("Mead" )      },
   {Style::Type::Wheat        , tr("Wheat")      },
   {Style::Type::Mixed        , tr("Mixed")      },
   {Style::Type::Cider        , tr("Cider")      },
};

bool Style::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   Style const & rhs = static_cast<Style const &>(other);
   // Base class will already have ensured names are equal
   return (
      this->m_category       == rhs.m_category       &&
      this->m_categoryNumber == rhs.m_categoryNumber &&
      this->m_styleLetter    == rhs.m_styleLetter    &&
      this->m_styleGuide     == rhs.m_styleGuide     &&
      this->m_type           == rhs.m_type
   );
}

ObjectStore & Style::getObjectStoreTypedInstance() const {
   return ObjectStoreTyped<Style>::getInstance();
}

TypeLookup const Style::typeLookup {
   "Style",
   {
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Style::category      , Style::m_category      ,           NonPhysicalQuantity::String     ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Style::categoryNumber, Style::m_categoryNumber,           NonPhysicalQuantity::String     ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Style::styleLetter   , Style::m_styleLetter   ,           NonPhysicalQuantity::String     ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Style::styleGuide    , Style::m_styleGuide    ,           NonPhysicalQuantity::String     ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Style::type          , Style::m_type          ,           NonPhysicalQuantity::Enum       ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Style::ogMin         , Style::m_ogMin         , Measurement::PhysicalQuantity::Density    ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Style::ogMax         , Style::m_ogMax         , Measurement::PhysicalQuantity::Density    ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Style::fgMin         , Style::m_fgMin         , Measurement::PhysicalQuantity::Density    ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Style::fgMax         , Style::m_fgMax         , Measurement::PhysicalQuantity::Density    ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Style::ibuMin        , Style::m_ibuMin        , Measurement::PhysicalQuantity::Bitterness ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Style::ibuMax        , Style::m_ibuMax        , Measurement::PhysicalQuantity::Bitterness ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Style::colorMin_srm  , Style::m_colorMin_srm  , Measurement::PhysicalQuantity::Color      ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Style::colorMax_srm  , Style::m_colorMax_srm  , Measurement::PhysicalQuantity::Color      ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Style::carbMin_vol   , Style::m_carbMin_vol   , Measurement::PhysicalQuantity::Carbonation),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Style::carbMax_vol   , Style::m_carbMax_vol   , Measurement::PhysicalQuantity::Carbonation),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Style::abvMin_pct    , Style::m_abvMin_pct    ,           NonPhysicalQuantity::Percentage ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Style::abvMax_pct    , Style::m_abvMax_pct    ,           NonPhysicalQuantity::Percentage ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Style::notes         , Style::m_notes         ,           NonPhysicalQuantity::String     ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Style::profile       , Style::m_profile       ,           NonPhysicalQuantity::String     ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Style::ingredients   , Style::m_ingredients   ,           NonPhysicalQuantity::String     ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Style::examples      , Style::m_examples      ,           NonPhysicalQuantity::String     ),
//      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Style::typeString    , Style::m_typeString    ),
   },
   // Parent class lookup
   &NamedEntity::typeLookup
};

//====== Constructors =========

// suitable for something that will be written to the db later
Style::Style(QString name) :
   NamedEntity     {name, true        },
   m_category      {""                },
   m_categoryNumber{""                },
   m_styleLetter   {""                },
   m_styleGuide    {""                },
   m_type          {Style::Type::Lager},
   m_ogMin         {0.0               },
   m_ogMax         {0.0               },
   m_fgMin         {0.0               },
   m_fgMax         {0.0               },
   m_ibuMin        {0.0               },
   m_ibuMax        {0.0               },
   m_colorMin_srm  {0.0               },
   m_colorMax_srm  {0.0               },
   m_carbMin_vol   {std::nullopt      },
   m_carbMax_vol   {std::nullopt      },
   m_abvMin_pct    {std::nullopt      },
   m_abvMax_pct    {std::nullopt      },
   m_notes         {""                },
   m_profile       {""                },
   m_ingredients   {""                },
   m_examples      {""                } {
   return;
}

Style::Style(NamedParameterBundle const & namedParameterBundle) :
   NamedEntity     {namedParameterBundle},
   m_category      {namedParameterBundle.val<QString              >(PropertyNames::Style::category      )},
   m_categoryNumber{namedParameterBundle.val<QString              >(PropertyNames::Style::categoryNumber)},
   m_styleLetter   {namedParameterBundle.val<QString              >(PropertyNames::Style::styleLetter   )},
   m_styleGuide    {namedParameterBundle.val<QString              >(PropertyNames::Style::styleGuide    )},
   m_type          {namedParameterBundle.val<Style::Type          >(PropertyNames::Style::type          )},
   m_ogMin         {namedParameterBundle.val<double               >(PropertyNames::Style::ogMin         )},
   m_ogMax         {namedParameterBundle.val<double               >(PropertyNames::Style::ogMax         )},
   m_fgMin         {namedParameterBundle.val<double               >(PropertyNames::Style::fgMin         )},
   m_fgMax         {namedParameterBundle.val<double               >(PropertyNames::Style::fgMax         )},
   m_ibuMin        {namedParameterBundle.val<double               >(PropertyNames::Style::ibuMin        )},
   m_ibuMax        {namedParameterBundle.val<double               >(PropertyNames::Style::ibuMax        )},
   m_colorMin_srm  {namedParameterBundle.val<double               >(PropertyNames::Style::colorMin_srm  )},
   m_colorMax_srm  {namedParameterBundle.val<double               >(PropertyNames::Style::colorMax_srm  )},
   m_carbMin_vol   {namedParameterBundle.val<std::optional<double>>(PropertyNames::Style::carbMin_vol   )},
   m_carbMax_vol   {namedParameterBundle.val<std::optional<double>>(PropertyNames::Style::carbMax_vol   )},
   m_abvMin_pct    {namedParameterBundle.val<std::optional<double>>(PropertyNames::Style::abvMin_pct    )},
   m_abvMax_pct    {namedParameterBundle.val<std::optional<double>>(PropertyNames::Style::abvMax_pct    )},
   m_notes         {namedParameterBundle.val<QString              >(PropertyNames::Style::notes         )},
   m_profile       {namedParameterBundle.val<QString              >(PropertyNames::Style::profile       )},
   m_ingredients   {namedParameterBundle.val<QString              >(PropertyNames::Style::ingredients   )},
   m_examples      {namedParameterBundle.val<QString              >(PropertyNames::Style::examples      )} {
   return;
}

Style::Style(Style const & other) :
   NamedEntity{other},
   m_category      {other.m_category      },
   m_categoryNumber{other.m_categoryNumber},
   m_styleLetter   {other.m_styleLetter   },
   m_styleGuide    {other.m_styleGuide    },
   m_type          {other.m_type          },
   m_ogMin         {other.m_ogMin         },
   m_ogMax         {other.m_ogMax         },
   m_fgMin         {other.m_fgMin         },
   m_fgMax         {other.m_fgMax         },
   m_ibuMin        {other.m_ibuMin        },
   m_ibuMax        {other.m_ibuMax        },
   m_colorMin_srm  {other.m_colorMin_srm  },
   m_colorMax_srm  {other.m_colorMax_srm  },
   m_carbMin_vol   {other.m_carbMin_vol   },
   m_carbMax_vol   {other.m_carbMax_vol   },
   m_abvMin_pct    {other.m_abvMin_pct    },
   m_abvMax_pct    {other.m_abvMax_pct    },
   m_notes         {other.m_notes         },
   m_profile       {other.m_profile       },
   m_ingredients   {other.m_ingredients   },
   m_examples      {other.m_examples      } {
   return;
}

Style::~Style() = default;

//============================================= "GETTER" MEMBER FUNCTIONS ==============================================
QString               Style::category      () const { return m_category      ; }
QString               Style::categoryNumber() const { return m_categoryNumber; }
QString               Style::styleLetter   () const { return m_styleLetter   ; }
QString               Style::styleGuide    () const { return m_styleGuide    ; }
Style::Type           Style::type          () const { return m_type          ; }
double                Style::ogMin         () const { return m_ogMin         ; }
double                Style::ogMax         () const { return m_ogMax         ; }
double                Style::fgMin         () const { return m_fgMin         ; }
double                Style::fgMax         () const { return m_fgMax         ; }
double                Style::ibuMin        () const { return m_ibuMin        ; }
double                Style::ibuMax        () const { return m_ibuMax        ; }
double                Style::colorMin_srm  () const { return m_colorMin_srm  ; }
double                Style::colorMax_srm  () const { return m_colorMax_srm  ; }
std::optional<double> Style::carbMin_vol   () const { return m_carbMin_vol   ; }
std::optional<double> Style::carbMax_vol   () const { return m_carbMax_vol   ; }
std::optional<double> Style::abvMin_pct    () const { return m_abvMin_pct    ; }
std::optional<double> Style::abvMax_pct    () const { return m_abvMax_pct    ; }
QString               Style::notes         () const { return m_notes         ; }
QString               Style::profile       () const { return m_profile       ; }
QString               Style::ingredients   () const { return m_ingredients   ; }
QString               Style::examples      () const { return m_examples      ; }

//==============================="SET" METHODS==================================
void Style::setCategory      (QString               const & val) { this->setAndNotify(PropertyNames::Style::category      , this->m_category      , val); }
void Style::setCategoryNumber(QString               const & val) { this->setAndNotify(PropertyNames::Style::categoryNumber, this->m_categoryNumber, val); }
void Style::setStyleLetter   (QString               const & val) { this->setAndNotify(PropertyNames::Style::styleLetter   , this->m_styleLetter   , val); }
void Style::setStyleGuide    (QString               const & val) { this->setAndNotify(PropertyNames::Style::styleGuide    , this->m_styleGuide    , val); }
void Style::setType          (Type                  const   val) { this->setAndNotify(PropertyNames::Style::type          , this->m_type          , val); }
void Style::setOgMin         (double                const   val) { this->setAndNotify(PropertyNames::Style::ogMin         , this->m_ogMin         , this->enforceMin(val, "og min"      )); }
void Style::setOgMax         (double                const   val) { this->setAndNotify(PropertyNames::Style::ogMax         , this->m_ogMax         , this->enforceMin(val, "og max"      )); }
void Style::setFgMin         (double                const   val) { this->setAndNotify(PropertyNames::Style::fgMin         , this->m_fgMin         , this->enforceMin(val, "fg min"      )); }
void Style::setFgMax         (double                const   val) { this->setAndNotify(PropertyNames::Style::fgMax         , this->m_fgMax         , this->enforceMin(val, "fg max"      )); }
void Style::setIbuMin        (double                const   val) { this->setAndNotify(PropertyNames::Style::ibuMin        , this->m_ibuMin        , this->enforceMin(val, "ibu min"     )); }
void Style::setIbuMax        (double                const   val) { this->setAndNotify(PropertyNames::Style::ibuMax        , this->m_ibuMax        , this->enforceMin(val, "ibu max"     )); }
void Style::setColorMin_srm  (double                const   val) { this->setAndNotify(PropertyNames::Style::colorMin_srm  , this->m_colorMin_srm  , this->enforceMin(val, "color min"   )); }
void Style::setColorMax_srm  (double                const   val) { this->setAndNotify(PropertyNames::Style::colorMax_srm  , this->m_colorMax_srm  , this->enforceMin(val, "color max"   )); }
void Style::setCarbMin_vol   (std::optional<double> const   val) { this->setAndNotify(PropertyNames::Style::carbMin_vol   , this->m_carbMin_vol   , this->enforceMin(val, "carb vol min")); }
void Style::setCarbMax_vol   (std::optional<double> const   val) { this->setAndNotify(PropertyNames::Style::carbMax_vol   , this->m_carbMax_vol   , this->enforceMin(val, "carb vol max")); }
void Style::setAbvMin_pct    (std::optional<double> const   val) { this->setAndNotify(PropertyNames::Style::abvMin_pct    , this->m_abvMin_pct    , this->enforceMinAndMax(val, "min abv pct", 0.0, 100.0)); }
void Style::setAbvMax_pct    (std::optional<double> const   val) { this->setAndNotify(PropertyNames::Style::abvMax_pct    , this->m_abvMax_pct    , this->enforceMinAndMax(val, "max abv pct", 0.0, 100.0)); }
void Style::setNotes         (QString               const & val) { this->setAndNotify(PropertyNames::Style::notes         , this->m_notes         , val); }
void Style::setProfile       (QString               const & val) { this->setAndNotify(PropertyNames::Style::profile       , this->m_profile       , val); }
void Style::setIngredients   (QString               const & val) { this->setAndNotify(PropertyNames::Style::ingredients   , this->m_ingredients   , val); }
void Style::setExamples      (QString               const & val) { this->setAndNotify(PropertyNames::Style::examples      , this->m_examples      , val); }

Recipe * Style::getOwningRecipe() {
   return ObjectStoreWrapper::findFirstMatching<Recipe>( [this](Recipe * rec) {return rec->uses(*this);} );
}
