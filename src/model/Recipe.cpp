/**
 * model/Recipe.cpp is part of Brewken, and is copyright the following authors 2009-2021:
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
 */
#include "model/Recipe.h"

#include <cmath> // For pow/log

#include <QDate>
#include <QDebug>
#include <QInputDialog>
#include <QList>
#include <QObject>
#include <QSharedPointer>

#include "Algorithms.h"
#include "Brewken.h"
#include "ColorMethods.h"
#include "database/ObjectStoreWrapper.h"
#include "HeatCalculations.h"
#include "IbuMethods.h"
#include "model/Equipment.h"
#include "model/Fermentable.h"
#include "model/Hop.h"
#include "model/Instruction.h"
#include "model/Mash.h"
#include "model/MashStep.h"
#include "model/Misc.h"
#include "model/Salt.h"
#include "model/Style.h"
#include "model/Water.h"
#include "model/Yeast.h"
#include "PersistentSettings.h"
#include "PhysicalConstants.h"
#include "PreInstruction.h"
#include "QueuedMethod.h"

static const QString kMashStepSection("mashStepTableModel");
static const QString kMiscTableSection("miscTableModel");
static const QString kFermentableTableSection("fermentableTable");
static const QString kHopTableSection("hopTable");
static const QString kSaltTableSection("saltTable");
static const QString kTabRecipeSection("tab_recipe");

namespace {

   /**
    * \brief Decide whether the supplied instance of (subclass of) NamedEntity needs to be copied before being added to
    *        a recipe.
    *
    * \param var The Hop/Fermentable/etc that we want to add to a Recipe
    *
    * \return A copy of var if it needs to be copied (either because it has no parent or because it is already used in
    *         another recipe), or var itself otherwise
    */
   template<class NE> std::shared_ptr<NE> copyIfNeeded(NE * var) {
      //
      // If the supplied Hop/Fermentable/etc has no parent then we need to make a copy of it, because it's the master
      // instance of that Hop/Fermentable/etc.
      //
      // Otherwise, if it has a parent, then whether we need to make a copy depends on whether it is already used in a
      // recipe (_including_ this one, because the same ingredient can be added more than once to a recipe - eg Hops
      // added at different times).
      //
      NE * parentOfVar = static_cast<NE *>(var->getParent());
      if (parentOfVar != nullptr) {
         // Parameter has a parent.  See if it (the parameter, not its parent!) is used in a recipe.
         // (NB: The parent of the NamedEntity is not the same thing as its parent recipe.  We should perhaps find some
         // different terms!)
         auto matchingRecipe = ObjectStoreTyped<Recipe>::getInstance().findFirstMatching(
            [var](Recipe * recipe) {return recipe->uses(*var);}
         );
         if (matchingRecipe == nullptr) {
            // The parameter is not already used in a recipe, so we'll be able to add it without making a copy
            return std::shared_ptr<NE>{var};
         }
      }

      // We need to make a copy.  (We'll rely on the copy constructor to do the right thing about parentage.)
      return ObjectStoreTyped<NE>::getInstance().insertCopyOf(var->key());
   }

   //
   // After we modified a property via a templated member function of Recipe, we need to tell the object store to
   // update the database.  These template specialisations map from property type to property name.
   //
   template<class NE> char const * const propertyToPropertyName();
   template<> char const * const propertyToPropertyName<Equipment>()   { return PropertyNames::Recipe::equipmentId; }
   template<> char const * const propertyToPropertyName<Fermentable>() { return PropertyNames::Recipe::fermentableIds; }
   template<> char const * const propertyToPropertyName<Hop>()         { return PropertyNames::Recipe::hopIds; }
   template<> char const * const propertyToPropertyName<Instruction>() { return PropertyNames::Recipe::instructionIds; }
   template<> char const * const propertyToPropertyName<Mash>()        { return PropertyNames::Recipe::mashId; }
   template<> char const * const propertyToPropertyName<Misc>()        { return PropertyNames::Recipe::miscIds; }
   template<> char const * const propertyToPropertyName<Salt>()        { return PropertyNames::Recipe::saltIds; }
   template<> char const * const propertyToPropertyName<Style>()       { return PropertyNames::Recipe::styleId; }
   template<> char const * const propertyToPropertyName<Water>()       { return PropertyNames::Recipe::waterIds; }
   template<> char const * const propertyToPropertyName<Yeast>()       { return PropertyNames::Recipe::yeastIds; }
   template<class NE> void updatePropertyInDb(Recipe const & recipe) {
      ObjectStoreWrapper::updateProperty(recipe, propertyToPropertyName<NE>());
      return;
   }
}


// This private implementation class holds all private non-virtual members of Recipe
class Recipe::impl {
public:

   /**
    * Constructor
    */
   impl(Recipe & recipe) :
      recipe{recipe},
      fermentableIds{},
      hopIds{},
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
    * \brief Make copies of the ingredients of a particular type (Hop, Fermentable, etc) from one Recipe and add them
    *        to another - typically because we are copying the Recipe.
    */
   template<class NE> void copyList(Recipe & us, Recipe const & other) {
      for (int otherIngId : other.pimpl->accessIds<NE>()) {
         // Make and store a copy of the current Hop/Fermentable/etc object we're looking at in the other Recipe
         auto ingToAdd = ObjectStoreTyped<NE>::getInstance().insertCopyOf(otherIngId);
         // Store the ID of the copy in our recipe
         this->accessIds<NE>().append(ingToAdd->key());
         // Connect signals so that we are notified when there are changes to the Hop/Fermentable/etc we just added to
         // our recipe.
         connect(ingToAdd.get(), SIGNAL(changed(QMetaProperty,QVariant)), &us, SLOT(acceptChangeToContainedObject(QMetaProperty,QVariant)));
      }
      return;
   }

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
    * \brief Get raw pointers to all ingredients etc of a particular type (Hop, Fermentable, etc) in this Recipe
    */
   template<class NE> QList<NE *> getAllMyRaw() {
      return ObjectStoreTyped<NE>::getInstance().getByIdsRaw(this->accessIds<NE>());
   }

   /**
    * \brief Create and add a new Hop/Fermentable/Instruction etc, first to the relevant Object Store and then to this
    *        Recipe
    */
   template<class NE> void addNew(std::shared_ptr<NE> ne) {
      ObjectStoreWrapper::insert(ne);
      this->accessIds<NE>().append(ne->key());
      updatePropertyInDb<Instruction>(recipe);
      return;
   }

   // Member variables
   Recipe & recipe;
   QVector<int> fermentableIds;
   QVector<int> hopIds;
   QVector<int> instructionIds;
   QVector<int> miscIds;
   QVector<int> saltIds;
   QVector<int> waterIds;
   QVector<int> yeastIds;

};

template<> QVector<int> & Recipe::impl::accessIds<Fermentable>() { return this->fermentableIds; }
template<> QVector<int> & Recipe::impl::accessIds<Hop>()         { return this->hopIds; }
template<> QVector<int> & Recipe::impl::accessIds<Instruction>() { return this->instructionIds; }
template<> QVector<int> & Recipe::impl::accessIds<Misc>()        { return this->miscIds; }
template<> QVector<int> & Recipe::impl::accessIds<Salt>()        { return this->saltIds; }
template<> QVector<int> & Recipe::impl::accessIds<Water>()       { return this->waterIds; }
template<> QVector<int> & Recipe::impl::accessIds<Yeast>()       { return this->yeastIds; }

static const QHash<QString, Recipe::Type> RECIPE_TYPE_STRING_TO_TYPE {
   {"Extract",      Recipe::Extract},
   {"Partial Mash", Recipe::PartialMash},
   {"All Grain",    Recipe::AllGrain}
};


bool Recipe::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   Recipe const & rhs = static_cast<Recipe const &>(other);
   // Base class will already have ensured names are equal
   return (
      this->m_type              == rhs.m_type              &&
      this->m_batchSize_l       == rhs.m_batchSize_l       &&
      this->m_boilSize_l        == rhs.m_boilSize_l        &&
      this->m_boilTime_min      == rhs.m_boilTime_min      &&
      this->m_efficiency_pct    == rhs.m_efficiency_pct    &&
      this->m_primaryAge_days   == rhs.m_primaryAge_days   &&
      this->m_primaryTemp_c     == rhs.m_primaryTemp_c     &&
      this->m_secondaryAge_days == rhs.m_secondaryAge_days &&
      this->m_secondaryTemp_c   == rhs.m_secondaryTemp_c   &&
      this->m_tertiaryAge_days  == rhs.m_tertiaryAge_days  &&
      this->m_tertiaryTemp_c    == rhs.m_tertiaryTemp_c    &&
      this->m_age               == rhs.m_age               &&
      this->m_ageTemp_c         == rhs.m_ageTemp_c         &&
      this->styleId          == rhs.styleId          &&
      this->m_og                == rhs.m_og                &&
      this->m_fg                == rhs.m_fg
   );
}

ObjectStore & Recipe::getObjectStoreTypedInstance() const {
   return ObjectStoreTyped<Recipe>::getInstance();
}

QString Recipe::classNameStr()
{
   static const QString name("Recipe");
   return name;
}

Recipe::Recipe(QString name, bool cache) :
   NamedEntity(-1, name, true),
   pimpl{new impl{*this}},
   m_type(QString("All Grain")),
   m_brewer(QString("")),
   m_asstBrewer(QString("Brewken: free beer software")),
   m_batchSize_l(0.0),
   m_boilSize_l(0.0),
   m_boilTime_min(0.0),
   m_efficiency_pct(0.0),
   m_fermentationStages(1),
   m_primaryAge_days(0.0),
   m_primaryTemp_c(0.0),
   m_secondaryAge_days(0.0),
   m_secondaryTemp_c(0.0),
   m_tertiaryAge_days(0.0),
   m_tertiaryTemp_c(0.0),
   m_age(0.0),
   m_ageTemp_c(0.0),
   m_date(QDate::currentDate()),
   m_carbonation_vols(0.0),
   m_forcedCarbonation(false),
   m_primingSugarName(QString("")),
   m_carbonationTemp_c(0.0),
   m_primingSugarEquiv(0.0),
   m_kegPrimingFactor(0.0),
   m_notes(QString("")),
   m_tasteNotes(QString("")),
   m_tasteRating(0.0),
   styleId(0),
   equipmentId(-1),
   m_og(1.0),
   m_fg(1.0),
   m_cacheOnly(cache)
{
   return;
}

