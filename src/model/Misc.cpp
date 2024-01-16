/*======================================================================================================================
 * model/Misc.cpp is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
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
#include "model/Misc.h"

#include <iostream>
#include <string>

#include <QDebug>
#include <QVector>

#include "database/ObjectStoreWrapper.h"
#include "model/Inventory.h"
#include "model/NamedParameterBundle.h"
#include "model/Recipe.h"

QString const Misc::LocalisedName = tr("Miscellaneous");

EnumStringMapping const Misc::typeStringMapping {
   {Misc::Type::Spice      , "spice"      },
   {Misc::Type::Fining     , "fining"     },
   {Misc::Type::Water_Agent, "water agent"},
   {Misc::Type::Herb       , "herb"       },
   {Misc::Type::Flavor     , "flavor"     },
   {Misc::Type::Other      , "other"      },
   {Misc::Type::Wood       , "wood"       },
};

EnumStringMapping const Misc::typeDisplayNames {
   {Misc::Type::Spice      , tr("Spice"      )},
   {Misc::Type::Fining     , tr("Fining"     )},
   {Misc::Type::Water_Agent, tr("Water Agent")},
   {Misc::Type::Herb       , tr("Herb"       )},
   {Misc::Type::Flavor     , tr("Flavor"     )},
   {Misc::Type::Other      , tr("Other"      )},
   {Misc::Type::Wood       , tr("Wood"       )},
};

bool Misc::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   Misc const & rhs = static_cast<Misc const &>(other);
   // Base class will already have ensured names are equal
   return (
      this->m_type == rhs.m_type
   );
}

ObjectStore & Misc::getObjectStoreTypedInstance() const {
   return ObjectStoreTyped<Misc>::getInstance();
}

TypeLookup const Misc::typeLookup {
   "Misc",
   {
///      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Misc::amount        , Misc::m_amount        , Measurement::ChoiceOfPhysicalQuantity::Mass_Volume      ),
///      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Misc::amountIsWeight, Misc::m_amountIsWeight,           NonPhysicalQuantity::Bool    ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Misc::notes         , Misc::m_notes         ,           NonPhysicalQuantity::String  ),
///      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Misc::time_min      , Misc::m_time_min      , Measurement::PhysicalQuantity::Time    ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Misc::type          , Misc::m_type          ,           NonPhysicalQuantity::Enum    ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Misc::useFor        , Misc::m_useFor        ,           NonPhysicalQuantity::String  ),
///      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Misc::use           , Misc::m_use           ,           NonPhysicalQuantity::Enum    ),
      // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
///      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::Misc::amountWithUnits, Misc::amountWithUnits, Measurement::ChoiceOfPhysicalQuantity::Mass_Volume),
///      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::Misc::amountWithUnits, Misc::amountWithUnits, Measurement::PqEitherMassOrVolume),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Misc::producer      , Misc::m_producer      ,           NonPhysicalQuantity::String  ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Misc::productId     , Misc::m_productId     ,           NonPhysicalQuantity::String  ),
   },
   // Parent class lookup.  NB: NamedEntityWithInventory not NamedEntity!
   {&NamedEntityWithInventory::typeLookup}
};
static_assert(std::is_base_of<NamedEntityWithInventory, Misc>::value);

//============================CONSTRUCTORS======================================

Misc::Misc(QString name) :
   NamedEntityWithInventory{name, true},
   m_type          {Misc::Type::Spice},
///   m_use           {std::nullopt     },
///   m_time_min      {0.0              },
///   m_amount        {0.0              },
///   m_amountIsWeight{false            },
   m_useFor        {""               },
   m_notes         {""               },
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   m_producer      {""               },
   m_productId     {""               } {
   return;
}

Misc::Misc(NamedParameterBundle const & namedParameterBundle) :
   NamedEntityWithInventory{namedParameterBundle},
   SET_REGULAR_FROM_NPB (m_type                , namedParameterBundle, PropertyNames::Misc::type     ),
///   SET_OPT_ENUM_FROM_NPB(m_use      , Misc::Use, namedParameterBundle, PropertyNames::Misc::use      ),
///   SET_REGULAR_FROM_NPB (m_time_min            , namedParameterBundle, PropertyNames::Misc::time_min ),
   SET_REGULAR_FROM_NPB (m_useFor              , namedParameterBundle, PropertyNames::Misc::useFor   ),
   SET_REGULAR_FROM_NPB (m_notes               , namedParameterBundle, PropertyNames::Misc::notes    ),
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   SET_REGULAR_FROM_NPB (m_producer            , namedParameterBundle, PropertyNames::Misc::producer ),
   SET_REGULAR_FROM_NPB (m_productId           , namedParameterBundle, PropertyNames::Misc::productId) {

///   this->setEitherOrReqParams(namedParameterBundle,
///                              PropertyNames::Misc::amount,
///                              PropertyNames::Misc::amountIsWeight,
///                              PropertyNames::Misc::amountWithUnits,
///                              Measurement::PhysicalQuantity::Mass,
///                              this->m_amount,
///                              this->m_amountIsWeight);
   return;
}

Misc::Misc(Misc const & other) :
   NamedEntityWithInventory{other                 },
   m_type                  {other.m_type          },
///   m_use                   {other.m_use           },
///   m_time_min              {other.m_time_min      },
///   m_amount                {other.m_amount        },
///   m_amountIsWeight        {other.m_amountIsWeight},
   m_useFor                {other.m_useFor        },
   m_notes                 {other.m_notes         },
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   m_producer              {other.m_producer      },
   m_productId             {other.m_productId     } {
   return;
}

Misc::~Misc() = default;

//============================"GET" METHODS=====================================
Misc::Type               Misc::type          () const { return                    m_type          ; }
///std::optional<Misc::Use> Misc::use           () const { return                    m_use           ; }
///std::optional<int>       Misc::useAsInt      () const { return Optional::toOptInt(m_use)          ; }
///double                   Misc::amount        () const { return                    m_amount        ; }
///double                   Misc::time_min      () const { return                    m_time_min      ; }
///bool                     Misc::amountIsWeight() const { return                    m_amountIsWeight; }
QString                  Misc::useFor        () const { return                    m_useFor        ; }
QString                  Misc::notes         () const { return                    m_notes         ; }
// ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
QString                  Misc::producer      () const { return                    m_producer      ; }
QString                  Misc::productId     () const { return                    m_productId     ; }

Measurement::Amount Misc::amountWithUnits() const { return Measurement::Amount{this->m_amount, this->m_amountIsWeight ? Measurement::Units::kilograms : Measurement::Units::liters}; }

//============================"SET" METHODS=====================================
void Misc::setType          (Type                     const   val) { this->setAndNotify( PropertyNames::Misc::type          , this->m_type          , val); }
///void Misc::setUse           (std::optional<Misc::Use> const   val) { this->setAndNotify( PropertyNames::Misc::use           , this->m_use           , val); }
///void Misc::setUseAsInt      (std::optional<int>       const   val) { this->setAndNotify( PropertyNames::Misc::use           , this->m_use           , Optional::fromOptInt<Use>(val)); }
void Misc::setUseFor        (QString                  const & val) { this->setAndNotify( PropertyNames::Misc::useFor        , this->m_useFor        , val); }
void Misc::setNotes         (QString                  const & val) { this->setAndNotify( PropertyNames::Misc::notes         , this->m_notes         , val); }
///void Misc::setAmountIsWeight(bool                     const   val) { this->setAndNotify( PropertyNames::Misc::amountIsWeight, this->m_amountIsWeight, val); }
///void Misc::setAmount        (double                   const   val) { this->setAndNotify( PropertyNames::Misc::amount        , this->m_amount        , this->enforceMin(val, "amount")); }
///void Misc::setTime_min      (double                   const   val) { this->setAndNotify( PropertyNames::Misc::time_min      , this->m_time_min      , this->enforceMin(val, "time_min"  )); }
// ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
void Misc::setProducer      (QString                  const & val) { this->setAndNotify(PropertyNames::Misc::producer       , this->m_producer      , val); }
void Misc::setProductId     (QString                  const & val) { this->setAndNotify(PropertyNames::Misc::productId      , this->m_productId     , val); }


///void Misc::setAmountWithUnits(Measurement::Amount const   val) {
///   this->setAndNotify(PropertyNames::Misc::amount        , this->m_amount        , val.quantity);
///   this->setAndNotify(PropertyNames::Misc::amountIsWeight, this->m_amountIsWeight, val.unit->getPhysicalQuantity() == Measurement::PhysicalQuantity::Mass);
///   return;
///}

//========================OTHER METHODS=========================================

Recipe * Misc::getOwningRecipe() const {
///   return ObjectStoreWrapper::findFirstMatching<Recipe>( [this](Recipe * rec) {return rec->uses(*this);} );
   return nullptr;
}

// Insert the boiler-plate stuff for inventory
INGREDIENT_BASE_COMMON_CODE(Misc)
