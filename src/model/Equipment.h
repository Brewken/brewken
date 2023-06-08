/*======================================================================================================================
 * model/Equipment.h is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Jeff Bailey <skydvr38@verizon.net>
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
#ifndef MODEL_EQUIPMENT_H
#define MODEL_EQUIPMENT_H
#pragma once

#include <QDomNode>
#include <QSqlRecord>

#include "model/NamedEntity.h"

//======================================================================================================================
//========================================== Start of property name constants ==========================================
// See comment in model/NamedEntity.h
#define AddPropertyName(property) namespace PropertyNames::Equipment { BtStringConst const property{#property}; }
AddPropertyName(batchSize_l          )
AddPropertyName(boilingPoint_c       )
AddPropertyName(boilSize_l           )
AddPropertyName(boilTime_min         )
AddPropertyName(calcBoilVolume       )
AddPropertyName(evapRate_lHr         )
AddPropertyName(evapRate_pctHr       )
AddPropertyName(grainAbsorption_LKg  )
AddPropertyName(hopUtilization_pct   )
AddPropertyName(lauterDeadspace_l    )
AddPropertyName(notes                )
AddPropertyName(topUpKettle_l        )
AddPropertyName(topUpWater_l         )
AddPropertyName(trubChillerLoss_l    )
AddPropertyName(tunSpecificHeat_calGC)
AddPropertyName(tunVolume_l          )
AddPropertyName(tunWeight_kg         )
#undef AddPropertyName
//=========================================== End of property name constants ===========================================
//======================================================================================================================


/*!
 * \class Equipment
 *
 * \brief Model representing a single equipment record.
 */
class Equipment : public NamedEntity {
   Q_OBJECT

public:
   static QString const LocalisedName;

   /**
    * \brief Mapping of names to types for the Qt properties of this class.  See \c NamedEntity::typeLookup for more
    *        info.
    */
   static TypeLookup const typeLookup;

   Equipment(QString t_name = "");
   Equipment(NamedParameterBundle const & namedParameterBundle);
   Equipment(Equipment const & other);

   virtual ~Equipment();

   /**
    * \brief The boil size in liters: the pre-boil volume used in this particular instance for this equipment setup.
    *        Note that this may be a calculated value depending on the calcBoilVolume property.
    */
   Q_PROPERTY(double boilSize_l            READ boilSize_l            WRITE setBoilSize_l            NOTIFY changedBoilSize_l )
   /**
    * \brief The batch size in liters, aka the target volume of the batch at the start of fermentation.
    */
   Q_PROPERTY(double batchSize_l           READ batchSize_l           WRITE setBatchSize_l           NOTIFY changedBatchSize_l )
   /**
    * \brief The mash tun volume in liters.                ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
    *        This parameter can be used to calculate if a particular mash and grain profile will fit in the mash tun.
    *        It may also be used for thermal calculations in the case of a partially full mash tun.
    */
   Q_PROPERTY(double tunVolume_l           READ tunVolume_l           WRITE setTunVolume_l           NOTIFY changedTunVolume_l )
   /**
    * \brief The tun mass in kg.                  ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
    *        Used primarily to calculate the thermal parameters of the mash tun – in conjunction with the volume and
    *        specific heat.
    */
   Q_PROPERTY(double tunWeight_kg          READ tunWeight_kg          WRITE setTunWeight_kg          NOTIFY changedTunWeight_kg )
   /**
    * \brief The mash tun specific heat in cal/(g*C)   ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
    *        This is usually a function of the material the mash tun is made of.  Typical ranges are 0.1-0.25 for metal
    *        and 0.2-0.5 for plastic materials.
    */
   Q_PROPERTY(double tunSpecificHeat_calGC READ tunSpecificHeat_calGC WRITE setTunSpecificHeat_calGC NOTIFY changedTunSpecificHeat_calGC )
   /**
    * \brief The top-up water in liters.               ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
    *        The amount of top up water normally added just prior to starting fermentation.  Usually used for extract
    *        brewing.
    */
   Q_PROPERTY(double topUpWater_l          READ topUpWater_l          WRITE setTopUpWater_l          NOTIFY changedTopUpWater_l )
   /**
    * \brief The loss to trub and chiller in liters.  ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
    *        The amount of wort normally lost during transition from the boiler to the fermentation vessel.  Includes
    *        both unusable wort due to trub and wort lost to the chiller and transfer systems.
    */
   Q_PROPERTY(double trubChillerLoss_l     READ trubChillerLoss_l     WRITE setTrubChillerLoss_l     NOTIFY changedTrubChillerLoss_l )
   /**
    * \brief The evaporation rate in percent of the boil size per hour. *** DO NOT USE. *** Only for BeerXML compatibility.  ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
    */
   Q_PROPERTY(double evapRate_pctHr        READ evapRate_pctHr        WRITE setEvapRate_pctHr        NOTIFY changedEvapRate_pctHr )
   /**
    * \brief The evaporation rate in liters/hr.  NB: Not part of BeerXML
    */
   Q_PROPERTY(double evapRate_lHr          READ evapRate_lHr          WRITE setEvapRate_lHr          NOTIFY changedEvapRate_lHr )
   /**
    * \brief The boil time in minutes: the normal amount of time one boils for this equipment setup.  This can be used
    *        with the evaporation rate to calculate the evaporation loss.         ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
    */
   Q_PROPERTY(double boilTime_min          READ boilTime_min          WRITE setBoilTime_min          NOTIFY changedBoilTime_min )
   /**
    * \brief Whether you want the boil volume to be automatically calculated.    ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
    */
   Q_PROPERTY(bool calcBoilVolume        READ calcBoilVolume        WRITE setCalcBoilVolume        NOTIFY changedCalcBoilVolume )
   /**
    * \brief The lauter tun's deadspace in liters.                              ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
    *        Amount lost to the lauter tun and equipment associated with the lautering process.
    */
   Q_PROPERTY(double lauterDeadspace_l     READ lauterDeadspace_l     WRITE setLauterDeadspace_l     NOTIFY changedLauterDeadspace_l )
   /**
    * \brief The kettle top up in liters.                                       ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
    *        Amount normally added to the boil kettle before the boil.
    */
   Q_PROPERTY(double  topUpKettle_l         READ topUpKettle_l         WRITE setTopUpKettle_l         NOTIFY changedTopUpKettle_l )
   /**
    * \brief The hop utilization factor. I do not believe this is used.         ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
    *        Large batch hop utilization.  This value should be 100% for batches less than 20 gallons, but may be higher
    *        (200% or more) for very large batch equipment.
    */
   Q_PROPERTY(double hopUtilization_pct    READ hopUtilization_pct    WRITE setHopUtilization_pct    NOTIFY changedHopUtilization_pct )
   /**
    * \brief The notes.
    */
   Q_PROPERTY(QString notes                READ notes                 WRITE setNotes                 NOTIFY changedNotes )
   /**
    * \brief How much water the grains absorb in liters/kg.  NB: Not part of BeerXML (but is part of BeerJSON)
    *
    *        The apparent volume absorbed by grain, typical values are 0.125 qt/lb (1.04 L/kg) for a mashtun,
    *        0.08 gal/lb (0.66 L/kg) for BIAB.
    */
   Q_PROPERTY(double grainAbsorption_LKg   READ grainAbsorption_LKg   WRITE setGrainAbsorption_LKg   NOTIFY changedGrainAbsorption_LKg )
   /**
    * \brief The boiling point of water in Celsius.  NB: Not part of BeerXML
    */
   Q_PROPERTY(double boilingPoint_c        READ boilingPoint_c        WRITE setBoilingPoint_c        NOTIFY changedBoilingPoint_c )