Recipe::Recipe(NamedParameterBundle const & namedParameterBundle) :
   NamedEntity{namedParameterBundle},
   pimpl{new impl{*this}},
   m_type              {
      // .:TODO:. Change so we store enum not string!
      RECIPE_TYPE_STRING_TO_TYPE.key(static_cast<Recipe::Type>(namedParameterBundle(PropertyNames::Recipe::recipeType).toInt()))
   },
   m_brewer            {namedParameterBundle(PropertyNames::Recipe::brewer            ).toString()},
   m_asstBrewer        {namedParameterBundle(PropertyNames::Recipe::asstBrewer        ).toString()},
   m_batchSize_l       {namedParameterBundle(PropertyNames::Recipe::batchSize_l       ).toDouble()},
   m_boilSize_l        {namedParameterBundle(PropertyNames::Recipe::boilSize_l        ).toDouble()},
   m_boilTime_min      {namedParameterBundle(PropertyNames::Recipe::boilTime_min      ).toDouble()},
   m_efficiency_pct    {namedParameterBundle(PropertyNames::Recipe::efficiency_pct    ).toDouble()},
   m_fermentationStages{namedParameterBundle(PropertyNames::Recipe::fermentationStages).toInt()},
   m_primaryAge_days   {namedParameterBundle(PropertyNames::Recipe::primaryAge_days   ).toDouble()},
   m_primaryTemp_c     {namedParameterBundle(PropertyNames::Recipe::primaryTemp_c     ).toDouble()},
   m_secondaryAge_days {namedParameterBundle(PropertyNames::Recipe::secondaryAge_days ).toDouble()},
   m_secondaryTemp_c   {namedParameterBundle(PropertyNames::Recipe::secondaryTemp_c   ).toDouble()},
   m_tertiaryAge_days  {namedParameterBundle(PropertyNames::Recipe::tertiaryAge_days  ).toDouble()},
   m_tertiaryTemp_c    {namedParameterBundle(PropertyNames::Recipe::tertiaryTemp_c    ).toDouble()},
   m_age               {namedParameterBundle(PropertyNames::Recipe::age               ).toDouble()},
   m_ageTemp_c         {namedParameterBundle(PropertyNames::Recipe::ageTemp_c         ).toDouble()},
   m_date              {namedParameterBundle(PropertyNames::Recipe::date              ).toDate()},
   m_carbonation_vols  {namedParameterBundle(PropertyNames::Recipe::carbonation_vols  ).toDouble()},
   m_forcedCarbonation {namedParameterBundle(PropertyNames::Recipe::forcedCarbonation ).toBool()},
   m_primingSugarName  {namedParameterBundle(PropertyNames::Recipe::primingSugarName  ).toString()},
   m_carbonationTemp_c {namedParameterBundle(PropertyNames::Recipe::carbonationTemp_c ).toDouble()},
   m_primingSugarEquiv {namedParameterBundle(PropertyNames::Recipe::primingSugarEquiv ).toDouble()},
   m_kegPrimingFactor  {namedParameterBundle(PropertyNames::Recipe::kegPrimingFactor  ).toDouble()},
   m_notes             {namedParameterBundle(PropertyNames::Recipe::notes             ).toString()},
   m_tasteNotes        {namedParameterBundle(PropertyNames::Recipe::tasteNotes        ).toString()},
   m_tasteRating       {namedParameterBundle(PropertyNames::Recipe::tasteRating       ).toDouble()},
   styleId          {namedParameterBundle(PropertyNames::Recipe::styleId           ).toInt()},
   mashId              {namedParameterBundle(PropertyNames::Recipe::mashId            ).toInt()},
   equipmentId         {namedParameterBundle(PropertyNames::Recipe::equipmentId       ).toInt()},
   m_og                {namedParameterBundle(PropertyNames::Recipe::og                ).toDouble()},
   m_fg                {namedParameterBundle(PropertyNames::Recipe::fg                ).toDouble()},
   m_cacheOnly         {false} {
   // At this stage, we haven't set any Hops, Fermentables, etc.  This is deliberate because the caller typically needs
   // to access subsidiary records to obtain this info.   Callers will usually use setters (setHopIds, etc but via
   // setProperty) to finish constructing the object.
   return;
}


Recipe::Recipe( Recipe const& other ) :
   NamedEntity(other),
   pimpl{new impl{*this}},
   m_type              (other.m_type),
   m_brewer            (other.m_brewer),
   m_asstBrewer        (other.m_asstBrewer),
   m_batchSize_l       (other.m_batchSize_l),
   m_boilSize_l        (other.m_boilSize_l),
   m_boilTime_min      (other.m_boilTime_min),
   m_efficiency_pct    (other.m_efficiency_pct),
   m_fermentationStages(other.m_fermentationStages),
   m_primaryAge_days   (other.m_primaryAge_days),
   m_primaryTemp_c     (other.m_primaryTemp_c),
   m_secondaryAge_days (other.m_secondaryAge_days),
   m_secondaryTemp_c   (other.m_secondaryTemp_c),
   m_tertiaryAge_days  (other.m_tertiaryAge_days),
   m_tertiaryTemp_c    (other.m_tertiaryTemp_c),
   m_age               (other.m_age),
   m_ageTemp_c         (other.m_ageTemp_c),
   m_date              (other.m_date),
   m_carbonation_vols  (other.m_carbonation_vols),
   m_forcedCarbonation (other.m_forcedCarbonation),
   m_primingSugarName  (other.m_primingSugarName),
   m_carbonationTemp_c (other.m_carbonationTemp_c),
   m_primingSugarEquiv (other.m_primingSugarEquiv),
   m_kegPrimingFactor  (other.m_kegPrimingFactor),
   m_notes             (other.m_notes),
   m_tasteNotes        (other.m_tasteNotes),
   m_tasteRating       (other.m_tasteRating),
//   styleId          (other.styleId),  Done in body
//   equipmentId         (other.equipmentId),  Done in body
   m_og                (other.m_og),
   m_fg                (other.m_fg),
   m_cacheOnly         (other.m_cacheOnly)
{
   setObjectName("Recipe"); // .:TBD:. Would be good to understand why we need this

   //
   // TODO: Template this!
   //
   // When we make a copy of a Recipe, it needs to be a deep(ish) copy.  In particular, we need to make copies of the
   // Hops, Fermentables etc as some attributes of the recipe (eg how much and when to add) are stored inside these
   // ingredients.
   //
   this->pimpl->copyList<Fermentable>(*this, other);
   this->pimpl->copyList<Hop>        (*this, other);
   this->pimpl->copyList<Instruction>(*this, other);
   this->pimpl->copyList<Misc>       (*this, other);
   this->pimpl->copyList<Salt>       (*this, other);
   this->pimpl->copyList<Water>      (*this, other);
   this->pimpl->copyList<Yeast>      (*this, other);

   // .:TBD:. What about BrewNotes?  We don't currently store their IDs in Recipe

   //
   // .:TBD:. What about Style, Mash, Equipment?
   //
   // Style surely can be shared, hence copy of its ID above
   // However, AFAICT, none of Style, Mash or Equipment are not shared between Recipes because users expect to be able
   // to edit them in one Recipe without changing the settings for any other Recipe.
   //
   auto equipment = ObjectStoreTyped<Equipment>::getInstance().insertCopyOf(other.equipmentId);
   this->equipmentId = equipment->key();
   connect(equipment.get(), SIGNAL(changed(QMetaProperty,QVariant)), this, SLOT(acceptChangeToContainedObject(QMetaProperty,QVariant)));

   auto mash = ObjectStoreTyped<Mash>::getInstance().insertCopyOf(other.mashId);
   this->mashId = mash->key();
   connect(mash.get(), SIGNAL(changed(QMetaProperty,QVariant)), this, SLOT(acceptChangeToContainedObject(QMetaProperty,QVariant)));

   auto style = ObjectStoreTyped<Style>::getInstance().insertCopyOf(other.mashId);
   this->styleId = style->key();
   connect(style.get(), SIGNAL(changed(QMetaProperty,QVariant)), this, SLOT(acceptChangeToContainedObject(QMetaProperty,QVariant)));

   this->recalcAll();

   // .:TODO:. What about these signals?
//   emit changed( metaProperty(*this, "recipes"), QVariant() );
//   emit newRecipeSignal(tmp);

   return;
}


// See https://herbsutter.com/gotw/_100/ for why we need to explicitly define the destructor here (and not in the
// header file)
Recipe::~Recipe() = default;


void Recipe::connectSignals() {
   // Connect fermentable, hop changed signals to their parent recipe.
   for (auto recipe : ObjectStoreTyped<Recipe>::getInstance().getAllRaw()) {
      qDebug() << Q_FUNC_INFO << "Connecting signals for Recipe #" << recipe->key();
      Equipment * equipment = recipe->equipment();
      if (equipment != nullptr) {
         connect(equipment, &NamedEntity::changed,           recipe, &Recipe::acceptEquipChange );
         connect(equipment, &Equipment::changedBoilSize_l,   recipe, &Recipe::setBoilSize_l);
         connect(equipment, &Equipment::changedBoilTime_min, recipe, &Recipe::setBoilTime_min);
      }

      QList<Fermentable *> fermentables = recipe->fermentables();
      for (auto fermentable : fermentables) {
         connect(fermentable, SIGNAL(changed(QMetaProperty,QVariant)), recipe, SLOT(acceptFermChange(QMetaProperty,QVariant)) );
      }

      QList<Hop *> hops = recipe->hops();
      for (auto hop : hops) {
         connect(hop, SIGNAL(changed(QMetaProperty,QVariant)), recipe, SLOT(acceptChangeToContainedObject(QMetaProperty,QVariant)) );
      }

      QList<Yeast *> yeasts = recipe->yeasts();
      for (auto yeast : yeasts) {
         connect(yeast, SIGNAL(changed(QMetaProperty,QVariant)), recipe, SLOT(acceptYeastChange(QMetaProperty,QVariant)) );
      }

      Mash * mash = recipe->mash();
      if (mash != nullptr) {
         connect(mash, SIGNAL(changed(QMetaProperty,QVariant)), recipe, SLOT(acceptMashChange(QMetaProperty,QVariant)) );
      }
   }

   return;
}

void Recipe::mashFermentableIns() {
   /*** Add grains ***/
   auto ins = std::make_shared<Instruction>();
   ins->setName(tr("Add grains"));
   QString str = tr("Add ");
   QList<QString> reagents = this->getReagents(this->fermentables());

   for (int ii = 0; ii < reagents.size(); ++ii ) {
      str += reagents.at(ii);
   }

   str += tr("to the mash tun.");
   ins->setDirections(str);

   this->pimpl->addNew(ins);

   return;
}

void Recipe::saltWater(Salt::WhenToAdd when) {

   if ( this->mash() == nullptr || this->salts().size() == 0 ) {
      return;
   }

   QStringList reagents = this->getReagents(salts(), when);
   if ( reagents.size() == 0 ) {
      return;
   }

   auto ins = std::make_shared<Instruction>();
   QString tmp = when == Salt::MASH ? tr("mash") : tr("sparge");
   ins->setName(tr("Modify %1 water").arg( tmp ));
   QString str = tr("Dissolve ");

   for (int ii = 0; ii < reagents.size(); ++ii ) {
      str += reagents.at(ii);
   }

   str += QString(tr(" into the %1 water").arg(tmp));
   ins->setDirections(str);

   this->pimpl->addNew(ins);

   return;
}

void Recipe::mashWaterIns() {

   if ( this->mash() == nullptr ) {
      return;
   }

   auto ins = std::make_shared<Instruction>();
   ins->setName(tr("Heat water"));
   QString str = tr("Bring ");
   QList<QString> reagents = getReagents(mash()->mashSteps());

   for (int ii = 0; ii < reagents.size(); ++ii ) {
      str += reagents.at(ii);
   }

   str += tr("for upcoming infusions.");
   ins->setDirections(str);

   this->pimpl->addNew(ins);

   return;
}

QVector<PreInstruction> Recipe::mashInstructions(double timeRemaining, double totalWaterAdded_l, unsigned int size)
{
   QVector<PreInstruction> preins;
   MashStep* mstep;
   QString str;
   unsigned int i;

   if( mash() == nullptr )
      return preins;

   QList<MashStep*> msteps = mash()->mashSteps();
   for( i = 0; i < size; ++i )
   {
      mstep = msteps[static_cast<int>(i)];

      if( mstep->isInfusion() )
      {
         str = tr("Add %1 water at %2 to mash to bring it to %3.")
               .arg(Brewken::displayAmount(mstep->infuseAmount_l(), kMashStepSection, PropertyNames::MashStep::infuseAmount_l, &Units::liters))
               .arg(Brewken::displayAmount(mstep->infuseTemp_c(), kMashStepSection, PropertyNames::MashStep::infuseTemp_c, &Units::celsius))
               .arg(Brewken::displayAmount(mstep->stepTemp_c(), kMashStepSection, PropertyNames::MashStep::stepTemp_c, &Units::celsius));
         totalWaterAdded_l += mstep->infuseAmount_l();
      }
      else if( mstep->isTemperature() )
      {
         str = tr("Heat mash to %1.").arg(Brewken::displayAmount(mstep->stepTemp_c(), kMashStepSection, PropertyNames::MashStep::stepTemp_c, &Units::celsius));
      }
      else if( mstep->isDecoction() )
      {
         str = tr("Bring %1 of the mash to a boil and return to the mash tun to bring it to %2.")
               .arg(Brewken::displayAmount(mstep->decoctionAmount_l(), kMashStepSection, PropertyNames::MashStep::decoctionAmount_l, &Units::liters))
               .arg(Brewken::displayAmount(mstep->stepTemp_c(), kMashStepSection, PropertyNames::MashStep::stepTemp_c, &Units::celsius));
      }

      str += tr(" Hold for %1.").arg(Brewken::displayAmount(mstep->stepTime_min(), kMashStepSection, PropertyNames::MashStep::stepTime_min, &Units::minutes));

      preins.push_back(PreInstruction(str, QString("%1 - %2").arg(mstep->typeStringTr()).arg(mstep->name()),
                  timeRemaining));
      timeRemaining -= mstep->stepTime_min();
   }
   return preins;
}

