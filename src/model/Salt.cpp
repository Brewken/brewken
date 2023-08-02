/*======================================================================================================================
 * model/Salt.cpp is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
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
#include "model/Salt.h"

#include <QDebug>

#include "database/ObjectStoreWrapper.h"
#include "model/NamedParameterBundle.h"
#include "model/Recipe.h"

QString const Salt::LocalisedName = tr("Salt");

EnumStringMapping const Salt::typeStringMapping {
   {Salt::Type::CaCl2         , "CaCl2"         },
   {Salt::Type::CaCO3         , "CaCO3"         },
   {Salt::Type::CaSO4         , "CaSO4"         },
   {Salt::Type::MgSO4         , "MgSO4"         },
   {Salt::Type::NaCl          , "NaCl"          },
   {Salt::Type::NaHCO3        , "NaHCO3"        },
   {Salt::Type::LacticAcid    , "LacticAcid"    },
   {Salt::Type::H3PO4         , "H3PO4"         },
   {Salt::Type::AcidulatedMalt, "AcidulatedMalt"},
};

EnumStringMapping const Salt::typeDisplayNames {
   {Salt::Type::CaCl2                  , tr("CaCl2"           " (Calcium chloride)"  )},
   {Salt::Type::CaCO3                  , tr("CaCO3"           " (Calcium carbonate)" )},
   {Salt::Type::CaSO4                  , tr("CaSO4"           " (Calcium sulfate)"   )},
   {Salt::Type::MgSO4                  , tr("MgSO4"           " (Magnesium sulfate)" )},
   {Salt::Type::NaCl                   , tr("NaCl"            " (Sodium chloride)"   )},
   {Salt::Type::NaHCO3                 , tr("NaHCO3"          " (Sodium bicarbonate)")},
   {Salt::Type::LacticAcid             , tr("Lactic Acid"                            )},
   {Salt::Type::H3PO4                  , tr("H3PO4"           " (Phosphoric acid)"   )},
   {Salt::Type::AcidulatedMalt         , tr("Acidulated Malt"                        )},
};

EnumStringMapping const Salt::whenToAddStringMapping {
   {Salt::WhenToAdd::NEVER , "never" },
   {Salt::WhenToAdd::MASH  , "mash"  },
   {Salt::WhenToAdd::SPARGE, "sparge"},
   {Salt::WhenToAdd::RATIO , "ratio" },
   {Salt::WhenToAdd::EQUAL , "equal" },
};

EnumStringMapping const Salt::whenToAddDisplayNames {
   {Salt::WhenToAdd::NEVER , tr("Never" )},
   {Salt::WhenToAdd::MASH  , tr("Mash"  )},
   {Salt::WhenToAdd::SPARGE, tr("Sparge")},
   {Salt::WhenToAdd::RATIO , tr("Ratio" )},
   {Salt::WhenToAdd::EQUAL , tr("Equal" )},
};

bool Salt::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   Salt const & rhs = static_cast<Salt const &>(other);
   // Base class will already have ensured names are equal
   return (
      this->m_whenToAdd == rhs.m_whenToAdd &&
      this->m_type   == rhs.m_type
   );
}

ObjectStore & Salt::getObjectStoreTypedInstance() const {
   return ObjectStoreTyped<Salt>::getInstance();
}

TypeLookup const Salt::typeLookup {
   "Salt",
   {
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Salt::amount        , Salt::m_amount          , Measurement::PqEitherMassOrVolume      ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Salt::amountIsWeight, Salt::m_amountIsWeight  ,         NonPhysicalQuantity::Bool      ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Salt::isAcid        , Salt::m_is_acid         ,         NonPhysicalQuantity::Bool      ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Salt::percentAcid   , Salt::m_percent_acid    ,         NonPhysicalQuantity::Percentage),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Salt::type          , Salt::m_type            ,         NonPhysicalQuantity::Enum      ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Salt::whenToAdd     , Salt::m_whenToAdd       ,         NonPhysicalQuantity::Enum      ),

      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::Salt::amountWithUnits, Salt::amountWithUnits, Measurement::PqEitherMassOrVolume  ),
   },
   // Parent class lookup
   &NamedEntity::typeLookup
};

Salt::Salt(QString name) :
   NamedEntity      {name, true},
   m_amount         {0.0},
   m_whenToAdd      {Salt::WhenToAdd::NEVER},
   m_type           {Salt::Type::CaCl2},
   m_amountIsWeight{true},
   m_percent_acid   {0.0},
   m_is_acid        {false} {
   return;
}

Salt::Salt(NamedParameterBundle const & namedParameterBundle) :
   NamedEntity       {namedParameterBundle},
   SET_REGULAR_FROM_NPB (m_whenToAdd   , namedParameterBundle, PropertyNames::Salt::whenToAdd  ),
   SET_REGULAR_FROM_NPB (m_type        , namedParameterBundle, PropertyNames::Salt::type       ),
   SET_REGULAR_FROM_NPB (m_percent_acid, namedParameterBundle, PropertyNames::Salt::percentAcid),
   SET_REGULAR_FROM_NPB (m_is_acid     , namedParameterBundle, PropertyNames::Salt::isAcid     ) {

   this->setEitherOrReqParams<MassOrVolumeAmt>(namedParameterBundle,
                                               PropertyNames::Salt::amount,
                                               PropertyNames::Salt::amountIsWeight,
                                               PropertyNames::Salt::amountWithUnits,
                                               this->m_amount,
                                               this->m_amountIsWeight);
   return;
}

Salt::Salt(Salt const & other) :
   NamedEntity       {other                   },
   m_amount          {other.m_amount          },
   m_whenToAdd       {other.m_whenToAdd       },
   m_type            {other.m_type            },
   m_amountIsWeight{other.m_amountIsWeight},
   m_percent_acid    {other.m_percent_acid    },
   m_is_acid         {other.m_is_acid         } {
   return;
}

Salt::~Salt() = default;

//============================================= "GETTER" MEMBER FUNCTIONS ==============================================
double          Salt::amount        () const { return this->m_amount          ; }
Salt::WhenToAdd Salt::whenToAdd     () const { return this->m_whenToAdd       ; }
Salt::Type      Salt::type          () const { return this->m_type            ; }
bool            Salt::isAcid        () const { return this->m_is_acid         ; }
bool            Salt::amountIsWeight() const { return this->m_amountIsWeight; }
double          Salt::percentAcid   () const { return this->m_percent_acid    ; }

MassOrVolumeAmt                             Salt::amountWithUnits    () const { return MassOrVolumeAmt{this->m_amount, this->m_amountIsWeight ? Measurement::Units::kilograms : Measurement::Units::liters}; }

//============================================= "SETTER" MEMBER FUNCTIONS ==============================================
void Salt::setAmount(double val) {
   this->setAndNotify(PropertyNames::Salt::amount, this->m_amount, val);
}

void Salt::setWhenToAdd(Salt::WhenToAdd val) {
   this->setAndNotify(PropertyNames::Salt::whenToAdd, this->m_whenToAdd, val);
}

// This may come to haunt me, but I am setting the isAcid flag and the
// amount_is_weight flags here.
//
// 2023-06-02: MY: In for a penny, in for a pound.  I've moved the logic that "automatically" works out the acidity from
// SaltTableModel to here too.  But TBD I think we want to take another look at this at some point.
void Salt::setType(Salt::Type type) {
   bool isAcid = false;
   double newPercentAcid = m_percent_acid;
   if (type == Salt::Type::LacticAcid ||
       type == Salt::Type::H3PO4      ||
       type == Salt::Type::AcidulatedMalt) {
      isAcid = true;
   } else {
      newPercentAcid = 0.0;
   }
   this->setAndNotify(PropertyNames::Salt::type,           this->m_type,    type);
   this->setAndNotify(PropertyNames::Salt::isAcid,         this->m_is_acid, isAcid);
   this->setAndNotify(PropertyNames::Salt::amountIsWeight, this->m_amountIsWeight, !(type == Salt::Type::LacticAcid || type == Salt::Type::H3PO4));
   if (isAcid && newPercentAcid == 0.0) {
      switch (type) {
         case Salt::Type::LacticAcid    : newPercentAcid = 88; break;
         case Salt::Type::H3PO4         : newPercentAcid = 10; break;
         case Salt::Type::AcidulatedMalt: newPercentAcid =  2; break;
         // The next line should be unreachable!
         default                        : Q_ASSERT(false); break;
      }
   }
   this->setPercentAcid(newPercentAcid);
   return;
}

void Salt::setAmountIsWeight(bool val) {
   this->setAndNotify(PropertyNames::Salt::amountIsWeight, this->m_amountIsWeight, val);
}

void Salt::setIsAcid(bool val) {
   this->setAndNotify(PropertyNames::Salt::isAcid, this->m_is_acid, val);
}

void Salt::setPercentAcid(double val) {
   // .:TBD:. Maybe we should check here that we are an acid...
   this->setAndNotify(PropertyNames::Salt::percentAcid, this->m_percent_acid, val);
}

void Salt::setAmountWithUnits(MassOrVolumeAmt const   val) {
   this->setAndNotify(PropertyNames::Salt::amount        , this->m_amount        , val.quantity());
   this->setAndNotify(PropertyNames::Salt::amountIsWeight, this->m_amountIsWeight, val.isMass()  );
   return;
}

//====== maths ===========
// All of these the per gram, per liter
// these values are taken from Bru'n Water's execellent water knowledge page
// https://sites.google.com/site/brunwater/water-knowledge
// the numbers are derived by dividing the molecular weight of the ion by the
// molecular weight of the molecule in grams and then multiplying by 1000 to
// mg
// eg:
//    NaHCO3 84 g/mol
//       Na provides    23 g/mol
//       HCO3 provides  61 g/mol (ish)
//     So 1 g of NaHCO3 in 1L of water provides 1000*(61/84) = 726 ppm HCO3
//
// the magic 1000 is here because masses are stored as kg. We need it in grams
// for this part
double Salt::Ca() const {
   if ( m_whenToAdd == Salt::WhenToAdd::NEVER ) {
      return 0.0;
   }

   switch (m_type) {
      case Salt::Type::CaCl2: return 272.0 * m_amount * 1000.0;
      case Salt::Type::CaCO3: return 200.0 * m_amount * 1000.0;
      case Salt::Type::CaSO4: return 232.0 * m_amount * 1000.0;
      default: return 0.0;
   }
}

double Salt::Cl() const {
   if ( m_whenToAdd == Salt::WhenToAdd::NEVER ) {
      return 0.0;
   }

   switch (m_type) {
      case Salt::Type::CaCl2: return 483 * m_amount * 1000.0;
      case Salt::Type::NaCl: return 607 * m_amount * 1000.0;
      default: return 0.0;
   }
}

double Salt::CO3() const {
   if ( m_whenToAdd == Salt::WhenToAdd::NEVER ) {
      return 0.0;
   }

   return m_type == Salt::Type::CaCO3 ? 610.0  * m_amount * 1000.0: 0.0;
}

double Salt::HCO3() const {
   if ( m_whenToAdd == Salt::WhenToAdd::NEVER ) {
      return 0.0;
   }

   return m_type == Salt::Type::NaHCO3 ? 726.0 * m_amount * 1000.0: 0.0;
}

double Salt::Mg() const {
   if ( m_whenToAdd == Salt::WhenToAdd::NEVER ) {
      return 0.0;
   }
   return m_type == Salt::Type::MgSO4 ? 99.0 * m_amount * 1000.0: 0.0;
}

double Salt::Na() const {
   if ( m_whenToAdd == Salt::WhenToAdd::NEVER ) {
      return 0.0;
   }
   switch (m_type) {
      case Salt::Type::NaCl: return 393.0 * m_amount * 1000.0;
      case Salt::Type::NaHCO3: return 274.0 * m_amount * 1000.0;
      default: return 0.0;
   }
}

double Salt::SO4() const {
   if ( m_whenToAdd == Salt::WhenToAdd::NEVER ) {
      return 0.0;
   }
   switch (m_type) {
      case Salt::Type::CaSO4: return 558.0 * m_amount * 1000.0;
      case Salt::Type::MgSO4: return 389.0 * m_amount * 1000.0;
      default: return 0.0;
   }
}

Recipe * Salt::getOwningRecipe() const {
   return ObjectStoreWrapper::findFirstMatching<Recipe>( [this](Recipe * rec) {return rec->uses(*this);} );
}
