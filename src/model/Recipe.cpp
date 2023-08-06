/*======================================================================================================================
 * model/Recipe.cpp is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Greg Greenaae <ggreenaae@gmail.com>
 *   • Greg Meess <Daedalus12@gmail.com>
 *   • Jonathon Harding <github@jrhardin.net>
 *   • Kregg Kemper <gigatropolis@yahoo.com>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
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
#include "model/Recipe.h"

#include <cmath> // For pow/log
#include <compare> //

#include <QDate>
#include <QDebug>
#include <QInputDialog>
#include <QList>
#include <QObject>

#include "Algorithms.h"
#include "database/ObjectStoreWrapper.h"
#include "HeatCalculations.h"
#include "Localization.h"
#include "measurement/ColorMethods.h"
#include "measurement/IbuMethods.h"
#include "measurement/Measurement.h"
#include "model/Boil.h"
#include "model/BoilStep.h"
#include "model/Equipment.h"
#include "model/Fermentable.h"
#include "model/Fermentation.h"
#include "model/FermentationStep.h"
#include "model/Hop.h"
#include "model/Instruction.h"
#include "model/Mash.h"
#include "model/MashStep.h"
#include "model/Misc.h"
#include "model/NamedParameterBundle.h"
#include "model/RecipeAdditionHop.h"
#include "model/Salt.h"
#include "model/Style.h"
#include "model/Water.h"
#include "model/Yeast.h"
#include "PersistentSettings.h"
#include "PhysicalConstants.h"

namespace {

   /**
    * \brief This is used to assist the creation of instructions.
    */
   struct PreInstruction {
      QString text;
      QString title;
      double  time;
   };
   auto operator<=>(PreInstruction const & lhs, PreInstruction const & rhs) {
      return lhs.time <=> rhs.time;
   }

   /**
    * \brief Check whether the supplied instance of (subclass of) NamedEntity (a) is an "instance of use of" (ie has a
    *        parent) and (b) is not used in any Recipe.
    */
   template<class NE> bool isUnusedInstanceOfUseOf(NE & var) {
      NE * parentOfVar = static_cast<NE *>(var.getParent());
      if (nullptr == parentOfVar) {
         // The var has no parent and so is not "an instance of use of"
         return false;
      }

      qDebug() <<
         Q_FUNC_INFO << var.metaObject()->className() << "#" << var.key() << "has parent #" << parentOfVar->key();
      //
      // Parameter has a parent.  See if it (the parameter, not its parent!) is used in a recipe.
      // (NB: The parent of the NamedEntity is not the same thing as its parent recipe.  We should perhaps find some
      // different terms!)
      //
      auto matchingRecipe = ObjectStoreTyped<Recipe>::getInstance().findFirstMatching(
         // NB: Important to do the lambda capture of var here by reference, otherwise we'll be passing in a copy of
         //     var, which won't have an ID and therefore will never give a match.
         [&var](Recipe * recipe) {
            return recipe->uses(var);
         }
      );
      if (matchingRecipe == nullptr) {
         // The parameter is not already used in a recipe, so we'll be able to add it without making a copy
         // Note that we can't just take the address of var and use it to make a new shared_ptr as that would mean
         // we had two completely unrelated shared_ptr objects (one in the object store and one newly created here)
         // pointing to the same address.  We need to get an instance of shared_ptr that's copied from (and thus
         // shares the internal reference count of) the one held by the object store.
         qDebug() << Q_FUNC_INFO << var.metaObject()->className() << "#" << var.key() << "not used in any recipe";
         return true;
      }

      // The var is used in another Recipe.  (We shouldn't really find ourselves in this position, but the way the rest
      // of the code works means that, even if we do, we should recover OK - or at least not make the situation any
      // worse.)
      qWarning() <<
         Q_FUNC_INFO << var.metaObject()->className() << "#" << var.key() <<
         "is unexpectedly already used in recipe #" << matchingRecipe->key();
      return false;
   }

   /**
    * \brief Decide whether the supplied instance of (subclass of) NamedEntity needs to be copied before being added to
    *        a recipe.
    *
    * \param var The Hop/Fermentable/etc that we want to add to a Recipe.  We'll either add it directly or make a copy
    *            of it and add that.
    *
    * \return A copy of var if it needs to be copied (either because it has no parent or because it is already used in
    *         another recipe), or var itself otherwise
    */
   template<class NE> std::shared_ptr<NE> copyIfNeeded(NE & var) {
      // It's the caller's responsibility to ensure var is already in an ObjectStore
      Q_ASSERT(var.key() > 0);

      //
      // If the supplied Hop/Fermentable/etc has no parent then we need to make a copy of it, because it's the master
      // instance of that Hop/Fermentable/etc.
      //
      // Otherwise, if it has a parent, then whether we need to make a copy depends on whether it is already used in a
      // recipe (_including_ this one, because the same ingredient can be added more than once to a recipe - eg Hops
      // added at different times).
      //
      // All this logic is handled in isUnusedInstanceOfUseOf() because it's the same process for checking it's OK to
      // delete something when it's been removed from a Recipe.
      //
      if (isUnusedInstanceOfUseOf(var)) {
         return ObjectStoreWrapper::getById<NE>(var.key());
      }

      qDebug() << Q_FUNC_INFO << "Making copy of " << var.metaObject()->className() << "#" << var.key();

      // We need to make a copy...
      auto copy = std::make_shared<NE>(var);
      // ...then make sure the copy is a "child" (ie "instance of use of")...
      copy->makeChild(var);
      // ...and finally ensure the copy is stored.
      ObjectStoreWrapper::insert(copy);
      return copy;
   }
   template<> std::shared_ptr<RecipeAdditionHop> copyIfNeeded(RecipeAdditionHop & var) {
      qDebug() << Q_FUNC_INFO << "Making copy of " << var.metaObject()->className() << "#" << var.key();

      auto copy = std::make_shared<RecipeAdditionHop>(var);
      ObjectStoreWrapper::insert(copy);
      return copy;
   }

   //
   // After we modified a property via a templated member function of Recipe, we need to tell the object store to
   // update the database.  These template specialisations map from property type to property name.
   //
   template<class NE> BtStringConst const & propertyToPropertyName();
   template<> BtStringConst const & propertyToPropertyName<Boil        >() { return PropertyNames::Recipe::boilId        ; }
   template<> BtStringConst const & propertyToPropertyName<Equipment   >() { return PropertyNames::Recipe::equipmentId   ; }
   template<> BtStringConst const & propertyToPropertyName<Fermentable >() { return PropertyNames::Recipe::fermentableIds; }
   template<> BtStringConst const & propertyToPropertyName<Fermentation>() { return PropertyNames::Recipe::fermentationId; }
   template<> BtStringConst const & propertyToPropertyName<RecipeAdditionHop>() { return PropertyNames::Recipe::hopAdditionIds        ; }
   template<> BtStringConst const & propertyToPropertyName<Instruction >() { return PropertyNames::Recipe::instructionIds; }
   template<> BtStringConst const & propertyToPropertyName<Mash        >() { return PropertyNames::Recipe::mashId        ; }
   template<> BtStringConst const & propertyToPropertyName<Misc        >() { return PropertyNames::Recipe::miscIds       ; }
   template<> BtStringConst const & propertyToPropertyName<Salt        >() { return PropertyNames::Recipe::saltIds       ; }
   template<> BtStringConst const & propertyToPropertyName<Style       >() { return PropertyNames::Recipe::styleId       ; }
   template<> BtStringConst const & propertyToPropertyName<Water       >() { return PropertyNames::Recipe::waterIds      ; }
   template<> BtStringConst const & propertyToPropertyName<Yeast       >() { return PropertyNames::Recipe::yeastIds      ; }

///   QHash<QString, Recipe::Type> const RECIPE_TYPE_STRING_TO_TYPE {
///      {"Extract",      Recipe::Type::Extract},
///      {"Partial Mash", Recipe::Type::PartialMash},
///      {"All Grain",    Recipe::Type::AllGrain}
///   };


   bool isFermentableSugar(Fermentable * fermy) {
      if (fermy->type() == Fermentable::Type::Sugar && fermy->name() == "Milk Sugar (Lactose)") {
         return false;
      }

      return true;
   }
}


// This private implementation class holds all private non-virtual members of Recipe
class Recipe::impl {
public:

   /**
    * Constructor
    */
   impl(Recipe & self) :
      m_self{self},
      fermentableIds{},
      instructionIds{},
      miscIds{},
      saltIds{},
      waterIds{},
      yeastIds{} {
      return;
   }

   /**
    * Destructor
    */
   ~impl() = default;

   /**
    * \brief Make copies of the additions of a particular type (\c RecipeAdditionHop, \c RecipeAdditionFermentable,
    *        etc) from one \c Recipe and add them to another - typically because we are copying the \c Recipe.
    */
   template<class RA> void copyAdditions(Recipe & us, Recipe const & other) {
      for (RA * otherAddition : other.pimpl->allMyRaw<RA>()) {
         // Make and store a copy of the current Hop/Fermentable/etc object we're looking at in the other Recipe
         std::shared_ptr<RA> ourAddition = std::make_shared<RA>(*otherAddition);
         this->m_self.addAddition(ourAddition);
      }
      return;
   }

   /**
    * \brief Make copies of the ingredients of a particular type (Hop, Fermentable, etc) from one Recipe and add them
    *        to another - typically because we are copying the Recipe.
    */
   template<class NE> void copyList(Recipe & us, Recipe const & other) {
      qDebug() << Q_FUNC_INFO;
      for (int otherIngId : other.pimpl->accessIds<NE>()) {
         // Make and store a copy of the current Hop/Fermentable/etc object we're looking at in the other Recipe
         auto otherIngredient = ObjectStoreWrapper::getById<NE>(otherIngId);
         auto ourIngredient = copyIfNeeded(*otherIngredient);
         // Store the ID of the copy in our recipe
         this->accessIds<NE>().append(ourIngredient->key());

         qDebug() <<
            Q_FUNC_INFO << "After adding" << ourIngredient->metaObject()->className() << "#" << ourIngredient->key() <<
            ", Recipe" << us.name() << "has" << this->accessIds<NE>().size() << "of" <<
            NE::staticMetaObject.className();

         // Connect signals so that we are notified when there are changes to the Hop/Fermentable/etc we just added to
         // our recipe.
         connect(ourIngredient.get(), &NamedEntity::changed, &us, &Recipe::acceptChangeToContainedObject);
      }
      return;
   }

   /**
    * \brief If the Recipe is about to be deleted, we delete all the things that belong to it.
    */
   template<class NE> void hardDeleteAdditions() {
      qDebug() << Q_FUNC_INFO;
      for (int id : this->allMyIds<NE>()) {
         ObjectStoreWrapper::hardDelete<NE>(id);
      }
   }

   /**
    * \brief If the Recipe is about to be deleted, we delete all the things that belong to it.  Note that, with the
    *        exception of Instruction, what we are actually deleting here is not the Hops/Fermentables/etc but the "use
    *        of" Hops/Fermentables/etc records (which are distinguished by having a parent ID.
    */
   template<class NE> void hardDeleteAllMy() {
      qDebug() << Q_FUNC_INFO;
      for (auto id : this->accessIds<NE>()) {
         ObjectStoreWrapper::hardDelete<NE>(id);
      }
      return;
   }

   /**
    * \brief Get shared pointers to all this Recipe's BrewNotes or RecipeAdditions of a particular type
    *        (RecipeAdditionHop, RecipeAdditionFermentable, etc).
    */
   template<class NE>
   QList<std::shared_ptr<NE>> allMy() const {
      int const recipeId = this->m_self.key();
      return ObjectStoreWrapper::findAllMatching<NE>([recipeId](std::shared_ptr<NE> ne) {
                                                        return ne->recipeId() == recipeId;
                                                     });
   }

   /**
    * \brief Get raw pointers to all this Recipe's BrewNotes or RecipeAdditions of a particular type (RecipeAdditionHop,
    *        RecipeAdditionFermentable, etc).
    */
   template<class NE>
   QList<NE *> allMyRaw() const {
      int const recipeId = this->m_self.key();
      return ObjectStoreWrapper::findAllMatching<NE>([recipeId](NE const * ne) {
                                                        return ne->recipeId() == recipeId;
                                                     });
   }

   /**
    * \brief Get IDs of all this Recipe's BrewNotes or RecipeAdditions of a particular type (RecipeAdditionHop,
    *        RecipeAdditionFermentable, etc).
    */
   template<class NE>
   QVector<int> allMyIds() const {
      int const recipeId = this->m_self.key();
      return ObjectStoreWrapper::idsOfAllMatching<NE>([recipeId](NE const * ne) {
                                                         return ne->recipeId() == recipeId;
                                                      });
   }

   //
   // .:TODO:. This will go away once we lose junction tables etc for Fermentables, Miscs and replace them with proper
   //          RecipeAddition objects.
   //
   // Inside the class implementation, it's useful to be able to access fermentableIds, hopIds, etc in templated
   // functions.  This allows us to write this->accessIds<NE>() in such a function and have it resolve to
   // this->accessIds<Fermentable>(), this->accessIds<Hop>(), etc, which in turn returns this->fermentableIds,
   // this->hopIds.
   //
   // Note that the specialisations need to be defined outside the class
   //
   template<class NE> QVector<int> & accessIds();

   /**
    * \brief Get shared pointers to all ingredients etc of a particular type (Hop, Fermentable, etc) in this Recipe
    */
   template<class NE> QList< std::shared_ptr<NE> > getAllMy() {
      return ObjectStoreTyped<NE>::getInstance().getByIds(this->accessIds<NE>());
   }

   /**
    * \brief Get raw pointers to all ingredients etc of a particular type (Hop, Fermentable, etc) in this Recipe
    */
   template<class NE> QList<NE *> getAllMyRaw() {
      return ObjectStoreTyped<NE>::getInstance().getByIdsRaw(this->accessIds<NE>());
   }

   /**
    * \brief Connect signals for this Recipe.  See comment for \c Recipe::connectSignalsForAllRecipes for more
    *        explanation.
    */
   void connectSignals() {
      Equipment * equipment = this->m_self.equipment();
      if (equipment) {
         // We used to have special signals for changes to Equipment's boilSize_l and boilTime_min properties, but these
         // are now picked up in Recipe::acceptChangeToContainedObject from the generic `changed` signal
         connect(equipment, &NamedEntity::changed,           &this->m_self, &Recipe::acceptChangeToContainedObject);
      }

      QList<Fermentable *> fermentables = this->m_self.fermentables();
      for (auto fermentable : fermentables) {
         connect(fermentable, &NamedEntity::changed, &this->m_self, &Recipe::acceptChangeToContainedObject);
      }

      QList<RecipeAdditionHop *> hopAdditions = this->m_self.hopAdditions();
      for (auto hopAddition : hopAdditions) {
         connect(hopAddition->hop(), &NamedEntity::changed, &this->m_self, &Recipe::acceptChangeToContainedObject);
      }

      QList<Yeast *> yeasts = this->m_self.yeasts();
      for (auto yeast : yeasts) {
         connect(yeast, &NamedEntity::changed, &this->m_self, &Recipe::acceptChangeToContainedObject);
      }

      Mash * mash = this->m_self.mash();
      if (mash) {
         connect(mash, &NamedEntity::changed, &this->m_self, &Recipe::acceptChangeToContainedObject);
      }

      return;
   }

   template<class NE>
   void setStepOwner(std::shared_ptr<NE> val, int & ourId, BtStringConst const & property) {
      if (val->key() == ourId) {
         return;
      }

      if (ourId > 0) {
         std::shared_ptr<NE> oldVal = ObjectStoreWrapper::getById<NE>(ourId);
         disconnect(oldVal.get(), nullptr, &this->m_self, nullptr);
      }

      std::shared_ptr<NE> valToAdd = copyIfNeeded(*val);
      ourId = valToAdd->key();
      this->m_self.propagatePropertyChange(propertyToPropertyName<NE>());

      connect(valToAdd.get(), &NamedEntity::changed, &this->m_self, &Recipe::acceptChangeToContainedObject);
      emit this->m_self.changed(this->m_self.metaProperty(*property), QVariant::fromValue<NE *>(valToAdd.get()));

      this->m_self.recalcAll();

      return;
   }

   template<class NE>
   void setStepOwner(NE * val, int & ourId, BtStringConst const & property) {
      Q_ASSERT(val);
      this->setStepOwner<NE>(ObjectStoreWrapper::getSharedFromRaw(val), ourId, property);
      return;
   }

   template<class NE>
   void setStepOwner(std::optional<std::shared_ptr<NE>> val, int & ourId, BtStringConst const & property) {
      if (!val && ourId < 0) {
         // No change (from "not set" to "not set")
         return;
      }
      if (val && val.value()->key() == ourId) {
         // No change (same object as we already have)
         return;
      }

      if (ourId > 0) {
         std::shared_ptr<NE> oldVal = ObjectStoreWrapper::getById<NE>(ourId);
         disconnect(oldVal.get(), nullptr, &this->m_self, nullptr);
         // TBD: We should probably delete oldVal here as we "own" it
      }

      if (!val) {
         ourId = -1;
         return;
      }

      std::shared_ptr<NE> valToAdd = copyIfNeeded(**val);
      ourId = valToAdd->key();
      this->m_self.propagatePropertyChange(propertyToPropertyName<NE>());

      connect(valToAdd.get(), &NamedEntity::changed, &this->m_self, &Recipe::acceptChangeToContainedObject);
      emit this->m_self.changed(this->m_self.metaProperty(*property), QVariant::fromValue<NE *>(valToAdd.get()));

      this->m_self.recalcAll();

      return;
   }

