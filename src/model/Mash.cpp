/**
 * model/Mash.cpp is part of Brewken, and is copyright the following authors 2009-2021:
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
 */
#include "model/Mash.h"

#include <iostream>
#include <string>

#include <QObject>

#include "model/MashStep.h"
#include "Brewken.h"
#include "database/Database.h"
#include "database/TableSchemaConst.h"
#include "database/MashSchema.h"

bool Mash::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   Mash const & rhs = static_cast<Mash const &>(other);
   // Base class will already have ensured names are equal
   return (
      this->m_grainTemp_c           == rhs.m_grainTemp_c           &&
      this->m_tunTemp_c             == rhs.m_tunTemp_c             &&
      this->m_spargeTemp_c          == rhs.m_spargeTemp_c          &&
      this->m_ph                    == rhs.m_ph                    &&
      this->m_tunWeight_kg          == rhs.m_tunWeight_kg          &&
      this->m_tunSpecificHeat_calGC == rhs.m_tunSpecificHeat_calGC
      // .:TBD:. Should we check MashSteps too?
   );
}

DbRecords & Mash::getDbNamedEntityRecordsInstance() const {
   return DbNamedEntityRecords<Mash>::getInstance();
}

QString Mash::classNameStr()
{
   static const QString name("Mash");
   return name;
}

Mash::Mash(DatabaseConstants::DbTableId table, int key)
   : NamedEntity(table, key, QString(), true),
     m_grainTemp_c(0.0),
     m_notes(QString()),
     m_tunTemp_c(0.0),
     m_spargeTemp_c(0.0),
     m_ph(0.0),
     m_tunWeight_kg(0.0),
     m_tunSpecificHeat_calGC(0.0),
     m_equipAdjust(true),
     m_cacheOnly(false)
{
}

Mash::Mash(QString name, bool cache)
   : NamedEntity(DatabaseConstants::MASHTABLE, -1, name, true),
     m_grainTemp_c(0.0),
     m_notes(QString()),
     m_tunTemp_c(0.0),
     m_spargeTemp_c(0.0),
     m_ph(0.0),
     m_tunWeight_kg(0.0),
     m_tunSpecificHeat_calGC(0.0),
     m_equipAdjust(true),
     m_cacheOnly(cache)
{
}

Mash::Mash(Mash const & other) :
   NamedEntity{other},
   m_grainTemp_c          {other.m_grainTemp_c          },
   m_notes                {other.m_notes                },
   m_tunTemp_c            {other.m_tunTemp_c            },
   m_spargeTemp_c         {other.m_spargeTemp_c         },
   m_ph                   {other.m_ph                   },
   m_tunWeight_kg         {other.m_tunWeight_kg         },
   m_tunSpecificHeat_calGC{other.m_tunSpecificHeat_calGC},
   m_equipAdjust          {other.m_equipAdjust          },
   m_cacheOnly            {other.m_cacheOnly            } {

   // Deep copy of MashSteps
   for (int mashStepId : other.mashStepIds) {
      // Make and store a copy of the current MashStep object we're looking at in the other Mash
      auto mashStepToAdd = DbNamedEntityRecords<MashStep>::getInstance().insertCopyOf(mashStepId);
      // Store the ID of the copy in our recipe
      this->mashStepIds.append(mashStepToAdd->key());
      // Connect signals so that we are notified when there are changes to the MashStep we just added to
      // our Mash.
      connect(mashStepToAdd.get(), &NamedEntity::changed, this, &Mash::acceptMashStepChange);
   }

   return;
}


Mash::Mash(NamedParameterBundle & namedParameterBundle) :
   NamedEntity{namedParameterBundle, DatabaseConstants::MASHTABLE},
     m_grainTemp_c          {namedParameterBundle(PropertyNames::Mash::grainTemp_c          ).toDouble()},
     m_notes                {namedParameterBundle(PropertyNames::Mash::notes                ).toString()},
     m_tunTemp_c            {namedParameterBundle(PropertyNames::Mash::tunTemp_c            ).toDouble()},
     m_spargeTemp_c         {namedParameterBundle(PropertyNames::Mash::spargeTemp_c         ).toDouble()},
     m_ph                   {namedParameterBundle(PropertyNames::Mash::ph                   ).toDouble()},
     m_tunWeight_kg         {namedParameterBundle(PropertyNames::Mash::tunWeight_kg         ).toDouble()},
     m_tunSpecificHeat_calGC{namedParameterBundle(PropertyNames::Mash::tunSpecificHeat_calGC).toDouble()},
     m_equipAdjust          {namedParameterBundle(PropertyNames::Mash::equipAdjust          ).toBool()},
     m_cacheOnly{false} {
   return;
}

