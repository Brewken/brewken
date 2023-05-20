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

// This is not stored in BeerJSON, so we leave the original capitalisation
EnumStringMapping const Misc::useStringMapping {
   {Misc::Use::Boil     , "Boil"     },
   {Misc::Use::Mash     , "Mash"     },
   {Misc::Use::Primary  , "Primary"  },
   {Misc::Use::Secondary, "Secondary"},
   {Misc::Use::Bottling , "Bottling" }
};

EnumStringMapping const Misc::useDisplayNames {
   {Misc::Use::Boil     , tr("Boil"     )},
   {Misc::Use::Mash     , tr("Mash"     )},
   {Misc::Use::Primary  , tr("Primary"  )},
   {Misc::Use::Secondary, tr("Secondary")},
   {Misc::Use::Bottling , tr("Bottling" )}
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
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Misc::amount        , Misc::m_amount        , Measurement::PqEitherMassOrVolume      ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Misc::amountIsWeight, Misc::m_amountIsWeight,           NonPhysicalQuantity::Bool    ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Misc::notes         , Misc::m_notes         ,           NonPhysicalQuantity::String  ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Misc::time_min      , Misc::m_time_min      , Measurement::PhysicalQuantity::Time    ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Misc::type          , Misc::m_type          ,           NonPhysicalQuantity::Enum    ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Misc::useFor        , Misc::m_useFor        ,           NonPhysicalQuantity::String  ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Misc::use           , Misc::m_use           ,           NonPhysicalQuantity::Enum    ),
      // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::Misc::amountWithUnits, Misc::amountWithUnits, Measurement::PqEitherMassOrVolume),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Misc::producer      , Misc::m_producer      ,           NonPhysicalQuantity::String  ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Misc::productId     , Misc::m_productId     ,           NonPhysicalQuantity::String  ),
   },
   // Parent class lookup.  NB: NamedEntityWithInventory not NamedEntity!
   &NamedEntityWithInventory::typeLookup
};
static_assert(std::is_base_of<NamedEntityWithInventory, Misc>::value);

//============================CONSTRUCTORS======================================

Misc::Misc(QString name) :
   NamedEntityWithInventory{name, true},
   m_type          {Misc::Type::Spice},
   m_use           {std::nullopt     },
   m_time_min      {0.0              },
   m_amount        {0.0              },
   m_amountIsWeight{false            },
   m_useFor        {""               },
   m_notes         {""               },
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   m_producer      {""               },
   m_productId     {""               } {
   return;
}

Misc::Misc(NamedParameterBundle const & namedParameterBundle) :
   NamedEntityWithInventory{namedParameterBundle},
   m_type                  {namedParameterBundle.val<Misc::Type      >(PropertyNames::Misc::type     )},
   m_use                   {namedParameterBundle.optEnumVal<Misc::Use>(PropertyNames::Misc::use      )},
   m_time_min              {namedParameterBundle.val<double          >(PropertyNames::Misc::time_min )},
   m_useFor                {namedParameterBundle.val<QString         >(PropertyNames::Misc::useFor   )},
   m_notes                 {namedParameterBundle.val<QString         >(PropertyNames::Misc::notes    )},
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   m_producer              {namedParameterBundle.val<QString         >(PropertyNames::Misc::producer )},
   m_productId             {namedParameterBundle.val<QString         >(PropertyNames::Misc::productId)} {

   this->setEitherOrReqParams<MassOrVolumeAmt>(namedParameterBundle, PropertyNames::Misc::amount, PropertyNames::Misc::amountIsWeight, PropertyNames::Misc::amountWithUnits, this->m_amount, this->m_amountIsWeight);

   return;
}

Misc::Misc(Misc const & other) :
   NamedEntityWithInventory{other                 },
   m_type                  {other.m_type          },
   m_use                   {other.m_use           },
   m_time_min              {other.m_time_min      },
   m_amount                {other.m_amount        },
   m_amountIsWeight        {other.m_amountIsWeight},
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
std::optional<Misc::Use> Misc::use           () const { return                    m_use           ; }
std::optional<int>       Misc::useAsInt      () const { return Optional::toOptInt(m_use)          ; }
double                   Misc::amount        () const { return                    m_amount        ; }
double                   Misc::time_min      () const { return                    m_time_min      ; }
bool                     Misc::amountIsWeight() const { return                    m_amountIsWeight; }
QString                  Misc::useFor        () const { return                    m_useFor        ; }
QString                  Misc::notes         () const { return                    m_notes         ; }
// ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
QString                  Misc::producer      () const { return                    m_producer      ; }
QString                  Misc::productId     () const { return                    m_productId     ; }

MassOrVolumeAmt Misc::amountWithUnits() const { return MassOrVolumeAmt{this->m_amount, this->m_amountIsWeight ? Measurement::Units::kilograms : Measurement::Units::liters}; }

//============================"SET" METHODS=====================================
void Misc::setType          (Type                     const   val) { this->setAndNotify( PropertyNames::Misc::type          , this->m_type          , val); }
void Misc::setUse           (std::optional<Misc::Use> const   val) { this->setAndNotify( PropertyNames::Misc::use           , this->m_use           , val); }
void Misc::setUseAsInt      (std::optional<int>       const   val) { this->setAndNotify( PropertyNames::Misc::use           , this->m_use           , Optional::fromOptInt<Use>(val)); }
void Misc::setUseFor        (QString                  const & val) { this->setAndNotify( PropertyNames::Misc::useFor        , this->m_useFor        , val); }
void Misc::setNotes         (QString                  const & val) { this->setAndNotify( PropertyNames::Misc::notes         , this->m_notes         , val); }
void Misc::setAmountIsWeight(bool                     const   val) { this->setAndNotify( PropertyNames::Misc::amountIsWeight, this->m_amountIsWeight, val); }
void Misc::setAmount        (double                   const   val) { this->setAndNotify( PropertyNames::Misc::amount        , this->m_amount        , this->enforceMin(val, "amount")); }
void Misc::setTime_min      (double                   const   val) { this->setAndNotify( PropertyNames::Misc::time_min      , this->m_time_min      , this->enforceMin(val, "time_min"  )); }
// ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
void Misc::setProducer      (QString                  const & val) { this->setAndNotify(PropertyNames::Misc::producer       , this->m_producer      , val); }
void Misc::setProductId     (QString                  const & val) { this->setAndNotify(PropertyNames::Misc::productId      , this->m_productId     , val); }


void Misc::setAmountWithUnits(MassOrVolumeAmt const   val) {
   this->setAndNotify(PropertyNames::Misc::amount        , this->m_amount        , val.quantity());
   this->setAndNotify(PropertyNames::Misc::amountIsWeight, this->m_amountIsWeight, val.isMass()  );
   return;
}

//========================OTHER METHODS=========================================
double     Misc::inventory() const {
   return InventoryUtils::getAmount(*this);
}

void Misc::setInventoryAmount(double var) {
   InventoryUtils::setAmount(*this, var);
   return;
}

Recipe * Misc::getOwningRecipe() {
   return ObjectStoreWrapper::findFirstMatching<Recipe>( [this](Recipe * rec) {return rec->uses(*this);} );
}