   QVector<PreInstruction> mashInstructions(double timeRemaining,
                                            double totalWaterAdded_l,
                                            [[maybe_unused]] unsigned int size) {
      QVector<PreInstruction> preins;
      if (!m_self.mash()) {
         return preins;
      }

      for (auto step : m_self.mash()->mashSteps()) {
         QString str;
         if (step->isInfusion()) {
            str = tr("Add %1 water at %2 to mash to bring it to %3.")
                  .arg(Measurement::displayAmount(Measurement::Amount{step->amount_l(), Measurement::Units::liters}))
                  .arg(Measurement::displayAmount(Measurement::Amount{step->infuseTemp_c().value_or(step->stepTemp_c()), Measurement::Units::celsius}))
                  .arg(Measurement::displayAmount(Measurement::Amount{step->stepTemp_c(), Measurement::Units::celsius}));
            totalWaterAdded_l += step->amount_l();
         } else if (step->isTemperature()) {
            str = tr("Heat mash to %1.").arg(Measurement::displayAmount(Measurement::Amount{step->stepTemp_c(),
                                                                                          Measurement::Units::celsius}));
         } else if (step->isDecoction()) {
            str = tr("Bring %1 of the mash to a boil and return to the mash tun to bring it to %2.")
                  .arg(Measurement::displayAmount(Measurement::Amount{step->amount_l(),
                                                                     Measurement::Units::liters}))
                  .arg(Measurement::displayAmount(Measurement::Amount{step->stepTemp_c(), Measurement::Units::celsius}));
         }

         str += tr(" Hold for %1.").arg(Measurement::displayAmount(Measurement::Amount{step->stepTime_min(),
                                                                                       Measurement::Units::minutes}));

         preins.push_back(PreInstruction(str, QString("%1 - %2").arg(MashStep::typeDisplayNames[step->type()]).arg(step->name()),
                                       timeRemaining));
         timeRemaining -= step->stepTime_min();
      }
      return preins;
   }

   QVector<PreInstruction> hopSteps(RecipeAddition::Stage const stage) {
      // TBD: What about hopAddition->addAtTime_mins()?
      QVector<PreInstruction> preins;
      for (auto hopAddition : m_self.hopAdditions()) {
         Hop * hop = hopAddition->hop();
         if (hopAddition->stage() == stage) {
            QString str;
            switch (stage) {
               case RecipeAddition::Stage::Mash:
                  str = tr("Put %1 %2 into mash for %3.");
                  break;
               case RecipeAddition::Stage::Boil:
                  if (hopAddition->isFirstWort()) {
                     str = tr("Put %1 %2 into first wort for %3.");
                  } else if (hopAddition->isAroma()) {
                     str = tr("Steep %1 %2 in wort for %3.");
                  } else {
                     str = tr("Put %1 %2 into boil for %3.");
                  }
                  break;
               case RecipeAddition::Stage::Fermentation:
                  str = tr("Put %1 %2 into fermenter for %3.");
                  break;
               case RecipeAddition::Stage::Packaging:
                  // We don't really support this yet, but best to say something if we read in a recipe that has this
                  str = tr("Put %1 %2 into packaging for %3.");
                  break;
               // NB: No default case as we want compiler to warn us if we missed a value above
            }

            str = str.arg(Measurement::displayAmount(hopAddition->amountWithUnits()))
                     .arg(hop->name())
                     .arg(Measurement::displayAmount(Measurement::Amount{hopAddition->duration_mins().value_or(0.0), Measurement::Units::minutes}));

            preins.push_back(PreInstruction(str, tr("Hop addition"), hopAddition->duration_mins().value_or(0.0)));
         }
      }
      return preins;
   }

   QVector<PreInstruction> miscSteps(Misc::Use type) {
      QVector<PreInstruction> preins;

      QList<Misc *> mlist = m_self.miscs();
      int size = mlist.size();
      for (unsigned int i = 0; static_cast<int>(i) < size; ++i) {
         QString str;
         Misc * misc = mlist[static_cast<int>(i)];
         if (misc->use() == type) {
            if (type == Misc::Use::Boil) {
               str = tr("Put %1 %2 into boil for %3.");
            } else if (type == Misc::Use::Bottling) {
               str = tr("Use %1 %2 at bottling for %3.");
            } else if (type == Misc::Use::Mash) {
               str = tr("Put %1 %2 into mash for %3.");
            } else if (type == Misc::Use::Primary) {
               str = tr("Put %1 %2 into primary for %3.");
            } else if (type == Misc::Use::Secondary) {
               str = tr("Put %1 %2 into secondary for %3.");
            } else {
               qWarning() << "Recipe::getMiscSteps(): Unrecognized misc use.";
               str = tr("Use %1 %2 for %3.");
            }

            str = str .arg(Measurement::displayAmount(Measurement::Amount{
                                                         misc->amount(),
                                                         misc->amountIsWeight() ? Measurement::Units::kilograms : Measurement::Units::liters
                                                      }))
                  .arg(misc->name())
                  .arg(Measurement::displayAmount(Measurement::Amount{misc->time_min(), Measurement::Units::minutes}));

            preins.push_back(PreInstruction(str, tr("Misc addition"), misc->time_min()));
         }
      }
      return preins;
   }

   PreInstruction boilFermentablesPre(double timeRemaining) {
      QString str = tr("Boil or steep ");
      QList<Fermentable *> flist = m_self.fermentables();
      int size = flist.size();
      for (int i = 0; static_cast<int>(i) < size; ++i) {
         Fermentable * ferm = flist[i];
         if (ferm->isMashed() || ferm->addAfterBoil() || ferm->isExtract()) {
            continue;
         }

         str += QString("%1 %2, ")
               .arg(Measurement::displayAmount(ferm->amountWithUnits()))
               .arg(ferm->name());
      }
      str += ".";

      return PreInstruction(str, tr("Boil/steep fermentables"), timeRemaining);
   }

   bool hasBoilFermentable() {
      int i;
      for (i = 0; static_cast<int>(i) < m_self.fermentables().size(); ++i) {
         Fermentable * ferm = m_self.fermentables()[i];
         if (ferm->isMashed() || ferm->addAfterBoil()) {
            continue;
         } else {
            return true;
         }
      }
      return false;
   }

   bool hasBoilExtract() {
      int i;
      for (i = 0; static_cast<int>(i) < m_self.fermentables().size(); ++i) {
         Fermentable * ferm = m_self.fermentables()[i];
         if (ferm->isExtract()) {
            return true;
         } else {
            continue;
         }
      }
      return false;
   }

   PreInstruction addExtracts(double timeRemaining) const {
      QString str = tr("Raise water to boil and then remove from heat. Stir in  ");
      const QList<Fermentable *> flist = m_self.fermentables();
      int size = flist.size();
      for (int i = 0; static_cast<int>(i) < size; ++i) {
         const Fermentable * ferm = flist[i];
         if (ferm->isExtract()) {
            str += QString("%1 %2, ")
                  .arg(Measurement::displayAmount(ferm->amountWithUnits()))
                  .arg(ferm->name());
         }
      }
      str += ".";

      return PreInstruction(str, tr("Add Extracts to water"), timeRemaining);
   }

   void addPreinstructions(QVector<PreInstruction> preins) {
      // Add instructions in descending mash time order.
      std::sort(preins.begin(), preins.end(), std::greater<PreInstruction>());
      for (int ii = 0; ii < preins.size(); ++ii) {
         PreInstruction pi = preins[ii];

         auto ins = std::make_shared<Instruction>();
         ins->setName(pi.title);
         ins->setDirections(pi.text);
         ins->setInterval(pi.time);

         m_self.add(ins);
      }
      return;
   }


   /**
    * \brief This does the logic for \c nonOptBoil, \c nonOptFermentation, etc
    *
    * \param propertyName IN
    * \param itemId IN/OUT
    */
   template<class T>
   std::shared_ptr<T> nonOptionalItem(BtStringConst const & propertyName, int & itemId) {
      if (itemId < 0) {
         std::shared_ptr<T> item{std::make_shared<T>()};
         this->m_self.setAndNotify(propertyName, itemId, ObjectStoreWrapper::insert(item));
      }
      return ObjectStoreWrapper::getById<T>(itemId);
   }

   /**
    * \brief Returns the boil size in liters, or the supplied value if there is either no boil or no boil size set on
    *        the boil.
    */
   double boilSizeInLitersOr(double const defaultValue) const {
      if (this->m_self.m_boilId < 0) {
         return defaultValue;
      }
      return (*this->m_self.boil())->preBoilSize_l().value_or(defaultValue);
   }

   /**
    * \brief Returns the boil time in minutes, or the supplied value if there is no boil.
    */
   double boilTimeInMinutesOr(double const defaultValue) const {
      if (this->m_self.m_boilId < 0) {
         return defaultValue;
      }
      return (*this->m_self.boil())->boilTime_mins();
   }

   //================================================ Member variables =================================================
   Recipe & m_self;
   QVector<int> fermentableIds;
///   QVector<int> m_hopAdditionIds;
   QVector<int> instructionIds;
   QVector<int> miscIds;
   QVector<int> saltIds;
   QVector<int> waterIds;
   QVector<int> yeastIds;

};

template<> QVector<int> & Recipe::impl::accessIds<Fermentable>() { return this->fermentableIds; }
///template<> QVector<int> & Recipe::impl::accessIds<RecipeAdditionHop>()         { return this->m_hopAdditionIds; }
template<> QVector<int> & Recipe::impl::accessIds<Instruction>() { return this->instructionIds; }
template<> QVector<int> & Recipe::impl::accessIds<Misc>()        { return this->miscIds; }
template<> QVector<int> & Recipe::impl::accessIds<Salt>()        { return this->saltIds; }
template<> QVector<int> & Recipe::impl::accessIds<Water>()       { return this->waterIds; }
template<> QVector<int> & Recipe::impl::accessIds<Yeast>()       { return this->yeastIds; }

QString const Recipe::LocalisedName = tr("Recipe");

// Note that Recipe::typeStringMapping and Recipe::FormMapping are as defined by BeerJSON, but we also use them for the DB and
// for the UI.  We can't use them for BeerXML as it only supports subsets of these types.
EnumStringMapping const Recipe::typeStringMapping {
   {Recipe::Type::Extract    , "extract"     },
   {Recipe::Type::PartialMash, "partial mash"},
   {Recipe::Type::AllGrain   , "all grain"   },
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   {Recipe::Type::Cider      , "cider"       },
   {Recipe::Type::Kombucha   , "kombucha"    },
   {Recipe::Type::Soda       , "soda"        },
   {Recipe::Type::Other      , "other"       },
   {Recipe::Type::Mead       , "mead"        },
   {Recipe::Type::Wine       , "wine"        },
};

EnumStringMapping const Recipe::typeDisplayNames {
   {Recipe::Type::Extract    , tr("Extract"     )},
   {Recipe::Type::PartialMash, tr("Partial Mash")},
   {Recipe::Type::AllGrain   , tr("All Grain"   )},
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   {Recipe::Type::Cider      , tr("Cider"       )},
   {Recipe::Type::Kombucha   , tr("Kombucha"    )},
   {Recipe::Type::Soda       , tr("Soda"        )},
   {Recipe::Type::Other      , tr("Other"       )},
   {Recipe::Type::Mead       , tr("Mead"        )},
   {Recipe::Type::Wine       , tr("Wine"        )},
};


bool Recipe::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   Recipe const & rhs = static_cast<Recipe const &>(other);
   // Base class will already have ensured names are equal
   return (
      this->m_type              == rhs.m_type              &&
      this->m_batchSize_l       == rhs.m_batchSize_l       &&
      this->m_efficiency_pct    == rhs.m_efficiency_pct    &&
      this->m_primaryAge_days   == rhs.m_primaryAge_days   &&
      this->m_primaryTemp_c     == rhs.m_primaryTemp_c     &&
      this->m_secondaryAge_days == rhs.m_secondaryAge_days &&
      this->m_secondaryTemp_c   == rhs.m_secondaryTemp_c   &&
      this->m_tertiaryAge_days  == rhs.m_tertiaryAge_days  &&
      this->m_tertiaryTemp_c    == rhs.m_tertiaryTemp_c    &&
      this->m_age               == rhs.m_age               &&
      this->m_ageTemp_c         == rhs.m_ageTemp_c         &&
      ObjectStoreWrapper::compareById<Style    >(this->m_styleId,     rhs.m_styleId    ) &&
      ObjectStoreWrapper::compareById<Mash     >(this->m_mashId,      rhs.m_mashId     ) &&
      ObjectStoreWrapper::compareById<Boil     >(this->m_boilId,      rhs.m_boilId     ) &&
      ObjectStoreWrapper::compareById<Equipment>(this->m_equipmentId, rhs.m_equipmentId) &&
      this->m_og                == rhs.m_og                &&
      this->m_fg                == rhs.m_fg                &&
      ObjectStoreWrapper::compareListByIds<Fermentable      >(this->pimpl->fermentableIds  , rhs.pimpl->fermentableIds  ) &&
///      ObjectStoreWrapper::compareListByIds<RecipeAdditionHop>(this->pimpl->m_hopAdditionIds, rhs.pimpl->m_hopAdditionIds) &&
      ObjectStoreWrapper::compareListByIds<Instruction      >(this->pimpl->instructionIds  , rhs.pimpl->instructionIds  ) &&
      ObjectStoreWrapper::compareListByIds<Misc             >(this->pimpl->miscIds         , rhs.pimpl->miscIds         ) &&
      ObjectStoreWrapper::compareListByIds<Salt             >(this->pimpl->saltIds         , rhs.pimpl->saltIds         ) &&
      ObjectStoreWrapper::compareListByIds<Water            >(this->pimpl->waterIds        , rhs.pimpl->waterIds        ) &&
      ObjectStoreWrapper::compareListByIds<Yeast            >(this->pimpl->yeastIds        , rhs.pimpl->yeastIds        )

      // TODO: What about BrewNote and RecipeAdditionHop
   );
}

ObjectStore & Recipe::getObjectStoreTypedInstance() const {
   return ObjectStoreTyped<Recipe>::getInstance();
}

TypeLookup const Recipe::typeLookup {
   "Recipe",
   {
      // Note that the age_days, primaryAge_days, secondaryAge_days, tertiaryAge_days properties are dimensionless
      // because:
      //    - It's not meaningful to measure them with greater precision
      //    - The canonical unit for Measurement::PhysicalQuantity::Time is Measurement::Units::minutes, so we'd have to
      //      either store as minutes or do some special-case handling to say we're not storing in canonical units.
      //      Both would be ugly
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::type              , Recipe::m_type              ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::brewer            , Recipe::m_brewer            ,           NonPhysicalQuantity::String        ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::asstBrewer        , Recipe::m_asstBrewer        ,           NonPhysicalQuantity::String        ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::batchSize_l       , Recipe::m_batchSize_l       , Measurement::PhysicalQuantity::Volume        ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::efficiency_pct    , Recipe::m_efficiency_pct    ,           NonPhysicalQuantity::Percentage    ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::fermentationStages, Recipe::m_fermentationStages,           NonPhysicalQuantity::Count         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::primaryAge_days   , Recipe::m_primaryAge_days   ,           NonPhysicalQuantity::Dimensionless ), // See comment above for why Dimensionless, not Time
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::primaryTemp_c     , Recipe::m_primaryTemp_c     , Measurement::PhysicalQuantity::Temperature   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::secondaryAge_days , Recipe::m_secondaryAge_days ,           NonPhysicalQuantity::Dimensionless ), // See comment above for why Dimensionless, not Time
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::secondaryTemp_c   , Recipe::m_secondaryTemp_c   , Measurement::PhysicalQuantity::Temperature   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::tertiaryAge_days  , Recipe::m_tertiaryAge_days  ,           NonPhysicalQuantity::Dimensionless ), // See comment above for why Dimensionless, not Time
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::tertiaryTemp_c    , Recipe::m_tertiaryTemp_c    , Measurement::PhysicalQuantity::Temperature   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::age_days          , Recipe::m_age               ,           NonPhysicalQuantity::Dimensionless ), // See comment above for why Dimensionless, not Time
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::ageTemp_c         , Recipe::m_ageTemp_c         , Measurement::PhysicalQuantity::Temperature   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::date              , Recipe::m_date              ,           NonPhysicalQuantity::Date          ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::carbonation_vols  , Recipe::m_carbonation_vols  , Measurement::PhysicalQuantity::Carbonation   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::forcedCarbonation , Recipe::m_forcedCarbonation ,           NonPhysicalQuantity::Bool          ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::primingSugarName  , Recipe::m_primingSugarName  ,           NonPhysicalQuantity::String        ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::carbonationTemp_c , Recipe::m_carbonationTemp_c , Measurement::PhysicalQuantity::Temperature   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::primingSugarEquiv , Recipe::m_primingSugarEquiv ,           NonPhysicalQuantity::Dimensionless ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::kegPrimingFactor  , Recipe::m_kegPrimingFactor  ,           NonPhysicalQuantity::Dimensionless ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::notes             , Recipe::m_notes             ,           NonPhysicalQuantity::String        ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::tasteNotes        , Recipe::m_tasteNotes        ,           NonPhysicalQuantity::String        ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::tasteRating       , Recipe::m_tasteRating       ,           NonPhysicalQuantity::Dimensionless ),
//      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::style             , Recipe::m_style             ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::styleId           , Recipe::m_styleId             ), //<<
//      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::mash              , Recipe::m_mash              ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::mashId            , Recipe::m_mashId              ), //<<
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::boilId            , Recipe::m_boilId              ), //<<
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::fermentationId    , Recipe::m_fermentationId      ), //<<
//      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::equipment         , Recipe::m_equipment         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::equipmentId       , Recipe::m_equipmentId         ), //<<
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::og                , Recipe::m_og                , Measurement::PhysicalQuantity::Density       ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::fg                , Recipe::m_fg                , Measurement::PhysicalQuantity::Density       ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::locked            , Recipe::m_locked            ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::ancestorId        , Recipe::m_ancestor_id       ), //<<
//      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::ancestors        , Recipe::m_ancestor_id       ), //<<

      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::ABV_pct           , Recipe::m_ABV_pct           ,           NonPhysicalQuantity::Percentage    ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::boilGrav          , Recipe::m_boilGrav          , Measurement::PhysicalQuantity::Density       ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::boilVolume_l      , Recipe::m_boilVolume_l      , Measurement::PhysicalQuantity::Volume        ),
