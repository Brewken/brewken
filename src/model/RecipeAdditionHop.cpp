/*======================================================================================================================
 * model/RecipeAdditionHop.cpp is part of Brewken, and is copyright the following authors 2023:
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
#include "model/RecipeAdditionHop.h"

#include "database/ObjectStoreTyped.h"
#include "database/ObjectStoreWrapper.h"
#include "model/NamedParameterBundle.h"
#include "model/Boil.h"
#include "model/BoilStep.h"

QString const RecipeAdditionHop::LocalisedName = tr("Hop Addition");

ObjectStore & RecipeAdditionHop::getObjectStoreTypedInstance() const {
   return ObjectStoreTyped<RecipeAdditionHop>::getInstance();
}

TypeLookup const RecipeAdditionHop::typeLookup {
   "RecipeAdditionHop",
   {
      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::RecipeAdditionHop::hop, RecipeAdditionHop::hop),
   },
   // Parent class lookup.  NB: RecipeAdditionMassOrVolume not NamedEntity!
   &RecipeAdditionMassOrVolume::typeLookup
};
static_assert(std::is_base_of<RecipeAdditionMassOrVolume, RecipeAdditionHop>::value);

//
// This is a compile-time check that HasTypeLookup is working properly.  It doesn't particularly belong here, but I
// didn't yet find a much better place for it to live!
//
static_assert(HasTypeLookup<Hop>);
static_assert(!HasTypeLookup<QString>);


RecipeAdditionHop::RecipeAdditionHop(QString name, int const recipeId, int const hopId) :
   RecipeAdditionMassOrVolume{name, recipeId, hopId},
   RecipeAdditionBase<RecipeAdditionHop, Hop>{} {
   return;
}

RecipeAdditionHop::RecipeAdditionHop(NamedParameterBundle const & namedParameterBundle) :
   RecipeAdditionMassOrVolume{namedParameterBundle},
   RecipeAdditionBase<RecipeAdditionHop, Hop>{} {
   //
   // If the addition stage is not specified then we assume it is boil, as this is the first stage at which it is usual
   // to add hops.
   //
   m_stage = namedParameterBundle.val<RecipeAddition::Stage>(PropertyNames::RecipeAddition::stage,
                                                             RecipeAddition::Stage::Boil);
   return;
}

RecipeAdditionHop::RecipeAdditionHop(RecipeAdditionHop const & other) :
   RecipeAdditionMassOrVolume{other},
   RecipeAdditionBase<RecipeAdditionHop, Hop>{} {
   return;
}

RecipeAdditionHop::~RecipeAdditionHop() = default;

Hop * RecipeAdditionHop::hop() const {
   if (this->m_ingredientId <= 0) {
      return nullptr;
   }
   return ObjectStoreWrapper::getByIdRaw<Hop>(this->m_ingredientId);
}

bool RecipeAdditionHop::isFirstWort() const {
   //
   // In switching from Hop::use to RecipeAddition::stage, there is no longer an explicit flag for First Wort Hops.
   // Instead, a first wort addition is simply(!) one that occurs at the beginning of step 1 of the boil if that step
   // ramps from mash end temperature to boil temperature.
   //
   // We could work this out in a single if statement, but it would be too horrible to look at, so we simply go through
   // all the conditions that have to be satisfied.
   //
   if (this->stage() != RecipeAddition::Stage::Boil) { return false; }

   // First Wort must be the first step of the boil, during ramp-up from mashout and before the boil proper
   if (!this->step() || *this->step() != 1) { return false; }

   Recipe const * recipe = this->getOwningRecipe();
   if (!recipe->boil()) { return false; }

   auto boil = *recipe->boil();
   if (boil->boilSteps().empty()) { return false; }

   auto boilStep = boil->boilSteps().first();
   if (!boilStep->startTemp_c() || *boilStep->startTemp_c() > Boil::minimumBoilTemperature_c) {return false; }

   return true;
}

bool RecipeAdditionHop::isAroma() const {
   //
   // In switching from Hop::use to RecipeAddition::stage, there is no longer an explicit flag for Aroma Hops, ie those
   // added after the boil (aka zero minute hops).
   //
   if (this->stage() != RecipeAddition::Stage::Boil) { return false; }

   // Aroma must be after the first step of the boil
   if (!this->step() || *this->step() == 1) { return false; }

   Recipe const * recipe = this->getOwningRecipe();
   if (!recipe->boil()) { return false; }

   auto boil = *recipe->boil();
   if (boil->boilSteps().empty()) { return false; }

   int const numBoilSteps = boil->boilSteps().size();
   if (*this->step() > numBoilSteps) {
      qCritical() <<
         Q_FUNC_INFO << "RecipeAdditionHop #" << this->key() << "in Recipe #" << this->m_recipeId <<
         "has boil step #" << *this->step() << "but boil only has" << numBoilSteps << "steps.  This is probably a bug!";
      return false;
   }

   // Remember RecipeAddition steps are numbered from 1, but vectors are indexed from 0
   auto boilStep = boil->boilSteps()[*this->step() - 1];
   if (!boilStep->endTemp_c() || *boilStep->endTemp_c() > Boil::minimumBoilTemperature_c) { return false; }

   return true;
}



Recipe * RecipeAdditionHop::getOwningRecipe() const {
   return ObjectStoreWrapper::getByIdRaw<Recipe>(this->m_recipeId);
}

void RecipeAdditionHop::setHop(Hop * const val) {
   if (val) {
      this->m_ingredientId = val->key();
   } else {
      this->m_ingredientId = -1;
   }
   return;
}
