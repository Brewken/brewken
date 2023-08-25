/*======================================================================================================================
 * model/RecipeAdditionMassOrVolume.cpp is part of Brewken, and is copyright the following authors 2023:
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
#include "model/RecipeAdditionMassOrVolume.h"

#include "model/NamedParameterBundle.h"

QString const RecipeAdditionMassOrVolume::LocalisedName = tr("Recipe Addition (Mass or Volume)");

bool RecipeAdditionMassOrVolume::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   RecipeAdditionMassOrVolume const & rhs = static_cast<RecipeAdditionMassOrVolume const &>(other);
   // Base class will already have ensured names are equal
   return (
      this->m_amount         == rhs.m_amount         &&
      this->m_amountIsWeight == rhs.m_amountIsWeight &&
      // Parent classes have to be equal too
      this->RecipeAddition::isEqualTo(other)
   );
}

TypeLookup const RecipeAdditionMassOrVolume::typeLookup {
   "RecipeAdditionMassOrVolume",
   {
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::RecipeAdditionMassOrVolume::amount        , RecipeAdditionMassOrVolume::m_amount        , Measurement::ChoiceOfPhysicalQuantity::Mass_Volume),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::RecipeAdditionMassOrVolume::amountIsWeight, RecipeAdditionMassOrVolume::m_amountIsWeight, NonPhysicalQuantity::Bool        ),

      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::RecipeAdditionMassOrVolume::amountWithUnits, RecipeAdditionMassOrVolume::amountWithUnits, Measurement::ChoiceOfPhysicalQuantity::Mass_Volume),
   },
   // Parent class lookup.  NB: RecipeAddition not NamedEntity!
   {&RecipeAddition::typeLookup}
};
static_assert(std::is_base_of<RecipeAddition, RecipeAdditionMassOrVolume>::value);

RecipeAdditionMassOrVolume::RecipeAdditionMassOrVolume(QString name, int const recipeId, int const ingredientId) :
   RecipeAddition  {name, recipeId, ingredientId},
   m_amount        {0.0 },
   m_amountIsWeight{true} {
   return;
}

RecipeAdditionMassOrVolume::RecipeAdditionMassOrVolume(NamedParameterBundle const & namedParameterBundle) :
   RecipeAddition{namedParameterBundle} {
   // Note we specify the optional seventh parameter (defaultIsFirstUnit) here as true because, eg, if we're reading in
   // from BeerXML, the amountIsWeight flag might not be present, in which case we should assume weight.
   this->setEitherOrReqParams<MassOrVolumeAmt>(namedParameterBundle,
                                               PropertyNames::RecipeAdditionMassOrVolume::amount         ,
                                               PropertyNames::RecipeAdditionMassOrVolume::amountIsWeight ,
                                               PropertyNames::RecipeAdditionMassOrVolume::amountWithUnits,
                                               this->m_amount,
                                               this->m_amountIsWeight,
                                               true);
   return;
}

RecipeAdditionMassOrVolume::RecipeAdditionMassOrVolume(RecipeAdditionMassOrVolume const & other) :
   RecipeAddition{other                        },
   m_amount        {other.m_amount        },
   m_amountIsWeight{other.m_amountIsWeight} {
   return;
}

RecipeAdditionMassOrVolume::~RecipeAdditionMassOrVolume() = default;

//============================================= "GETTER" MEMBER FUNCTIONS ==============================================
double          RecipeAdditionMassOrVolume::amount         () const { return this->m_amount        ; }
bool            RecipeAdditionMassOrVolume::amountIsWeight () const { return this->m_amountIsWeight; }
MassOrVolumeAmt RecipeAdditionMassOrVolume::amountWithUnits() const { return MassOrVolumeAmt{this->m_amount, this->m_amountIsWeight ? Measurement::Units::kilograms : Measurement::Units::liters}; }

//============================================= "SETTER" MEMBER FUNCTIONS ==============================================
void RecipeAdditionMassOrVolume::setAmount         (double          const val) { this->setAndNotify(PropertyNames::RecipeAdditionMassOrVolume::amount        , this->m_amount        , this->enforceMin(val, "amount")); return; }
void RecipeAdditionMassOrVolume::setAmountIsWeight (bool            const val) { this->setAndNotify(PropertyNames::RecipeAdditionMassOrVolume::amountIsWeight, this->m_amountIsWeight,                  val           ); return; }
void RecipeAdditionMassOrVolume::setAmountWithUnits(MassOrVolumeAmt const val){
   this->setAndNotify(PropertyNames::RecipeAdditionMassOrVolume::amount        , this->m_amount        , val.quantity);
   this->setAndNotify(PropertyNames::RecipeAdditionMassOrVolume::amountIsWeight, this->m_amountIsWeight, val.isMass  ());
   return;
}