//      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::brewNotes         , Recipe::m_brewNotes         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::calories          , Recipe::m_calories          ,           NonPhysicalQuantity::Dimensionless ), // .:TBD:. One day this should perhaps become Measurement::PhysicalQuantity::Energy
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::color_srm         , Recipe::m_color_srm         , Measurement::PhysicalQuantity::Color         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::fermentableIds    , Recipe::impl::fermentableIds),
//      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::fermentables      , Recipe::m_fermentables      ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::finalVolume_l     , Recipe::m_finalVolume_l     , Measurement::PhysicalQuantity::Volume        ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::grainsInMash_kg   , Recipe::m_grainsInMash_kg   , Measurement::PhysicalQuantity::Mass          ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::grains_kg         , Recipe::m_grains_kg         , Measurement::PhysicalQuantity::Mass          ),
//      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::hops              , Recipe::m_hops              ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::IBU               , Recipe::m_IBU               , Measurement::PhysicalQuantity::Bitterness    ),
//      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::IBUs              , Recipe::m_IBUs              ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::instructionIds    , Recipe::impl::instructionIds),
//      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::instructions      , Recipe::m_instructions      ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::miscIds           , Recipe::impl::miscIds       ),
//      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::miscs             , Recipe::m_miscs             ),
//      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::points            , Recipe::m_points            ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::postBoilVolume_l  , Recipe::m_postBoilVolume_l  , Measurement::PhysicalQuantity::Volume        ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::saltIds           , Recipe::impl::saltIds       ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::SRMColor          , Recipe::m_SRMColor          ), // NB: This is an RGB display color
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::waterIds          , Recipe::impl::waterIds      ),
//      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::waters            , Recipe::m_waters            ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::wortFromMash_l    , Recipe::m_wortFromMash_l    , Measurement::PhysicalQuantity::Volume        ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::yeastIds          , Recipe::impl::yeastIds      ),
//      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Recipe::yeasts            , Recipe::m_yeasts            ),

      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::Recipe::hopAdditionIds, Recipe::hopAdditionIds    ),
      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::Recipe::boilSize_l    , Recipe::boilSize_l        , Measurement::PhysicalQuantity::Volume        ),
      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::Recipe::boilTime_min  , Recipe::boilTime_min      , Measurement::PhysicalQuantity::Time          ),
   },
   // Parent class lookup
   &NamedEntity::typeLookup
};

Recipe::Recipe(QString name) :
   NamedEntity         {name, true                   },
   pimpl               {std::make_unique<impl>(*this)},
   m_type              {Recipe::Type::AllGrain       },
   m_brewer            {""                           },
   m_asstBrewer        {"Brewken: free beer software"},
   m_batchSize_l       {0.0                          },
   m_efficiency_pct    {0.0                          },
   m_fermentationStages{1                            },
   m_primaryAge_days   {0.0                          },
   m_primaryTemp_c     {0.0                          },
   m_secondaryAge_days {0.0                          },
   m_secondaryTemp_c   {0.0                          },
   m_tertiaryAge_days  {0.0                          },
   m_tertiaryTemp_c    {0.0                          },
   m_age               {0.0                          },
   m_ageTemp_c         {0.0                          },
   m_date              {QDate::currentDate()         },
   m_carbonation_vols  {0.0                          },
   m_forcedCarbonation {false                        },
   m_primingSugarName  {""                           },
   m_carbonationTemp_c {0.0                          },
   m_primingSugarEquiv {0.0                          },
   m_kegPrimingFactor  {0.0                          },
   m_notes             {""                           },
   m_tasteNotes        {""                           },
   m_tasteRating       {0.0                          },
   m_styleId           {-1                           },
   m_equipmentId       {-1                           },
   m_mashId            {-1                           },
   m_boilId            {-1                           },
   m_fermentationId    {-1                           },
   m_og                {1.0                          },
   m_fg                {1.0                          },
   m_locked            {false                        },
   m_ancestor_id       {-1                           },
   m_ancestors         {},
   m_hasDescendants    {false                        } {
   return;
}

Recipe::Recipe(NamedParameterBundle const & namedParameterBundle) :
   NamedEntity         {namedParameterBundle         },
   pimpl               {std::make_unique<impl>(*this)},
   SET_REGULAR_FROM_NPB (m_type              , namedParameterBundle, PropertyNames::Recipe::type              ),
   SET_REGULAR_FROM_NPB (m_brewer            , namedParameterBundle, PropertyNames::Recipe::brewer            ),
   SET_REGULAR_FROM_NPB (m_asstBrewer        , namedParameterBundle, PropertyNames::Recipe::asstBrewer        ),
   SET_REGULAR_FROM_NPB (m_batchSize_l       , namedParameterBundle, PropertyNames::Recipe::batchSize_l       ),
   SET_REGULAR_FROM_NPB (m_efficiency_pct    , namedParameterBundle, PropertyNames::Recipe::efficiency_pct    ),
   SET_REGULAR_FROM_NPB (m_fermentationStages, namedParameterBundle, PropertyNames::Recipe::fermentationStages),
   SET_REGULAR_FROM_NPB (m_primaryAge_days   , namedParameterBundle, PropertyNames::Recipe::primaryAge_days   ),
   SET_REGULAR_FROM_NPB (m_primaryTemp_c     , namedParameterBundle, PropertyNames::Recipe::primaryTemp_c     ),
   SET_REGULAR_FROM_NPB (m_secondaryAge_days , namedParameterBundle, PropertyNames::Recipe::secondaryAge_days ),
   SET_REGULAR_FROM_NPB (m_secondaryTemp_c   , namedParameterBundle, PropertyNames::Recipe::secondaryTemp_c   ),
   SET_REGULAR_FROM_NPB (m_tertiaryAge_days  , namedParameterBundle, PropertyNames::Recipe::tertiaryAge_days  ),
   SET_REGULAR_FROM_NPB (m_tertiaryTemp_c    , namedParameterBundle, PropertyNames::Recipe::tertiaryTemp_c    ),
   SET_REGULAR_FROM_NPB (m_age               , namedParameterBundle, PropertyNames::Recipe::age_days          ),
   SET_REGULAR_FROM_NPB (m_ageTemp_c         , namedParameterBundle, PropertyNames::Recipe::ageTemp_c         ),
   SET_REGULAR_FROM_NPB (m_date              , namedParameterBundle, PropertyNames::Recipe::date              ),
   SET_REGULAR_FROM_NPB (m_carbonation_vols  , namedParameterBundle, PropertyNames::Recipe::carbonation_vols  ),
   SET_REGULAR_FROM_NPB (m_forcedCarbonation , namedParameterBundle, PropertyNames::Recipe::forcedCarbonation ),
   SET_REGULAR_FROM_NPB (m_primingSugarName  , namedParameterBundle, PropertyNames::Recipe::primingSugarName  ),
   SET_REGULAR_FROM_NPB (m_carbonationTemp_c , namedParameterBundle, PropertyNames::Recipe::carbonationTemp_c ),
   SET_REGULAR_FROM_NPB (m_primingSugarEquiv , namedParameterBundle, PropertyNames::Recipe::primingSugarEquiv ),
   SET_REGULAR_FROM_NPB (m_kegPrimingFactor  , namedParameterBundle, PropertyNames::Recipe::kegPrimingFactor  ),
   SET_REGULAR_FROM_NPB (m_notes             , namedParameterBundle, PropertyNames::Recipe::notes             ),
   SET_REGULAR_FROM_NPB (m_tasteNotes        , namedParameterBundle, PropertyNames::Recipe::tasteNotes        ),
   SET_REGULAR_FROM_NPB (m_tasteRating       , namedParameterBundle, PropertyNames::Recipe::tasteRating       ),
   SET_REGULAR_FROM_NPB (m_styleId           , namedParameterBundle, PropertyNames::Recipe::styleId           ),
   SET_REGULAR_FROM_NPB (m_equipmentId       , namedParameterBundle, PropertyNames::Recipe::equipmentId       ),
   SET_REGULAR_FROM_NPB (m_mashId            , namedParameterBundle, PropertyNames::Recipe::mashId            ),
   SET_REGULAR_FROM_NPB (m_boilId            , namedParameterBundle, PropertyNames::Recipe::boilId            , -1),
   SET_REGULAR_FROM_NPB (m_fermentationId    , namedParameterBundle, PropertyNames::Recipe::fermentationId    , -1),
   SET_REGULAR_FROM_NPB (m_og                , namedParameterBundle, PropertyNames::Recipe::og                ),
   SET_REGULAR_FROM_NPB (m_fg                , namedParameterBundle, PropertyNames::Recipe::fg                ),
   SET_REGULAR_FROM_NPB (m_locked            , namedParameterBundle, PropertyNames::Recipe::locked            ),
   SET_REGULAR_FROM_NPB (m_ancestor_id       , namedParameterBundle, PropertyNames::Recipe::ancestorId        ),
   m_ancestors         {},
   m_hasDescendants    {false} {
   // At this stage, we haven't set any Hops, Fermentables, etc.  This is deliberate because the caller typically needs
   // to access subsidiary records to obtain this info.   Callers will usually use setters (setHopIds, etc but via
   // setProperty) to finish constructing the object.

   // We still need to support boilSize_l and boilTime_min for BeerXML
   if (namedParameterBundle.contains(PropertyNames::Recipe::boilSize_l)) {
      this->nonOptBoil()->setPreBoilSize_l(namedParameterBundle.val<double>(PropertyNames::Recipe::boilSize_l));
   }
   if (namedParameterBundle.contains(PropertyNames::Recipe::boilTime_min)) {
      this->nonOptBoil()->setBoilTime_mins(namedParameterBundle.val<double>(PropertyNames::Recipe::boilTime_min));
   }

   return;
}


Recipe::Recipe(Recipe const & other) :
   NamedEntity{other},
   pimpl{std::make_unique<impl>(*this)},
   m_type              {other.m_type              },
   m_brewer            {other.m_brewer            },
   m_asstBrewer        {other.m_asstBrewer        },
   m_batchSize_l       {other.m_batchSize_l       },
   m_efficiency_pct    {other.m_efficiency_pct    },
   m_fermentationStages{other.m_fermentationStages},
   m_primaryAge_days   {other.m_primaryAge_days   },
   m_primaryTemp_c     {other.m_primaryTemp_c     },
   m_secondaryAge_days {other.m_secondaryAge_days },
   m_secondaryTemp_c   {other.m_secondaryTemp_c   },
   m_tertiaryAge_days  {other.m_tertiaryAge_days  },
   m_tertiaryTemp_c    {other.m_tertiaryTemp_c    },
   m_age               {other.m_age               },
   m_ageTemp_c         {other.m_ageTemp_c         },
   m_date              {other.m_date              },
   m_carbonation_vols  {other.m_carbonation_vols  },
   m_forcedCarbonation {other.m_forcedCarbonation },
   m_primingSugarName  {other.m_primingSugarName  },
   m_carbonationTemp_c {other.m_carbonationTemp_c },
   m_primingSugarEquiv {other.m_primingSugarEquiv },
   m_kegPrimingFactor  {other.m_kegPrimingFactor  },
   m_notes             {other.m_notes             },
   m_tasteNotes        {other.m_tasteNotes        },
   m_tasteRating       {other.m_tasteRating       },
   m_styleId           {other.m_styleId           },  // But see additional logic in body
   m_equipmentId       {other.m_equipmentId       },  // But see additional logic in body
   m_mashId            {other.m_mashId            },  // But see additional logic in body
   m_boilId            {other.m_boilId            },  // But see additional logic in body
   m_fermentationId    {other.m_fermentationId     },  // But see additional logic in body
   m_og                {other.m_og                },
   m_fg                {other.m_fg                },
   m_locked            {other.m_locked            },
   // Copying a Recipe doesn't copy its descendants
   m_ancestor_id       {-1                        },
   m_ancestors         {},
   m_hasDescendants    {false                     } {
   setObjectName("Recipe"); // .:TBD:. Would be good to understand why we need this

   //
   // We don't want to be versioning something while we're still constructing it
   //
   NamedEntityModifyingMarker modifyingMarker(*this);

   //
   // When we make a copy of a Recipe, it needs to be a deep(ish) copy.  In particular, we need to make copies of the
   // Hops, Fermentables etc as some attributes of the recipe (eg how much and when to add) are stored inside these
   // ingredients.
   //
   // We _don't_ want to copy BrewNotes (an instance of brewing the Recipe).  (This is easy not to do as we don't
   // currently store BrewNote IDs in Recipe.)
   //
   this->pimpl->copyList<Fermentable>(*this, other);
   this->pimpl->copyAdditions<RecipeAdditionHop>(*this, other);
   this->pimpl->copyList<Instruction>(*this, other);
   this->pimpl->copyList<Misc       >(*this, other);
   this->pimpl->copyList<Salt       >(*this, other);
   this->pimpl->copyList<Water      >(*this, other);
   this->pimpl->copyList<Yeast      >(*this, other);

   //
   // You might think that Style, Mash and Equipment could safely be shared between Recipes.   However, AFAICT, none of
   // them is.  Presumably this is because users expect to be able to edit them in one Recipe without changing the
   // settings for any other Recipe.
   //
   // We also need to be careful here as one or more of these may not be set to a valid value.
   //
   if (other.m_styleId        > 0) { auto item = copyIfNeeded(*ObjectStoreWrapper::getById<Style       >(other.m_styleId       )); this->m_styleId        = item->key(); }
   if (other.m_equipmentId    > 0) { auto item = copyIfNeeded(*ObjectStoreWrapper::getById<Equipment   >(other.m_equipmentId   )); this->m_equipmentId    = item->key(); }
   if (other.m_mashId         > 0) { auto item = copyIfNeeded(*ObjectStoreWrapper::getById<Mash        >(other.m_mashId        )); this->m_mashId         = item->key(); }
   if (other.m_boilId         > 0) { auto item = copyIfNeeded(*ObjectStoreWrapper::getById<Boil        >(other.m_boilId        )); this->m_boilId         = item->key(); }
   if (other.m_fermentationId > 0) { auto item = copyIfNeeded(*ObjectStoreWrapper::getById<Fermentation>(other.m_fermentationId)); this->m_fermentationId = item->key(); }

   this->pimpl->connectSignals();

   this->recalcAll();

   return;
}

// See https://herbsutter.com/gotw/_100/ for why we need to explicitly define the destructor here (and not in the
// header file)
Recipe::~Recipe() = default;

void Recipe::setKey(int key) {
   //
   // This function is called because we've just inserted a new Recipe in the DB and we now know its primary key.  By
   // convention, a new Recipe with no ancestor should have itself as its own ancestor.  So we need to check whether to
   // set that default here (which will then result in a DB update).  Otherwise, ancestor ID would remain as null.
   //
   // .:TBD:. Would it really be so bad for Ancestor ID to be NULL in the DB when there is no direct ancestor?
   //
   this->NamedEntity::setKey(key);
   if (this->m_ancestor_id <= 0) {
      qDebug() << Q_FUNC_INFO << "Setting default ancestor ID on Recipe #" << key;

      // We want to store the new ancestor ID in the DB, but we don't want to signal the UI about this change, so
      // suppress signal sending.
      this->setAncestorId(key, false);
   }
   return;
}


void Recipe::connectSignalsForAllRecipes() {
   qDebug() << Q_FUNC_INFO << "Connecting signals for all Recipes";
   // Connect fermentable, hop changed signals to their parent recipe
   for (auto recipe : ObjectStoreTyped<Recipe>::getInstance().getAllRaw()) {
//      qDebug() << Q_FUNC_INFO << "Connecting signals for Recipe #" << recipe->key();
      recipe->pimpl->connectSignals();
   }

   return;
}


void Recipe::mashFermentableIns() {
   /*** Add grains ***/
   auto ins = std::make_shared<Instruction>();
   ins->setName(tr("Add grains"));
   QString str = tr("Add ");
   QList<QString> reagents = this->getReagents(this->fermentables());

   for (int ii = 0; ii < reagents.size(); ++ii) {
      str += reagents.at(ii);
   }

   str += tr("to the mash tun.");
   ins->setDirections(str);

   this->add(ins);

   return;
}

void Recipe::saltWater(Salt::WhenToAdd when) {

   if (this->mash() == nullptr || this->salts().size() == 0) {
      return;
   }

   QStringList reagents = this->getReagents(salts(), when);
   if (reagents.size() == 0) {
      return;
   }

   auto ins = std::make_shared<Instruction>();
   QString tmp = when == Salt::WhenToAdd::MASH ? tr("mash") : tr("sparge");
   ins->setName(tr("Modify %1 water").arg(tmp));
   QString str = tr("Dissolve ");

   for (int ii = 0; ii < reagents.size(); ++ii) {
      str += reagents.at(ii);
   }

   str += QString(tr(" into the %1 water").arg(tmp));
   ins->setDirections(str);

   this->add(ins);

   return;
}

void Recipe::mashWaterIns() {

   if (this->mash() == nullptr) {
      return;
   }

   auto ins = std::make_shared<Instruction>();
   ins->setName(tr("Heat water"));
   QString str = tr("Bring ");
   QList<QString> reagents = getReagents(mash()->mashSteps());

   for (int ii = 0; ii < reagents.size(); ++ii) {
      str += reagents.at(ii);
   }

   str += tr("for upcoming infusions.");
   ins->setDirections(str);

   this->add(ins);

   return;
}

