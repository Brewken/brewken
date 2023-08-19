/*======================================================================================================================
 * model/Mash.cpp is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Mattias Måhl <mattias@kejsarsten.com>
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
#include "model/Mash.h"

#include <iostream>
#include <string>

#include <QObject>

#include "database/ObjectStoreWrapper.h"
#include "model/MashStep.h"
#include "model/NamedParameterBundle.h"
#include "model/Recipe.h"

QString const Mash::LocalisedName = tr("Mash");

bool Mash::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   Mash const & rhs = static_cast<Mash const &>(other);
   // Base class will already have ensured names are equal
   return (
      this->m_grainTemp_c           == rhs.m_grainTemp_c           &&
      this->m_tunTemp_c             == rhs.m_tunTemp_c             &&
      this->m_spargeTemp_c          == rhs.m_spargeTemp_c          &&
      this->m_ph                    == rhs.m_ph                    &&
      this->m_mashTunWeight_kg          == rhs.m_mashTunWeight_kg          &&
      this->m_mashTunSpecificHeat_calGC == rhs.m_mashTunSpecificHeat_calGC
      // .:TBD:. Should we check MashSteps too?
   );
}

ObjectStore & Mash::getObjectStoreTypedInstance() const {
   return ObjectStoreTyped<Mash>::getInstance();
}

TypeLookup const Mash::typeLookup {
   "Mash",
   {
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Mash::equipAdjust          , Mash::m_equipAdjust          ,           NonPhysicalQuantity::Bool                ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Mash::grainTemp_c          , Mash::m_grainTemp_c          , Measurement::PhysicalQuantity::Temperature         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Mash::notes                , Mash::m_notes                ,           NonPhysicalQuantity::String              ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Mash::ph                   , Mash::m_ph                   , Measurement::PhysicalQuantity::Acidity             ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Mash::spargeTemp_c         , Mash::m_spargeTemp_c         , Measurement::PhysicalQuantity::Temperature         ),
//      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Mash::totalMashWater_l     , Mash::m_totalMashWater_l     ), // Calculated, not stored
//      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Mash::totalTime            , Mash::m_totalTime            ), // Calculated, not stored
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Mash::mashTunSpecificHeat_calGC, Mash::m_mashTunSpecificHeat_calGC, Measurement::PhysicalQuantity::SpecificHeatCapacity),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Mash::tunTemp_c            , Mash::m_tunTemp_c            , Measurement::PhysicalQuantity::Temperature         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Mash::mashTunWeight_kg     , Mash::m_mashTunWeight_kg     , Measurement::PhysicalQuantity::Mass                ),

      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::Mash::mashSteps        , Mash::mashSteps        ),
      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::Mash::mashStepsDowncast, Mash::mashStepsDowncast),
   },
   // Parent class lookup
   {&NamedEntity::typeLookup}
};

//==================================================== CONSTRUCTORS ====================================================

Mash::Mash(QString name) :
   NamedEntity{name, true},
   StepOwnerBase<Mash, MashStep>{},
   m_grainTemp_c              {0.0 },
   m_notes                    {""  },
   m_tunTemp_c                {0.0 },
   m_spargeTemp_c             {0.0 },
   m_ph                       {0.0 },
   m_mashTunWeight_kg         {0.0 },
   m_mashTunSpecificHeat_calGC{0.0 },
   m_equipAdjust              {true} {
   return;
}

Mash::Mash(NamedParameterBundle const & namedParameterBundle) :
   NamedEntity                {namedParameterBundle},
   StepOwnerBase<Mash, MashStep>{},
   SET_REGULAR_FROM_NPB (m_grainTemp_c              , namedParameterBundle, PropertyNames::Mash::grainTemp_c              ),
   SET_REGULAR_FROM_NPB (m_notes                    , namedParameterBundle, PropertyNames::Mash::notes                    ),
   SET_REGULAR_FROM_NPB (m_tunTemp_c                , namedParameterBundle, PropertyNames::Mash::tunTemp_c                ),
   SET_REGULAR_FROM_NPB (m_spargeTemp_c             , namedParameterBundle, PropertyNames::Mash::spargeTemp_c             ),
   SET_REGULAR_FROM_NPB (m_ph                       , namedParameterBundle, PropertyNames::Mash::ph                       ),
   SET_REGULAR_FROM_NPB (m_mashTunWeight_kg         , namedParameterBundle, PropertyNames::Mash::mashTunWeight_kg         ),
   SET_REGULAR_FROM_NPB (m_mashTunSpecificHeat_calGC, namedParameterBundle, PropertyNames::Mash::mashTunSpecificHeat_calGC),
   SET_REGULAR_FROM_NPB (m_equipAdjust              , namedParameterBundle, PropertyNames::Mash::equipAdjust              ) {
   return;
}

Mash::Mash(Mash const & other) :
   NamedEntity{other},
   StepOwnerBase<Mash, MashStep>{other},
   m_grainTemp_c              {other.m_grainTemp_c              },
   m_notes                    {other.m_notes                    },
   m_tunTemp_c                {other.m_tunTemp_c                },
   m_spargeTemp_c             {other.m_spargeTemp_c             },
   m_ph                       {other.m_ph                       },
   m_mashTunWeight_kg         {other.m_mashTunWeight_kg         },
   m_mashTunSpecificHeat_calGC{other.m_mashTunSpecificHeat_calGC},
   m_equipAdjust              {other.m_equipAdjust              } {
   return;
}

Mash::~Mash() = default;

///void Mash::connectSignals() {
///   for (auto mash : ObjectStoreTyped<Mash>::getInstance().getAllRaw()) {
///      for (auto mashStep : mash->mashSteps()) {
///         connect(mashStep.get(), &NamedEntity::changed, mash, &Mash::acceptStepChange);
///      }
///   }
///   return;
///}
///
///void Mash::setKey(int key) {
///   this->doSetKey(key);
///   return;
///}

//============================================= "GETTER" MEMBER FUNCTIONS ==============================================
double                Mash::grainTemp_c              () const { return this->m_grainTemp_c              ; }
QString               Mash::notes                    () const { return this->m_notes                    ; }
std::optional<double> Mash::tunTemp_c                () const { return this->m_tunTemp_c                ; }
std::optional<double> Mash::spargeTemp_c             () const { return this->m_spargeTemp_c             ; }
std::optional<double> Mash::ph                       () const { return this->m_ph                       ; }
std::optional<double> Mash::mashTunWeight_kg         () const { return this->m_mashTunWeight_kg         ; }
std::optional<double> Mash::mashTunSpecificHeat_calGC() const { return this->m_mashTunSpecificHeat_calGC; }
bool                  Mash::equipAdjust              () const { return this->m_equipAdjust              ; }

//============================================= "SETTER" MEMBER FUNCTIONS ==============================================
void Mash::setGrainTemp_c              (double                const   val) { this->setAndNotify(PropertyNames::Mash::grainTemp_c              , this->m_grainTemp_c              , val                                              ); return; }
void Mash::setNotes                    (QString               const & val) { this->setAndNotify(PropertyNames::Mash::notes                    , this->m_notes                    , val                                              ); return; }
void Mash::setTunTemp_c                (std::optional<double> const   val) { this->setAndNotify(PropertyNames::Mash::tunTemp_c                , this->m_tunTemp_c                , val                                              ); return; }
void Mash::setSpargeTemp_c             (std::optional<double> const   val) { this->setAndNotify(PropertyNames::Mash::spargeTemp_c             , this->m_spargeTemp_c             , val                                              ); return; }
void Mash::setPh                       (std::optional<double> const   val) { this->setAndNotify(PropertyNames::Mash::ph                       , this->m_ph                       , this->enforceMinAndMax(val, "pH", 0.0, 14.0, 7.0)); return; }
void Mash::setTunWeight_kg             (std::optional<double> const   val) { this->setAndNotify(PropertyNames::Mash::mashTunWeight_kg         , this->m_mashTunWeight_kg         , this->enforceMin(val, "tun weight")              ); return; }
void Mash::setMashTunSpecificHeat_calGC(std::optional<double> const   val) { this->setAndNotify(PropertyNames::Mash::mashTunSpecificHeat_calGC, this->m_mashTunSpecificHeat_calGC, this->enforceMin(val, "specific heat")           ); return; }
void Mash::setEquipAdjust              (bool                  const   val) { this->setAndNotify(PropertyNames::Mash::equipAdjust              , this->m_equipAdjust              , val                                              ); return; }

// === other methods ===
double Mash::totalMashWater_l() {
   double waterAdded_l = 0.0;

   for (auto step : this-> mashSteps()) {
      if (step->isInfusion()) {
         waterAdded_l += step->amount_l();
      }
   }

   return waterAdded_l;
}

double Mash::totalInfusionAmount_l() const {
   double waterAdded_l = 0.0;

   for (auto step :  this->mashSteps()) {
      if (step->isInfusion() && !step->isSparge() ) {
         waterAdded_l += step->amount_l();
      }
   }

   return waterAdded_l;
}

double Mash::totalSpargeAmount_l() const {
   double waterAdded_l = 0.0;

   for (auto step : this-> mashSteps()) {
      if (step->isSparge()) {
         waterAdded_l += step->amount_l();
      }
   }

   return waterAdded_l;
}

double Mash::totalTime() {
   double totalTime = 0.0;
   for (auto step : this-> mashSteps()) {
      totalTime += step->stepTime_min();
   }
   return totalTime;
}

bool Mash::hasSparge() const {
   for (auto step : this-> mashSteps()) {
      if (step->isSparge()) {
         return true;
      }
   }
   return false;
}

///QList<std::shared_ptr<MashStep>> Mash::mashSteps() const {
///   return this->steps();
///}
///
///QList<std::shared_ptr<NamedEntity>> Mash::mashStepsDowncast() const {
///   return this->stepsDowncast();
///}
///
///void Mash::setMashStepsDowncast(QList<std::shared_ptr<NamedEntity>> const & val) {
///   this->setStepsDowncast(val);
///   return;
///}

void Mash::acceptStepChange([[maybe_unused]] QMetaProperty prop,
                            [[maybe_unused]] QVariant      val) {
   MashStep * stepSender = qobject_cast<MashStep*>(sender());
   if (!stepSender) {
      return;
   }

   // If one of our mash steps changed, our calculated properties may also change, so we need to emit some signals
   if (stepSender->ownerId() == this->key()) {
      emit changed(metaProperty(*PropertyNames::Mash::totalMashWater_l), QVariant());
      emit changed(metaProperty(*PropertyNames::Mash::totalTime), QVariant());
   }

   return;
}

///Recipe * Mash::getOwningRecipe() const {
///   return this->doGetOwningRecipe();
//////   return ObjectStoreWrapper::findFirstMatching<Recipe>( [this](Recipe * rec) {return rec->uses(*this);} );
///}

///void Mash::hardDeleteOwnedEntities() {
///   this->doHardDeleteOwnedEntities();
///   return;
//////   // It's the MashStep that stores its Mash ID, so all we need to do is delete our MashSteps then the subsequent
//////   // database delete of this Mash won't hit any foreign key problems.
//////   auto mashSteps = this->mashSteps();
//////   for (auto mashStep : mashSteps) {
//////      ObjectStoreWrapper::hardDelete<MashStep>(*mashStep);
//////   }
//////   return;
///}

// Insert boiler-plate wrapper functions that call down to StepOwnerBase
STEP_OWNER_COMMON_CODE(Mash, mash)