QVector<PreInstruction> Recipe::hopSteps(Hop::Use type)
{
   QVector<PreInstruction> preins;
   QString str;
   unsigned int i;
   int size;

   preins.clear();
   QList<Hop*> hlist = hops();
   size = hlist.size();
   for( i = 0; static_cast<int>(i) < size; ++i )
   {
      Hop* hop = hlist[static_cast<int>(i)];
      if( hop->use() == type )
      {
         if( type == Hop::Boil )
            str = tr("Put %1 %2 into boil for %3.");
         else if( type == Hop::Dry_Hop )
            str = tr("Put %1 %2 into fermenter for %3.");
         else if( type == Hop::First_Wort )
            str = tr("Put %1 %2 into first wort for %3.");
         else if( type == Hop::Mash )
            str = tr("Put %1 %2 into mash for %3.");
         else if( type == Hop::UseAroma )
            str = tr("Steep %1 %2 in wort for %3.");
         else
         {
            qWarning() << "Recipe::hopSteps(): Unrecognized hop use.";
            str = tr("Use %1 %2 for %3");
         }

         str = str.arg(Brewken::displayAmount(hop->amount_kg(), kHopTableSection, PropertyNames::Hop::amount_kg, &Units::kilograms))
                  .arg(hop->name())
                  .arg(Brewken::displayAmount(hop->time_min(),kHopTableSection, PropertyNames::Misc::time,  &Units::minutes));

         preins.push_back(PreInstruction(str, tr("Hop addition"), hop->time_min()));
      }
   }
   return preins;
}

QVector<PreInstruction> Recipe::miscSteps(Misc::Use type)
{
   QVector<PreInstruction> preins;
   QString str;
   Unit const * kindOf;
   unsigned int i;
   int size;

   QList<Misc*> mlist = miscs();
   size = mlist.size();
   for( i = 0; static_cast<int>(i) < size; ++i )
   {
      Misc* misc = mlist[static_cast<int>(i)];
      if( misc->use() == type )
      {
         if( type == Misc::Boil )
            str = tr("Put %1 %2 into boil for %3.");
         else if( type == Misc::Bottling )
            str = tr("Use %1 %2 at bottling for %3.");
         else if( type == Misc::Mash )
            str = tr("Put %1 %2 into mash for %3.");
         else if( type == Misc::Primary )
            str = tr("Put %1 %2 into primary for %3.");
         else if( type == Misc::Secondary )
            str = tr("Put %1 %2 into secondary for %3.");
         else
         {
            qWarning() << "Recipe::getMiscSteps(): Unrecognized misc use.";
            str = tr("Use %1 %2 for %3.");
         }

         kindOf = misc->amountIsWeight() ? &Units::kilograms : &Units::liters;
         str = str .arg(Brewken::displayAmount(misc->amount(), kMiscTableSection, PropertyNames::Misc::amount, kindOf))
                   .arg(misc->name())
                   .arg(Brewken::displayAmount(misc->time(), kMiscTableSection, PropertyNames::Misc::time, &Units::minutes));

         preins.push_back(PreInstruction(str, tr("Misc addition"), misc->time()));
      }
   }
   return preins;
}

void Recipe::firstWortHopsIns() {
   QList<QString> reagents = getReagents(hops(), true);
   if ( reagents.size() == 0 ) {
      return;
   }

   QString str = tr("Do first wort hopping with ");

   for ( int ii = 0; ii < reagents.size(); ++ii ) {
      str += reagents.at(ii);
   }
   str += ".";

   auto ins = std::make_shared<Instruction>();
   ins->setName(tr("First wort hopping"));
   ins->setDirections(str);

   this->pimpl->addNew(ins);

   return;
}

void Recipe::topOffIns() {
   Equipment* e = this->equipment();
   if ( e == nullptr ) {
      return;
   }

   double wortInBoil_l = wortFromMash_l() - e->lauterDeadspace_l();
   QString str = tr("You should now have %1 wort.")
      .arg(Brewken::displayAmount( wortInBoil_l, kTabRecipeSection, PropertyNames::Recipe::boilSize_l, &Units::liters));
   if ( e->topUpKettle_l() != 0.0 ) {
      return;
   }

   wortInBoil_l += e->topUpKettle_l();
   QString tmp = tr(" Add %1 water to the kettle, bringing pre-boil volume to %2.")
      .arg(Brewken::displayAmount(e->topUpKettle_l(), kTabRecipeSection, PropertyNames::Recipe::boilSize_l,  &Units::liters))
      .arg(Brewken::displayAmount(wortInBoil_l, kTabRecipeSection, PropertyNames::Recipe::boilSize_l,  &Units::liters));

   str += tmp;

   auto ins = std::make_shared<Instruction>();
   ins->setName(tr("Pre-boil"));
   ins->setDirections(str);
   ins->addReagent(tmp);

   this->pimpl->addNew(ins);

   return;
}

bool Recipe::hasBoilFermentable()
{
   int i;
   for ( i = 0; static_cast<int>(i) < fermentables().size(); ++i )
   {
      Fermentable* ferm = fermentables()[i];
      if( ferm->isMashed() || ferm->addAfterBoil() )
         continue;
      else
         return true;
   }
   return false;
}

bool Recipe::hasBoilExtract()
{
   int i;
   for ( i = 0; static_cast<int>(i) < fermentables().size(); ++i )
   {
      Fermentable* ferm = fermentables()[i];
      if( ferm->isExtract() )
         return true;
      else
         continue;
   }
   return false;
}

PreInstruction Recipe::boilFermentablesPre(double timeRemaining)
{
   QString str;
   int i;
   int size;

   str = tr("Boil or steep ");
   QList<Fermentable*> flist = fermentables();
   size = flist.size();
   for( i = 0; static_cast<int>(i) < size; ++i )
   {
     Fermentable* ferm = flist[i];
     if( ferm->isMashed() || ferm->addAfterBoil() || ferm->isExtract() )
       continue;

     str += QString("%1 %2, ")
          .arg(Brewken::displayAmount(ferm->amount_kg(), kFermentableTableSection, PropertyNames::Fermentable::amount_kg, &Units::kilograms))
          .arg(ferm->name());
   }
   str += ".";

   return PreInstruction(str, tr("Boil/steep fermentables"), timeRemaining);
}

bool Recipe::isFermentableSugar(Fermentable *fermy)
{
  if (fermy->type() == Fermentable::Sugar && fermy->name() == "Milk Sugar (Lactose)" )
    return false;
  else
    return true;
}

PreInstruction Recipe::addExtracts(double timeRemaining) const
{
   QString str;
   int i;
   int size;

   str = tr("Raise water to boil and then remove from heat. Stir in  ");
   const QList<Fermentable*> flist = fermentables();
   size = flist.size();
   for( i = 0; static_cast<int>(i) < size; ++i )
   {
      const Fermentable* ferm = flist[i];
      if ( ferm->isExtract() )
      {
         str += QString("%1 %2, ")
            .arg(Brewken::displayAmount(ferm->amount_kg(), kFermentableTableSection, PropertyNames::Fermentable::amount_kg, &Units::kilograms))
            .arg(ferm->name());
      }
   }
   str += ".";

   return PreInstruction(str, tr("Add Extracts to water"), timeRemaining);
}