void Recipe::firstWortHopsIns() {
   QList<QString> reagents = getReagents(this->hopAdditions(), true);
   if (reagents.size() == 0) {
      return;
   }

   QString str = tr("Do first wort hopping with ");

   for (int ii = 0; ii < reagents.size(); ++ii) {
      str += reagents.at(ii);
   }
   str += ".";

   auto ins = std::make_shared<Instruction>();
   ins->setName(tr("First wort hopping"));
   ins->setDirections(str);

   this->add(ins);

   return;
}

void Recipe::topOffIns() {
   Equipment * e = this->equipment();
   if (e == nullptr) {
      return;
   }

   double wortInBoil_l = wortFromMash_l() - e->getLauteringDeadspaceLoss_l();
   QString str = tr("You should now have %1 wort.")
                 .arg(Measurement::displayAmount(Measurement::Amount{wortInBoil_l, Measurement::Units::liters}));
   if (!e->topUpKettle_l() || *e->topUpKettle_l() == 0.0) {
      return;
   }

   wortInBoil_l += *e->topUpKettle_l();
   QString tmp = tr(" Add %1 water to the kettle, bringing pre-boil volume to %2.")
                 .arg(Measurement::displayAmount(Measurement::Amount{*e->topUpKettle_l(), Measurement::Units::liters}))
                 .arg(Measurement::displayAmount(Measurement::Amount{wortInBoil_l, Measurement::Units::liters}));

   str += tmp;

   auto ins = std::make_shared<Instruction>();
   ins->setName(tr("Pre-boil"));
   ins->setDirections(str);
   ins->addReagent(tmp);

   this->add(ins);

   return;
}

void Recipe::postboilFermentablesIns() {
   QString tmp;
   bool hasFerms = false;

   QString str = tr("Add ");
   QList<Fermentable *> flist = this->fermentables();
   int size = flist.size();
   for (int ii = 0; ii < size; ++ii) {
      Fermentable * ferm = flist[ii];
      if (!ferm->addAfterBoil()) {
         continue;
      }

      hasFerms = true;
      tmp = QString("%1 %2, ")
            .arg(Measurement::displayAmount(ferm->amountWithUnits()))
            .arg(ferm->name());
      str += tmp;
   }
   str += tr("to the boil at knockout.");

   if (!hasFerms) {
      return;
   }

   auto ins = std::make_shared<Instruction>();
   ins->setName(tr("Knockout additions"));
   ins->setDirections(str);
   ins->addReagent(tmp);

   this->add(ins);

   return;
}

void Recipe::postboilIns() {
   Equipment * e = equipment();
   if (e == nullptr) {
      return;
   }

   double wortInBoil_l = wortFromMash_l() - e->getLauteringDeadspaceLoss_l();
   wortInBoil_l += e->topUpKettle_l().value_or(0.0);

   double wort_l = e->wortEndOfBoil_l(wortInBoil_l);
   QString str = tr("You should have %1 wort post-boil.")
                 .arg(Measurement::displayAmount(Measurement::Amount{wort_l, Measurement::Units::liters}));
   str += tr("\nYou anticipate losing %1 to trub and chiller loss.")
          .arg(Measurement::displayAmount(Measurement::Amount{e->kettleTrubChillerLoss_l(), Measurement::Units::liters}));
   wort_l -= e->kettleTrubChillerLoss_l();
   if (e->topUpWater_l() > 0.0)
      str += tr("\nAdd %1 top up water into primary.")
             .arg(Measurement::displayAmount(Measurement::Amount{e->topUpWater_l().value_or(Equipment::default_topUpWater_l), Measurement::Units::liters}));
   wort_l += e->topUpWater_l().value_or(Equipment::default_topUpWater_l);
   str += tr("\nThe final volume in the primary is %1.")
          .arg(Measurement::displayAmount(Measurement::Amount{wort_l, Measurement::Units::liters}));

   auto ins = std::make_shared<Instruction>();
   ins->setName(tr("Post boil"));
   ins->setDirections(str);
   this->add(ins);

   return;
}


void Recipe::generateInstructions() {
   double timeRemaining;
   double totalWaterAdded_l = 0.0;

   if (!this->instructions().empty()) {
      this->clearInstructions();
   }

   QVector<PreInstruction> preinstructions;

   // Mash instructions

   int size = (mash() == nullptr) ? 0 : mash()->mashSteps().size();
   if (size > 0) {
      /*** prepare mashed fermentables ***/
      this->mashFermentableIns();

      /*** salt the water ***/
      saltWater(Salt::WhenToAdd::MASH);
      saltWater(Salt::WhenToAdd::SPARGE);

      /*** Prepare water additions ***/
      this->mashWaterIns();

      timeRemaining = mash()->totalTime();

      /*** Generate the mash instructions ***/
      preinstructions = this->pimpl->mashInstructions(timeRemaining, totalWaterAdded_l, size);

      /*** Hops mash additions ***/
      preinstructions += this->pimpl->hopSteps(RecipeAddition::Stage::Mash);

      /*** Misc mash additions ***/
      preinstructions += this->pimpl->miscSteps(Misc::Use::Mash);

      /*** Add the preinstructions into the instructions ***/
      this->pimpl->addPreinstructions(preinstructions);

   } // END mash instructions.

   // First wort hopping
   this->firstWortHopsIns();

   // Need to top up the kettle before boil?
   topOffIns();

   // Boil instructions
   preinstructions.clear();

   // Find boil time.
   if (equipment() != nullptr) {
      timeRemaining = equipment()->boilTime_min().value_or(Equipment::default_boilTime_min);
   } else {
      timeRemaining =
         Measurement::qStringToSI(QInputDialog::getText(nullptr,
                                                        tr("Boil time"),
                                                        tr("You did not configure an equipment (which you really should), so tell me the boil time.")),
                                  Measurement::PhysicalQuantity::Time).quantity();
   }

   QString str = tr("Bring the wort to a boil and hold for %1.").arg(
      Measurement::displayAmount(Measurement::Amount{timeRemaining, Measurement::Units::minutes})
   );

   auto startBoilIns = std::make_shared<Instruction>();
   startBoilIns->setName(tr("Start boil"));
   startBoilIns->setInterval(timeRemaining);
   startBoilIns->setDirections(str);
   this->add(startBoilIns);

   /*** Get fermentables unless we haven't added yet ***/
   if (this->pimpl->hasBoilFermentable()) {
      preinstructions.push_back(this->pimpl->boilFermentablesPre(timeRemaining));
   }

   // add the intructions for including Extracts to wort
   if (this->pimpl->hasBoilExtract()) {
      preinstructions.push_back(this->pimpl->addExtracts(timeRemaining - 1));
   }

   /*** Boiled hops ***/
   preinstructions += this->pimpl->hopSteps(RecipeAddition::Stage::Boil);

   /*** Boiled miscs ***/
   preinstructions += this->pimpl->miscSteps(Misc::Use::Boil);

   // END boil instructions.

   // Add instructions in descending mash time order.
   this->pimpl->addPreinstructions(preinstructions);

   // FLAMEOUT
   auto flameoutIns = std::make_shared<Instruction>();
   flameoutIns->setName(tr("Flameout"));
   flameoutIns->setDirections(tr("Stop boiling the wort."));
   this->add(flameoutIns);

   // TODO: These get included in RecipeAddition::Stage::Boil above.  But we're going to want to rework this anyway to
   //       order by stage, step, time.
   // Steeped aroma hops
   // preinstructions = this->pimpl->hopSteps(Hop::Use::Aroma);
   this->pimpl->addPreinstructions(preinstructions);

   // Fermentation instructions
   preinstructions.clear();

   /*** Fermentables added after boil ***/
   postboilFermentablesIns();

   /*** post boil ***/
   postboilIns();

   /*** Primary yeast ***/
   str = tr("Cool wort and pitch ");
   QList<Yeast *> ylist = yeasts();
   for (int ii = 0; ii < ylist.size(); ++ii) {
      Yeast * yeast = ylist[ii];
      if (! yeast->addToSecondary()) {
         str += tr("%1 %2 yeast, ").arg(yeast->name()).arg(Yeast::typeDisplayNames[yeast->type()]);
      }
   }
   str += tr("to the primary.");

   auto pitchIns = std::make_shared<Instruction>();
   pitchIns->setName(tr("Pitch yeast"));
   pitchIns->setDirections(str);
   this->add(pitchIns);
   /*** End primary yeast ***/

   /*** Primary misc ***/
   this->pimpl->addPreinstructions(this->pimpl->miscSteps(Misc::Use::Primary));

   str = tr("Let ferment until FG is %1.").arg(
      Measurement::displayAmount(Measurement::Amount{fg(), Measurement::Units::specificGravity}, 3)
   );

   auto fermentIns = std::make_shared<Instruction>();
   fermentIns->setName(tr("Ferment"));
   fermentIns->setDirections(str);
   this->add(fermentIns);

   str = tr("Transfer beer to secondary.");
   auto transferIns = std::make_shared<Instruction>();
   transferIns->setName(tr("Transfer to secondary"));
   transferIns->setDirections(str);
   this->add(transferIns);

   /*** Secondary misc ***/
   this->pimpl->addPreinstructions(this->pimpl->miscSteps(Misc::Use::Secondary));

   /*** Dry hopping ***/
   this->pimpl->addPreinstructions(this->pimpl->hopSteps(RecipeAddition::Stage::Fermentation));

   // END fermentation instructions. Let everybody know that now is the time
   // to update instructions
   emit changed(metaProperty(*PropertyNames::Recipe::instructions), this->instructions().size());

   return;
}

QString Recipe::nextAddToBoil(double & time) {
   double max = 0;
   bool foundSomething = false;
   QString ret;

   // Search hop additions
   for (auto hopAddition : this->hopAdditions()) {
      if (hopAddition->stage() != RecipeAddition::Stage::Boil) {
         continue;
      }
      if (!hopAddition->addAtTime_mins()) {
         continue;
      }
      double const addAtTime_mins = *hopAddition->addAtTime_mins();
      if (addAtTime_mins < time && addAtTime_mins > max) {
         ret = tr("Add %1 %2 to boil at %3.")
               .arg(Measurement::displayAmount(hopAddition->amountWithUnits()))
               .arg(hopAddition->hop()->name())
               .arg(Measurement::displayAmount(Measurement::Amount{addAtTime_mins, Measurement::Units::minutes}));

         max = addAtTime_mins;
         foundSomething = true;
      }
   }

   // Search miscs
   QList<Misc *> mmiscs = miscs();
   auto size = mmiscs.size();
   for (int i = 0; i < size; ++i) {
      Misc * m = mmiscs[i];
      if (m->use() != Misc::Use::Boil) {
         continue;
      }
      if (m->time_min() < time && m->time_min() > max) {
         ret = tr("Add %1 %2 to boil at %3.");
         ret = ret.arg(Measurement::displayAmount(m->amountWithUnits()));

         ret = ret.arg(m->name());
         ret = ret.arg(Measurement::displayAmount(Measurement::Amount{m->time_min(), Measurement::Units::minutes}));
         max = m->time_min();
         foundSomething = true;
      }
   }

   time = foundSomething ? max : -1.0;
   return ret;
}

//============================Relational Setters===============================
template<class NE> std::shared_ptr<NE> Recipe::add(std::shared_ptr<NE> ne) {
   // It's a coding error if we've ended up with a null shared_ptr
   Q_ASSERT(ne);

   // If the object being added is not already in the ObjectStore, we need to add it.  This is typically when we're
   // adding a new Instruction (which is an object owned by Recipe) or undoing a remove of a Hop/Fermentable/etc (thus
   // the "instance of use of" object was previously in the ObjectStore, but was removed and now we want to add it
   // back).
   if (ne->key() <= 0) {
      // With shared pointer parameter, ObjectStoreWrapper::insert returns what we passed it (ie our shared pointer
      // remains valid after the call).
      qDebug() << Q_FUNC_INFO << "Inserting" << ne->metaObject()->className() << "in object store";
      ObjectStoreWrapper::insert(ne);
   } else {
      //
      // The object was already in the ObjectStore, so let's check whether we need to copy it.
      //
      // Note that std::shared_ptr does all the right things if this assignment ends up boiling down to ne = ne!
      //
      ne = copyIfNeeded(*ne);
   }

   this->pimpl->accessIds<NE>().append(ne->key());
   connect(ne.get(), &NamedEntity::changed, this, &Recipe::acceptChangeToContainedObject);
   this->propagatePropertyChange(propertyToPropertyName<NE>());

   this->recalcIfNeeded(ne->metaObject()->className());
   return ne;
}

//
// Instantiate the above template function for the types that are going to use it
// (This is all just a trick to allow the template definition to be here in the .cpp file and not in the header, which
// means, amongst other things, that we can reference the pimpl.)
//
template std::shared_ptr<Fermentable> Recipe::add(std::shared_ptr<Fermentable> var);
template std::shared_ptr<Misc       > Recipe::add(std::shared_ptr<Misc       > var);
template std::shared_ptr<Yeast      > Recipe::add(std::shared_ptr<Yeast      > var);
template std::shared_ptr<Water      > Recipe::add(std::shared_ptr<Water      > var);
template std::shared_ptr<Salt       > Recipe::add(std::shared_ptr<Salt       > var);
template std::shared_ptr<Instruction> Recipe::add(std::shared_ptr<Instruction> var);
template<> std::shared_ptr<Hop        > Recipe::add(std::shared_ptr<Hop        > var) { Q_ASSERT(false); return nullptr; };

template<class RA> std::shared_ptr<RA> Recipe::addAddition(std::shared_ptr<RA> addition) {
   // It's a coding error if we've ended up with a null shared_ptr
   Q_ASSERT(addition);

   // Tell the addition that it belongs to this recipe
   addition->setRecipeId(this->key());

   // Recipe additions are owned by the Recipe, so, if the object being added is not already in the ObjectStore, we need
   // to add it.
   if (addition->key() <= 0) {
      // With shared pointer parameter, ObjectStoreWrapper::insert returns what we passed it (ie our shared pointer
      // remains valid after the call).
      qDebug() <<
         Q_FUNC_INFO << "Inserting" << addition->metaObject()->className() << "for" <<
         addition->ingredient()->metaObject()->className() << "#" << addition->ingredient()->key() << "in object store";
      ObjectStoreWrapper::insert(addition);
   }

   connect(addition->ingredient().get(), &NamedEntity::changed, this, &Recipe::acceptChangeToContainedObject);
   this->propagatePropertyChange(propertyToPropertyName<RA>());

   this->recalcIfNeeded(addition->ingredient()->metaObject()->className());
   return addition;
}
template std::shared_ptr<RecipeAdditionHop> Recipe::addAddition(std::shared_ptr<RecipeAdditionHop> addition);

template<class NE> bool Recipe::uses(NE const & val) const {
   int idToLookFor = val.key();
   if (idToLookFor <= 0) {
      //
      // We shouldn't be trying to look for a Fermentable/Hop/etc that hasn't even been stored (and therefore does not
      // yet have an ID).  The most likely reason for this happening would be a coding error that results in a copy of
      // a Fermentable/Hop/etc being taken and passed in as the parameter to this function (because copies do not take
      // the ID of the thing from which they were copied).
      //
      qCritical() <<
         Q_FUNC_INFO << "Trying to search for use of" << val.metaObject()->className() << "that is not stored!";
      return false;
   }

   auto match = std::find_if(this->pimpl->accessIds<NE>().cbegin(),
                             this->pimpl->accessIds<NE>().cend(),
   [idToLookFor](int id) {
      return idToLookFor == id;
   });

   return match != this->pimpl->accessIds<NE>().cend();
}
template bool Recipe::uses(Fermentable  const & val) const;
template bool Recipe::uses(Instruction  const & val) const;
template bool Recipe::uses(Misc         const & val) const;
template bool Recipe::uses(Salt         const & val) const;
template bool Recipe::uses(Water        const & val) const;
template bool Recipe::uses(Yeast        const & val) const;
template<> bool Recipe::uses<Equipment   > (Equipment    const & val) const { return val.key() == this->m_equipmentId   ; }
template<> bool Recipe::uses<Style       > (Style        const & val) const { return val.key() == this->m_styleId       ; }
template<> bool Recipe::uses<Mash        > (Mash         const & val) const { return val.key() == this->m_mashId        ; }
// ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
template<> bool Recipe::uses<Boil        > (Boil         const & val) const { return val.key() == this->m_boilId        ; }
template<> bool Recipe::uses<Fermentation> (Fermentation const & val) const { return val.key() == this->m_fermentationId; }
template<> bool Recipe::uses<RecipeAdditionHop>(RecipeAdditionHop          const & val) const { return val.recipeId() == this->key(); }

template<class NE> std::shared_ptr<NE> Recipe::remove(std::shared_ptr<NE> var) {
   // It's a coding error to supply a null shared pointer
   Q_ASSERT(var);

   int idToRemove = var->key();
   if (!this->pimpl->accessIds<NE>().removeOne(idToRemove)) {
      // It's a coding error if we try to remove something from the Recipe that wasn't in it in the first place!
      qCritical() <<
         Q_FUNC_INFO << "Tried to remove" << var->metaObject()->className() << "with ID" << idToRemove <<
         "but couldn't find it in Recipe #" << this->key();
      Q_ASSERT(false);
   } else {
      this->propagatePropertyChange(propertyToPropertyName<NE>());
      this->recalcIBU(); // .:TODO:. Don't need to do this recalculation when it's Instruction
   }

   //
   // Because Hop/Fermentable/etc objects in a Recipe are actually "Instance of use of Hop/Fermentable/etc" we usually
   // want to delete the object from the ObjectStore at this point.  But, because we're a bit paranoid, we'll check
   // first that the object we're removing has a parent (ie really is an "instance of use of") and is not used in any
   // other Recipes.
   //
   if (isUnusedInstanceOfUseOf(*var)) {
      qDebug() <<
         Q_FUNC_INFO << "Deleting" << var->metaObject()->className() << "#" << var->key() <<
         "as it is \"instance of use of\" that is no longer needed";
      ObjectStoreWrapper::hardDelete<NE>(var->key());
   }
   // The caller now owns the removed object unless and until they pass it in to Recipe::add() (typically to undo the
   // remove).
   return var;
}
template std::shared_ptr<Fermentable> Recipe::remove(std::shared_ptr<Fermentable> var);
template std::shared_ptr<Misc       > Recipe::remove(std::shared_ptr<Misc       > var);
template std::shared_ptr<Yeast      > Recipe::remove(std::shared_ptr<Yeast      > var);
template std::shared_ptr<Water      > Recipe::remove(std::shared_ptr<Water      > var);
template std::shared_ptr<Salt       > Recipe::remove(std::shared_ptr<Salt       > var);
template std::shared_ptr<Instruction> Recipe::remove(std::shared_ptr<Instruction> var);

