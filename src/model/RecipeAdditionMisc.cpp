/*======================================================================================================================
 * model/RecipeAdditionMisc.cpp is part of Brewken, and is copyright the following authors 2023-2024:
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
#include "model/RecipeAdditionMisc.h"

#include "database/ObjectStoreTyped.h"
#include "database/ObjectStoreWrapper.h"
#include "model/NamedParameterBundle.h"
#include "model/Boil.h"
#include "model/BoilStep.h"

QString const RecipeAdditionMisc::LocalisedName = tr("Misc Addition");

EnumStringMapping const RecipeAdditionMisc::useStringMapping {
   {Misc::Use::Boil     , "Boil"     },
   {Misc::Use::Mash     , "Mash"     },
   {Misc::Use::Primary  , "Primary"  },
   {Misc::Use::Secondary, "Secondary"},
   {Misc::Use::Bottling , "Bottling" }
};

EnumStringMapping const RecipeAdditionMisc::useDisplayNames {
   {Misc::Use::Boil     , tr("Boil"     )},
   {Misc::Use::Mash     , tr("Mash"     )},
   {Misc::Use::Primary  , tr("Primary"  )},
   {Misc::Use::Secondary, tr("Secondary")},
   {Misc::Use::Bottling , tr("Bottling" )}
};

ObjectStore & RecipeAdditionMisc::getObjectStoreTypedInstance() const {
   return ObjectStoreTyped<RecipeAdditionMisc>::getInstance();
}

TypeLookup const RecipeAdditionMisc::typeLookup {
   "RecipeAdditionMisc",
   {
      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::RecipeAdditionMisc::misc, RecipeAdditionMisc::misc),
      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::RecipeAdditionMisc::use, RecipeAdditionMisc::use),
   },
   // Parent classes lookup.  NB: RecipeAddition not NamedEntity!
   {&RecipeAddition::typeLookup,
    std::addressof(IngredientAmount<RecipeAdditionMisc, Misc>::typeLookup)}
};
static_assert(std::is_base_of<RecipeAddition, RecipeAdditionMisc>::value);
static_assert(std::is_base_of<IngredientAmount<RecipeAdditionMisc, Misc>, RecipeAdditionMisc>::value);

//
// This is a compile-time check that HasTypeLookup is working properly.  It doesn't particularly belong here, but I
// didn't yet find a much better place for it to live!
//
static_assert(HasTypeLookup<Misc>);
static_assert(!HasTypeLookup<QString>);


RecipeAdditionMisc::RecipeAdditionMisc(QString name, int const recipeId, int const hopId) :
   RecipeAddition{name, recipeId, hopId},
   RecipeAdditionBase<RecipeAdditionMisc, Misc>{},
   IngredientAmount<RecipeAdditionMisc, Misc>{} {
   return;
}

RecipeAdditionMisc::RecipeAdditionMisc(NamedParameterBundle const & namedParameterBundle) :
   RecipeAddition{namedParameterBundle},
   RecipeAdditionBase<RecipeAdditionMisc, Misc>{},
   IngredientAmount<RecipeAdditionMisc, Misc>{namedParameterBundle} {
   //
   // If the addition stage is not specified then we assume it is boil, as this is the first stage at which it is usual
   // to add hops.
   //
   m_stage = namedParameterBundle.val<RecipeAddition::Stage>(PropertyNames::RecipeAddition::stage,
                                                             RecipeAddition::Stage::Boil);
///   qDebug() << Q_FUNC_INFO << "RecipeAdditionMisc #" << this->key() << ": Recipe #" << this->m_recipeId << ", Misc #" << this->m_ingredientId;
   return;
}

RecipeAdditionMisc::RecipeAdditionMisc(RecipeAdditionMisc const & other) :
   RecipeAddition{other},
   RecipeAdditionBase<RecipeAdditionMisc, Misc>{},
   IngredientAmount<RecipeAdditionMisc, Misc>{other} {
   return;
}

RecipeAdditionMisc::~RecipeAdditionMisc() = default;

//============================================= "GETTER" MEMBER FUNCTIONS ==============================================
RecipeAdditionMisc::Use  RecipeAdditionMisc::use() const {
   switch (this->stage()) {
      case RecipeAddition::Stage::Mash:
         return RecipeAdditionMisc::Use::Mash;

      case RecipeAddition::Stage::Boil:
         if (this->isFirstWort()) {
            return RecipeAdditionMisc::Use::First_Wort;
         }
         if (this->isAroma()) {
            return RecipeAdditionMisc::Use::Aroma;
         }
         return RecipeAdditionMisc::Use::Boil;

      case RecipeAddition::Stage::Fermentation:
      case RecipeAddition::Stage::Packaging:
         return RecipeAdditionMisc::Use::Dry_Misc;

      // No default case as we want the compiler to warn us if we missed a case above
   }

   // This should be unreachable, but putting a return statement here prevents compiler warnings
   return RecipeAdditionMisc::Use::Boil;
}

Misc * RecipeAdditionMisc::misc() const {
   // Normally there should always be a valid Misc in a RecipeAdditionMisc.  (The Recipe ID may be -1 if the addition is
   // only just about to be added to the Recipe or has just been removed from it, but there's no great reason for the
   // Misc ID not to be valid).
   if (this->m_ingredientId <= 0) {
      qWarning() << Q_FUNC_INFO << "No Misc set on RecipeAdditionMisc #" << this->key();
      return nullptr;
   }

///   qDebug() << Q_FUNC_INFO << "RecipeAdditionMisc #" << this->key() << ": Recipe #" << this->m_recipeId << ", Misc #" << this->m_ingredientId << "@" << ObjectStoreWrapper::getByIdRaw<Misc>(this->m_ingredientId);
   return ObjectStoreWrapper::getByIdRaw<Misc>(this->m_ingredientId);
}

bool RecipeAdditionMisc::isFirstWort() const {
   //
   // In switching from Misc::use to RecipeAddition::stage, there is no longer an explicit flag for First Wort Miscs.
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

bool RecipeAdditionMisc::isAroma() const {
   //
   // In switching from Misc::use to RecipeAddition::stage, there is no longer an explicit flag for Aroma Miscs, ie those
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
         Q_FUNC_INFO << "RecipeAdditionMisc #" << this->key() << "in Recipe #" << this->m_recipeId <<
         "has boil step #" << *this->step() << "but boil only has" << numBoilSteps << "steps.  This is probably a bug!";
      return false;
   }

   // Remember RecipeAddition steps are numbered from 1, but vectors are indexed from 0
   auto boilStep = boil->boilSteps()[*this->step() - 1];
   if (!boilStep->endTemp_c() || *boilStep->endTemp_c() > Boil::minimumBoilTemperature_c) { return false; }

   return true;
}

Recipe * RecipeAdditionMisc::getOwningRecipe() const {
   return ObjectStoreWrapper::getByIdRaw<Recipe>(this->m_recipeId);
}

NamedEntity * RecipeAdditionMisc::ensureExists(BtStringConst const & property) {
   if (property == PropertyNames::RecipeAdditionMisc::misc) {
      // It's a coding error if a RecipeAdditionMisc doesn't have a Misc by the time we're accessing it via the property
      // system.
      Misc * misc = this->misc();
      if (!misc) {
         qCritical() << Q_FUNC_INFO << "No Misc set on RecipeAdditionMisc #" << this->key();
         // Stop here on debug builds
         Q_ASSERT(false);
      }
      return misc;
   }
   // It's a coding error if we're asked to "create" a relational property we don't know about
   qCritical() << Q_FUNC_INFO << "Don't know how to ensure property" << property << "exists";
   // Stop here on debug builds
   Q_ASSERT(false);
   return nullptr;
}

//============================================= "SETTER" MEMBER FUNCTIONS ==============================================
void RecipeAdditionMisc::setUse(RecipeAdditionMisc::Use const val) {
   switch (val) {
      case RecipeAdditionMisc::Use::Mash:
         this->setStage(RecipeAddition::Stage::Mash);
         break;

      case RecipeAdditionMisc::Use::First_Wort:
         // A first wort misc is in the ramp-up stage of the boil
         this->setStage(RecipeAddition::Stage::Boil);
         this->recipe()->nonOptBoil()->ensureStandardProfile();
         this->setStep(1);
         break;

      case RecipeAdditionMisc::Use::Boil:
         this->setStage(RecipeAddition::Stage::Boil);
         this->recipe()->nonOptBoil()->ensureStandardProfile();
         this->setStep(2);
         break;

      case RecipeAdditionMisc::Use::Aroma:
         // An aroma misc is added during the post-boil
         this->setStage(RecipeAddition::Stage::Boil);
         this->recipe()->nonOptBoil()->ensureStandardProfile();
         this->setStep(3);
         break;

      case RecipeAdditionMisc::Use::Dry_Misc:
         this->setStage(RecipeAddition::Stage::Fermentation);
         break;

      // No default case as we want the compiler to warn us if we missed a case above
   }
   return;
}

void RecipeAdditionMisc::setMisc(Misc * const val) {
   if (val) {
      this->setIngredientId(val->key());
      this->setName(tr("Addition of %1").arg(val->name()));
   } else {
      // Normally we don't want to invalidate the Misc on a RecipeAdditionMisc, because it doesn't buy us anything.
      qWarning() << Q_FUNC_INFO << "Null Misc set on RecipeAdditionMisc #" << this->key();
      this->setIngredientId(-1);
      this->setName(tr("Invalid!"));
   }
   return;
}

// Boilerplate code for IngredientAmount
INGREDIENT_AMOUNT_COMMON_CODE(RecipeAdditionMisc, Misc)
