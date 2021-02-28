/**
 * mash.cpp is part of Brewken, and is copyright the following authors 2009-2020:
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
#include <QVector>
#include "model/Mashstep.h"
#include "Brewken.h"
#include "database/Database.h"
#include <QDomElement>
#include <QDomText>
#include <QObject>

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
   );
}


QString Mash::classNameStr()
{
   static const QString name("Mash");
   return name;
}

Mash::Mash(Brewken::DBTable table, int key)
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
   : NamedEntity(Brewken::MASHTABLE, -1, name, true),
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

Mash::Mash(Brewken::DBTable table, int key, QSqlRecord rec)
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

QList<MashStep*> Mash::mashSteps() const
{
   return Database::instance().mashSteps(this);
}

void Mash::acceptMashStepChange(QMetaProperty prop, QVariant /*val*/)
{
   int i;
   MashStep* stepSender = qobject_cast<MashStep*>(sender());
   if( stepSender == 0 )
      return;

   // If one of our mash steps changed, our calculated properties
   // may also change, so we need to emit some signals.
   i = mashSteps().indexOf(stepSender);
   if( i >= 0 )
   {
      emit changed(metaProperty("totalMashWater_l"), QVariant());
      emit changed(metaProperty("totalTime"), QVariant());
   }
}

MashStep * Mash::addMashStep(MashStep * mashStep) {
   mashStep->setMash(this);
   mashStep->insertInDatabase();
   return mashStep;
}

MashStep * Mash::removeMashStep(MashStep * mashStep) {
   Database::instance().removeFrom(this, mashStep);
   return mashStep;
}


int Mash::insertInDatabase() {
   return Database::instance().insertMash(this);
}

void Mash::removeFromDatabase() {
   Database::instance().remove(this);
}