template<class RA> std::shared_ptr<RA> Recipe::removeAddition(std::shared_ptr<RA> addition) {
   // It's a coding error to supply a null shared pointer
   Q_ASSERT(addition);

   // It's a coding error if we try to remove something from the Recipe that wasn't in it in the first place!
   Q_ASSERT(addition->recipeId() == this->key());

   addition->setRecipeId(-1);

   disconnect(addition->ingredient().get(), &NamedEntity::changed, this, &Recipe::acceptChangeToContainedObject);
   this->propagatePropertyChange(propertyToPropertyName<RA>());

   this->recalcIfNeeded(addition->ingredient()->metaObject()->className());

   //
   // Because RecipeAdditionHop etc objects are owned by their Recipe, we need to delete the object from the ObjectStore
   // at this point.
   //
   qDebug() << Q_FUNC_INFO << "Deleting" << addition->metaObject()->className() << "#" << addition->key();
   ObjectStoreWrapper::hardDelete<RA>(addition->key());

   // The caller now owns the removed object unless and until they pass it in to Recipe::add() (typically to undo the
   // remove).
   return addition;
}
template std::shared_ptr<RecipeAdditionHop> Recipe::removeAddition(std::shared_ptr<RecipeAdditionHop> addition);

int Recipe::instructionNumber(Instruction const & ins) const {
   // C++ arrays etc are indexed from 0, but for end users we want instruction numbers to start from 1
   return this->pimpl->instructionIds.indexOf(ins.key()) + 1;
}

void Recipe::swapInstructions(Instruction * ins1, Instruction * ins2) {

   int indexOf1 = this->pimpl->instructionIds.indexOf(ins1->key());
   int indexOf2 = this->pimpl->instructionIds.indexOf(ins2->key());

   // We can't swap them if we can't find both of them
   // There's no point swapping them if they're the same
   if (-1 == indexOf1 || -1 == indexOf2 || indexOf1 == indexOf2) {
      return;
   }

   // As of Qt 5.14 we could write:
   //    this->pimpl->instructionIds.swapItemsAt(indexOf1, indexOf2);
   // However, we still need to support slightly older versions of Qt (5.12 in particular), hence the more cumbersome
   // way here.
   std::swap(this->pimpl->instructionIds[indexOf1], this->pimpl->instructionIds[indexOf2]);

   ObjectStoreWrapper::updateProperty(*this, PropertyNames::Recipe::instructionIds);
   return;
}

void Recipe::clearInstructions() {
   for (int ii : this->pimpl->instructionIds) {
      ObjectStoreTyped<Instruction>::getInstance().softDelete(ii);
   }
   this->pimpl->instructionIds.clear();
   this->propagatePropertyChange(propertyToPropertyName<Instruction>());
   return;
}

void Recipe::insertInstruction(Instruction const & ins, int pos) {
   if (this->pimpl->instructionIds.contains(ins.key())) {
      qDebug() <<
         Q_FUNC_INFO << "Request to insert instruction ID" << ins.key() << "at position" << pos << "for recipe #" <<
         this->key() << "ignored as this instruction is already in the list at position" <<
         this->instructionNumber(ins);
      return;
   }

   // The position should be indexed from 1, so it's a coding error if it's less than this
   Q_ASSERT(pos >= 1);

   qDebug() <<
      Q_FUNC_INFO << "Inserting instruction #" << ins.key() << "(" << ins.name() << ") at position" << pos <<
      "in list of" << this->pimpl->instructionIds.size();
   this->pimpl->instructionIds.insert(pos - 1, ins.key());
   this->propagatePropertyChange(propertyToPropertyName<Instruction>());
   return;
}

void Recipe::setStyle(Style * var) {
   if (var->key() == this->m_styleId) {
      return;
   }

   std::shared_ptr<Style> styleToAdd = copyIfNeeded(*var);
   this->m_styleId = styleToAdd->key();
   this->propagatePropertyChange(propertyToPropertyName<Style>());
   return;
}

void Recipe::setEquipment(Equipment * var) {
   if (var->key() == this->m_equipmentId) {
      return;
   }

   std::shared_ptr<Equipment> equipmentToAdd = copyIfNeeded(*var);
   this->m_equipmentId = equipmentToAdd->key();
   this->propagatePropertyChange(propertyToPropertyName<Equipment>());
   return;
}

// .:TBD:. We need to think about when/how we're going to detect changes to the Boil object referred to by this->m_boilId...

void Recipe::setMash        (std::shared_ptr<Mash        > val) { this->pimpl->setStepOwner<Mash        >(val, this->m_mashId        , PropertyNames::Recipe::mash        ); return; }
void Recipe::setMash        (Mash *                        val) { this->pimpl->setStepOwner<Mash        >(val, this->m_mashId        , PropertyNames::Recipe::mash        ); return; }
void Recipe::setBoil        (std::optional<std::shared_ptr<Boil>> val) { this->pimpl->setStepOwner<Boil        >(val, this->m_boilId        , PropertyNames::Recipe::boil        ); return; }
///void Recipe::setBoil        (Boil *                        val) { this->pimpl->setStepOwner<Boil        >(val, this->m_boilId        , PropertyNames::Recipe::boil        ); return; }
void Recipe::setFermentation(std::shared_ptr<Fermentation> val) { this->pimpl->setStepOwner<Fermentation>(val, this->m_fermentationId, PropertyNames::Recipe::fermentation); return; }
void Recipe::setFermentation(Fermentation *                val) { this->pimpl->setStepOwner<Fermentation>(val, this->m_fermentationId, PropertyNames::Recipe::fermentation); return; }

///void Recipe::setMash(Mash * var) {
///   if (var->key() == this->m_mashId) {
///      return;
///   }
///
///   // .:TBD:. Do we need to disconnect the old Mash?
///
///   std::shared_ptr<Mash> mashToAdd = copyIfNeeded(*var);
///   this->m_mashId = mashToAdd->key();
///   this->propagatePropertyChange(propertyToPropertyName<Mash>());
///
///   connect(mashToAdd.get(), &NamedEntity::changed, this, &Recipe::acceptChangeToContainedObject);
///   emit this->changed(this->metaProperty(*PropertyNames::Recipe::mash), QVariant::fromValue<Mash *>(mashToAdd.get()));
///
///   this->recalcAll();
///
///   return;
///}

// Note that, because these setBlahId member functions are supposed only to be used by by ObjectStore, and are not
// intended for more general use, they do not call setAndNofify
void Recipe::setStyleId       (int const id) { this->m_styleId        = id; return; }
void Recipe::setEquipmentId   (int const id) { this->m_equipmentId    = id; return; }
void Recipe::setMashId        (int const id) { this->m_mashId         = id; return; }
void Recipe::setBoilId        (int const id) { this->m_boilId         = id; return; }
void Recipe::setFermentationId(int const id) { this->m_fermentationId = id; return; }

void Recipe::setFermentableIds(QVector<int> ids) {    this->pimpl->fermentableIds = ids; return; }
void Recipe::setInstructionIds(QVector<int> ids) {    this->pimpl->instructionIds = ids; return; }
void Recipe::setMiscIds       (QVector<int> ids) {    this->pimpl->miscIds        = ids; return; }
void Recipe::setSaltIds       (QVector<int> ids) {    this->pimpl->saltIds        = ids; return; }
void Recipe::setWaterIds      (QVector<int> ids) {    this->pimpl->waterIds       = ids; return; }
void Recipe::setYeastIds      (QVector<int> ids) {    this->pimpl->yeastIds       = ids; return; }

//==============================="SET" METHODS=================================
void Recipe::setType(Recipe::Type const val) {
   this->setAndNotify(PropertyNames::Recipe::type, this->m_type, val);
   return;
}

void Recipe::setBrewer(QString const & var) {
   this->setAndNotify(PropertyNames::Recipe::brewer, this->m_brewer, var);
   return;
}

void Recipe::setBatchSize_l(double var) {
   this->setAndNotify(PropertyNames::Recipe::batchSize_l,
                      this->m_batchSize_l,
                      this->enforceMin(var, "batch size"));

   // NOTE: this is bad, but we have to call recalcAll(), because the estimated
   // boil/batch volumes depend on the target volumes when there are no mash
   // steps to actually provide an estimate for the volumes.
   recalcAll();
}

///[[deprecated]] void Recipe::setBoilSize_l(double var) {
///   this->setAndNotify(PropertyNames::Recipe::boilSize_l,
///                      this->m_boilSize_l,
///                      this->enforceMin(var, "boil size"));
///
///   // NOTE: this is bad, but we have to call recalcAll(), because the estimated
///   // boil/batch volumes depend on the target volumes when there are no mash
///   // steps to actually provide an estimate for the volumes.
///   recalcAll();
///   return;
///}

///void Recipe::setBoilTime_min(double var) {
///   this->setAndNotify(PropertyNames::Recipe::boilTime_min,
///                      this->m_boilTime_min,
///                      this->enforceMin(var, "boil time"));
///   return;
///}

void Recipe::setEfficiency_pct(double var) {
   this->setAndNotify(PropertyNames::Recipe::efficiency_pct,
                      this->m_efficiency_pct,
                      this->enforceMinAndMax(var, "efficiency", 0.0, 100.0, 70.0));

   // If you change the efficency, you really should recalc. And I'm afraid it
   // means recalc all, since og and fg will change, which means your ratios
   // change
   recalcAll();
}

void Recipe::setAsstBrewer(const QString & var) {
   this->setAndNotify(PropertyNames::Recipe::asstBrewer,
                      this->m_asstBrewer,
                      var);
   return;
}

void Recipe::setNotes(const QString & var) {
   this->setAndNotify(PropertyNames::Recipe::notes, this->m_notes, var);
   return;
}

void Recipe::setTasteNotes(const QString & var) {
   this->setAndNotify(PropertyNames::Recipe::tasteNotes, this->m_tasteNotes, var);
   return;
}

void Recipe::setTasteRating(double var) {
   this->setAndNotify(PropertyNames::Recipe::tasteRating, this->m_tasteRating, this->enforceMinAndMax(var, "taste rating", 0.0, 50.0, 0.0));
   return;
}

void Recipe::setOg(double var) {
   this->setAndNotify(PropertyNames::Recipe::og, this->m_og, this->enforceMin(var, "og", 0.0, 1.0));
   return;
}

void Recipe::setFg(double var) {
   this->setAndNotify(PropertyNames::Recipe::fg, this->m_fg, this->enforceMin(var, "fg", 0.0, 1.0));
   return;
}

void Recipe::setFermentationStages(int var) {
   this->setAndNotify(PropertyNames::Recipe::fermentationStages, this->m_fermentationStages,
                                   this->enforceMin(var, "stages"));
   return;
}

void Recipe::setPrimaryAge_days(double var) {
   this->setAndNotify(PropertyNames::Recipe::primaryAge_days, this->m_primaryAge_days, this->enforceMin(var, "primary age"));
   return;
}

void Recipe::setPrimaryTemp_c(double var) {
   this->setAndNotify(PropertyNames::Recipe::primaryTemp_c, this->m_primaryTemp_c, var);
   return;
}

void Recipe::setSecondaryAge_days(double var) {
   this->setAndNotify(PropertyNames::Recipe::secondaryAge_days, this->m_secondaryAge_days, this->enforceMin(var, "secondary age"));
   return;
}

void Recipe::setSecondaryTemp_c(double var) {
   this->setAndNotify(PropertyNames::Recipe::secondaryTemp_c, this->m_secondaryTemp_c, var);
   return;
}

void Recipe::setTertiaryAge_days(double var) {
   this->setAndNotify(PropertyNames::Recipe::tertiaryAge_days, this->m_tertiaryAge_days, this->enforceMin(var, "tertiary age"));
   return;
}

void Recipe::setTertiaryTemp_c(double var) {
   this->setAndNotify(PropertyNames::Recipe::tertiaryTemp_c, this->m_tertiaryTemp_c, var);
   return;
}

void Recipe::setAge_days(double var) {
   this->setAndNotify(PropertyNames::Recipe::age_days, this->m_age, this->enforceMin(var, "age"));
   return;
}

void Recipe::setAgeTemp_c(double var) {
   this->setAndNotify(PropertyNames::Recipe::ageTemp_c, this->m_ageTemp_c, var);
   return;
}

void Recipe::setDate(const QDate & var) {
   this->setAndNotify(PropertyNames::Recipe::date, this->m_date, var);
   return;
}

void Recipe::setCarbonation_vols(double var) {
   this->setAndNotify(PropertyNames::Recipe::carbonation_vols, this->m_carbonation_vols, this->enforceMin(var, "carb"));
   return;
}

void Recipe::setForcedCarbonation(bool var) {
   this->setAndNotify(PropertyNames::Recipe::forcedCarbonation, this->m_forcedCarbonation, var);
   return;
}

void Recipe::setPrimingSugarName(const QString & var) {
   this->setAndNotify(PropertyNames::Recipe::primingSugarName, this->m_primingSugarName, var);
   return;
}

void Recipe::setCarbonationTemp_c(double var) {
   this->setAndNotify(PropertyNames::Recipe::carbonationTemp_c, this->m_carbonationTemp_c, var);
   return;
}

void Recipe::setPrimingSugarEquiv(double var) {
   this->setAndNotify(PropertyNames::Recipe::primingSugarEquiv, this->m_primingSugarEquiv, this->enforceMin(var, "priming sugar equiv", 0.0, 1.0));
   return;
}

void Recipe::setKegPrimingFactor(double var) {
   this->setAndNotify(PropertyNames::Recipe::kegPrimingFactor, this->m_kegPrimingFactor, this->enforceMin(var, "keg priming factor", 0.0, 1.0));
   return;
}

void Recipe::setLocked(bool isLocked) {
   // Locking a Recipe doesn't count as changing it for the purposes of versioning or the UI, so no call to setAndNotify
   // here.
   if (this->newValueMatchesExisting(PropertyNames::Recipe::locked, this->m_locked, isLocked)) {
      return;
   }
   this->m_locked = isLocked;
   this->propagatePropertyChange(PropertyNames::Recipe::locked);
   return;
}

QList<Recipe *> Recipe::ancestors() const {
   // If we know we have some ancestors, and we didn't yet load them, do so now
   if (this->m_ancestor_id > 0 && this->m_ancestor_id != this->key() && this->m_ancestors.size() == 0) {
      // NB: In previous versions of the code, we included the Recipe in the list along with its ancestors, but it's
      //     now just the ancestors in the list.
      Recipe * ancestor = const_cast<Recipe *>(this);
      while (ancestor->m_ancestor_id > 0 && ancestor->m_ancestor_id != ancestor->key()) {
         ancestor = ObjectStoreWrapper::getByIdRaw<Recipe>(ancestor->m_ancestor_id);
         ancestor->m_hasDescendants = true;
         this->m_ancestors.append(ancestor);
      }
   }

   return this->m_ancestors;
}

bool Recipe::hasAncestors() const {
   return this->ancestors().size() > 0;
}

bool Recipe::isMyAncestor(Recipe const & maybe) const {
   return this->ancestors().contains(const_cast<Recipe *>(&maybe));
}

bool Recipe::hasDescendants() const {
   return this->m_hasDescendants;
}
void Recipe::setHasDescendants(bool spawned) {
   // This is not explicitly stored in the database, so no setAndNotify call etc here
   this->m_hasDescendants = spawned;
   return;
}

void Recipe::setAncestorId(int ancestorId, bool notify) {
   // Setting Recipe's ancestor ID doesn't count as changing it for the purposes of versioning or the UI, so no call to
   // setAndNotify here.  However, we do want the DB to get updated, so we do call propagatePropertyChange.
   if (this->newValueMatchesExisting(PropertyNames::Recipe::ancestorId, this->m_ancestor_id, ancestorId)) {
      return;
   }
   this->m_ancestor_id = ancestorId;
   this->propagatePropertyChange(PropertyNames::Recipe::ancestorId, notify);
   return;
}

void Recipe::setAncestor(Recipe & ancestor) {
   //
   // Typical usage is:
   //    - Recipe A is about to be modified
   //    - We create Recipe B as a deep copy of Recipe A
   //    - Recipe B becomes Recipe A's immediate ancestor, via call to this function
   //    - Recipe A is modified
   // This means that, if Recipe A already has a direct ancestor, then Recipe B needs to take it
   //
   qDebug() <<
      Q_FUNC_INFO << "Setting Recipe #" << ancestor.key() << "to be immediate prior version (ancestor) of Recipe #" <<
      this->key();

   if (this->m_ancestor_id > 0 && this->m_ancestor_id != this->key()) {
      // We already have ancestors (aka previous versions)

      if (&ancestor == this) {
         // Setting a Recipe to be its own ancestor is a kooky way of saying we want the Recipe not to have any
         // ancestors
         if (this->ancestors().size() > 0) {
            // We have some ancestors so we just have to tell the immediate one that it no longer has descendants
            this->ancestors().at(0)->setHasDescendants(false);
            this->ancestors().clear();
         }
      } else {
         // Give our existing ancestors them to the new direct ancestor (aka immediate prior version).  Note that it's
         // a coding error if this new direct ancestor already has its own ancestors.
         Q_ASSERT(ancestor.m_ancestor_id == ancestor.key() || ancestor.m_ancestor_id <= 0);
         ancestor.m_ancestor_id = this->m_ancestor_id;
         ancestor.m_ancestors = this->ancestors();
      }
   }

   // Skip most of the remaining work if we're really setting "no ancestors"
   if (&ancestor != this) {
      // Either we verified the lazy-load of this->m_ancestors in the call to this->ancestors() in the if statement above
      // or it should have been empty to begin with.  In both cases, we should be good to append the new ancestor here.
      this->m_ancestors.append(&ancestor);

      ancestor.setDisplay(false);
      ancestor.setLocked(true);
      ancestor.setHasDescendants(true);
   }

   this->setAncestorId(ancestor.key());

   return;
}