   //============================================ "GETTER" MEMBER FUNCTIONS ============================================
   double  boilSize_l           () const;
   double  batchSize_l          () const;
   double  tunVolume_l          () const;
   double  tunWeight_kg         () const;
   double  tunSpecificHeat_calGC() const;
   double  topUpWater_l         () const;
   double  trubChillerLoss_l    () const;
   double  evapRate_pctHr       () const;
   double  evapRate_lHr         () const;
   double  boilTime_min         () const;
   bool    calcBoilVolume       () const;
   double  lauterDeadspace_l    () const;
   double  topUpKettle_l        () const;
   double  hopUtilization_pct   () const;
   QString notes                () const;
   double  grainAbsorption_LKg  () const;
   double  boilingPoint_c       () const;

   //============================================ "SETTER" MEMBER FUNCTIONS ============================================
   void setBoilSize_l           (double  const   val);
   void setBatchSize_l          (double  const   val);
   void setTunVolume_l          (double  const   val);
   void setTunWeight_kg         (double  const   val);
   void setTunSpecificHeat_calGC(double  const   val);
   void setTopUpWater_l         (double  const   val);
   void setTrubChillerLoss_l    (double  const   val);
   void setEvapRate_pctHr       (double  const   val);
   void setEvapRate_lHr         (double  const   val);
   void setBoilTime_min         (double  const   val);
   void setCalcBoilVolume       (bool    const   val);
   void setLauterDeadspace_l    (double  const   val);
   void setTopUpKettle_l        (double  const   val);
   void setHopUtilization_pct   (double  const   val);
   void setNotes                (QString const & val);
   void setGrainAbsorption_LKg  (double  const   val);
   void setBoilingPoint_c       (double  const   val);

   //! \brief Calculate how much wort is left immediately at knockout.
   double wortEndOfBoil_l( double kettleWort_l ) const;

   virtual Recipe * getOwningRecipe();

signals:
   void changedBoilSize_l(double);
   void changedBatchSize_l(double);
   void changedTunVolume_l(double);
   void changedTunWeight_kg(double);
   void changedTunSpecificHeat_calGC(double);
   void changedTopUpWater_l(double);
   void changedTrubChillerLoss_l(double);
   void changedEvapRate_pctHr(double);
   void changedEvapRate_lHr(double);
   void changedBoilTime_min(double);
   void changedCalcBoilVolume(bool);
   void changedLauterDeadspace_l(double);
   void changedTopUpKettle_l(double);
   void changedHopUtilization_pct(double);
   void changedNotes(QString);
   void changedGrainAbsorption_LKg(double);
   void changedBoilingPoint_c(double);

protected:
   virtual bool isEqualTo(NamedEntity const & other) const;
   virtual ObjectStore & getObjectStoreTypedInstance() const;

private:
   double  m_boilSize_l           ;
   double  m_batchSize_l          ;
   double  m_tunVolume_l          ;
   double  m_tunWeight_kg         ;
   double  m_tunSpecificHeat_calGC;
   double  m_topUpWater_l         ;
   double  m_trubChillerLoss_l    ;
   double  m_evapRate_pctHr       ;
   double  m_evapRate_lHr         ;
   double  m_boilTime_min         ;
   bool    m_calcBoilVolume       ;
   double  m_lauterDeadspace_l    ;
   double  m_topUpKettle_l        ;
   double  m_hopUtilization_pct   ;
   QString m_notes                ;
   double  m_grainAbsorption_LKg  ;
   double  m_boilingPoint_c       ;

   // Calculate the boil size.
   void doCalculations();
};

Q_DECLARE_METATYPE( Equipment* )

#endif