void Recipe::postboilFermentablesIns() {
   QString tmp;
   bool hasFerms = false;

   QString str = tr("Add ");
   QList<Fermentable*> flist = this->fermentables();
   int size = flist.size();
   for (int ii = 0; ii < size; ++ii ) {
      Fermentable* ferm = flist[ii];
      if (!ferm->addAfterBoil()) {
         continue;
      }

      hasFerms = true;
      tmp = QString("%1 %2, ")
             .arg(Brewken::displayAmount(ferm->amount_kg(), kFermentableTableSection, PropertyNames::Fermentable::amount_kg, &Units::kilograms))
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

   this->pimpl->addNew(ins);

   return;
}

void Recipe::postboilIns() {
   Equipment* e = equipment();
   if ( e == nullptr ) {
      return;
   }

   double wortInBoil_l = wortFromMash_l() - e->lauterDeadspace_l();
   if ( e->topUpKettle_l() != 0.0 ) {
      wortInBoil_l += e->topUpKettle_l();
   }

   double wort_l = e->wortEndOfBoil_l(wortInBoil_l);
   QString str = tr("You should have %1 wort post-boil.")
         .arg(Brewken::displayAmount( wort_l, kTabRecipeSection, PropertyNames::Recipe::batchSize_l, &Units::liters));
   str += tr("\nYou anticipate losing %1 to trub and chiller loss.")
         .arg(Brewken::displayAmount( e->trubChillerLoss_l(), kTabRecipeSection, PropertyNames::Recipe::batchSize_l,  &Units::liters));
   wort_l -= e->trubChillerLoss_l();
   if( e->topUpWater_l() > 0.0 )
         str += tr("\nAdd %1 top up water into primary.")
            .arg(Brewken::displayAmount( e->topUpWater_l(), kTabRecipeSection, PropertyNames::Recipe::batchSize_l,  &Units::liters));
   wort_l += e->topUpWater_l();
   str += tr("\nThe final volume in the primary is %1.")
            .arg(Brewken::displayAmount(wort_l, kTabRecipeSection, PropertyNames::Recipe::batchSize_l,  &Units::liters));

   auto ins = std::make_shared<Instruction>();
   ins->setName(tr("Post boil"));
   ins->setDirections(str);
   this->pimpl->addNew(ins);

   return;
}

void Recipe::addPreinstructions( QVector<PreInstruction> preins ) {
    // Add instructions in descending mash time order.
    std::sort( preins.begin(), preins.end(), std::greater<PreInstruction>() );
    for (int ii = 0; ii < preins.size(); ++ii) {
       PreInstruction pi = preins[ii];

       auto ins = std::make_shared<Instruction>();
       ins->setName(pi.getTitle());
       ins->setDirections(pi.getText());
       ins->setInterval(pi.getTime());

       this->pimpl->addNew(ins);
    }
    return;
}

void Recipe::generateInstructions() {
   double timeRemaining;
   double totalWaterAdded_l = 0.0;

   if ( !this->instructions().empty() ) {
      this->clearInstructions();
   }

   QVector<PreInstruction> preinstructions;

   // Mash instructions

   int size = (mash() == nullptr) ? 0 : mash()->mashSteps().size();
   if ( size > 0 ) {
     /*** prepare mashed fermentables ***/
     this->mashFermentableIns();

     /*** salt the water ***/
     saltWater(Salt::MASH);
     saltWater(Salt::SPARGE);

     /*** Prepare water additions ***/
     this->mashWaterIns();

     timeRemaining = mash()->totalTime();

     /*** Generate the mash instructions ***/
     preinstructions = mashInstructions(timeRemaining, totalWaterAdded_l, size);

      /*** Hops mash additions ***/
     preinstructions += hopSteps(Hop::Mash);

      /*** Misc mash additions ***/
     preinstructions += miscSteps(Misc::Mash);

     /*** Add the preinstructions into the instructions ***/
     addPreinstructions(preinstructions);

   } // END mash instructions.

   // First wort hopping
   this->firstWortHopsIns();

   // Need to top up the kettle before boil?
   topOffIns();

   // Boil instructions
   preinstructions.clear();

   // Find boil time.
   if ( equipment() != nullptr ) {
      timeRemaining = equipment()->boilTime_min();
   } else {
      timeRemaining = Brewken::qStringToSI(QInputDialog::getText(nullptr,
                                        tr("Boil time"),
                                        tr("You did not configure an equipment (which you really should), so tell me the boil time.")),
                                        &Units::minutes);
   }

   QString str = tr("Bring the wort to a boil and hold for %1.").arg(Brewken::displayAmount( timeRemaining, "tab_recipe", "boilTime_min", &Units::minutes));

   auto startBoilIns = std::make_shared<Instruction>();
   startBoilIns->setName(tr("Start boil"));
   startBoilIns->setInterval(timeRemaining);
   startBoilIns->setDirections(str);
   this->pimpl->addNew(startBoilIns);

   /*** Get fermentables unless we haven't added yet ***/
   if ( hasBoilFermentable() ) {
      preinstructions.push_back(boilFermentablesPre(timeRemaining));
   }

   // add the intructions for including Extracts to wort
   if ( hasBoilExtract() ) {
      preinstructions.push_back(addExtracts(timeRemaining-1));
   }

   /*** Boiled hops ***/
   preinstructions += hopSteps(Hop::Boil);

   /*** Boiled miscs ***/
   preinstructions += miscSteps(Misc::Boil);

   // END boil instructions.

   // Add instructions in descending mash time order.
   addPreinstructions(preinstructions);

   // FLAMEOUT
   auto flameoutIns = std::make_shared<Instruction>();
   flameoutIns->setName(tr("Flameout"));
   flameoutIns->setDirections(tr("Stop boiling the wort."));
   this->pimpl->addNew(flameoutIns);

   // Steeped aroma hops
   preinstructions.clear();
   preinstructions += hopSteps(Hop::UseAroma);
   addPreinstructions(preinstructions);

   // Fermentation instructions
   preinstructions.clear();

   /*** Fermentables added after boil ***/
   postboilFermentablesIns();

   /*** post boil ***/
   postboilIns();

   /*** Primary yeast ***/
   str = tr("Cool wort and pitch ");
   QList<Yeast*> ylist = yeasts();
   for (int ii = 0; ii < ylist.size(); ++ii) {
      Yeast* yeast = ylist[ii];
      if ( ! yeast->addToSecondary() ) {
         str += tr("%1 %2 yeast, ").arg(yeast->name()).arg(yeast->typeStringTr());
      }
   }
   str += tr("to the primary.");

   auto pitchIns = std::make_shared<Instruction>();
   pitchIns->setName(tr("Pitch yeast"));
   pitchIns->setDirections(str);
   this->pimpl->addNew(pitchIns);
   /*** End primary yeast ***/

   /*** Primary misc ***/
   addPreinstructions(miscSteps(Misc::Primary));

   str = tr("Let ferment until FG is %1.")
         .arg(Brewken::displayAmount(fg(), "tab_recipe", "fg", &Units::sp_grav, 3));

   auto fermentIns = std::make_shared<Instruction>();
   fermentIns->setName(tr("Ferment"));
   fermentIns->setDirections(str);
   this->pimpl->addNew(fermentIns);

   str = tr("Transfer beer to secondary.");
   auto transferIns = std::make_shared<Instruction>();
   transferIns->setName(tr("Transfer to secondary"));
   transferIns->setDirections(str);
   this->pimpl->addNew(transferIns);

   /*** Secondary misc ***/
   addPreinstructions(miscSteps(Misc::Secondary));

   /*** Dry hopping ***/
   addPreinstructions(hopSteps(Hop::Dry_Hop));

   // END fermentation instructions. Let everybody know that now is the time
   // to update instructions
   emit changed( metaProperty("instructions"), instructions().size() );

   return;
}

QString Recipe::nextAddToBoil(double& time)
{
   int i, size;
   double max = 0;
   bool foundSomething = false;
   Hop* h;
   QList<Hop*> hhops = hops();
   Misc* m;
   QList<Misc*> mmiscs = miscs();
   QString ret;

   // Search hops
   size = hhops.size();
   for( i = 0; i < size; ++i )
   {
      h = hhops[i];
      if( h->use() != Hop::Boil )
         continue;
      if( h->time_min() < time && h->time_min() > max )
      {
         ret = tr("Add %1 %2 to boil at %3.")
               .arg(Brewken::displayAmount(h->amount_kg(), kHopTableSection, PropertyNames::Hop::amount_kg, &Units::kilograms))
               .arg(h->name())
               .arg(Brewken::displayAmount(h->time_min(), kHopTableSection, PropertyNames::Misc::time,  &Units::minutes));

         max = h->time_min();
         foundSomething = true;
      }
   }

   // Search miscs
   size = mmiscs.size();
   for( i = 0; i < size; ++i )
   {
      m = mmiscs[i];
      if( m->use() != Misc::Boil )
         continue;
      if( m->time() < time && m->time() > max )
      {
         ret = tr("Add %1 %2 to boil at %3.");
         if( m->amountIsWeight() )
            ret = ret.arg(Brewken::displayAmount(m->amount(), kMiscTableSection, PropertyNames::Misc::amount, &Units::kilograms));
         else
            ret = ret.arg(Brewken::displayAmount(m->amount(), kMiscTableSection, PropertyNames::Misc::amount,  &Units::liters));

         ret = ret.arg(m->name());
         ret = ret.arg(Brewken::displayAmount(m->time(), kMiscTableSection, PropertyNames::Misc::time, &Units::minutes));
         max = m->time();
         foundSomething = true;
      }
   }

   time = foundSomething ? max : -1.0;
   return ret;
}

//============================Relational Setters===============================
// See comment in header file for why we can't put this template definition there (essentially because we can't call a
// member function of Database in the header without creating circular dependencies), and hence why we need the
// subsequent lines as a "trick" to ensure all the right versions of the template are instantiated in an externally-
// visible way.
/*template<class T> T * Recipe::addNamedEntity(T * var) {
   // If the supplied parameter has no parent then we need to make a copy of it - or rather tell the Database object
   // to make a copy.  We'll then get back a pointer to the copy.  If it does have a parent then we need to check
   // whether it's already in used in another recipe.  If not, we can just add it directly, and we'll get back the
   // same pointer we passed in.  Otherwise we get its parent and make another copy of that.
   T * parentOfVar = static_cast<T *>(var->getParent());
   if (parentOfVar != nullptr) {
      // Parameter has a parent.  See if it (the parameter, not its parent!) is used in a recipe.
      // (NB: The parent of the NamedEntity is not the same thing as its parent recipe.  We should perhaps find some
      // different terms!)
      Recipe * usedIn = Database::instance().getParentRecipe(var);
      if (usedIn == nullptr) {
         // The parameter is not already used in a recipe, so we can add it without making a copy
         return Database::instance().addToRecipe(this, var, true);
      }

      // The parameter is already used in a recipe, so we need to add a copy of its parent
      return Database::instance().addToRecipe(this, parentOfVar, false);
   }

   // Parameter has no parent, so add a copy of it
   return Database::instance().addToRecipe(this, var, false);
}*/


template<class NE> NE * Recipe::add(NE * var) {
   std::shared_ptr<NE> neToAdd = copyIfNeeded(var);
   this->pimpl->accessIds<NE>().append(neToAdd->key());
   connect(var, SIGNAL(changed(QMetaProperty,QVariant)), this, SLOT(acceptChangeToContainedObject(QMetaProperty,QVariant)));

   updatePropertyInDb<NE>(*this);
   this->recalcIBU(); // .:TODO:. Don't need to do this recalculation when it's Instruction

   return neToAdd.get();
}

//
// Instantiate the above template function for the types that are going to use it
// (This is all just a trick to allow the template definition to be here in the .cpp file and not in the header, which
// means, amongst other things, that we can reference the pimpl.)
//
template Hop *         Recipe::add(Hop *         var);
template Fermentable * Recipe::add(Fermentable * var);
template Misc *        Recipe::add(Misc *        var);
template Yeast *       Recipe::add(Yeast *       var);
template Water *       Recipe::add(Water *       var);
template Salt *        Recipe::add(Salt *        var);
template Instruction * Recipe::add(Instruction * var);

template<class NE> bool Recipe::uses(NE const & var) const {
   int idToLookFor = var.key();
   auto match = std::find_if(this->pimpl->accessIds<NE>().cbegin(),
                             this->pimpl->accessIds<NE>().cend(),
                             [idToLookFor](int id){ return idToLookFor == id; });
   return match != this->pimpl->accessIds<NE>().cend();
}
template bool Recipe::uses(Fermentable  const & var) const;
template bool Recipe::uses(Hop          const & var) const;
template bool Recipe::uses(Instruction  const & var) const;
template bool Recipe::uses(Misc         const & var) const;
template bool Recipe::uses(Salt         const & var) const;
template bool Recipe::uses(Water        const & var) const;
template bool Recipe::uses(Yeast        const & var) const;
template<> bool Recipe::uses<Equipment> (Equipment  const & var) const { return var.key() == this->equipmentId; }
template<> bool Recipe::uses<Mash>      (Mash       const & var) const { return var.key() == this->mashId; }
template<> bool Recipe::uses<Style>     (Style      const & var) const { return var.key() == this->styleId; }

/*NamedEntity * Recipe::removeNamedEntity( NamedEntity *var )
{
//   qDebug() << QString("%1").arg(Q_FUNC_INFO);

   // brewnotes a bit odd
   if ( dynamic_cast<BrewNote*>(var) ) {
      // the cast is required to force the template to gets it thing right
      Database::instance().remove(qobject_cast<BrewNote*>(var));
      return var;
   } else {
      return Database::instance().removeNamedEntityFromRecipe( this, var );
   }
}*/

template<class NE> NE * Recipe::remove(NE * var) {
   int idToLookFor = var->key();
   if (!this->pimpl->accessIds<NE>().removeOne(idToLookFor)) {
      // This shouldn't happen, but it doesn't inherently break anything, so just log a warning and carry on
      qWarning() << Q_FUNC_INFO << "Tried to remove object with ID" << idToLookFor << "but couldn't find it";
   } else {
      updatePropertyInDb<NE>(*this);
      this->recalcIBU(); // .:TODO:. Don't need to do this recalculation when it's Instruction
   }
   return var;
}
template Hop *         Recipe::remove(Hop *         var);
template Fermentable * Recipe::remove(Fermentable * var);
template Misc *        Recipe::remove(Misc *        var);
template Yeast *       Recipe::remove(Yeast *       var);
template Water *       Recipe::remove(Water *       var);
template Salt *        Recipe::remove(Salt *        var);
template Instruction * Recipe::remove(Instruction * var);

int Recipe::instructionNumber(Instruction const & ins) const {
   return this->pimpl->instructionIds.indexOf(ins.key());
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
   updatePropertyInDb<Instruction>(*this);
   return;
}

void Recipe::insertInstruction(Instruction* ins, int pos) {
   if( ins == nullptr || !(instructions().contains(ins)) ) {
      return;
   }

   this->pimpl->instructionIds.insert(pos, ins->key());
   updatePropertyInDb<Instruction>(*this);
   return;
}

void Recipe::setStyle(Style * var) {
   if (var->key() == this->styleId) {
      return;
   }

   std::shared_ptr<Style> styleToAdd = copyIfNeeded(var);
   this->styleId = styleToAdd->key();
   updatePropertyInDb<Style>(*this);
   return;
}

void Recipe::setEquipment(Equipment * var) {
   if (var->key() == this->equipmentId) {
      return;
   }

   std::shared_ptr<Equipment> equipmentToAdd = copyIfNeeded(var);
   this->equipmentId = equipmentToAdd->key();
   updatePropertyInDb<Equipment>(*this);
   return;
}

void Recipe::setMash(Mash * var) {
   if (var->key() == this->mashId) {
      return;
   }

   // .:TBD:. Do we need to disconnect the old Mash?

   std::shared_ptr<Mash> mashToAdd = copyIfNeeded(var);
   this->mashId = mashToAdd->key();
   updatePropertyInDb<Mash>(*this);

   connect(mashToAdd.get(), SIGNAL(changed(QMetaProperty,QVariant)), this, SLOT(acceptMashChange(QMetaProperty,QVariant)));
   emit this->changed(this->metaProperty("mash"), NamedEntity::qVariantFromPtr(mashToAdd.get()));

   this->recalcAll();

   return;
}

/*

void Recipe::setStyleId(int id) {
   this->styleId = id;
}

void Recipe::setEquipmentId(int id) {
   this->equipmentId = id;
}

void Recipe::setMashId(int id) {
   auto & objectStore = ObjectStoreTyped<Mash>::getInstance();

   // It's a coding error to supply an ID to a non-existent object.  If it happens, log some diagnostics then bail.
   Q_ASSERT(objectStore.contains(id));

   this->setMash(objectStore.getByIdRaw(id));
   return;
}

void Recipe::setFermentableIds(QVector<int> fermentableIds) {
   this->pimpl->fermentableIds = fermentableIds;
   return;
}

void Recipe::setHopIds(QVector<int> hopIds) {
   this->pimpl->hopIds = hopIds;
   return;
}

void Recipe::setInstructionIds(QVector<int> instructionIds) {
   this->pimpl->instructionIds = instructionIds;
   return;
}

void Recipe::setMiscIds(QVector<int> miscIds) {
   this->pimpl->miscIds = miscIds;
   return;
}

void Recipe::setSaltIds(QVector<int> saltIds) {
   this->pimpl->saltIds = saltIds;
   return;
}

void Recipe::setWaterIds(QVector<int> waterIds) {
   this->pimpl->waterIds = waterIds;
   return;
}

void Recipe::setYeastIds(QVector<int> yeastIds) {
   this->pimpl->yeastIds = yeastIds;
   return;
}
*/

//==============================="SET" METHODS=================================
void Recipe::setRecipeType(Recipe::Type var) {
   this->setType(RECIPE_TYPE_STRING_TO_TYPE.key(var));
   return;
}

void Recipe::setType( const QString &var )
{
   QString tmp;
   if ( ! isValidType(var) ) {
      qWarning() << QString("Recipe: invalid type: %1").arg(var);
      tmp = "All Grain";
   }
   else {
      tmp = QString(var);
   }
   m_type = tmp;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::type, tmp );
   }
}