Recipe * Recipe::revertToPreviousVersion() {
   // If there are no ancestors then there is nothing to do
   if (!this->hasAncestors()) {
      return nullptr;
   }

   // Reactivate our immediate ancestor (aka previous version)
   Recipe * ancestor = ObjectStoreWrapper::getByIdRaw<Recipe>(this->m_ancestor_id);
   ancestor->setDisplay(true);
   ancestor->setLocked(false);
   ancestor->setHasDescendants(false);

   // Then forget we ever had any ancestors
   this->setAncestorId(this->key());

   return ancestor;
}


//==========================Calculated Getters============================

double Recipe::og() {
   if (m_uninitializedCalcs) {
      recalcAll();
   }
   return m_og;
}

double Recipe::fg() {
   if (m_uninitializedCalcs) {
      recalcAll();
   }
   return m_fg;
}

double Recipe::color_srm() {
   if (m_uninitializedCalcs) {
      recalcAll();
   }
   return m_color_srm;
}

double Recipe::ABV_pct() {
   if (m_uninitializedCalcs) {
      recalcAll();
   }
   return m_ABV_pct;
}

double Recipe::IBU() {
   if (m_uninitializedCalcs) {
      recalcAll();
   }
   return m_IBU;
}

QList<double> Recipe::IBUs() {
   if (m_uninitializedCalcs) {
      recalcAll();
   }
   return m_ibus;
}

double Recipe::boilGrav() {
   if (m_uninitializedCalcs) {
      recalcAll();
   }
   return m_boilGrav;
}

double Recipe::calories12oz() {
   if (m_uninitializedCalcs) {
      recalcAll();
   }
   return m_calories;
}

double Recipe::calories33cl() {
   if (m_uninitializedCalcs) {
      recalcAll();
   }
   return m_calories * 3.3 / 3.55;
}

double Recipe::wortFromMash_l() {
   if (m_uninitializedCalcs) {
      recalcAll();
   }
   return m_wortFromMash_l;
}

double Recipe::boilVolume_l() {
   if (m_uninitializedCalcs) {
      recalcAll();
   }
   return m_boilVolume_l;
}

double Recipe::postBoilVolume_l() {
   if (m_uninitializedCalcs) {
      recalcAll();
   }
   return m_postBoilVolume_l;
}

double Recipe::finalVolume_l() {
   if (m_uninitializedCalcs) {
      recalcAll();
   }
   return m_finalVolume_l;
}

QColor Recipe::SRMColor() {
   if (m_uninitializedCalcs) {
      recalcAll();
   }
   return m_SRMColor;
}

double Recipe::grainsInMash_kg() {
   if (m_uninitializedCalcs) {
      recalcAll();
   }
   return m_grainsInMash_kg;
}

double Recipe::grains_kg() {
   if (m_uninitializedCalcs) {
      recalcAll();
   }
   return m_grains_kg;
}

double Recipe::points() {
   if (m_uninitializedCalcs) {
      recalcAll();
   }
   return (m_og - 1.0) * 1e3;
}

//=========================Relational Getters=============================
Style * Recipe::style     () const { return ObjectStoreWrapper::getByIdRaw<Style>(this->m_styleId); }
int     Recipe::getStyleId() const { return                                       this->m_styleId ; }
Equipment * Recipe::equipment     () const { return ObjectStoreWrapper::getByIdRaw<Equipment>(this->m_equipmentId); }
int         Recipe::getEquipmentId() const { return                                           this->m_equipmentId ; }
std::shared_ptr<Mash>         Recipe::getMash          () const { return ObjectStoreWrapper::getById   <Mash        >(this->m_mashId); }
Mash *                        Recipe::mash             () const { return ObjectStoreWrapper::getByIdRaw<Mash        >(this->m_mashId); }
int                           Recipe::getMashId        () const { return                                              this->m_mashId ; }
// ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
std::optional<std::shared_ptr<Boil>> Recipe::boil() const {
   // In BeerJSON, boil is an optional record.  There are people making beer without boiling -- eg see
   // https://byo.com/article/raw-ale/.  So we ought to support it.
   if (this->m_boilId < 0) {
      // Negative ID just means there isn't one -- because this is how we store "NULL" for a foreign key
      return std::nullopt;
   }
   auto retVal = ObjectStoreWrapper::getById<Boil>(this->m_boilId);
   if (!retVal) {
      // I would think it's a coding error to have a seemingly valid boil ID that's not in the database, but we try to
      // recover as best we can.
      qCritical() << Q_FUNC_INFO << "Invalid boil ID (" << this->m_boilId << ") on Recipe #" << this->key();
      return std::nullopt;
   }

   return retVal;
}
std::shared_ptr<Boil> Recipe::nonOptBoil() {
   return this->pimpl->nonOptionalItem<Boil>(PropertyNames::Recipe::boilId, this->m_boilId);
}

///Boil *                        Recipe::boil             () const { return ObjectStoreWrapper::getByIdRaw<Boil        >(this->m_boilId); }
int                           Recipe::getBoilId        () const { return                                              this->m_boilId ; }
std::shared_ptr<Fermentation> Recipe::getFermentation  () const { return ObjectStoreWrapper::getById   <Fermentation>(this->m_fermentationId); }
Fermentation *                Recipe::fermentation     () const { return ObjectStoreWrapper::getByIdRaw<Fermentation>(this->m_fermentationId); }
int                           Recipe::getFermentationId() const { return                                              this->m_fermentationId ; }

QList<Instruction *> Recipe::instructions() const {
   return this->pimpl->getAllMyRaw<Instruction>();
}
QVector<int> Recipe::getInstructionIds() const {
   return this->pimpl->instructionIds;
}
QList<BrewNote *> Recipe::brewNotes() const {
   // The Recipe owns its BrewNotes, but, for the moment at least, it's the BrewNote that knows which Recipe it's in
   // rather than the Recipe which knows which BrewNotes it has, so we have to ask.
   int const recipeId = this->key();
   return ObjectStoreTyped<BrewNote>::getInstance().findAllMatching(
      [recipeId](BrewNote const * bn) {
         return bn->getRecipeId() == recipeId;
      }
   );
}

template<typename NE> QList< std::shared_ptr<NE> > Recipe::getAll() const {
   return this->pimpl->getAllMy<NE>();
}
//
// Instantiate the above template function for the types that are going to use it
// (This is all just a trick to allow the template definition to be here in the .cpp file and not in the header, which
// means, amongst other things, that we can reference the pimpl.)
//
template QList< std::shared_ptr<Fermentable> > Recipe::getAll<Fermentable>() const;
template QList< std::shared_ptr<Misc> > Recipe::getAll<Misc>() const;
template QList< std::shared_ptr<Salt> > Recipe::getAll<Salt>() const;
template QList< std::shared_ptr<Yeast> > Recipe::getAll<Yeast>() const;
template QList< std::shared_ptr<Water> > Recipe::getAll<Water>() const;
// Override for things that aren't stored in junction tables
template<> QList< std::shared_ptr<RecipeAdditionHop> > Recipe::getAll<RecipeAdditionHop>() const {
   return this->pimpl->allMy<RecipeAdditionHop>();
}

QList<RecipeAdditionHop *>  Recipe::hopAdditions() const { return this->pimpl->allMyRaw<RecipeAdditionHop>(); }
QVector<int>         Recipe::hopAdditionIds()      const { return this->pimpl->allMyIds<RecipeAdditionHop>();                 }
QList<Fermentable *> Recipe::fermentables()      const { return this->pimpl->getAllMyRaw<Fermentable>(); }
QVector<int>         Recipe::getFermentableIds() const { return this->pimpl->fermentableIds;             }
QList<Misc *>        Recipe::miscs()             const { return this->pimpl->getAllMyRaw<Misc>();        }
QVector<int>         Recipe::getMiscIds()        const { return this->pimpl->miscIds;                    }
QList<Yeast *>       Recipe::yeasts()            const { return this->pimpl->getAllMyRaw<Yeast>();       }
QVector<int>         Recipe::getYeastIds()       const { return this->pimpl->yeastIds;                   }
QList<Water *>       Recipe::waters()            const { return this->pimpl->getAllMyRaw<Water>();       }
QVector<int>         Recipe::getWaterIds()       const { return this->pimpl->waterIds;                   }
QList<Salt *>        Recipe::salts()             const { return this->pimpl->getAllMyRaw<Salt>();        }
QVector<int>         Recipe::getSaltIds()        const { return this->pimpl->saltIds;                    }
int                  Recipe::getAncestorId()     const { return this->m_ancestor_id;                     }

//==============================Getters===================================
Recipe::Type Recipe::type()          const { return m_type;               }
QString Recipe::brewer()             const { return m_brewer;             }
QString Recipe::asstBrewer()         const { return m_asstBrewer;         }
QString Recipe::notes()              const { return m_notes;              }
QString Recipe::tasteNotes()         const { return m_tasteNotes;         }
QString Recipe::primingSugarName()   const { return m_primingSugarName;   }
bool    Recipe::forcedCarbonation()  const { return m_forcedCarbonation;  }
double  Recipe::batchSize_l()        const { return m_batchSize_l;        }
[[deprecated]] double Recipe::boilSize_l()         const {
   // We still need to support boilSize_l for BeerXML
   return this->pimpl->boilSizeInLitersOr(0.0);
}
[[deprecated]] double Recipe::boilTime_min()       const {
   // We still need to support boilSize_l for BeerXML
   return this->pimpl->boilTimeInMinutesOr(0.0);

}
double  Recipe::efficiency_pct()     const { return m_efficiency_pct;     }
double  Recipe::tasteRating()        const { return m_tasteRating;        }
double  Recipe::primaryAge_days()    const { return m_primaryAge_days;    }
double  Recipe::primaryTemp_c()      const { return m_primaryTemp_c;      }
double  Recipe::secondaryAge_days()  const { return m_secondaryAge_days;  }
double  Recipe::secondaryTemp_c()    const { return m_secondaryTemp_c;    }
double  Recipe::tertiaryAge_days()   const { return m_tertiaryAge_days;   }
double  Recipe::tertiaryTemp_c()     const { return m_tertiaryTemp_c;     }
double  Recipe::age_days()           const { return m_age;                }
double  Recipe::ageTemp_c()          const { return m_ageTemp_c;          }
double  Recipe::carbonation_vols()   const { return m_carbonation_vols;   }
double  Recipe::carbonationTemp_c()  const { return m_carbonationTemp_c;  }
double  Recipe::primingSugarEquiv()  const { return m_primingSugarEquiv;  }
double  Recipe::kegPrimingFactor()   const { return m_kegPrimingFactor;   }
int     Recipe::fermentationStages() const { return m_fermentationStages; }
QDate   Recipe::date()               const { return m_date;               }
bool    Recipe::locked()             const { return m_locked;             }

//=============================Adders and Removers========================================


double Recipe::batchSizeNoLosses_l() {
   double ret = batchSize_l();
   Equipment * e = equipment();
   if (e) {
      ret += e->kettleTrubChillerLoss_l();
   }

   return ret;
}

//==============================Recalculators==================================

void Recipe::recalcIfNeeded(QString classNameOfWhatWasAddedOrChanged) {
   qDebug() << Q_FUNC_INFO << classNameOfWhatWasAddedOrChanged;
   // We could just compare with "Hop", "Equipment", etc but there's then no compile-time checking of typos.  Using
   // ::staticMetaObject.className() is a bit more clunky but it's safer.

   if (classNameOfWhatWasAddedOrChanged == Hop::staticMetaObject.className()) {
      this->recalcIBU();
      return;
   }

   if (classNameOfWhatWasAddedOrChanged == Equipment::staticMetaObject.className() ||
       classNameOfWhatWasAddedOrChanged == Fermentable::staticMetaObject.className() ||
       classNameOfWhatWasAddedOrChanged == Mash::staticMetaObject.className()) {
      this->recalcAll();
      return;
   }

   if (classNameOfWhatWasAddedOrChanged == Yeast::staticMetaObject.className()) {
      this->recalcOgFg();
      this->recalcABV_pct();
      return;
   }

   return;
}

void Recipe::recalcAll() {
   // WARNING
   // Infinite recursion possible, since these methods will emit changed(),
   // causing other objects to call finalVolume_l() for example, which may
   // cause another call to recalcAll() and so on.
   //
   // GSG: Now only emit when _uninitializedCalcs is true, which helps some.

   // Someone has already called this function back in the call stack, so return to avoid recursion.
   if (! m_recalcMutex.tryLock()) {
      return;
   }

   // Times are in seconds, and are cumulative.
   recalcGrainsInMash_kg(); // 0.01
   recalcGrains_kg(); // 0.03
   recalcVolumeEstimates(); // 0.06
   recalcColor_srm(); // 0.08
   recalcSRMColor(); // 0.08
   recalcOgFg(); // 0.11
   recalcABV_pct(); // 0.12
   recalcBoilGrav(); // 0.14
   recalcIBU(); // 0.15
   recalcCalories();

   m_uninitializedCalcs = false;

   m_recalcMutex.unlock();
   return;
}

void Recipe::recalcABV_pct() {
   double ret;

   // The complex formula, and variations comes from Ritchie Products Ltd, (Zymurgy, Summer 1995, vol. 18, no. 2)
   // Michael L. Hall’s article Brew by the Numbers: Add Up What’s in Your Beer, and Designing Great Beers by Daniels.
   ret = (76.08 * (m_og_fermentable - m_fg_fermentable) / (1.775 - m_og_fermentable)) * (m_fg_fermentable / 0.794);

   if (! qFuzzyCompare(ret, m_ABV_pct)) {
      m_ABV_pct = ret;
      if (!m_uninitializedCalcs) {
         emit changed(metaProperty(*PropertyNames::Recipe::ABV_pct), m_ABV_pct);
      }
   }
   return;
}

void Recipe::recalcColor_srm() {
   double mcu = 0.0;

   for (auto const * ii : this->fermentables()) {
      if (ii->amountIsWeight()) {
         // Conversion factor for lb/gal to kg/l = 8.34538.
         mcu += ii->color_srm() * 8.34538 * ii->amount() / m_finalVolumeNoLosses_l;
      } else {
         // .:TBD:. What do do about liquids
         qWarning() <<
            Q_FUNC_INFO << "Unimplemented branch for handling color of liquid fermentables - #" << ii->key() << ":" <<
            ii->name();
      }
   }

   double ret = ColorMethods::mcuToSrm(mcu);

   if (! qFuzzyCompare(m_color_srm, ret)) {
      m_color_srm = ret;
      if (!m_uninitializedCalcs) {
         emit changed(metaProperty(*PropertyNames::Recipe::color_srm), m_color_srm);
      }
   }

   return;
}

void Recipe::recalcIBU() {
   double ibus = 0.0;

   // Bitterness due to hops...
   m_ibus.clear();
   for (auto const hopAddition : this->hopAdditions()) {
      double tmp = ibuFromHopAddition(hopAddition);
      m_ibus.append(tmp);
      ibus += tmp;
   }

   // Bitterness due to hopped extracts...
   for (auto const * ii : this->fermentables()) {
      if (ii->amountIsWeight()) {
         // Conversion factor for lb/gal to kg/l = 8.34538.
         ibus += ii->ibuGalPerLb() * (ii->amount() / batchSize_l()) / 8.34538;
      } else {
         // .:TBD:. What do do about liquids
         qWarning() <<
            Q_FUNC_INFO << "Unimplemented branch for handling IBU of liquid fermentables - #" << ii->key() << ":" <<
            ii->name();
      }
   }

   if (! qFuzzyCompare(ibus, m_IBU)) {
      m_IBU = ibus;
      if (!m_uninitializedCalcs) {
         emit changed(metaProperty(*PropertyNames::Recipe::IBU), m_IBU);
      }
   }

   return;
}

