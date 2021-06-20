/**
 * model/Equipment.cpp is part of Brewken, and is copyright the following authors 2009-2021:
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
#include "model/Equipment.h"

#include <QDomElement>
#include <QDomText>
#include <QObject>
#include <QVector>

#include "Brewken.h"
#include "database/ObjectStoreWrapper.h"
#include "HeatCalculations.h"


bool Equipment::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   Equipment const & rhs = static_cast<Equipment const &>(other);
   // Base class will already have ensured names are equal
   return (
      this->m_boilSize_l            == rhs.m_boilSize_l            &&
      this->m_batchSize_l           == rhs.m_batchSize_l           &&
      this->m_tunVolume_l           == rhs.m_tunVolume_l           &&
      this->m_tunWeight_kg          == rhs.m_tunWeight_kg          &&
      this->m_tunSpecificHeat_calGC == rhs.m_tunSpecificHeat_calGC &&
      this->m_topUpWater_l          == rhs.m_topUpWater_l          &&
      this->m_trubChillerLoss_l     == rhs.m_trubChillerLoss_l     &&
      this->m_evapRate_pctHr        == rhs.m_evapRate_pctHr        &&
      this->m_evapRate_lHr          == rhs.m_evapRate_lHr          &&
      this->m_boilTime_min          == rhs.m_boilTime_min          &&
      this->m_lauterDeadspace_l     == rhs.m_lauterDeadspace_l     &&
      this->m_topUpKettle_l         == rhs.m_topUpKettle_l         &&
      this->m_hopUtilization_pct    == rhs.m_hopUtilization_pct
   );
}

ObjectStore & Equipment::getObjectStoreTypedInstance() const {
   return ObjectStoreTyped<Equipment>::getInstance();
}


//=============================CONSTRUCTORS=====================================
Equipment::Equipment(QString t_name, bool cacheOnly) :
   NamedEntity(-1, cacheOnly, t_name, true),
   m_boilSize_l(22.927),
   m_batchSize_l(18.927),
   m_tunVolume_l(0.0),
   m_tunWeight_kg(0.0),
   m_tunSpecificHeat_calGC(0.0),
   m_topUpWater_l(0.0),
   m_trubChillerLoss_l(1.0),
   m_evapRate_pctHr(0.0),
   m_evapRate_lHr(4.0),
   m_boilTime_min(60.0),
   m_calcBoilVolume(true),
   m_lauterDeadspace_l(0.0),
   m_topUpKettle_l(0.0),
   m_hopUtilization_pct(100.0),
   m_notes(QString()),
   m_grainAbsorption_LKg(1.086),
   m_boilingPoint_c(100.0) {
   return;
}

Equipment::Equipment(NamedParameterBundle const & namedParameterBundle) :
   NamedEntity{namedParameterBundle},
   m_boilSize_l           {namedParameterBundle(PropertyNames::Equipment::boilSize_l           ).toDouble()},
   m_batchSize_l          {namedParameterBundle(PropertyNames::Equipment::batchSize_l          ).toDouble()},
   m_tunVolume_l          {namedParameterBundle(PropertyNames::Equipment::tunVolume_l          ).toDouble()},
   m_tunWeight_kg         {namedParameterBundle(PropertyNames::Equipment::tunWeight_kg         ).toDouble()},
   m_tunSpecificHeat_calGC{namedParameterBundle(PropertyNames::Equipment::tunSpecificHeat_calGC).toDouble()},
   m_topUpWater_l         {namedParameterBundle(PropertyNames::Equipment::topUpWater_l         ).toDouble()},
   m_trubChillerLoss_l    {namedParameterBundle(PropertyNames::Equipment::trubChillerLoss_l    ).toDouble()},
   m_evapRate_pctHr       {namedParameterBundle(PropertyNames::Equipment::evapRate_pctHr       ).toDouble()},
   m_evapRate_lHr         {namedParameterBundle(PropertyNames::Equipment::evapRate_lHr         ).toDouble()},
   m_boilTime_min         {namedParameterBundle(PropertyNames::Equipment::boilTime_min         ).toDouble()},
   m_calcBoilVolume       {namedParameterBundle(PropertyNames::Equipment::calcBoilVolume       ).toBool()},
   m_lauterDeadspace_l    {namedParameterBundle(PropertyNames::Equipment::lauterDeadspace_l    ).toDouble()},
   m_topUpKettle_l        {namedParameterBundle(PropertyNames::Equipment::topUpKettle_l        ).toDouble()},
   m_hopUtilization_pct   {namedParameterBundle(PropertyNames::Equipment::hopUtilization_pct   ).toDouble()},
   m_notes                {namedParameterBundle(PropertyNames::Equipment::notes                ).toString()},
   m_grainAbsorption_LKg  {namedParameterBundle(PropertyNames::Equipment::grainAbsorption_LKg  ).toDouble()},
   m_boilingPoint_c       {namedParameterBundle(PropertyNames::Equipment::boilingPoint_c       ).toDouble()} {
   return;
}

Equipment::Equipment(Equipment const & other) :
   NamedEntity            {other                        },
   m_boilSize_l           {other.m_boilSize_l           },
   m_batchSize_l          {other.m_batchSize_l          },
   m_tunVolume_l          {other.m_tunVolume_l          },
   m_tunWeight_kg         {other.m_tunWeight_kg         },
   m_tunSpecificHeat_calGC{other.m_tunSpecificHeat_calGC},
   m_topUpWater_l         {other.m_topUpWater_l         },
   m_trubChillerLoss_l    {other.m_trubChillerLoss_l    },
   m_evapRate_pctHr       {other.m_evapRate_pctHr       },
   m_evapRate_lHr         {other.m_evapRate_lHr         },
   m_boilTime_min         {other.m_boilTime_min         },
   m_calcBoilVolume       {other.m_calcBoilVolume       },
   m_lauterDeadspace_l    {other.m_lauterDeadspace_l    },
   m_topUpKettle_l        {other.m_topUpKettle_l        },
   m_hopUtilization_pct   {other.m_hopUtilization_pct   },
   m_notes                {other.m_notes                },
   m_grainAbsorption_LKg  {other.m_grainAbsorption_LKg  },
   m_boilingPoint_c       {other.m_boilingPoint_c       } {
   return;
}

QString Equipment::classNameStr()
{
   static const QString name("Equipment");
   return name;
}

//============================"SET" METHODS=====================================

void Equipment::setBoilSize_l( double var )
{
   if( var < 0.0 )
   {
      qWarning() << QString("Equipment: boil size negative: %1").arg(var);
      return;
   }
   else
   {
      m_boilSize_l = var;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Equipment::boilSize_l, var);
         emit changedBoilSize_l(var);
      }
   }
}

void Equipment::setBatchSize_l( double var )
{
   if( var < 0.0 )
   {
      qWarning() << QString("Equipment: batch size negative: %1").arg(var);
      return;
   }
   else
   {
      m_batchSize_l = var;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Equipment::batchSize_l, var);
         doCalculations();
      }
   }
}

void Equipment::setTunVolume_l( double var )
{
   if( var < 0.0 )
   {
      qWarning() << QString("Equipment: tun volume negative: %1").arg(var);
      return;
   }
   else
   {
      m_tunVolume_l = var;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Equipment::tunVolume_l, var);
      }
   }
}

void Equipment::setTunWeight_kg( double var )
{
   if( var < 0.0 )
   {
      qWarning() << QString("Equipment: tun weight negative: %1").arg(var);
      return;
   }
   else
   {
      m_tunWeight_kg = var;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Equipment::tunWeight_kg, var);
      }
   }
}

void Equipment::setTunSpecificHeat_calGC( double var )
{
   if( var < 0.0 )
   {
      qWarning() << QString("Equipment: tun sp heat negative: %1").arg(var);
      return;
   }
   else
   {
      m_tunSpecificHeat_calGC = var;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Equipment::tunSpecificHeat_calGC, var);
      }
   }
}

void Equipment::setTopUpWater_l( double var )
{
   if( var < 0.0 )
   {
      qWarning() << QString("Equipment: top up water negative: %1").arg(var);
      return;
   }
   else
   {
      m_topUpWater_l = var;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Equipment::topUpWater_l,var);
         doCalculations();
      }
   }
}

void Equipment::setTrubChillerLoss_l( double var )
{
   if( var < 0.0 )
   {
      qWarning() << QString("Equipment: trub chiller loss negative: %1").arg(var);
      return;
   }
   else
   {
      m_trubChillerLoss_l = var;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Equipment::trubChillerLoss_l, var);
         doCalculations();
      }
   }
}

void Equipment::setEvapRate_pctHr( double var )
{
   if( var < 0.0 || var > 100.0)
   {
      qWarning() << QString("Equipment: 0 < evap rate < 100: %1").arg(var);
      return;
   }
   else
   {
      m_evapRate_pctHr = var;
      m_evapRate_lHr = var/100.0 * m_batchSize_l;

      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Equipment::evapRate_pctHr, var);
         setEasy(PropertyNames::Equipment::evapRate_lHr, var/100.0 * batchSize_l() ); // We always use this one, so set it.
      }
      // Right now, I am claiming this needs to happen regardless m_cacheOnly.
      // I could be wrong
      doCalculations();
   }
}

void Equipment::setEvapRate_lHr( double var )
{
   if( var < 0.0 )
   {
      qWarning() << QString("Equipment: evap rate negative: %1").arg(var);
      return;
   }
   else
   {
      m_evapRate_lHr = var;
      m_evapRate_pctHr = var/batchSize_l() * 100.0;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Equipment::evapRate_lHr, var);
         setEasy(PropertyNames::Equipment::evapRate_pctHr, var/batchSize_l() * 100.0 ); // We don't use it, but keep it current.
      }
      doCalculations();
   }
}

void Equipment::setBoilTime_min( double var )
{
   if( var < 0.0 )
   {
      qWarning() << QString("Equipment: boil time negative: %1").arg(var);
      return;
   }
   else
   {
      m_boilTime_min = var;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Equipment::boilTime_min, var);
         emit changedBoilTime_min(var);
      }
      doCalculations();
   }
}

void Equipment::setCalcBoilVolume( bool var )
{
   m_calcBoilVolume = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Equipment::calcBoilVolume, var);
   }
   if ( var ) {
      doCalculations();
   }
}

void Equipment::setLauterDeadspace_l( double var )
{
   if( var < 0.0 )
   {
      qWarning() << QString("Equipment: deadspace negative: %1").arg(var);
      return;
   }
   else
   {
      m_lauterDeadspace_l = var;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Equipment::lauterDeadspace_l, var);
      }
   }
}

void Equipment::setTopUpKettle_l( double var )
{
   if( var < 0.0 )
   {
      qWarning() << QString("Equipment: top up kettle negative: %1").arg(var);
      return;
   }
   else
   {
      m_topUpKettle_l = var;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Equipment::topUpKettle_l, var);
      }
   }
}

void Equipment::setHopUtilization_pct( double var )
{
   if( var < 0.0 )
   {
      qWarning() << QString("Equipment: 0 < hop utilization: %1").arg(var);
      return;
   }
   else
   {
      m_hopUtilization_pct = var;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Equipment::hopUtilization_pct, var);
      }
   }
}

void Equipment::setNotes( const QString &var )
{
   m_notes = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Equipment::notes, var);
   }
}

void Equipment::setGrainAbsorption_LKg(double var)
{
   if( var < 0.0 )
   {
      qWarning() << QString("Equipment: absorption < 0: %1").arg(var);
      return;
   }
   else
   {
      m_grainAbsorption_LKg = var;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Equipment::grainAbsorption_LKg, var);
      }
   }
}

void Equipment::setBoilingPoint_c(double var)
{
   if ( var < 0.0 )
   {
      qWarning() << QString("Equipment: boiling point of water < 0: %1").arg(var);
      return;
   }
   else
   {
      m_boilingPoint_c = var;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Equipment::boilingPoint_c, var);
      }
   }
}

//============================"GET" METHODS=====================================

QString Equipment::notes() const { return m_notes; }
bool Equipment::calcBoilVolume() const { return m_calcBoilVolume; }
double Equipment::boilSize_l() const { return m_boilSize_l; }
double Equipment::batchSize_l() const { return m_batchSize_l; }
double Equipment::tunVolume_l() const { return m_tunVolume_l; }
double Equipment::tunWeight_kg() const { return m_tunWeight_kg; }
double Equipment::tunSpecificHeat_calGC() const { return m_tunSpecificHeat_calGC; }
double Equipment::topUpWater_l() const { return m_topUpWater_l; }
double Equipment::trubChillerLoss_l() const { return m_trubChillerLoss_l; }
double Equipment::evapRate_pctHr() const { return m_evapRate_pctHr; }
double Equipment::evapRate_lHr() const { return m_evapRate_lHr; }
double Equipment::boilTime_min() const { return m_boilTime_min; }
double Equipment::lauterDeadspace_l() const { return m_lauterDeadspace_l; }
double Equipment::topUpKettle_l() const { return m_topUpKettle_l; }
double Equipment::hopUtilization_pct() const { return m_hopUtilization_pct; }
double Equipment::grainAbsorption_LKg() { return m_grainAbsorption_LKg; }
double Equipment::boilingPoint_c() const { return m_boilingPoint_c; }

void Equipment::doCalculations()
{
   // Only do the calculation if we're asked to.
   if( ! calcBoilVolume() )
      return;

   setBoilSize_l( batchSize_l() - topUpWater_l() + trubChillerLoss_l() + (boilTime_min()/(double)60)*evapRate_lHr());
}

double Equipment::wortEndOfBoil_l( double kettleWort_l ) const
{
   //return kettleWort_l * (1 - (boilTime_min/(double)60) * (evapRate_pctHr/(double)100) );

   return kettleWort_l - (boilTime_min()/(double)60)*evapRate_lHr();
}
