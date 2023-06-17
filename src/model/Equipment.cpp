/*======================================================================================================================
 * model/Equipment.cpp is part of Brewken, and is copyright the following authors 2009-2023:
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
#include "model/Equipment.h"

#include "database/ObjectStoreWrapper.h"
#include "HeatCalculations.h"
#include "model/NamedParameterBundle.h"
#include "model/Recipe.h"

QString const Equipment::LocalisedName = tr("Equipment");

bool Equipment::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   Equipment const & rhs = static_cast<Equipment const &>(other);
   // Base class will already have ensured names are equal
   return (
      this->m_boilSize_l            == rhs.m_boilSize_l            &&
      this->m_batchSize_l           == rhs.m_batchSize_l           &&
      this->m_mashTunVolume_l       == rhs.m_mashTunVolume_l           &&
      this->m_mashTunWeight_kg          == rhs.m_mashTunWeight_kg          &&
      this->m_mashTunSpecificHeat_calGC == rhs.m_mashTunSpecificHeat_calGC &&
      this->m_topUpWater_l          == rhs.m_topUpWater_l          &&
      this->m_trubChillerLoss_l     == rhs.m_trubChillerLoss_l     &&
      this->m_evapRate_pctHr        == rhs.m_evapRate_pctHr        &&
      this->m_kettleEvaporationPerHour_l          == rhs.m_kettleEvaporationPerHour_l          &&
      this->m_boilTime_min          == rhs.m_boilTime_min          &&
      this->m_lauterDeadspace_l     == rhs.m_lauterDeadspace_l     &&
      this->m_topUpKettle_l         == rhs.m_topUpKettle_l         &&
      this->m_hopUtilization_pct    == rhs.m_hopUtilization_pct
   );
}

ObjectStore & Equipment::getObjectStoreTypedInstance() const {
   return ObjectStoreTyped<Equipment>::getInstance();
}

TypeLookup const Equipment::typeLookup {
   "Equipment",
   {
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Equipment::batchSize_l          , Equipment::m_batchSize_l          , Measurement::PhysicalQuantity::Volume              ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Equipment::boilingPoint_c       , Equipment::m_boilingPoint_c       , Measurement::PhysicalQuantity::Temperature         ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Equipment::boilSize_l           , Equipment::m_boilSize_l           , Measurement::PhysicalQuantity::Volume              ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Equipment::boilTime_min         , Equipment::m_boilTime_min         , Measurement::PhysicalQuantity::Time                ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Equipment::calcBoilVolume       , Equipment::m_calcBoilVolume       ,           NonPhysicalQuantity::Bool                ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Equipment::kettleEvaporationPerHour_l         , Equipment::m_kettleEvaporationPerHour_l         , Measurement::PhysicalQuantity::Volume              ), // The "per hour" bit is fixed, so we simplify
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Equipment::evapRate_pctHr       , Equipment::m_evapRate_pctHr       ,           NonPhysicalQuantity::Percentage          ), // The "per hour" bit is fixed, so we simplify
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Equipment::mashTunGrainAbsorption_LKg  , Equipment::m_mashTunGrainAbsorption_LKg  ,           NonPhysicalQuantity::Dimensionless       ), // Not really dimensionless...
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Equipment::hopUtilization_pct   , Equipment::m_hopUtilization_pct   ,           NonPhysicalQuantity::Percentage          ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Equipment::lauterDeadspace_l    , Equipment::m_lauterDeadspace_l    , Measurement::PhysicalQuantity::Volume              ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Equipment::kettleNotes                , Equipment::m_kettleNotes                                                                     ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Equipment::topUpKettle_l        , Equipment::m_topUpKettle_l        , Measurement::PhysicalQuantity::Volume              ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Equipment::topUpWater_l         , Equipment::m_topUpWater_l         , Measurement::PhysicalQuantity::Volume              ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Equipment::trubChillerLoss_l    , Equipment::m_trubChillerLoss_l    , Measurement::PhysicalQuantity::Volume              ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Equipment::mashTunSpecificHeat_calGC, Equipment::m_mashTunSpecificHeat_calGC, Measurement::PhysicalQuantity::SpecificHeatCapacity),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Equipment::mashTunVolume_l      , Equipment::m_mashTunVolume_l          , Measurement::PhysicalQuantity::Volume              ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Equipment::mashTunWeight_kg         , Equipment::m_mashTunWeight_kg         , Measurement::PhysicalQuantity::Mass                ),
   },
   // Parent class lookup
   &NamedEntity::typeLookup
};

//=============================CONSTRUCTORS=====================================
Equipment::Equipment(QString name) :
   NamedEntity            {name, true},
   m_boilSize_l           {22.927},
   m_batchSize_l          {18.927},
   m_mashTunVolume_l          {0.0   },
   m_mashTunWeight_kg         {0.0   },
   m_mashTunSpecificHeat_calGC{0.0   },
   m_topUpWater_l         {0.0   },
   m_trubChillerLoss_l    {1.0   },
   m_evapRate_pctHr       {0.0   },
   m_kettleEvaporationPerHour_l         {4.0   },
   m_boilTime_min         {60.0  },
   m_calcBoilVolume       {true  },
   m_lauterDeadspace_l    {0.0   },
   m_topUpKettle_l        {0.0   },
   m_hopUtilization_pct   {100.0 },
   m_kettleNotes                {""    },
   m_mashTunGrainAbsorption_LKg  {1.086 },
   m_boilingPoint_c       {100.0 } {
   return;
}

// The default values below are set for the following fields that are not part of BeerXML 1.0 standard and so will
// not be present in BeerXML files (unless we wrote them) but will be present in the database:
//    - kettleEvaporationPerHour_l
//    - mashTunGrainAbsorption_LKg
//    - boilingPoint_c
//
Equipment::Equipment(NamedParameterBundle const & namedParameterBundle) :
   NamedEntity{namedParameterBundle},
   m_boilSize_l           {namedParameterBundle.val<double >(PropertyNames::Equipment::boilSize_l                )},
   m_batchSize_l          {namedParameterBundle.val<double >(PropertyNames::Equipment::batchSize_l               )},
   m_mashTunVolume_l          {namedParameterBundle.val<double >(PropertyNames::Equipment::mashTunVolume_l               )},
   m_mashTunWeight_kg         {namedParameterBundle.val<double >(PropertyNames::Equipment::mashTunWeight_kg              )},
   m_mashTunSpecificHeat_calGC{namedParameterBundle.val<double >(PropertyNames::Equipment::mashTunSpecificHeat_calGC     )},
   m_topUpWater_l         {namedParameterBundle.val<double >(PropertyNames::Equipment::topUpWater_l              )},
   m_trubChillerLoss_l    {namedParameterBundle.val<double >(PropertyNames::Equipment::trubChillerLoss_l         )},
   m_evapRate_pctHr       {namedParameterBundle.val<double >(PropertyNames::Equipment::evapRate_pctHr            )},
   m_kettleEvaporationPerHour_l         {namedParameterBundle.val<double >(PropertyNames::Equipment::kettleEvaporationPerHour_l,        4.0  )},
   m_boilTime_min         {namedParameterBundle.val<double >(PropertyNames::Equipment::boilTime_min              )},
   m_calcBoilVolume       {namedParameterBundle.val<bool   >(PropertyNames::Equipment::calcBoilVolume            )},
   m_lauterDeadspace_l    {namedParameterBundle.val<double >(PropertyNames::Equipment::lauterDeadspace_l         )},
   m_topUpKettle_l        {namedParameterBundle.val<double >(PropertyNames::Equipment::topUpKettle_l             )},
   m_hopUtilization_pct   {namedParameterBundle.val<double >(PropertyNames::Equipment::hopUtilization_pct        )},
   m_kettleNotes                {namedParameterBundle.val<QString>(PropertyNames::Equipment::kettleNotes                     )},
   m_mashTunGrainAbsorption_LKg  {namedParameterBundle.val<double >(PropertyNames::Equipment::mashTunGrainAbsorption_LKg, 1.086)},
   m_boilingPoint_c       {namedParameterBundle.val<double >(PropertyNames::Equipment::boilingPoint_c,      100.0)} {
   return;
}

Equipment::Equipment(Equipment const & other) :
   NamedEntity            {other                        },
   m_boilSize_l           {other.m_boilSize_l           },
   m_batchSize_l          {other.m_batchSize_l          },
   m_mashTunVolume_l          {other.m_mashTunVolume_l          },
   m_mashTunWeight_kg         {other.m_mashTunWeight_kg         },
   m_mashTunSpecificHeat_calGC{other.m_mashTunSpecificHeat_calGC},
   m_topUpWater_l         {other.m_topUpWater_l         },
   m_trubChillerLoss_l    {other.m_trubChillerLoss_l    },
   m_evapRate_pctHr       {other.m_evapRate_pctHr       },
   m_kettleEvaporationPerHour_l         {other.m_kettleEvaporationPerHour_l         },
   m_boilTime_min         {other.m_boilTime_min         },
   m_calcBoilVolume       {other.m_calcBoilVolume       },
   m_lauterDeadspace_l    {other.m_lauterDeadspace_l    },
   m_topUpKettle_l        {other.m_topUpKettle_l        },
   m_hopUtilization_pct   {other.m_hopUtilization_pct   },
   m_kettleNotes                {other.m_kettleNotes                },
   m_mashTunGrainAbsorption_LKg  {other.m_mashTunGrainAbsorption_LKg  },
   m_boilingPoint_c       {other.m_boilingPoint_c       } {
   return;
}

Equipment::~Equipment() = default;

//============================================= "GETTER" MEMBER FUNCTIONS ==============================================

QString Equipment::kettleNotes                () const { return m_kettleNotes                ; }
bool    Equipment::calcBoilVolume       () const { return m_calcBoilVolume       ; }
double  Equipment::boilSize_l           () const { return m_boilSize_l           ; }
double  Equipment::batchSize_l          () const { return m_batchSize_l          ; }
double  Equipment::mashTunVolume_l          () const { return m_mashTunVolume_l          ; }
double  Equipment::mashTunWeight_kg         () const { return m_mashTunWeight_kg         ; }
double  Equipment::mashTunSpecificHeat_calGC() const { return m_mashTunSpecificHeat_calGC; }
double  Equipment::topUpWater_l         () const { return m_topUpWater_l         ; }
double  Equipment::trubChillerLoss_l    () const { return m_trubChillerLoss_l    ; }
double  Equipment::evapRate_pctHr       () const { return m_evapRate_pctHr       ; }
double  Equipment::kettleEvaporationPerHour_l         () const { return m_kettleEvaporationPerHour_l         ; }
double  Equipment::boilTime_min         () const { return m_boilTime_min         ; }
double  Equipment::lauterDeadspace_l    () const { return m_lauterDeadspace_l    ; }
double  Equipment::topUpKettle_l        () const { return m_topUpKettle_l        ; }
double  Equipment::hopUtilization_pct   () const { return m_hopUtilization_pct   ; }
double  Equipment::mashTunGrainAbsorption_LKg  () const { return m_mashTunGrainAbsorption_LKg  ; }
double  Equipment::boilingPoint_c       () const { return m_boilingPoint_c       ; }

//============================================= "SETTER" MEMBER FUNCTIONS ==============================================

// The logic through here is similar to what's in Hop. Unfortunately, the additional signals don't allow quite the
// compactness.
void Equipment::setBoilSize_l(double const val) {
   this->setAndNotify(PropertyNames::Equipment::boilSize_l,
                      this->m_boilSize_l,
                      this->enforceMin(val, "boil size"));
}

void Equipment::setBatchSize_l(double const val) {
   this->setAndNotify(PropertyNames::Equipment::batchSize_l,
                      this->m_batchSize_l,
                      this->enforceMin(val, "batch size"));
   if (this->key() > 0) {
      doCalculations();
   }
}

void Equipment::setMashTunVolume_l(double const val) {
   this->setAndNotify(PropertyNames::Equipment::mashTunVolume_l,
                      this->m_mashTunVolume_l,
                      this->enforceMin(val, "tun volume"));
}

void Equipment::setMashTunWeight_kg(double const val) {
   this->setAndNotify(PropertyNames::Equipment::mashTunWeight_kg,
                      this->m_mashTunWeight_kg,
                      this->enforceMin(val, "tun weight"));
}

void Equipment::setMashTunSpecificHeat_calGC(double const val) {
   this->setAndNotify(PropertyNames::Equipment::mashTunSpecificHeat_calGC,
                      this->m_mashTunSpecificHeat_calGC,
                      this->enforceMin(val, "tun specific heat"));
}

void Equipment::setTopUpWater_l(double const val) {
   this->setAndNotify(PropertyNames::Equipment::topUpWater_l,
                      this->m_topUpWater_l,
                      this->enforceMin(val, "top-up water"));
   if (this->key() > 0) {
      doCalculations();
   }
}

void Equipment::setTrubChillerLoss_l(double const val) {
   this->setAndNotify(PropertyNames::Equipment::trubChillerLoss_l,
                      this->m_trubChillerLoss_l,
                      this->enforceMin(val, "trub chiller loss"));
   if (this->key() > 0) {
      doCalculations();
   }
}

void Equipment::setEvapRate_pctHr(double const val) {
   // NOTE: We never use evapRate_pctHr, but we do use kettleEvaporationPerHour_l. So keep them
   //       synced, and implement the former in terms of the latter.
   this->setKettleEvaporationPerHour_l(val/100.0 * batchSize_l());
   return;
}

void Equipment::setKettleEvaporationPerHour_l(double const val) {
   // NOTE: We never use evapRate_pctHr, but we maintain here anyway.
   // Because both values are stored in the DB, and because we only want to call prepareForPropertyChange() once, we
   // can't use the setAndNotify() helper function
   this->prepareForPropertyChange(PropertyNames::Equipment::kettleEvaporationPerHour_l);
   this->m_kettleEvaporationPerHour_l = this->enforceMin(val, "evap rate");
   this->m_evapRate_pctHr = this->m_kettleEvaporationPerHour_l/batchSize_l() * 100.0; // We don't use it, but keep it current.
   this->propagatePropertyChange(PropertyNames::Equipment::kettleEvaporationPerHour_l);
   this->propagatePropertyChange(PropertyNames::Equipment::evapRate_pctHr);

   // Right now, I am claiming this needs to happen regardless of whether we're yet stored in the database.
   // I could be wrong
   doCalculations();
}

void Equipment::setBoilTime_min(double const val) {
   if (this->setAndNotify(PropertyNames::Equipment::boilTime_min,
                          this->m_boilTime_min,
                          this->enforceMin(val, "boil time"))) {
      doCalculations();
   }
   return;
}

void Equipment::setCalcBoilVolume(bool const val) {
   this->setAndNotify(PropertyNames::Equipment::calcBoilVolume, this->m_calcBoilVolume, val);
   if ( val ) {
      doCalculations();
   }
}

void Equipment::setLauterDeadspace_l(double const val) {
   this->setAndNotify(PropertyNames::Equipment::lauterDeadspace_l, this->m_lauterDeadspace_l, this->enforceMin(val, "deadspace"));
}

void Equipment::setTopUpKettle_l(double const val) {
   this->setAndNotify(PropertyNames::Equipment::topUpKettle_l, this->m_topUpKettle_l, this->enforceMin(val, "top-up kettle"));
}

void Equipment::setHopUtilization_pct(double const val) {
   this->setAndNotify(PropertyNames::Equipment::hopUtilization_pct, this->m_hopUtilization_pct, this->enforceMin(val, "hop utilization"));
}

void Equipment::setKettleNotes( const QString &val ) {
   this->setAndNotify(PropertyNames::Equipment::kettleNotes, this->m_kettleNotes, val);
}

void Equipment::setMashTunGrainAbsorption_LKg(double const val) {
   this->setAndNotify(PropertyNames::Equipment::mashTunGrainAbsorption_LKg, this->m_mashTunGrainAbsorption_LKg, this->enforceMin(val, "absorption"));
}

void Equipment::setBoilingPoint_c(double const val) {
   this->setAndNotify(PropertyNames::Equipment::boilingPoint_c, this->m_boilingPoint_c, this->enforceMin(val, "boiling point of water"));
}


void Equipment::doCalculations() {
   // Only do the calculation if we're asked to.
   if (!this->calcBoilVolume()) {
      return;
   }

   this->setBoilSize_l(this->batchSize_l() -
                       this->topUpWater_l() +
                       this->trubChillerLoss_l() +
                       (this->boilTime_min()/(double)60)*this->kettleEvaporationPerHour_l());
   return;
}

double Equipment::wortEndOfBoil_l( double kettleWort_l ) const {
   //return kettleWort_l * (1 - (boilTime_min/(double)60) * (evapRate_pctHr/(double)100) );

   return kettleWort_l - (boilTime_min()/(double)60)*kettleEvaporationPerHour_l();
}

// Although it's a similar one-liner implementation for many subclasses of NamedEntity, we can't push the
// implementation of this down to the base class, as Recipe::uses() is templated and won't work with type erasure.
Recipe * Equipment::getOwningRecipe() {
   return ObjectStoreWrapper::findFirstMatching<Recipe>( [this](Recipe * rec) {return rec->uses(*this);} );
}