void Recipe::recalcVolumeEstimates() {
   double waterAdded_l;
   double absorption_lKg;
   double tmp = 0.0;
   double tmp_wfm = 0.0;
   double tmp_bv = 0.0;
   double tmp_fv = 0.0;
   double tmp_pbv = 0.0;

   // wortFromMash_l ==========================
   if (mash() == nullptr) {
      m_wortFromMash_l = 0.0;
   } else {
      waterAdded_l = mash()->totalMashWater_l();
      if (equipment() != nullptr) {
         absorption_lKg = equipment()->mashTunGrainAbsorption_LKg().value_or(Equipment::default_mashTunGrainAbsorption_LKg);
      } else {
         absorption_lKg = PhysicalConstants::grainAbsorption_Lkg;
      }

      tmp_wfm = (waterAdded_l - absorption_lKg * m_grainsInMash_kg);
   }

   // boilVolume_l ==============================

   if (equipment() != nullptr) {
      tmp = tmp_wfm - equipment()->getLauteringDeadspaceLoss_l() + equipment()->topUpKettle_l().value_or(Equipment::default_topUpKettle_l);
   } else {
      tmp = tmp_wfm;
   }

   // .:TODO:. Assumptions below about liquids are almost certainly wrong, also TBD what other cases we have to cover
   // Need to account for extract/sugar volume also.
   for (auto const * ii : this->fermentables()) {
      switch (ii->type()) {
         case Fermentable::Type::Extract:
            if (ii->amountIsWeight()) {
               tmp += ii->amount() / PhysicalConstants::liquidExtractDensity_kgL;
            } else {
               tmp += ii->amount();
            }
            break;
         case Fermentable::Type::Sugar:
            if (ii->amountIsWeight()) {
               tmp += ii->amount() / PhysicalConstants::sucroseDensity_kgL;
            } else {
               tmp += ii->amount();
            }
            break;
         case Fermentable::Type::Dry_Extract:
            if (ii->amountIsWeight()) {
               tmp += ii->amount() / PhysicalConstants::dryExtractDensity_kgL;
            } else {
               tmp += ii->amount();
            }
            break;
      }
   }

   if (tmp <= 0.0) {
      // Give up.
      tmp = this->pimpl->boilSizeInLitersOr(0.0);
   }

   tmp_bv = tmp;

   // finalVolume_l ==============================

   // NOTE: the following figure is not based on the other volume estimates
   // since we want to show og,fg,ibus,etc. as if the collected wort is correct.
   m_finalVolumeNoLosses_l = batchSizeNoLosses_l();
   if (equipment() != nullptr) {
      //_finalVolumeNoLosses_l = equipment()->wortEndOfBoil_l(tmp_bv) + equipment()->topUpWater_l();
      tmp_fv = equipment()->wortEndOfBoil_l(tmp_bv) + equipment()->topUpWater_l().value_or(Equipment::default_topUpWater_l) - equipment()->kettleTrubChillerLoss_l();
   } else {
      m_finalVolume_l = tmp_bv - 4.0; // This is just shooting in the dark. Can't do much without an equipment.
      //_finalVolumeNoLosses_l = _finalVolume_l;
   }

   // postBoilVolume_l ===========================

   if (equipment() != nullptr) {
      tmp_pbv = equipment()->wortEndOfBoil_l(tmp_bv);
   } else {
      tmp_pbv = batchSize_l(); // Give up.
   }

   if (! qFuzzyCompare(tmp_wfm, m_wortFromMash_l)) {
      m_wortFromMash_l = tmp_wfm;
      if (!m_uninitializedCalcs) {
         emit changed(metaProperty(*PropertyNames::Recipe::wortFromMash_l), m_wortFromMash_l);
      }
   }

   // TODO: Still need to get rid of m_boilVolume_l
   if (! qFuzzyCompare(tmp_bv, m_boilVolume_l)) {
      m_boilVolume_l = tmp_bv;
      if (!m_uninitializedCalcs) {
         emit changed(metaProperty(*PropertyNames::Recipe::boilVolume_l), m_boilVolume_l);
      }
   }

   if (! qFuzzyCompare(tmp_fv, m_finalVolume_l)) {
      m_finalVolume_l = tmp_fv;
      if (!m_uninitializedCalcs) {
         emit changed(metaProperty(*PropertyNames::Recipe::finalVolume_l), m_finalVolume_l);
      }
   }

   if (! qFuzzyCompare(tmp_pbv, m_postBoilVolume_l)) {
      m_postBoilVolume_l = tmp_pbv;
      if (!m_uninitializedCalcs) {
         emit changed(metaProperty(*PropertyNames::Recipe::postBoilVolume_l), m_postBoilVolume_l);
      }
   }
   return;
}

void Recipe::recalcGrainsInMash_kg() {
   double ret = 0.0;

   for (auto const * ii : this->fermentables()) {
      if (ii->type() == Fermentable::Type::Grain && ii->isMashed()) {
         if (ii->amountIsWeight()) {
            ret += ii->amount();
         } else {
            qWarning() <<
               Q_FUNC_INFO << "Ignoring fermentable #" << ii->key() << "(" << ii->name() << ") as measured by "
               "volume";
         }
      }
   }

   if (! qFuzzyCompare(ret, m_grainsInMash_kg)) {
      m_grainsInMash_kg = ret;
      if (!m_uninitializedCalcs) {
         emit changed(metaProperty(*PropertyNames::Recipe::grainsInMash_kg), m_grainsInMash_kg);
      }
   }
   return;
}

void Recipe::recalcGrains_kg() {
   double ret = 0.0;

   for (auto const * ii : this->fermentables()) {
      // .:TODO:. Need to think about what, if anything, we need to do for other Fermantable types here
      if (ii->type() == Fermentable::Type::Grain) {
         // I wouldn't have thought you would want to measure grain by volume, but best to check
         if (ii->amountIsWeight()) {
            ret += ii->amount();
         } else {
            qWarning() <<
               Q_FUNC_INFO << "Ignoring fermentable #" << ii->key() << "(" << ii->name() << ") as measured by "
               "volume";
         }
      }
   }

   if (! qFuzzyCompare(ret, m_grains_kg)) {
      m_grains_kg = ret;
      if (!m_uninitializedCalcs) {
         emit changed(metaProperty(*PropertyNames::Recipe::grains_kg), m_grains_kg);
      }
   }
   return;
}

void Recipe::recalcSRMColor() {
   QColor tmp = Algorithms::srmToColor(m_color_srm);

   if (tmp != m_SRMColor) {
      m_SRMColor = tmp;
      if (!m_uninitializedCalcs) {
         emit changed(metaProperty(*PropertyNames::Recipe::SRMColor), m_SRMColor);
      }
   }
   return;
}

// the formula in here are taken from http://hbd.org/ensmingr/
void Recipe::recalcCalories() {
   double startPlato, finishPlato, RE, abw, oog, ffg, tmp;

   oog = m_og;
   ffg = m_fg;

   // Need to translate OG and FG into plato
   startPlato  = -463.37 + (668.72 * oog) - (205.35 * oog * oog);
   finishPlato = -463.37 + (668.72 * ffg) - (205.35 * ffg * ffg);

   // RE (real extract)
   RE = (0.1808 * startPlato) + (0.8192 * finishPlato);

   // Alcohol by weight?
   abw = (startPlato - RE) / (2.0665 - (0.010665 * startPlato));

   // The final results of this formular are calories per 100 ml.
   // The 3.55 puts it in terms of 12 oz. I really should have stored it
   // without that adjust.
   tmp = ((6.9 * abw) + 4.0 * (RE - 0.1)) * ffg * 3.55;

   //! If there are no fermentables in the recipe, if there is no mash, etc.,
   //  then the calories/12 oz ends up negative. Since negative doesn't make
   //  sense, set it to 0
   if (tmp < 0) {
      tmp = 0;
   }

   if (! qFuzzyCompare(tmp, m_calories)) {
      m_calories = tmp;
      if (!m_uninitializedCalcs) {
         emit changed(metaProperty(*PropertyNames::Recipe::calories), m_calories);
      }
   }
   return;
}

// other efficiency calculations need access to the maximum theoretical sugars
// available. The only way I can see of doing that which doesn't suck is to
// split that calcuation out of recalcOgFg();
QHash<QString, double> Recipe::calcTotalPoints() {
   int i;
   double sugar_kg_ignoreEfficiency = 0.0;
   double sugar_kg                  = 0.0;
   double nonFermentableSugars_kg    = 0.0;
   double lateAddition_kg           = 0.0;
   double lateAddition_kg_ignoreEff = 0.0;

   Fermentable * ferm;

   QList<Fermentable *> ferms = fermentables();
   QHash<QString, double> ret;

   for (i = 0; static_cast<int>(i) < ferms.size(); ++i) {
      ferm = ferms[i];

      // If we have some sort of non-grain, we have to ignore efficiency.
      if (ferm->isSugar() || ferm->isExtract()) {
         sugar_kg_ignoreEfficiency += ferm->equivSucrose_kg();

         if (ferm->addAfterBoil()) {
            lateAddition_kg_ignoreEff += ferm->equivSucrose_kg();
         }

         if (!isFermentableSugar(ferm)) {
            nonFermentableSugars_kg += ferm->equivSucrose_kg();
         }
      } else {
         sugar_kg += ferm->equivSucrose_kg();

         if (ferm->addAfterBoil()) {
            lateAddition_kg += ferm->equivSucrose_kg();
         }
      }
   }

   ret.insert("sugar_kg", sugar_kg);
   ret.insert("nonFermentableSugars_kg", nonFermentableSugars_kg);
   ret.insert("sugar_kg_ignoreEfficiency", sugar_kg_ignoreEfficiency);
   ret.insert("lateAddition_kg", lateAddition_kg);
   ret.insert("lateAddition_kg_ignoreEff", lateAddition_kg_ignoreEff);

   return ret;
}

void Recipe::recalcBoilGrav() {
   double sugar_kg = 0.0;
   double sugar_kg_ignoreEfficiency = 0.0;
   double lateAddition_kg           = 0.0;
   double lateAddition_kg_ignoreEff = 0.0;
   double ret;
   QHash<QString, double> sugars;

   sugars = calcTotalPoints();
   sugar_kg = sugars.value("sugar_kg");
   sugar_kg_ignoreEfficiency = sugars.value("sugar_kg_ignoreEfficiency");
   lateAddition_kg = sugars.value("lateAddition_kg");
   lateAddition_kg_ignoreEff = sugars.value("lateAddition_kg_ignoreEff");

   // Since the efficiency refers to how much sugar we get into the fermenter,
   // we need to adjust for that here.
   sugar_kg = (efficiency_pct() / 100.0 * (sugar_kg - lateAddition_kg) + sugar_kg_ignoreEfficiency -
               lateAddition_kg_ignoreEff);

   ret = Algorithms::PlatoToSG_20C20C(Algorithms::getPlato(sugar_kg, this->pimpl->boilSizeInLitersOr(0.0)));

   if (! qFuzzyCompare(ret, m_boilGrav)) {
      m_boilGrav = ret;
      if (!m_uninitializedCalcs) {
         emit changed(metaProperty(*PropertyNames::Recipe::boilGrav), m_boilGrav);
      }
   }
   return;
}

void Recipe::recalcOgFg() {
   int i;
   double plato;
   double sugar_kg = 0;
   double sugar_kg_ignoreEfficiency = 0.0;
   double nonFermentableSugars_kg = 0.0;
   double kettleWort_l = 0.0;
   double postBoilWort_l = 0.0;
   double ratio = 0.0;
   double ferm_kg = 0.0;
   double attenuation_pct = 0.0;
   double tmp_og, tmp_fg, tmp_pnts, tmp_ferm_pnts, tmp_nonferm_pnts;
   Yeast * yeast;

   m_og_fermentable = m_fg_fermentable = 0.0;

   // The first time through really has to get the _og and _fg from the
   // database, not use the initialized values of 1. I (maf) tried putting
   // this in the initialize, but it just hung. So I moved it here, but only
   // if if we aren't initialized yet.
   //
   // GSG: This doesn't work, this og and fg are already set to 1.0 so
   // until we load these values from the database on startup, we have
   // to calculate.
   if (m_uninitializedCalcs) {
      m_og = Localization::toDouble(*this, PropertyNames::Recipe::og, Q_FUNC_INFO);
      m_fg = Localization::toDouble(*this, PropertyNames::Recipe::fg, Q_FUNC_INFO);
   }

   // Find out how much sugar we have.
   QHash<QString, double> sugars = calcTotalPoints();
   sugar_kg                  = sugars.value("sugar_kg");  // Mass of sugar that *is* affected by mash efficiency
   sugar_kg_ignoreEfficiency =
      sugars.value("sugar_kg_ignoreEfficiency");  // Mass of sugar that *is not* affected by mash efficiency
   nonFermentableSugars_kg    =
      sugars.value("nonFermentableSugars_kg");  // Mass of sugar that is not fermentable (also counted in sugar_kg_ignoreEfficiency)

   // We might lose some sugar in the form of Trub/Chiller loss and lauter deadspace.
   if (equipment() != nullptr) {

      kettleWort_l = (m_wortFromMash_l - equipment()->getLauteringDeadspaceLoss_l()) + equipment()->topUpKettle_l().value_or(Equipment::default_topUpKettle_l);
      postBoilWort_l = equipment()->wortEndOfBoil_l(kettleWort_l);
      ratio = (postBoilWort_l - equipment()->kettleTrubChillerLoss_l()) / postBoilWort_l;
      if (ratio > 1.0) { // Usually happens when we don't have a mash yet.
         ratio = 1.0;
      } else if (ratio < 0.0) {
         ratio = 0.0;
      } else if (Algorithms::isNan(ratio)) {
         ratio = 1.0;
      }
      // Ignore this again since it should be included in efficiency.
      //sugar_kg *= ratio;
      sugar_kg_ignoreEfficiency *= ratio;
      if (nonFermentableSugars_kg != 0.0) {
         nonFermentableSugars_kg *= ratio;
      }
   }

   // Total sugars after accounting for efficiency and mash losses. Implicitly includes non-fermentable sugars
   sugar_kg = sugar_kg * efficiency_pct() / 100.0 + sugar_kg_ignoreEfficiency;
   plato = Algorithms::getPlato(sugar_kg, m_finalVolumeNoLosses_l);

   tmp_og = Algorithms::PlatoToSG_20C20C(plato);    // og from all sugars
   tmp_pnts = (tmp_og - 1) * 1000.0; // points from all sugars
   if (nonFermentableSugars_kg != 0.0) {
      ferm_kg = sugar_kg - nonFermentableSugars_kg;  // Mass of only fermentable sugars
      plato = Algorithms::getPlato(ferm_kg, m_finalVolumeNoLosses_l);   // Plato from fermentable sugars
      m_og_fermentable = Algorithms::PlatoToSG_20C20C(plato);    // og from only fermentable sugars
      plato = Algorithms::getPlato(nonFermentableSugars_kg, m_finalVolumeNoLosses_l);   // Plate from non-fermentable sugars
      tmp_nonferm_pnts = ((Algorithms::PlatoToSG_20C20C(plato)) - 1) * 1000.0; // og points from non-fermentable sugars
   } else {
      m_og_fermentable = tmp_og;
      tmp_nonferm_pnts = 0;
   }

   // Calculage FG
   QList<Yeast *> yeasties = yeasts();
   for (i = 0; static_cast<int>(i) < yeasties.size(); ++i) {
      yeast = yeasties[i];
      // Get the yeast with the greatest attenuation.
      if (yeast->attenuation_pct() > attenuation_pct) {
         attenuation_pct = yeast->getTypicalAttenuation_pct();
      }
   }
   // This means we have yeast, but they neglected to provide attenuation percentages.
   if (yeasties.size() > 0 && attenuation_pct <= 0.0)  {
      attenuation_pct = Yeast::DefaultAttenuation_pct; // Use an average attenuation.
   }

   if (nonFermentableSugars_kg != 0.0) {
      tmp_ferm_pnts = (tmp_pnts - tmp_nonferm_pnts) * (1.0 - attenuation_pct / 100.0); // fg points from fermentable sugars
      tmp_pnts = tmp_ferm_pnts + tmp_nonferm_pnts;  // FG points from both fermentable and non-fermentable sugars
      //tmp_pnts *= (1.0 - attenuation_pct/100.0);  // WTF, this completely ignores all the calculations about non-fermentable sugars and just converts everything!
      tmp_fg =  1 + tmp_pnts / 1000.0; // new FG value
      m_fg_fermentable =  1 + tmp_ferm_pnts / 1000.0; // FG from fermentables only
   } else {
      tmp_pnts *= (1.0 - attenuation_pct / 100.0);
      tmp_fg =  1 + tmp_pnts / 1000.0;
      m_fg_fermentable = tmp_fg;
   }

   if (! qFuzzyCompare(m_og, tmp_og)) {
      m_og     = tmp_og;
      // NOTE: We don't want to do this on the first load of the recipe.
      // NOTE: We are we recalculating all of these on load? Shouldn't we be
      // reading these values from the database somehow?
      //
      // GSG: Yes we can, but until the code is added to intialize these calculated
      // values from the database, we can calculate them on load. They should be
      // the same as the database values since the database values were set with
      // these functions in the first place.
      if (!m_uninitializedCalcs) {
         this->propagatePropertyChange(PropertyNames::Recipe::og, false);
         emit changed(metaProperty(*PropertyNames::Recipe::og), m_og);
         emit changed(metaProperty(*PropertyNames::Recipe::points), (m_og - 1.0) * 1e3);
      }
   }

   if (! qFuzzyCompare(tmp_fg, m_fg)) {
      m_fg     = tmp_fg;
      if (!m_uninitializedCalcs) {
         this->propagatePropertyChange(PropertyNames::Recipe::fg, false);
         emit changed(metaProperty(*PropertyNames::Recipe::fg), m_fg);
      }
   }
   return;
}

//====================================Helpers===========================================

