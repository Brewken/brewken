/*======================================================================================================================
 * model/RecipeAdditionYeast.cpp is part of Brewken, and is copyright the following authors 2023-2024:
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
#include "model/RecipeAdditionYeast.h"

#include "database/ObjectStoreTyped.h"
#include "database/ObjectStoreWrapper.h"
#include "model/NamedParameterBundle.h"
#include "model/Boil.h"
#include "model/BoilStep.h"

QString const RecipeAdditionYeast::LocalisedName = tr("Yeast Addition");

EnumStringMapping const RecipeAdditionYeast::useStringMapping {
   {RecipeAdditionYeast::Use::Mash      , "Mash"      },
   {RecipeAdditionYeast::Use::First_Wort, "First Wort"},
   {RecipeAdditionYeast::Use::Boil      , "Boil"      },
   {RecipeAdditionYeast::Use::Aroma     , "Aroma"     },
   {RecipeAdditionYeast::Use::Dry_Yeast   , "Dry Yeast"   },
};

EnumStringMapping const RecipeAdditionYeast::useDisplayNames {
   {RecipeAdditionYeast::Use::Mash      , tr("Mash"      )},
   {RecipeAdditionYeast::Use::First_Wort, tr("First Wort")},
   {RecipeAdditionYeast::Use::Boil      , tr("Boil"      )},
   {RecipeAdditionYeast::Use::Aroma     , tr("Post-Boil" )},
   {RecipeAdditionYeast::Use::Dry_Yeast   , tr("Dry Yeast"   )},
};

ObjectStore & RecipeAdditionYeast::getObjectStoreTypedInstance() const {
   return ObjectStoreTyped<RecipeAdditionYeast>::getInstance();
}

TypeLookup const RecipeAdditionYeast::typeLookup {
   "RecipeAdditionYeast",
   {
      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::RecipeAdditionYeast::yeast, RecipeAdditionYeast::yeast),
      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::RecipeAdditionYeast::use, RecipeAdditionYeast::use),
   },
   // Parent classes lookup.  NB: RecipeAddition not NamedEntity!
   {&RecipeAddition::typeLookup,
    std::addressof(IngredientAmount<RecipeAdditionYeast, Yeast>::typeLookup)}
};
static_assert(std::is_base_of<RecipeAddition, RecipeAdditionYeast>::value);
static_assert(std::is_base_of<IngredientAmount<RecipeAdditionYeast, Yeast>, RecipeAdditionYeast>::value);

//
// This is a compile-time check that HasTypeLookup is working properly.  It doesn't particularly belong here, but I
// didn't yet find a much better place for it to live!
//
static_assert(HasTypeLookup<Yeast>);
static_assert(!HasTypeLookup<QString>);


RecipeAdditionYeast::RecipeAdditionYeast(QString name, int const recipeId, int const hopId) :
   RecipeAddition{name, recipeId, hopId},
   RecipeAdditionBase<RecipeAdditionYeast, Yeast>{},
   IngredientAmount<RecipeAdditionYeast, Yeast>{} {
   return;
}

RecipeAdditionYeast::RecipeAdditionYeast(NamedParameterBundle const & namedParameterBundle) :
   RecipeAddition{namedParameterBundle},
   RecipeAdditionBase<RecipeAdditionYeast, Yeast>{},
   IngredientAmount<RecipeAdditionYeast, Yeast>{namedParameterBundle} {
   //
   // If the addition stage is not specified then we assume it is boil, as this is the first stage at which it is usual
   // to add hops.
   //
   m_stage = namedParameterBundle.val<RecipeAddition::Stage>(PropertyNames::RecipeAddition::stage,
                                                             RecipeAddition::Stage::Boil);
///   qDebug() << Q_FUNC_INFO << "RecipeAdditionYeast #" << this->key() << ": Recipe #" << this->m_recipeId << ", Yeast #" << this->m_ingredientId;
   return;
}

RecipeAdditionYeast::RecipeAdditionYeast(RecipeAdditionYeast const & other) :
   RecipeAddition{other},
   RecipeAdditionBase<RecipeAdditionYeast, Yeast>{},
   IngredientAmount<RecipeAdditionYeast, Yeast>{other} {
   return;
}

RecipeAdditionYeast::~RecipeAdditionYeast() = default;

//============================================= "GETTER" MEMBER FUNCTIONS ==============================================
RecipeAdditionYeast::Use  RecipeAdditionYeast::use() const {
   switch (this->stage()) {
      case RecipeAddition::Stage::Mash:
         return RecipeAdditionYeast::Use::Mash;

      case RecipeAddition::Stage::Boil:
         if (this->isFirstWort()) {
            return RecipeAdditionYeast::Use::First_Wort;
         }
         if (this->isAroma()) {
            return RecipeAdditionYeast::Use::Aroma;
         }
         return RecipeAdditionYeast::Use::Boil;

      case RecipeAddition::Stage::Fermentation:
      case RecipeAddition::Stage::Packaging:
         return RecipeAdditionYeast::Use::Dry_Yeast;

      // No default case as we want the compiler to warn us if we missed a case above
   }

   // This should be unreachable, but putting a return statement here prevents compiler warnings
   return RecipeAdditionYeast::Use::Boil;
}

Yeast * RecipeAdditionYeast::yeast() const {
   // Normally there should always be a valid Yeast in a RecipeAdditionYeast.  (The Recipe ID may be -1 if the addition is
   // only just about to be added to the Recipe or has just been removed from it, but there's no great reason for the
   // Yeast ID not to be valid).
   if (this->m_ingredientId <= 0) {
      qWarning() << Q_FUNC_INFO << "No Yeast set on RecipeAdditionYeast #" << this->key();
      return nullptr;
   }

///   qDebug() << Q_FUNC_INFO << "RecipeAdditionYeast #" << this->key() << ": Recipe #" << this->m_recipeId << ", Yeast #" << this->m_ingredientId << "@" << ObjectStoreWrapper::getByIdRaw<Yeast>(this->m_ingredientId);
   return ObjectStoreWrapper::getByIdRaw<Yeast>(this->m_ingredientId);
}

bool RecipeAdditionYeast::isFirstWort() const {
   //
   // In switching from Yeast::use to RecipeAddition::stage, there is no longer an explicit flag for First Wort Yeasts.
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

bool RecipeAdditionYeast::isAroma() const {
   //
   // In switching from Yeast::use to RecipeAddition::stage, there is no longer an explicit flag for Aroma Yeasts, ie those
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
         Q_FUNC_INFO << "RecipeAdditionYeast #" << this->key() << "in Recipe #" << this->m_recipeId <<
         "has boil step #" << *this->step() << "but boil only has" << numBoilSteps << "steps.  This is probably a bug!";
      return false;
   }

   // Remember RecipeAddition steps are numbered from 1, but vectors are indexed from 0
   auto boilStep = boil->boilSteps()[*this->step() - 1];
   if (!boilStep->endTemp_c() || *boilStep->endTemp_c() > Boil::minimumBoilTemperature_c) { return false; }

   return true;
}

Recipe * RecipeAdditionYeast::getOwningRecipe() const {
   return ObjectStoreWrapper::getByIdRaw<Recipe>(this->m_recipeId);
}

NamedEntity * RecipeAdditionYeast::ensureExists(BtStringConst const & property) {
   if (property == PropertyNames::RecipeAdditionYeast::yeast) {
      // It's a coding error if a RecipeAdditionYeast doesn't have a Yeast by the time we're accessing it via the property
      // system.
      Yeast * yeast = this->yeast();
      if (!yeast) {
         qCritical() << Q_FUNC_INFO << "No Yeast set on RecipeAdditionYeast #" << this->key();
         // Stop here on debug builds
         Q_ASSERT(false);
      }
      return yeast;
   }
   // It's a coding error if we're asked to "create" a relational property we don't know about
   qCritical() << Q_FUNC_INFO << "Don't know how to ensure property" << property << "exists";
   // Stop here on debug builds
   Q_ASSERT(false);
   return nullptr;
}

//============================================= "SETTER" MEMBER FUNCTIONS ==============================================
void RecipeAdditionYeast::setUse(RecipeAdditionYeast::Use const val) {
   switch (val) {
      case RecipeAdditionYeast::Use::Mash:
         this->setStage(RecipeAddition::Stage::Mash);
         break;

      case RecipeAdditionYeast::Use::First_Wort:
         // A first wort yeast is in the ramp-up stage of the boil
         this->setStage(RecipeAddition::Stage::Boil);
         this->recipe()->nonOptBoil()->ensureStandardProfile();
         this->setStep(1);
         break;

      case RecipeAdditionYeast::Use::Boil:
         this->setStage(RecipeAddition::Stage::Boil);
         this->recipe()->nonOptBoil()->ensureStandardProfile();
         this->setStep(2);
         break;

      case RecipeAdditionYeast::Use::Aroma:
         // An aroma yeast is added during the post-boil
         this->setStage(RecipeAddition::Stage::Boil);
         this->recipe()->nonOptBoil()->ensureStandardProfile();
         this->setStep(3);
         break;

      case RecipeAdditionYeast::Use::Dry_Yeast:
         this->setStage(RecipeAddition::Stage::Fermentation);
         break;

      // No default case as we want the compiler to warn us if we missed a case above
   }
   return;
}

void RecipeAdditionYeast::setYeast(Yeast * const val) {
   if (val) {
      this->setIngredientId(val->key());
      this->setName(tr("Addition of %1").arg(val->name()));
   } else {
      // Normally we don't want to invalidate the Yeast on a RecipeAdditionYeast, because it doesn't buy us anything.
      qWarning() << Q_FUNC_INFO << "Null Yeast set on RecipeAdditionYeast #" << this->key();
      this->setIngredientId(-1);
      this->setName(tr("Invalid!"));
   }
   return;
}

// Boilerplate code for IngredientAmount
INGREDIENT_AMOUNT_COMMON_CODE(RecipeAdditionYeast, Yeast)