Mash::Mash(DatabaseConstants::DbTableId table, int key, QSqlRecord rec)
   : NamedEntity(table, key, rec.value(kcolName).toString(), rec.value(kcolDisplay).toBool()),
     m_grainTemp_c(rec.value(kcolMashGrainTemp).toDouble()),
     m_notes(rec.value(kcolNotes).toString()),
     m_tunTemp_c(rec.value(kcolMashTunTemp).toDouble()),
     m_spargeTemp_c(rec.value(kcolMashSpargeTemp).toDouble()),
     m_ph(rec.value(kcolPH).toDouble()),
     m_tunWeight_kg(rec.value(kcolMashTunWeight).toDouble()),
     m_tunSpecificHeat_calGC(rec.value(kcolMashTunSpecHeat).toDouble()),
     m_equipAdjust(rec.value(kcolMashEquipAdjust).toBool()),
     m_cacheOnly(false)
{
}

void Mash::connectSignals() {
   for (auto mash : DbNamedEntityRecords<Mash>::getInstance().getAllRaw()) {
      for (auto mashStep : mash->mashSteps()) {
         connect(mashStep, SIGNAL(changed(QMetaProperty,QVariant)), mash, SLOT(acceptMashStepChange(QMetaProperty,QVariant)) );
      }
   }
   return;
}

void Mash::setGrainTemp_c( double var )
{
   m_grainTemp_c = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Mash::grainTemp_c, var);
   }
}

void Mash::setNotes( const QString& var )
{
   m_notes = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Mash::notes, var);
   }
}

void Mash::setTunTemp_c( double var )
{
   m_tunTemp_c = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Mash::tunTemp_c, var);
   }
}

void Mash::setSpargeTemp_c( double var )
{
   m_spargeTemp_c = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Mash::spargeTemp_c, var);
   }
}

void Mash::setEquipAdjust( bool var )
{
   m_equipAdjust = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Mash::equipAdjust, var);
   }
}

void Mash::setPh( double var )
{
   if( var < 0.0 || var > 14.0 )
   {
      qWarning() << QString("Mash: 0 < pH < 14: %1").arg(var);
      return;
   }
   else
   {
      m_ph = var;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Mash::ph, var);
      }
   }
}

void Mash::setTunWeight_kg( double var )
{
   if( var < 0.0 )
   {
      qWarning() << QString("Mash: tun weight < 0: %1").arg(var);
      return;
   }
   else
   {
      m_tunWeight_kg = var;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Mash::tunWeight_kg, var);
      }
   }
}

void Mash::setTunSpecificHeat_calGC( double var )
{
   if( var < 0.0 )
   {
      qWarning() << QString("Mash: sp heat < 0: %1").arg(var);
      return;
   }
   else
   {
      m_tunSpecificHeat_calGC = var;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Mash::tunSpecificHeat_calGC, var);
      }
   }
}

void Mash::setMashStepIds(QList<int> ids) {
   this->mashStepIds = ids.toVector();
   return;
}

void Mash::swapMashSteps(MashStep const & ms1, MashStep const & ms2) {
   int indexOf1 = this->mashStepIds.indexOf(ms1.key());
   int indexOf2 = this->mashStepIds.indexOf(ms2.key());

   // We can't swap them if we can't find both of them
   // There's no point swapping them if they're the same
   if (-1 == indexOf1 || -1 == indexOf2 || indexOf1 == indexOf2) {
      return;
   }

   // As of Qt 5.14 we could write:
   //    this->mashStepIds.swapItemsAt(indexOf1, indexOf2);
   // However, we still need to support slightly older versions of Qt (5.12 in particular), hence the more cumbersome
   // way here.
   std::swap(this->mashStepIds[indexOf1], this->mashStepIds[indexOf2]);

   ObjectStoreWrapper::updateProperty(*this, PropertyNames::Mash::mashStepIds);
   return;
}