void Recipe::setBrewer( const QString &var )
{
   m_brewer = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::brewer, var );
   }
}

void Recipe::setBatchSize_l( double var )
{
   double tmp;
   if( var < 0.0 )
   {
      qWarning() << QString("Recipe: batch size < 0: %1").arg(var);
      tmp = 0;
   }
   else
   {
      tmp = var;
   }

   m_batchSize_l = tmp;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::batchSize_l, tmp );
   }

   // NOTE: this is bad, but we have to call recalcAll(), because the estimated
   // boil/batch volumes depend on the target volumes when there are no mash
   // steps to actually provide an estimate for the volumes.
   recalcAll();
}

void Recipe::setBoilSize_l( double var )
{
   double tmp;
   if( var < 0.0 )
   {
      qWarning() << QString("Recipe: boil size < 0: %1").arg(var);
      tmp = 0;
   }
   else
   {
      tmp = var;
   }

   m_boilSize_l = tmp;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::boilSize_l, tmp );
   }

   // NOTE: this is bad, but we have to call recalcAll(), because the estimated
   // boil/batch volumes depend on the target volumes when there are no mash
   // steps to actually provide an estimate for the volumes.
   recalcAll();
}

void Recipe::setBoilTime_min( double var )
{
   double tmp;
   if( var < 0.0 )
   {
      qWarning() << QString("Recipe: boil time < 0: %1").arg(var);
      tmp = 0;
   }
   else
   {
      tmp = var;
   }

   m_boilTime_min = tmp;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::boilTime_min, tmp);
   }
}

void Recipe::setEfficiency_pct( double var )
{
   double tmp;
   if( var < 0.0  || var > 100.0 )
   {
      qWarning() << QString("Recipe: 0 < efficiency < 100: %1").arg(var);
      tmp = 70;
   }
   else
   {
      tmp = var;
   }

   m_efficiency_pct = tmp;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::efficiency_pct, tmp );
   }

   // If you change the efficency, you really should recalc. And I'm afraid it
   // means recalc all, since og and fg will change, which means your ratios
   // change
   recalcAll();
}

void Recipe::setAsstBrewer( const QString &var )
{
   m_asstBrewer = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::asstBrewer, var );
   }
}

void Recipe::setNotes( const QString &var )
{
   m_notes = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::notes, var );
   }
}

void Recipe::setTasteNotes( const QString &var )
{
   m_tasteNotes = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::tasteNotes, var );
   }
}

void Recipe::setTasteRating( double var )
{
   double tmp;
   if( var < 0.0 || var > 50.0 )
   {
      qWarning() << QString("Recipe: 0 < taste rating < 50: %1").arg(var);
      tmp = 0;
   }
   else
   {
      tmp = var;
   }

   m_tasteRating = tmp;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::tasteRating, tmp );
   }
}

void Recipe::setOg( double var )
{
   double tmp;
   if( var < 0.0 )
   {
      qWarning() << QString("Recipe: og < 0: %1").arg(var);
      tmp = 1.0;
   }
   else
   {
      tmp = var;
   }

   m_og = tmp;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::og, tmp );
   }
}

void Recipe::setFg( double var )
{
   double tmp;
   if( var < 0.0 )
   {
      qWarning() << QString("Recipe: fg < 0: %1").arg(var);
      tmp = 1.0;
   }
   else
   {
      tmp = var;
   }

   m_fg = tmp;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::fg, tmp );
   }
}

void Recipe::setFermentationStages( int var )
{
   int tmp;
   if( var < 0 )
   {
      qWarning() << QString("Recipe: stages < 0: %1").arg(var);
      tmp = 0;
   }
   else
   {
      tmp = var;
   }

   m_fermentationStages = tmp;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::fermentationStages, tmp );
   }
}

void Recipe::setPrimaryAge_days( double var )
{
   double tmp;
   if( var < 0.0 )
   {
      qWarning() << QString("Recipe: primary age < 0: %1").arg(var);
      tmp = 0;
   }
   else
   {
      tmp = var;
   }

   m_primaryAge_days = tmp;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::primaryAge_days, tmp );
   }
}

void Recipe::setPrimaryTemp_c( double var )
{
   m_primaryTemp_c = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::primaryTemp_c, var );
   }
}

void Recipe::setSecondaryAge_days( double var )
{
   double tmp;
   if( var < 0.0 )
   {
      qWarning() << QString("Recipe: secondary age < 0: %1").arg(var);
      tmp = 0;
   }
   else
   {
      tmp = var;
   }

   m_secondaryAge_days = tmp;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::secondaryAge_days, tmp );
   }
}

void Recipe::setSecondaryTemp_c( double var )
{
   m_secondaryTemp_c = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::secondaryTemp_c, var );
   }
}

void Recipe::setTertiaryAge_days( double var )
{
   double tmp;
   if( var < 0.0 )
   {
      qWarning() << QString("Recipe: tertiary age < 0: %1").arg(var);
      tmp = 0;
   }
   else
   {
      tmp = var;
   }

   m_tertiaryAge_days = tmp;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::tertiaryAge_days, tmp );
   }
}

void Recipe::setTertiaryTemp_c( double var )
{
   m_tertiaryTemp_c = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::tertiaryTemp_c, var );
   }
}

void Recipe::setAge_days( double var )
{
   double tmp;
   if( var < 0.0 )
   {
      qWarning() << QString("Recipe: age < 0: %1").arg(var);
      tmp = 0;
   }
   else
   {
      tmp = var;
   }

   m_age = tmp;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::age, tmp );
   }
}

void Recipe::setAgeTemp_c( double var )
{
   m_ageTemp_c = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::ageTemp_c, var );
   }
}

void Recipe::setDate( const QDate &var ) {
   m_date = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::date, var.toString(Qt::ISODate));
   }
}

void Recipe::setCarbonation_vols( double var )
{
   double tmp;
   if( var < 0.0 )
   {
      qWarning() << QString("Recipe: carb < 0: %1").arg(var);
      tmp = 0;
   }
   else
   {
      tmp = var;
   }

   m_carbonation_vols = tmp;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::carbonation_vols, tmp );
   }
}

void Recipe::setForcedCarbonation( bool var )
{
   m_forcedCarbonation = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::forcedCarbonation, var );
   }
}

void Recipe::setPrimingSugarName( const QString &var )
{
   m_primingSugarName = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::primingSugarName, var );
   }
}

void Recipe::setCarbonationTemp_c( double var )
{
   m_carbonationTemp_c = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::carbonationTemp_c, var );
   }
}

void Recipe::setPrimingSugarEquiv( double var )
{
   double tmp;
   if( var < 0.0 )
   {
      qWarning() << QString("Recipe: primingsugarequiv < 0: %1").arg(var);
      tmp = 1;
   }
   else
   {
      tmp = var;
   }

   m_primingSugarEquiv = tmp;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::primingSugarEquiv, tmp );
   }
}

void Recipe::setKegPrimingFactor( double var )
{
   double tmp;

   if( var < 0.0 )
   {
      qWarning() << QString("Recipe: keg priming factor < 0: %1").arg(var);
      tmp = 1;
   }
   else
   {
      tmp = var;
   }

   m_kegPrimingFactor = tmp;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Recipe::kegPrimingFactor, tmp );
   }
}

void Recipe::setCacheOnly( bool cache ) { m_cacheOnly = cache; }

//==========================Calculated Getters============================

double Recipe::og()
{
   if( m_uninitializedCalcs )
      recalcAll();
   return m_og;
}

double Recipe::fg()
{
   if( m_uninitializedCalcs )
      recalcAll();
   return m_fg;
}

double Recipe::color_srm()
{
   if( m_uninitializedCalcs )
      recalcAll();
   return m_color_srm;
}

double Recipe::ABV_pct()
{
   if( m_uninitializedCalcs )
      recalcAll();
   return m_ABV_pct;
}

double Recipe::IBU()
{
   if( m_uninitializedCalcs )
      recalcAll();
   return m_IBU;
}

QList<double> Recipe::IBUs()
{
   if( m_uninitializedCalcs )
      recalcAll();
   return m_ibus;
}

double Recipe::boilGrav()
{
   if( m_uninitializedCalcs )
      recalcAll();
   return m_boilGrav;
}

double Recipe::calories12oz()
{
   if( m_uninitializedCalcs )
      recalcAll();
   return m_calories;
}

double Recipe::calories33cl()
{
   if( m_uninitializedCalcs )
      recalcAll();
   return m_calories *3.3/3.55;
}

double Recipe::wortFromMash_l()
{
   if( m_uninitializedCalcs )
      recalcAll();
   return m_wortFromMash_l;
}