double Recipe::ibuFromHopAddition(RecipeAdditionHop const * hopAddition) {
   Equipment * equip = this->equipment();
   double ibus = 0.0;
   double fwhAdjust = Localization::toDouble(
      PersistentSettings::value(PersistentSettings::Names::firstWortHopAdjustment, 1.1).toString(),
      Q_FUNC_INFO
   );
   double mashHopAdjust = Localization::toDouble(
      PersistentSettings::value(PersistentSettings::Names::mashHopAdjustment, 0).toString(),
      Q_FUNC_INFO
   );

   if (hopAddition == nullptr) {
      return 0.0;
   }

   // It's a coding error to ask one recipe about another's hop additions!
   Q_ASSERT(hopAddition->recipeId() == this->key());

   double AArating = hopAddition->hop()->alpha_pct() / 100.0;
   // .:TBD.JSON:.  What to do if hopAddition is measured by volume?
   if (!hopAddition->amountIsWeight()) {
      qCritical() << Q_FUNC_INFO << "Using Hop volume as weight - THIS IS PROBABLY WRONG!";
   }
   double grams = hopAddition->amount() * 1000.0;
   double minutes = hopAddition->addAtTime_mins().value_or(0.0);
   // Assume 100% utilization until further notice
   double hopUtilization = 1.0;
   // Assume 60 min boil until further notice
   int boilTime = 60;

   // NOTE: we used to carefully calculate the average boil gravity and use it in the
   // IBU calculations. However, due to John Palmer
   // (http://homebrew.stackexchange.com/questions/7343/does-wort-gravity-affect-hopAddition-utilization),
   // it seems more appropriate to just use the OG directly, since it is the total
   // amount of break material that truly affects the IBUs.

   if (equip) {
      hopUtilization = equip->hopUtilization_pct().value_or(Equipment::default_hopUtilization_pct) / 100.0;
      boilTime = static_cast<int>(equip->boilTime_min().value_or(Equipment::default_boilTime_min));
   }

   if (hopAddition->isFirstWort()) {
      ibus = fwhAdjust * IbuMethods::getIbus(AArating, grams, m_finalVolumeNoLosses_l, m_og, boilTime);
   } else if (hopAddition->stage() == RecipeAddition::Stage::Boil) {
      ibus = IbuMethods::getIbus(AArating, grams, m_finalVolumeNoLosses_l, m_og, minutes);
   } else if (hopAddition->stage() == RecipeAddition::Stage::Mash && mashHopAdjust > 0.0) {
      ibus = mashHopAdjust * IbuMethods::getIbus(AArating, grams, m_finalVolumeNoLosses_l, m_og, boilTime);
   }

   // Adjust for hopAddition form. Tinseth's table was created from whole cone data,
   // and it seems other formulae are optimized that way as well. So, the
   // utilization is considered unadjusted for whole cones, and adjusted
   // up for plugs and pellets.
   //
   // - http://www.realbeer.com/hops/FAQ.html
   // - https://groups.google.com/forum/#!topic"brewtarget.h"lp/mv2qvWBC4sU
   auto const hopForm = hopAddition->hop()->form();
   if (hopForm) {
      switch (*hopForm) {
         case Hop::Form::Plug:
            hopUtilization *= 1.02;
            break;
         case Hop::Form::Pellet:
            hopUtilization *= 1.10;
            break;
         default:
            break;
      }
   }

   // Adjust for hopAddition utilization.
   ibus *= hopUtilization;

   return ibus;
}

///// this was fixed, but not with an at
///bool Recipe::isValidType(const QString & str) {
///   return RECIPE_TYPE_STRING_TO_TYPE.contains(str);
///}

QList<QString> Recipe::getReagents(QList<Fermentable *> ferms) {
   QList<QString> reagents;
   for (int ii = 0; ii < ferms.size(); ++ii) {
      if (ferms[ii]->isMashed()) {
         // .:TBD:.  This isn't the most elegant or accurate way of handling commas.  If we're returning a list, we
         // should probably leave it to the caller to put commas in for display.
         QString format;
         if (ii + 1 < ferms.size()) {
            format = "%1 %2, ";
         } else {
            format = "%1 %2 ";
         }
         reagents.append(
            format.arg(Measurement::displayAmount(ferms[ii]->amountWithUnits()))
                  .arg(ferms[ii]->name())
         );
      }
   }
   return reagents;
}


QList<QString> Recipe::getReagents(QList<RecipeAdditionHop *> hopAdditions, bool firstWort) {
   QList<QString> reagents;

   for (auto hopAddition : hopAdditions) {
      if (firstWort && (hopAddition->isFirstWort())) {
         QString tmp = QString("%1 %2,")
               .arg(Measurement::displayAmount(hopAddition->amountWithUnits()))
               .arg(hopAddition->hop()->name());
         reagents.append(tmp);
      }
   }
   return reagents;
}

QList<QString> Recipe::getReagents(QList< std::shared_ptr<MashStep> > msteps) {
   QList<QString> reagents;

   for (int ii = 0; ii < msteps.size(); ++ii) {
      if (!msteps[ii]->isInfusion()) {
         continue;
      }

      QString tmp;
      if (ii + 1 < msteps.size()) {
         tmp = tr("%1 water to %2, ")
               .arg(Measurement::displayAmount(Measurement::Amount{msteps[ii]->amount_l(), Measurement::Units::liters}))
               .arg(Measurement::displayAmount(Measurement::Amount{msteps[ii]->infuseTemp_c().value_or(msteps[ii]->stepTemp_c()), Measurement::Units::celsius}));
      } else {
         tmp = tr("%1 water to %2 ")
               .arg(Measurement::displayAmount(Measurement::Amount{msteps[ii]->amount_l(), Measurement::Units::liters}))
               .arg(Measurement::displayAmount(Measurement::Amount{msteps[ii]->infuseTemp_c().value_or(msteps[ii]->stepTemp_c()), Measurement::Units::celsius}));
      }
      reagents.append(tmp);
   }
   return reagents;
}


//! \brief send me a list of salts and if we are wanting to add to the
//! mash or the sparge, and I will return a list of instructions
QStringList Recipe::getReagents(QList<Salt *> salts, Salt::WhenToAdd wanted) {
   QString tmp;
   QStringList reagents = QStringList();

   for (int ii = 0; ii < salts.size(); ++ii) {
      Salt::WhenToAdd what = salts[ii]->whenToAdd();
      Measurement::Unit const & rightUnit = salts[ii]->amountIsWeight() ? Measurement::Units::kilograms : Measurement::Units::liters;
      if (what == wanted) {
         tmp = tr("%1 %2, ")
               .arg(Measurement::displayAmount(Measurement::Amount{salts[ii]->amount(), rightUnit}))
               .arg(salts[ii]->name());
      } else if (what == Salt::WhenToAdd::EQUAL) {
         tmp = tr("%1 %2, ")
               .arg(Measurement::displayAmount(Measurement::Amount{salts[ii]->amount(), rightUnit}))
               .arg(salts[ii]->name());
      } else if (what == Salt::WhenToAdd::RATIO) {
         double ratio = 1.0;
         if (wanted == Salt::WhenToAdd::SPARGE) {
            ratio = mash()->totalSpargeAmount_l() / mash()->totalInfusionAmount_l();
         }
         double amt = salts[ii]->amount() * ratio;
         tmp = tr("%1 %2, ")
               .arg(Measurement::displayAmount(Measurement::Amount{amt, rightUnit}))
               .arg(salts[ii]->name());
      } else {
         continue;
      }
      reagents.append(tmp);
   }
   // How many ways can we remove the trailing ", " because it really, really
   // annoys me?
   if (reagents.size() > 0) {
      QString fixin = reagents.takeLast();
      fixin.remove(fixin.lastIndexOf(","), 2);
      reagents.append(fixin);
   }
   return reagents;
}

//==========================Accept changes from ingredients====================

void Recipe::acceptChangeToContainedObject(QMetaProperty prop, QVariant val) {
   // This tells us which object sent us the signal
   QObject * signalSender = this->sender();
   if (signalSender != nullptr) {
      QString signalSenderClassName = signalSender->metaObject()->className();
      QString propName = prop.name();
      qDebug() <<
         Q_FUNC_INFO << "Signal received from " << signalSenderClassName << ": changed" << propName << "to" << val;;
      Equipment * equipment = qobject_cast<Equipment *>(signalSender);
      if (equipment) {
         qDebug() << Q_FUNC_INFO << "Equipment #" << equipment->key() << "(ours=" << this->m_equipmentId << ")";
         Q_ASSERT(equipment->key() == this->m_equipmentId);
         if (propName == *PropertyNames::Equipment::kettleBoilSize_l) {
            Q_ASSERT(val.canConvert<double>());
            qDebug() << Q_FUNC_INFO << "We" << (this->boil() ? "have" : "don't have") << "a boil";
            if (this->boil()) {
               (*this->boil())->setPreBoilSize_l(val.value<double>());
            }
         } else if (propName == PropertyNames::Equipment::boilTime_min) {
            Q_ASSERT(val.canConvert<double>());
            if (this->boil()) {
               (*this->boil())->setBoilTime_mins(val.value<double>());
            }
         }
      }
      this->recalcIfNeeded(signalSenderClassName);
   } else {
      qDebug() << Q_FUNC_INFO << "No sender";
   }
   return;
}

double Recipe::targetCollectedWortVol_l() {

   // Need to account for extract/sugar volume also.
   double postMashAdditionVolume_l = 0;

   for (Fermentable const * f : this->fermentables()) {
      switch (f->type()) {
         case Fermentable::Type::Extract:
            if (f->amountIsWeight()) {
               postMashAdditionVolume_l += f->amount() / PhysicalConstants::liquidExtractDensity_kgL;
            } else {
               // .:TBD:. This is probably incorrect!
               postMashAdditionVolume_l += f->amount();
            }
            break;
         case Fermentable::Type::Sugar:
            if (f->amountIsWeight()) {
               postMashAdditionVolume_l += f->amount() / PhysicalConstants::sucroseDensity_kgL;
            } else {
               // .:TBD:. This is probably incorrect!
               postMashAdditionVolume_l += f->amount();
            }
            break;
         case Fermentable::Type::Dry_Extract:
            if (f->amountIsWeight()) {
               postMashAdditionVolume_l += f->amount() / PhysicalConstants::dryExtractDensity_kgL;
            } else {
               // .:TBD:. This is probably incorrect!
               postMashAdditionVolume_l += f->amount();
            }
            break;
         // .:TODO:. Need to handle other types of Fermentable here, even if it's just to add a NO-OP to show the
         // compiler we didn't forget about them.  For now the compiler warning will help us remember this to-do!
      }
   }

   double boilSize_liters = this->pimpl->boilSizeInLitersOr(0.0);
   qDebug() << Q_FUNC_INFO << "Boil size:" << boilSize_liters;

   if (equipment()) {
      return boilSize_liters - equipment()->topUpKettle_l().value_or(Equipment::default_topUpKettle_l) - postMashAdditionVolume_l;
   } else {
      return boilSize_liters - postMashAdditionVolume_l;
   }
}

double Recipe::targetTotalMashVol_l() {

   double absorption_lKg;

   if (equipment()) {
      absorption_lKg = equipment()->mashTunGrainAbsorption_LKg().value_or(Equipment::default_mashTunGrainAbsorption_LKg);
   } else {
      absorption_lKg = PhysicalConstants::grainAbsorption_Lkg;
   }

   return targetCollectedWortVol_l() + absorption_lKg * grainsInMash_kg();
}

Recipe * Recipe::getOwningRecipe() const {
   // Recipes own themselves.
   // Because this is a const function, the compiler treats the `this` pointer as a pointer to const Recipe.  This is
   // why we need to cast it to return it.
   return const_cast<Recipe *>(this);
}

void Recipe::hardDeleteOwnedEntities() {
   // It's the BrewNote that stores its Recipe ID, so all we need to do is delete our BrewNotes then the subsequent
   // database delete of this Recipe won't hit any foreign key problems.
   auto brewNotes = this->brewNotes();
   for (auto brewNote : brewNotes) {
      ObjectStoreWrapper::hardDelete<BrewNote>(*brewNote);
   }

   this->pimpl->hardDeleteAllMy<Fermentable>();
   this->pimpl->hardDeleteAdditions<RecipeAdditionHop>();
   this->pimpl->hardDeleteAllMy<Instruction>();
   this->pimpl->hardDeleteAllMy<Misc>       ();
   this->pimpl->hardDeleteAllMy<Salt>       ();
   this->pimpl->hardDeleteAllMy<Water>      ();
   this->pimpl->hardDeleteAllMy<Yeast>      ();

   return;
}

void Recipe::hardDeleteOrphanedEntities() {
   //
   // Strictly a Recipe does not own its Mash.  However, if our Mash does not have a name and is not used by any other
   // Recipe, then we want to delete it, on the grounds that it's not one the user intended to reuse across multiple
   // Recipes.
   //
   // However, if we try to just delete the Mash Recipe::hardDeleteOwnedEntities(), we'd get a foreign key constraint
   // violation error from the DB as, at that point, the Mash ID is still referenced by this Recipe.  (Unsetting the
   // Mash ID in the Recipe record would be a bit tricky as we'd have to set it to NULL rather than just, say, -1 as,
   // otherwise we'll get a different foreign key constraint violation error (because the DB can't find a Mash row with
   // ID -1!).)
   //
   // At this point, however, the Recipe record has been removed from the database, so we can safely delete any orphaned
   // Mash record.
   //
   Mash * mash = this->mash();
   if (mash && mash->name() == "") {
      qDebug() << Q_FUNC_INFO << "Checking whether our unnamed Mash is used elsewhere";
      auto recipesUsingThisMash = ObjectStoreWrapper::findAllMatching<Recipe>(
         [mash](Recipe const * rec) {
            return rec->uses(*mash);
         }
      );
      if (1 == recipesUsingThisMash.size()) {
         qDebug() <<
            Q_FUNC_INFO << "Deleting unnamed Mash # " << mash->key() << " used only by Recipe #" << this->key();
         Q_ASSERT(recipesUsingThisMash.at(0)->key() == this->key());
         ObjectStoreWrapper::hardDelete<Mash>(*mash);
      }
   }
   return;
}


//======================================================================================================================
//====================================== Start of Functions in Helper Namespace ========================================
//======================================================================================================================
QList<BrewNote *> RecipeHelper::brewNotesForRecipeAndAncestors(Recipe const & recipe) {
   QList<BrewNote *> brewNotes = recipe.brewNotes();
   QList<Recipe *> ancestors = recipe.ancestors();
   for (auto ancestor : ancestors) {
      brewNotes.append(ancestor->brewNotes());
   }
   return brewNotes;
}

void RecipeHelper::prepareForPropertyChange(NamedEntity & ne, BtStringConst const & propertyName) {

   //
   // If the user has said they don't want versioning, just return
   //
   if (!RecipeHelper::getAutomaticVersioningEnabled()) {
      return;
   }

   qDebug() <<
      Q_FUNC_INFO << "Modifying: " << ne.metaObject()->className() << "#" << ne.key() << "property" << propertyName;

   //
   // If the object we're about to change a property on is a Recipe or is used in a Recipe, then it might need a new
   // version -- unless it's already being versioned.
   //
   Recipe * owner = ne.getOwningRecipe();
   if (!owner || owner->isBeingModified()) {
      // Change is not related to a recipe or the recipe is already being modified
      return;
   }

   //
   // Automatic versioning means that, once a recipe is brewed, it is "soft locked" and the first change should spawn a
   // new version.  Any subsequent change should not spawn a new version until it is brewed again.
   //
   if (owner->brewNotes().empty()) {
      // Recipe hasn't been brewed
      return;
   }

   // If the object we're about to change already has descendants, then we don't want to create new ones.
   if (owner->hasDescendants()) {
      qDebug() << Q_FUNC_INFO << "Recipe #" << owner->key() << "already has descendants, so not creating any more";
      return;
   }

   //
   // Once we've started doing versioning, we don't want to trigger it again on the same Recipe until we've finished
   //
   NamedEntityModifyingMarker ownerModifyingMarker(*owner);

   //
   // Versioning when modifying something in a recipe is *hard*.  If we copy the recipe, there is no easy way to say
   // "this ingredient in the old recipe is that ingredient in the new".  One approach would be to use the delete idea,
   // ie copy everything but what's being modified, clone what's being modified and add the clone to the copy.  Another
   // is to take a deep copy of the Recipe and make that the "prior version".
   //

   // Create a deep copy of the Recipe, and put it in the DB, so it has an ID.
   // (This will also emit signalObjectInserted for the new Recipe from ObjectStoreTyped<Recipe>.)
   qDebug() << Q_FUNC_INFO << "Copying Recipe" << owner->key();

   // We also don't want to trigger versioning on the newly spawned Recipe until we're completely done here!
   std::shared_ptr<Recipe> spawn = std::make_shared<Recipe>(*owner);
   NamedEntityModifyingMarker spawnModifyingMarker(*spawn);
   ObjectStoreWrapper::insert(spawn);

   qDebug() << Q_FUNC_INFO << "Copied Recipe #" << owner->key() << "to new Recipe #" << spawn->key();

   // We assert that the newly created version of the recipe has not yet been brewed (and therefore will not get
   // automatically versioned on subsequent changes before it is brewed).
   Q_ASSERT(spawn->brewNotes().empty());

   //
   // By default, copying a Recipe does not copy all its ancestry.  Here, we want the copy to become our ancestor (ie
   // previous version).  This will also emit a signalPropertyChanged from ObjectStoreTyped<Recipe>, which the UI can
   // pick up to update tree display of Recipes etc.
   //
   owner->setAncestor(*spawn);

   return;
}

/**
 * \brief Turn automatic versioning on or off
 */
void RecipeHelper::setAutomaticVersioningEnabled(bool enabled) {
   PersistentSettings::insert(PersistentSettings::Names::versioning, enabled);
   return;
}

/**
 * \brief Returns \c true if automatic versioning is enabled, \c false otherwise
 */
bool RecipeHelper::getAutomaticVersioningEnabled() {
   return PersistentSettings::value(PersistentSettings::Names::versioning, false).toBool();
}

RecipeHelper::SuspendRecipeVersioning::SuspendRecipeVersioning() {
   this->savedVersioningValue = RecipeHelper::getAutomaticVersioningEnabled();
   if (this->savedVersioningValue) {
      qDebug() << Q_FUNC_INFO << "Temporarily suspending automatic Recipe versioning";
      RecipeHelper::setAutomaticVersioningEnabled(false);
   }
   return;
}
RecipeHelper::SuspendRecipeVersioning::~SuspendRecipeVersioning() {
   if (this->savedVersioningValue) {
      qDebug() << Q_FUNC_INFO << "Re-enabling automatic Recipe versioning";
      RecipeHelper::setAutomaticVersioningEnabled(true);
   }
   return;
}

//======================================================================================================================
//======================================= End of Functions in Helper Namespace =========================================
//======================================================================================================================