void Mash::removeAllMashSteps()
{
   int i, size;
   QList<MashStep*> tmpSteps = mashSteps();
   size = tmpSteps.size();
   for( i = 0; i < size; ++i )
      Database::instance().removeFrom(this, tmpSteps[i]);
   emit mashStepsChanged();
}

void Mash::setCacheOnly(bool cache) { m_cacheOnly = cache; }

//============================="GET" METHODS====================================
QString Mash::notes() const { return m_notes; }

double Mash::grainTemp_c() const { return m_grainTemp_c; }

double Mash::tunTemp_c() const { return m_tunTemp_c; }

double Mash::spargeTemp_c() const { return m_spargeTemp_c; }

double Mash::ph() const { return m_ph; }

double Mash::tunWeight_kg() const { return m_tunWeight_kg; }

double Mash::tunSpecificHeat_calGC() const { return m_tunSpecificHeat_calGC; }

bool Mash::equipAdjust() const { return m_equipAdjust; }

bool Mash::cacheOnly() const { return m_cacheOnly; }

// === other methods ===
double Mash::totalMashWater_l()
{
   int i, size;
   double waterAdded_l = 0.0;
   QList<MashStep*> steps = mashSteps();
   MashStep* step;

   size = steps.size();
   for( i = 0; i < size; ++i ) {
      step = steps[i];

      if( step->isInfusion() )
         waterAdded_l += step->infuseAmount_l();
   }

   return waterAdded_l;
}

double Mash::totalInfusionAmount_l() const
{
   double waterAdded_l = 0.0;

   foreach( MashStep* i, mashSteps() ) {
      if( i->isInfusion() && ! i->isSparge() )
         waterAdded_l += i->infuseAmount_l();
   }

   return waterAdded_l;
}

double Mash::totalSpargeAmount_l() const
{
   double waterAdded_l = 0.0;

   foreach( MashStep* i, mashSteps() ) {
      if( i->isSparge() )
         waterAdded_l += i->infuseAmount_l();
   }

   return waterAdded_l;
}

double Mash::totalTime()
{
   int i, size;
   double totalTime = 0.0;
   QList<MashStep*> steps = mashSteps();
   MashStep* mstep;

   size = steps.size();
   for( i = 0; i < size; ++i )
   {
      mstep = steps[i];
      totalTime += mstep->stepTime_min();
   }
   return totalTime;
}

bool Mash::hasSparge() const
{
   foreach( MashStep* ms, mashSteps() ) {
      if ( ms->isSparge() ) {
         return true;
      }
   }

   return false;
}

QList<int> Mash::getMashStepIds() const {
   return this->mashStepIds.toList();
}

QList<MashStep*> Mash::mashSteps() const {
   return DbNamedEntityRecords<MashStep>::getInstance().getByIdsRaw(this->mashStepIds);
}

void Mash::acceptMashStepChange(QMetaProperty prop, QVariant /*val*/) {
   MashStep* stepSender = qobject_cast<MashStep*>(sender());
   if( stepSender == 0 )
      return;

   // If one of our mash steps changed, our calculated properties
   // may also change, so we need to emit some signals.
   int i = mashSteps().indexOf(stepSender);
   if( i >= 0 ) {
      emit changed(metaProperty("totalMashWater_l"), QVariant());
      emit changed(metaProperty("totalTime"), QVariant());
   }
}

MashStep * Mash::addMashStep(MashStep * mashStep) {
   this->mashStepIds.append(mashStep->key());
   ObjectStoreWrapper::updateProperty(*this, PropertyNames::Mash::mashStepIds);
   return mashStep;
}

MashStep * Mash::removeMashStep(MashStep * mashStep) {
   int indexOfStep = this->mashStepIds.indexOf(mashStep->key());
   if (indexOfStep < 0 ) {
      // This shouldn't happen, but it doesn't inherently break anything, so just log a warning and carry on
      qWarning() <<
         Q_FUNC_INFO << "Tried to remove MashStep #" << mashStep->key() << " (from Mash #" << this->key() <<
         ") but couldn't find it";
      return mashStep;
   }

   this->mashStepIds.removeAt(indexOfStep);
   ObjectStoreWrapper::updateProperty(*this, PropertyNames::Mash::mashStepIds);
   return mashStep;
}