double Recipe::boilVolume_l()
{
   if( m_uninitializedCalcs )
      recalcAll();
   return m_boilVolume_l;
}

double Recipe::postBoilVolume_l()
{
   if( m_uninitializedCalcs )
      recalcAll();
   return m_postBoilVolume_l;
}

double Recipe::finalVolume_l()
{
   if( m_uninitializedCalcs )
      recalcAll();
   return m_finalVolume_l;
}

QColor Recipe::SRMColor()
{
   if( m_uninitializedCalcs )
      recalcAll();
   return m_SRMColor;
}

double Recipe::grainsInMash_kg()
{
   if( m_uninitializedCalcs )
      recalcAll();
   return m_grainsInMash_kg;
}

double Recipe::grains_kg()
{
   if( m_uninitializedCalcs )
      recalcAll();
   return m_grains_kg;
}

double Recipe::points()
{
   if( m_uninitializedCalcs )
      recalcAll();
   return (m_og -1.0)*1e3;
}

//=========================Relational Getters=============================
Style* Recipe::style() { return ObjectStoreWrapper::getByIdRaw<Style>(this->styleId); }
int Recipe::getStyleId() const { return this->styleId; }
Mash* Recipe::mash() const { return ObjectStoreWrapper::getByIdRaw<Mash>(this->mashId); }
int Recipe::getMashId() const { return this->mashId; }
Equipment* Recipe::equipment() const { return ObjectStoreWrapper::getByIdRaw<Equipment>(this->equipmentId); }
int Recipe::getEquipmentId() const { return this->equipmentId; }

QList<Instruction*> Recipe::instructions() const { return this->pimpl->getAllMyRaw<Instruction>(); }
QVector<int> Recipe::getInstructionIds() const { return this->pimpl->instructionIds; }
QList<BrewNote*> Recipe::brewNotes() const {
   // The Recipe owns its BrewNotes, but, for the moment at least, it's the BrewNote that knows which Recipe it's in
   // rather than the Recipe which knows which BrewNotes it has, so we have to ask.
   int const recipeId = this->key();
   return ObjectStoreTyped<BrewNote>::getInstance().findAllMatching(
      [recipeId](BrewNote const * bn) {return bn->getRecipeId() == recipeId;}
   );
}
QList<Hop*> Recipe::hops() const { return this->pimpl->getAllMyRaw<Hop>(); }
QVector<int> Recipe::getHopIds() const { return this->pimpl->hopIds; }
QList<Fermentable*> Recipe::fermentables() const { return this->pimpl->getAllMyRaw<Fermentable>(); }
QVector<int> Recipe::getFermentableIds() const { return this->pimpl->fermentableIds; }
QList<Misc*> Recipe::miscs() const { return this->pimpl->getAllMyRaw<Misc>(); }
QVector<int> Recipe::getMiscIds() const { return this->pimpl->miscIds; }
QList<Yeast*> Recipe::yeasts() const { return this->pimpl->getAllMyRaw<Yeast>(); }
QVector<int> Recipe::getYeastIds() const { return this->pimpl->yeastIds; }
QList<Water*> Recipe::waters() const { return this->pimpl->getAllMyRaw<Water>(); }
QVector<int> Recipe::getWaterIds() const { return this->pimpl->waterIds; }
QList<Salt*> Recipe::salts() const { return this->pimpl->getAllMyRaw<Salt>(); }
QVector<int> Recipe::getSaltIds() const { return this->pimpl->saltIds; }

//==============================Getters===================================
Recipe::Type Recipe::recipeType() const {
   return RECIPE_TYPE_STRING_TO_TYPE.value(this->type());
}
QString Recipe::type() const { return m_type; }
QString Recipe::brewer() const { return m_brewer; }
QString Recipe::asstBrewer() const { return m_asstBrewer; }
QString Recipe::notes() const { return m_notes; }
QString Recipe::tasteNotes() const { return m_tasteNotes; }
QString Recipe::primingSugarName() const { return m_primingSugarName; }
bool Recipe::forcedCarbonation() const { return m_forcedCarbonation; }
double Recipe::batchSize_l() const { return m_batchSize_l; }
double Recipe::boilSize_l() const { return m_boilSize_l; }
double Recipe::boilTime_min() const { return m_boilTime_min; }
double Recipe::efficiency_pct() const { return m_efficiency_pct; }
double Recipe::tasteRating() const { return m_tasteRating; }
double Recipe::primaryAge_days() const { return m_primaryAge_days; }
double Recipe::primaryTemp_c() const { return m_primaryTemp_c; }
double Recipe::secondaryAge_days() const { return m_secondaryAge_days; }
double Recipe::secondaryTemp_c() const { return m_secondaryTemp_c; }
double Recipe::tertiaryAge_days() const { return m_tertiaryAge_days; }
double Recipe::tertiaryTemp_c() const { return m_tertiaryTemp_c; }
double Recipe::age_days() const { return m_age; }
double Recipe::ageTemp_c() const { return m_ageTemp_c; }
double Recipe::carbonation_vols() const { return m_carbonation_vols; }
double Recipe::carbonationTemp_c() const { return m_carbonationTemp_c; }
double Recipe::primingSugarEquiv() const { return m_primingSugarEquiv; }
double Recipe::kegPrimingFactor() const { return m_kegPrimingFactor; }
int Recipe::fermentationStages() const { return m_fermentationStages; }
QDate Recipe::date() const { return m_date; }
bool Recipe::cacheOnly() const { return m_cacheOnly; }

//=============================Adders and Removers========================================


double Recipe::batchSizeNoLosses_l()
{
   double ret = batchSize_l();
   Equipment* e = equipment();
   if( e )
      ret += e->trubChillerLoss_l();

   return ret;
}

//==============================Recalculators==================================

void Recipe::recalcAll()
{
   // WARNING
   // Infinite recursion possible, since these methods will emit changed(),
   // causing other objects to call finalVolume_l() for example, which may
   // cause another call to recalcAll() and so on.
   //
   // GSG: Now only emit when _uninitializedCalcs is true, which helps some.

   // Someone has already called this function back in the call stack, so return to avoid recursion.
   if( ! m_recalcMutex.tryLock() )
      return;

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
}

void Recipe::recalcABV_pct()
{
   double ret;

   // The complex formula, and variations comes from Ritchie Products Ltd, (Zymurgy, Summer 1995, vol. 18, no. 2)
   // Michael L. Hall’s article Brew by the Numbers: Add Up What’s in Your Beer, and Designing Great Beers by Daniels.
   ret = (76.08 * (m_og_fermentable - m_fg_fermentable) / (1.775 - m_og_fermentable)) * (m_fg_fermentable / 0.794);

   if ( ! qFuzzyCompare(ret,m_ABV_pct ) ) {
      m_ABV_pct = ret;
      if (!m_uninitializedCalcs)
      {
        emit changed( metaProperty(PropertyNames::Recipe::ABV_pct), m_ABV_pct );
      }
   }
}

void Recipe::recalcColor_srm()
{
   Fermentable *ferm;
   double mcu = 0.0;
   double ret;
   int i;

   QList<Fermentable*> ferms = fermentables();
   for( i = 0; static_cast<int>(i) < ferms.size(); ++i )
   {
      ferm = ferms[i];
      // Conversion factor for lb/gal to kg/l = 8.34538.
      mcu += ferm->color_srm()*8.34538 * ferm->amount_kg()/ m_finalVolumeNoLosses_l;
   }

   ret = ColorMethods::mcuToSrm(mcu);

   if ( ! qFuzzyCompare(m_color_srm, ret ) ) {
      m_color_srm = ret;
      if (!m_uninitializedCalcs)
      {
        emit changed( metaProperty(PropertyNames::Recipe::color_srm), m_color_srm );
      }
   }

}

void Recipe::recalcIBU()
{
   int i;
   double ibus = 0.0;
   double tmp = 0.0;

   // Bitterness due to hops...
   m_ibus.clear();
   QList<Hop*> hhops = hops();
   for( i = 0; i < hhops.size(); ++i ) {
      tmp = ibuFromHop(hhops[i]);
      m_ibus.append(tmp);
      ibus += tmp;
   }

   // Bitterness due to hopped extracts...
   QList<Fermentable*> ferms = fermentables();
   for( i = 0; static_cast<int>(i) < ferms.size(); ++i ) {
      // Conversion factor for lb/gal to kg/l = 8.34538.
      ibus +=
              ferms[i]->ibuGalPerLb() *
              (ferms[i]->amount_kg() / batchSize_l()) / 8.34538;
   }

   if ( ! qFuzzyCompare(ibus, m_IBU ) ) {
      m_IBU = ibus;
      if (!m_uninitializedCalcs)
      {
        emit changed( metaProperty(PropertyNames::Recipe::IBU), m_IBU );
      }
   }
}

void Recipe::recalcVolumeEstimates()
{
   double waterAdded_l;
   double absorption_lKg;
   double tmp = 0.0;
   double tmp_wfm = 0.0;
   double tmp_bv = 0.0;
   double tmp_fv = 0.0;
   double tmp_pbv = 0.0;

   // wortFromMash_l ==========================
   if( mash() == nullptr ) {
      m_wortFromMash_l = 0.0;
   }
   else
   {
      waterAdded_l = mash()->totalMashWater_l();
      if( equipment() != nullptr )
         absorption_lKg = equipment()->grainAbsorption_LKg();
      else
         absorption_lKg = PhysicalConstants::grainAbsorption_Lkg;

      tmp_wfm = (waterAdded_l - absorption_lKg * m_grainsInMash_kg);
   }

   // boilVolume_l ==============================

   if( equipment() != nullptr )
      tmp = tmp_wfm - equipment()->lauterDeadspace_l() + equipment()->topUpKettle_l();
   else
      tmp = tmp_wfm;

   // Need to account for extract/sugar volume also.
   QList<Fermentable*> ferms = fermentables();
   foreach( Fermentable* f, ferms )
   {
      Fermentable::Type type = f->type();
      if( type == Fermentable::Extract )
         tmp += f->amount_kg() / PhysicalConstants::liquidExtractDensity_kgL;
      else if( type == Fermentable::Sugar )
         tmp += f->amount_kg() / PhysicalConstants::sucroseDensity_kgL;
      else if( type == Fermentable::Dry_Extract )
         tmp += f->amount_kg() / PhysicalConstants::dryExtractDensity_kgL;
   }

   if( tmp <= 0.0 )
      tmp = boilSize_l(); // Give up.

   tmp_bv = tmp;

   // finalVolume_l ==============================

   // NOTE: the following figure is not based on the other volume estimates
   // since we want to show og,fg,ibus,etc. as if the collected wort is correct.
   m_finalVolumeNoLosses_l = batchSizeNoLosses_l();
   if( equipment() != nullptr )
   {
      //_finalVolumeNoLosses_l = equipment()->wortEndOfBoil_l(tmp_bv) + equipment()->topUpWater_l();
      tmp_fv = equipment()->wortEndOfBoil_l(tmp_bv) + equipment()->topUpWater_l() - equipment()->trubChillerLoss_l();
   }
   else
   {
        m_finalVolume_l = tmp_bv - 4.0; // This is just shooting in the dark. Can't do much without an equipment.
      //_finalVolumeNoLosses_l = _finalVolume_l;
   }

   // postBoilVolume_l ===========================

   if( equipment() != nullptr ) {
      tmp_pbv = equipment()->wortEndOfBoil_l( tmp_bv );
   }
   else {
      tmp_pbv = batchSize_l(); // Give up.
   }

   if ( ! qFuzzyCompare(tmp_wfm, m_wortFromMash_l ) ) {
      m_wortFromMash_l = tmp_wfm;
      if (!m_uninitializedCalcs) {
        emit changed( metaProperty(PropertyNames::Recipe::wortFromMash_l), m_wortFromMash_l );
      }
   }

   if ( ! qFuzzyCompare(tmp_bv, m_boilVolume_l ) ) {
        m_boilVolume_l = tmp_bv;
      if (!m_uninitializedCalcs) {
        emit changed( metaProperty(PropertyNames::Recipe::boilVolume_l), m_boilVolume_l );
      }
   }

   if ( ! qFuzzyCompare(tmp_fv, m_finalVolume_l ) ) {
       m_finalVolume_l = tmp_fv;
      if (!m_uninitializedCalcs) {
        emit changed( metaProperty(PropertyNames::Recipe::finalVolume_l), m_finalVolume_l );
      }
   }

   if ( ! qFuzzyCompare(tmp_pbv, m_postBoilVolume_l ) ) {
      m_postBoilVolume_l = tmp_pbv;
      if (!m_uninitializedCalcs) {
        emit changed( metaProperty(PropertyNames::Recipe::postBoilVolume_l), m_postBoilVolume_l );
      }
   }
}

void Recipe::recalcGrainsInMash_kg()
{
   int i, size;
   double ret = 0.0;
   Fermentable* ferm;

   QList<Fermentable*> ferms = fermentables();
   size = ferms.size();
   for( i = 0; i < size; ++i )
   {
      ferm = ferms[i];

      if( ferm->type() == Fermentable::Grain && ferm->isMashed() )
      {
         ret += ferm->amount_kg();
      }
   }

   if ( ! qFuzzyCompare(ret, m_grainsInMash_kg )  ) {
      m_grainsInMash_kg = ret;
      if (!m_uninitializedCalcs) {
        emit changed( metaProperty(PropertyNames::Recipe::grainsInMash_kg), m_grainsInMash_kg );
      }
   }
}

void Recipe::recalcGrains_kg()
{
   int i, size;
   double ret = 0.0;

   QList<Fermentable*> ferms = fermentables();
   size = ferms.size();
   for( i = 0; i < size; ++i )
      ret += ferms[i]->amount_kg();

   if ( ! qFuzzyCompare(ret, m_grains_kg ) ) {
      m_grains_kg = ret;
      if (!m_uninitializedCalcs) {
        emit changed( metaProperty(PropertyNames::Recipe::grains_kg), m_grains_kg );
      }
   }
}

void Recipe::recalcSRMColor()
{
   QColor tmp = Algorithms::srmToColor(m_color_srm);

   if ( tmp != m_SRMColor )
   {
      m_SRMColor = tmp;
      if (!m_uninitializedCalcs)
      {
        emit changed( metaProperty(PropertyNames::Recipe::SRMColor), m_SRMColor );
      }
   }
}

// the formula in here are taken from http://hbd.org/ensmingr/
void Recipe::recalcCalories()
{
   double startPlato, finishPlato, RE, abw, oog, ffg, tmp;

   oog = m_og;
   ffg = m_fg;

   // Need to translate OG and FG into plato
   startPlato  = -463.37 + ( 668.72 * oog ) - (205.35 * oog * oog);
   finishPlato = -463.37 + ( 668.72 * ffg ) - (205.35 * ffg * ffg);

   // RE (real extract)
   RE = (0.1808 * startPlato) + (0.8192 * finishPlato);

   // Alcohol by weight?
   abw = (startPlato-RE)/(2.0665 - (0.010665 * startPlato));

   // The final results of this formular are calories per 100 ml.
   // The 3.55 puts it in terms of 12 oz. I really should have stored it
   // without that adjust.
   tmp = ((6.9*abw) + 4.0 * (RE-0.1)) * ffg * 3.55;

   //! If there are no fermentables in the recipe, if there is no mash, etc.,
   //  then the calories/12 oz ends up negative. Since negative doesn't make
   //  sense, set it to 0
   if ( tmp < 0 ) {
      tmp = 0;
   }

   if ( ! qFuzzyCompare(tmp, m_calories ) ) {
      m_calories = tmp;
      if (!m_uninitializedCalcs) {
        emit changed( metaProperty(PropertyNames::Recipe::calories), m_calories );
      }
   }
}

// other efficiency calculations need access to the maximum theoretical sugars
// available. The only way I can see of doing that which doesn't suck is to
// split that calcuation out of recalcOgFg();
QHash<QString,double> Recipe::calcTotalPoints()
{
   int i;
   double sugar_kg_ignoreEfficiency = 0.0;
   double sugar_kg                  = 0.0;
   double nonFermentableSugars_kg    = 0.0;
   double lateAddition_kg           = 0.0;
   double lateAddition_kg_ignoreEff = 0.0;

   Fermentable* ferm;

   QList<Fermentable*> ferms = fermentables();
   QHash<QString,double> ret;

   for( i = 0; static_cast<int>(i) < ferms.size(); ++i )
   {
      ferm = ferms[i];

      // If we have some sort of non-grain, we have to ignore efficiency.
      if( ferm->isSugar() || ferm->isExtract() )
      {
         sugar_kg_ignoreEfficiency += ferm->equivSucrose_kg();

         if (ferm->addAfterBoil())
            lateAddition_kg_ignoreEff += ferm->equivSucrose_kg();

         if ( !isFermentableSugar(ferm) )
           nonFermentableSugars_kg += ferm->equivSucrose_kg();
      }
      else
      {
         sugar_kg += ferm->equivSucrose_kg();

         if (ferm->addAfterBoil())
            lateAddition_kg += ferm->equivSucrose_kg();
      }
   }

   ret.insert("sugar_kg", sugar_kg);
   ret.insert("nonFermentableSugars_kg", nonFermentableSugars_kg);
   ret.insert("sugar_kg_ignoreEfficiency", sugar_kg_ignoreEfficiency);
   ret.insert("lateAddition_kg", lateAddition_kg);
   ret.insert("lateAddition_kg_ignoreEff", lateAddition_kg_ignoreEff);

   return ret;

}

void Recipe::recalcBoilGrav()
{
   double sugar_kg = 0.0;
   double sugar_kg_ignoreEfficiency = 0.0;
   double lateAddition_kg           = 0.0;
   double lateAddition_kg_ignoreEff = 0.0;
   double ret;
   QHash<QString,double> sugars;

   sugars = calcTotalPoints();
   sugar_kg = sugars.value("sugar_kg");
   sugar_kg_ignoreEfficiency = sugars.value("sugar_kg_ignoreEfficiency");
   lateAddition_kg = sugars.value("lateAddition_kg");
   lateAddition_kg_ignoreEff = sugars.value("lateAddition_kg_ignoreEff");

   // Since the efficiency refers to how much sugar we get into the fermenter,
   // we need to adjust for that here.
   sugar_kg = (efficiency_pct()/100.0 * (sugar_kg - lateAddition_kg) + sugar_kg_ignoreEfficiency - lateAddition_kg_ignoreEff);

   ret = Algorithms::PlatoToSG_20C20C( Algorithms::getPlato(sugar_kg, boilSize_l()) );

   if ( ! qFuzzyCompare(ret, m_boilGrav ) ) {
      m_boilGrav = ret;
      if (!m_uninitializedCalcs)
      {
        emit changed( metaProperty(PropertyNames::Recipe::boilGrav), m_boilGrav );
      }
   }
}

void Recipe::recalcOgFg()
{
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
   Yeast* yeast;
   QHash<QString,double> sugars;

   m_og_fermentable = m_fg_fermentable = 0.0;

   // The first time through really has to get the _og and _fg from the
   // database, not use the initialized values of 1. I (maf) tried putting
   // this in the initialize, but it just hung. So I moved it here, but only
   // if if we aren't initialized yet.
   //
   // GSG: This doesn't work, this og and fg are already set to 1.0 so
   // until we load these values from the database on startup, we have
   // to calculate.
   if ( m_uninitializedCalcs )
   {
      m_og = Brewken::toDouble(this, PropertyNames::Recipe::og, "Recipe::recalcOgFg()");
      m_fg = Brewken::toDouble(this, PropertyNames::Recipe::fg, "Recipe::recalcOgFg()");
   }

   // Find out how much sugar we have.
   sugars = calcTotalPoints();
   sugar_kg                  = sugars.value("sugar_kg");  // Mass of sugar that *is* affected by mash efficiency
   sugar_kg_ignoreEfficiency = sugars.value("sugar_kg_ignoreEfficiency");  // Mass of sugar that *is not* affected by mash efficiency
   nonFermentableSugars_kg    = sugars.value("nonFermentableSugars_kg");  // Mass of sugar that is not fermentable (also counted in sugar_kg_ignoreEfficiency)

   // We might lose some sugar in the form of Trub/Chiller loss and lauter deadspace.
   if( equipment() != nullptr ) {

      kettleWort_l = (m_wortFromMash_l - equipment()->lauterDeadspace_l()) + equipment()->topUpKettle_l();
      postBoilWort_l = equipment()->wortEndOfBoil_l(kettleWort_l);
      ratio = (postBoilWort_l - equipment()->trubChillerLoss_l()) / postBoilWort_l;
      if( ratio > 1.0 ) // Usually happens when we don't have a mash yet.
         ratio = 1.0;
      else if( ratio < 0.0 )
         ratio = 0.0;
      else if( Algorithms::isNan(ratio) )
         ratio = 1.0;
      // Ignore this again since it should be included in efficiency.
      //sugar_kg *= ratio;
      sugar_kg_ignoreEfficiency *= ratio;
      if ( nonFermentableSugars_kg != 0.0 )
         nonFermentableSugars_kg *= ratio;
   }

   // Total sugars after accounting for efficiency and mash losses. Implicitly includes non-fermentable sugars
   sugar_kg = sugar_kg * efficiency_pct()/100.0 + sugar_kg_ignoreEfficiency;
   plato = Algorithms::getPlato( sugar_kg, m_finalVolumeNoLosses_l);

   tmp_og = Algorithms::PlatoToSG_20C20C( plato );  // og from all sugars
   tmp_pnts = (tmp_og-1)*1000.0;  // points from all sugars
   if ( nonFermentableSugars_kg != 0.0 )
   {
      ferm_kg = sugar_kg - nonFermentableSugars_kg;  // Mass of only fermentable sugars
      plato = Algorithms::getPlato( ferm_kg, m_finalVolumeNoLosses_l);  // Plato from fermentable sugars
      m_og_fermentable = Algorithms::PlatoToSG_20C20C( plato );  // og from only fermentable sugars
      plato = Algorithms::getPlato( nonFermentableSugars_kg, m_finalVolumeNoLosses_l);  // Plate from non-fermentable sugars
      tmp_nonferm_pnts = ((Algorithms::PlatoToSG_20C20C( plato ))-1)*1000.0;  // og points from non-fermentable sugars
   }
   else
   {
      m_og_fermentable = tmp_og;
      tmp_nonferm_pnts = 0;
   }

   // Calculage FG
   QList<Yeast*> yeasties = yeasts();
   for( i = 0; static_cast<int>(i) < yeasties.size(); ++i )
   {
      yeast = yeasties[i];
      // Get the yeast with the greatest attenuation.
      if( yeast->attenuation_pct() > attenuation_pct )
         attenuation_pct = yeast->attenuation_pct();
   }
   // This means we have yeast, but they neglected to provide attenuation percentages.
   if( yeasties.size() > 0 && attenuation_pct <= 0.0 )  {
      attenuation_pct = 75.0; // 75% is an average attenuation.
   }

   if ( nonFermentableSugars_kg != 0.0 )
   {
      tmp_ferm_pnts = (tmp_pnts-tmp_nonferm_pnts) * (1.0 - attenuation_pct/100.0);  // fg points from fermentable sugars
      tmp_pnts = tmp_ferm_pnts + tmp_nonferm_pnts;  // FG points from both fermentable and non-fermentable sugars
      //tmp_pnts *= (1.0 - attenuation_pct/100.0);  // WTF, this completely ignores all the calculations about non-fermentable sugars and just converts everything!
      tmp_fg =  1 + tmp_pnts/1000.0;  // new FG value
      m_fg_fermentable =  1 + tmp_ferm_pnts/1000.0;  // FG from fermentables only
   }
   else
   {
      tmp_pnts *= (1.0 - attenuation_pct/100.0);
      tmp_fg =  1 + tmp_pnts/1000.0;
      m_fg_fermentable = tmp_fg;
   }

   if ( ! qFuzzyCompare(m_og, tmp_og ) ) {
      m_og     = tmp_og;
      // NOTE: We don't want to do this on the first load of the recipe.
      // NOTE: We are we recalculating all of these on load? Shouldn't we be
      // reading these values from the database somehow?
      //
      // GSG: Yes we can, but until the code is added to intialize these calculated
      // values from the database, we can calculate them on load. They should be
      // the same as the database values since the database values were set with
      // these functions in the first place.
      if (!m_uninitializedCalcs)
      {
        setEasy(PropertyNames::Recipe::og, m_og, false );
        emit changed( metaProperty(PropertyNames::Recipe::og), m_og );
        emit changed( metaProperty(PropertyNames::Recipe::points), (m_og-1.0)*1e3 );
      }
   }

   if ( ! qFuzzyCompare(tmp_fg, m_fg ) ) {
      m_fg     = tmp_fg;
      if (!m_uninitializedCalcs)
      {
        setEasy(PropertyNames::Recipe::fg, m_fg, false );
        emit changed( metaProperty(PropertyNames::Recipe::fg), m_fg );
      }
   }
}

//====================================Helpers===========================================

double Recipe::ibuFromHop(Hop const* hop)
{
   Equipment* equip = equipment();
   double ibus = 0.0;
   double fwhAdjust = Brewken::toDouble(PersistentSettings::value("firstWortHopAdjustment", 1.1).toString(), "Recipe::ibmFromHop()");
   double mashHopAdjust = Brewken::toDouble(PersistentSettings::value("mashHopAdjustment", 0).toString(), "Recipe::ibmFromHop()");

   if( hop == nullptr )
      return 0.0;

   double AArating = hop->alpha_pct()/100.0;
   double grams = hop->amount_kg()*1000.0;
   double minutes = hop->time_min();
   // Assume 100% utilization until further notice
   double hopUtilization = 1.0;
   // Assume 60 min boil until further notice
   int boilTime = 60;

   // NOTE: we used to carefully calculate the average boil gravity and use it in the
   // IBU calculations. However, due to John Palmer
   // (http://homebrew.stackexchange.com/questions/7343/does-wort-gravity-affect-hop-utilization),
   // it seems more appropriate to just use the OG directly, since it is the total
   // amount of break material that truly affects the IBUs.

   if( equip ) {
      hopUtilization = equip->hopUtilization_pct() / 100.0;
      boilTime = static_cast<int>(equip->boilTime_min());
   }

   if( hop->use() == Hop::Boil)
      ibus = IbuMethods::getIbus( AArating, grams, m_finalVolumeNoLosses_l, m_og, minutes );
   else if( hop->use() == Hop::First_Wort )
      ibus = fwhAdjust * IbuMethods::getIbus( AArating, grams, m_finalVolumeNoLosses_l, m_og, boilTime );
   else if( hop->use() == Hop::Mash && mashHopAdjust > 0.0 )
      ibus = mashHopAdjust * IbuMethods::getIbus( AArating, grams, m_finalVolumeNoLosses_l, m_og, boilTime );

   // Adjust for hop form. Tinseth's table was created from whole cone data,
   // and it seems other formulae are optimized that way as well. So, the
   // utilization is considered unadjusted for whole cones, and adjusted
   // up for plugs and pellets.
   //
   // - http://www.realbeer.com/hops/FAQ.html
   // - https://groups.google.com/forum/#!topic"Brewken.h"lp/mv2qvWBC4sU
   switch( hop->form() ) {
      case Hop::Plug:
         hopUtilization *= 1.02;
         break;
      case Hop::Pellet:
         hopUtilization *= 1.10;
         break;
      default:
         break;
   }

   // Adjust for hop utilization.
   ibus *= hopUtilization;

   return ibus;
}

// this was fixed, but not with an at
bool Recipe::isValidType( const QString &str )
{
   return RECIPE_TYPE_STRING_TO_TYPE.contains(str);
}

QList<QString> Recipe::getReagents( QList<Fermentable*> ferms )
{
   QList<QString> reagents;
   QString format,tmp;

   for ( int i = 0; i < ferms.size(); ++i )
   {
      if ( ferms[i]->isMashed() )
      {
         if ( i+1 < ferms.size() )
         {
            tmp = QString("%1 %2, ")
                  .arg(Brewken::displayAmount(ferms[i]->amount_kg(), kFermentableTableSection, PropertyNames::Fermentable::amount_kg, &Units::kilograms))
                  .arg(ferms[i]->name());
         }
         else
         {
            tmp = QString("%1 %2 ")
                  .arg(Brewken::displayAmount(ferms[i]->amount_kg(), kFermentableTableSection, PropertyNames::Fermentable::amount_kg,  &Units::kilograms))
                  .arg(ferms[i]->name());
         }
         reagents.append(tmp);
      }
   }
   return reagents;
}

QList<QString> Recipe::getReagents(QList<Hop*> hops, bool firstWort)
{
   QString tmp;
   QList<QString> reagents;

   for( int i = 0; i < hops.size(); ++i )
   {
      if( firstWort && (hops[i]->use() == Hop::First_Wort) ) {
         tmp = QString("%1 %2,")
               .arg(Brewken::displayAmount(hops[i]->amount_kg(), kHopTableSection, PropertyNames::Hop::amount_kg,  &Units::kilograms))
               .arg(hops[i]->name());
         reagents.append(tmp);
      }
   }
   return reagents;
}

QList<QString> Recipe::getReagents( QList<MashStep*> msteps )
{
   QString tmp;
   QList<QString> reagents;

   for ( int i = 0; i < msteps.size(); ++i )
   {
      if( ! msteps[i]->isInfusion() )
         continue;

      if ( i+1 < msteps.size() )
      {
         tmp = tr("%1 water to %2, ")
                .arg(Brewken::displayAmount(msteps[i]->infuseAmount_l(), kMashStepSection, PropertyNames::MashStep::infuseAmount_l, &Units::liters))
                .arg(Brewken::displayAmount(msteps[i]->infuseTemp_c(), kMashStepSection, PropertyNames::MashStep::infuseTemp_c,  &Units::celsius));
      }
      else
      {
         tmp = tr("%1 water to %2 ")
                .arg(Brewken::displayAmount(msteps[i]->infuseAmount_l(), kMashStepSection, PropertyNames::MashStep::infuseAmount_l, &Units::liters))
                .arg(Brewken::displayAmount(msteps[i]->infuseTemp_c(), kMashStepSection, PropertyNames::MashStep::infuseTemp_c, &Units::celsius));
      }
      reagents.append(tmp);
   }
   return reagents;
}

//! \brief send me a list of salts and if we are wanting to add to the
//! mash or the sparge, and I will return a list of instructions
QStringList Recipe::getReagents( QList<Salt*> salts, Salt::WhenToAdd wanted)
{
   QString tmp;
   QStringList reagents = QStringList();

   for ( int i = 0; i < salts.size(); ++i )
   {
      Salt::WhenToAdd what = salts[i]->addTo();
      Unit const * rightUnit = salts[i]->amountIsWeight() ? &Units::kilograms: &Units::liters;
      if ( what == wanted ) {
         tmp = tr("%1 %2, ")
               .arg(Brewken::displayAmount(salts[i]->amount(), kSaltTableSection, PropertyNames::Salt::amount, rightUnit))
               .arg(salts[i]->name());
      }
      else if ( what == Salt::EQUAL ) {
         tmp = tr("%1 %2, ")
               .arg(Brewken::displayAmount(salts[i]->amount(), kSaltTableSection, PropertyNames::Salt::amount, rightUnit))
               .arg(salts[i]->name());
      }
      else if ( what == Salt::RATIO ) {
         double ratio = 1.0;
         if ( wanted == Salt::SPARGE )
            ratio = mash()->totalSpargeAmount_l()/mash()->totalInfusionAmount_l();
         double amt = salts[i]->amount() * ratio;
         tmp = tr("%1 %2, ")
               .arg(Brewken::displayAmount(amt, kSaltTableSection, PropertyNames::Salt::amount, rightUnit))
               .arg(salts[i]->name());
      }
      else {
         continue;
      }
      reagents.append(tmp);
   }
   // How many ways can we remove the trailing ", " because it really, really
   // annoys me?
   if ( reagents.size() > 0 ) {
      QString fixin = reagents.takeLast();
      fixin.remove( fixin.lastIndexOf(","), 2);
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
      qDebug() << Q_FUNC_INFO << "Signal received from " << signalSenderClassName;
      if (signalSenderClassName == "Hop") {
         this->recalcIBU();
      }
   } else {
      qDebug() << Q_FUNC_INFO << "No sender";
   }
   return;
}

void Recipe::acceptEquipChange(QMetaProperty prop, QVariant val) {
   recalcAll();
}

void Recipe::acceptFermChange(QMetaProperty prop, QVariant val)
{
   recalcAll();
}

void Recipe::onFermentableChanged()
{
   recalcAll();
}

/*void Recipe::acceptHopChange(QMetaProperty prop, QVariant val)
{
   recalcIBU();
}*/

void Recipe::acceptHopChange(Hop* hop)
{
   recalcIBU();
}

void Recipe::acceptYeastChange(QMetaProperty prop, QVariant val)
{
   recalcOgFg();
   recalcABV_pct();
}

void Recipe::acceptYeastChange(Yeast* yeast)
{
   recalcOgFg();
   recalcABV_pct();
}

void Recipe::acceptMashChange(QMetaProperty prop, QVariant val)
{
   Mash* mashSend = qobject_cast<Mash*>(sender());

   if ( mashSend == nullptr )
      return;

   recalcAll();
}

void Recipe::acceptMashChange(Mash* newMash)
{
   if ( newMash == mash() )
      recalcAll();
}

double Recipe::targetCollectedWortVol_l()
{

   // Need to account for extract/sugar volume also.
   float postMashAdditionVolume_l = 0;

   QList<Fermentable*> ferms = fermentables();
   foreach( Fermentable* f, ferms ) {
      Fermentable::Type type = f->type();
      if ( type == Fermentable::Extract ) {
         postMashAdditionVolume_l  += static_cast<float>(f->amount_kg() / PhysicalConstants::liquidExtractDensity_kgL);
      }
      else if ( type == Fermentable::Sugar ) {
         postMashAdditionVolume_l  += static_cast<float>(f->amount_kg() / PhysicalConstants::sucroseDensity_kgL);
      }
      else if ( type == Fermentable::Dry_Extract ) {
         postMashAdditionVolume_l  += static_cast<float>(f->amount_kg() / PhysicalConstants::dryExtractDensity_kgL);
      }
   }

   if ( equipment() ) {
      return boilSize_l() - equipment()->topUpKettle_l() - static_cast<double>(postMashAdditionVolume_l);
   }
   else {
      return boilSize_l() - static_cast<double>(postMashAdditionVolume_l);
   }
}

double Recipe::targetTotalMashVol_l()
{

   double absorption_lKg;

   if( equipment() ) {
      absorption_lKg = equipment()->grainAbsorption_LKg();
   }
   else {
      absorption_lKg = PhysicalConstants::grainAbsorption_Lkg;
   }


   return targetCollectedWortVol_l() + absorption_lKg * grainsInMash_kg();
}
