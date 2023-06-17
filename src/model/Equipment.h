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
AddPropertyName(batchSize_l              )
AddPropertyName(boilingPoint_c           )
AddPropertyName(boilSize_l               )
AddPropertyName(boilTime_min             )
AddPropertyName(calcBoilVolume           )
AddPropertyName(kettleEvaporationPerHour_l             )
AddPropertyName(evapRate_pctHr           )
AddPropertyName(mashTunGrainAbsorption_LKg      )
AddPropertyName(hopUtilization_pct       )
AddPropertyName(lauterDeadspace_l        )
AddPropertyName(kettleNotes                    )
AddPropertyName(topUpKettle_l            )
AddPropertyName(topUpWater_l             )
AddPropertyName(trubChillerLoss_l        )
AddPropertyName(mashTunSpecificHeat_calGC)
AddPropertyName(mashTunVolume_l          )
AddPropertyName(mashTunWeight_kg         )
#undef AddPropertyName
//=========================================== End of property name constants ===========================================
//======================================================================================================================


/*!
 * \class Equipment
 *
 * \brief Model representing a single equipment record.
 *
 *        This is where things get fun.  In BeerXML, Equipment is a single record representing all the hot-side
 *        equipment used in a recipe.  In BeerJSON however, the model is of a named array of EquipmentItemType objects,
 *        each of which can be one of "HLT", "Mash Tun", "Lauter Tun", "Brew Kettle", "Fermenter", "Aging Vessel" or
 *        "Packaging Vessel".
 *
 *        We take the view that it is right to have a single Equipment object, but that subdividing it into the 7
 *        categories of BeerJSON is also useful.  (Although nothing in BeerJSON precludes you from having multiple
 *        EquipmentItemType objects in a single EquipmentType array, we take the view that this would not be meaningful,
 *        and so we do not support it.)
 *
 *        There are a few wrinkles around the edges.  In BeerJSON, you don't have to have a record for a particular
 *        vessel (eg you might not have an "Aging Vessel") but, if a vessel record is present, it has to have values for
 *        "name", "maxiumum_volume" and "loss".  This means some \c Equipment fields should, technically, be "optional
 *        in certain circumstances" -- eg \c agingVesselLoss_l should be optional unless any of the other agingVessel
 *        fields are set.  However, for the moment at least, we simplify and say something is either optional or it's
 *        not.  And, since \c agingVesselLoss_l can be required, then it is never null.  (Our default values for "name",
 *        "maxiumum_volume" and "loss" are "", 0.0 and 0.0 respectively.)  The upshot of this is that, when we write an
 *        \c Equipment record out to BeerJSON, we will write records for all seven vessel types.  This is slightly ugly,
 *        but I don't think it has any significant bad consequences, eg it should be self evident to the user when there
 *        is no substantive data in a record.
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

   Equipment(QString name = "");
   Equipment(NamedParameterBundle const & namedParameterBundle);
   Equipment(Equipment const & other);

   virtual ~Equipment();

   /**
    * \brief The boil size in liters: the pre-boil volume used in this particular instance for this equipment setup.
    *        Note that this may be a calculated value depending on the calcBoilVolume property.
    *
    *        In BeerJSON, there is no record of whether this is a calculated value, it is just the maxiumum_volume of
    *        the "Brew Kettle".
    */
   Q_PROPERTY(double boilSize_l            READ boilSize_l            WRITE setBoilSize_l            )
   /**
    * \brief The batch size in liters, aka the target volume of the batch at the start of fermentation.
    *
    *        In BeerJSON, this corresponds to the maxiumum_volume of the "Fermenter".
    */
   Q_PROPERTY(double batchSize_l           READ batchSize_l           WRITE setBatchSize_l           )
   /**
    * \brief The mash tun volume in liters.                ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
    *        This parameter can be used to calculate if a particular mash and grain profile will fit in the mash tun.
    *        It may also be used for thermal calculations in the case of a partially full mash tun.
    */
   Q_PROPERTY(double mashTunVolume_l       READ mashTunVolume_l       WRITE setMashTunVolume_l           )
   /**
    * \brief The tun mass in kg.                  ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
    *        Used primarily to calculate the thermal parameters of the mash tun – in conjunction with the volume and
    *        specific heat.
    */
   Q_PROPERTY(double mashTunWeight_kg      READ mashTunWeight_kg      WRITE setMashTunWeight_kg          )
   /**
    * \brief The mash tun specific heat in cal/(g*C)   ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
    *        This is usually a function of the material the mash tun is made of.  Typical ranges are 0.1-0.25 for metal
    *        and 0.2-0.5 for plastic materials.
    */
   Q_PROPERTY(double mashTunSpecificHeat_calGC READ mashTunSpecificHeat_calGC WRITE setMashTunSpecificHeat_calGC )
   /**
    * \brief The top-up water in liters.               ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
    *        The amount of top up water normally added just prior to starting fermentation.  Usually used for extract
    *        brewing.
    *
    *        Note this is not stored in BeerJSON.  .:TBD.JSON:. Does this become part of the Recipe?
    */
   Q_PROPERTY(double topUpWater_l          READ topUpWater_l          WRITE setTopUpWater_l          )
   /**
    * \brief The loss to trub and chiller in liters.  ⮜⮜⮜ Optional in BeerXML but required in BeerJSON ⮞⮞⮞
    *
    *        The amount of wort normally lost during transition from the boiler to the fermentation vessel.  Includes
    *        both unusable wort due to trub and wort lost to the chiller and transfer systems.
    *
    *        Note that BeerJSON has a per-vessel "loss" field, so this is the brew kettle loss.
    *
    *        Since this is required in BeerJSON, we'll keep it required here, but default it (and the other loss fields)
    *        to 0.
    */
   Q_PROPERTY(double trubChillerLoss_l     READ trubChillerLoss_l     WRITE setTrubChillerLoss_l     )
   /**
    * \brief The evaporation rate in percent of the boil size per hour. *** DO NOT USE. *** Only for BeerXML compatibility.  ⮜⮜⮜ Optional in BeerXML.  Not supported in BeerJSON. ⮞⮞⮞
    */
   Q_PROPERTY(double evapRate_pctHr        READ evapRate_pctHr        WRITE setEvapRate_pctHr        )
   /**
    * \brief The evaporation rate in liters/hr.  NB: Not part of BeerXML
    *
    *        This is boil_rate_per_hour for Brew Kettle in BeerJSON: "The volume boiled off during 1 hour, measured
    *        before and after at room temperature."
    *
    *        Note that, although, strictly, this is a "volume per time" measurement, we follow BeerJSON's lead in
    *        treating the "per hour" bit as set in stone and thus simplify this down to a "volume" measurement in the
    *        UI.
    */
   Q_PROPERTY(double kettleEvaporationPerHour_l          READ kettleEvaporationPerHour_l          WRITE setKettleEvaporationPerHour_l          )
   /**
    * \brief The boil time in minutes: the normal amount of time one boils for this equipment setup.  This can be used
    *        with the evaporation rate to calculate the evaporation loss.         ⮜⮜⮜ Optional in BeerXML.  Not supported in BeerJSON. ⮞⮞⮞
    *
    *        This is not stored in BeerJSON.  (I don't see that boil time is really an attribute of equipment.  It seems
    *        more like a per-recipe field.)
    */
   Q_PROPERTY(double boilTime_min          READ boilTime_min          WRITE setBoilTime_min          )
   /**
    * \brief Whether you want the boil volume to be automatically calculated.    ⮜⮜⮜ Optional in BeerXML.  Not supported in BeerJSON. ⮞⮞⮞
    */
   Q_PROPERTY(bool calcBoilVolume        READ calcBoilVolume        WRITE setCalcBoilVolume          )
   /**
    * \brief The lauter tun's deadspace in liters.                              ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
    *        Amount lost to the lauter tun and equipment associated with the lautering process.
    *
    *        In BeerJSON, this is the "loss" of Lauter Tun.
    */
   Q_PROPERTY(double lauterDeadspace_l     READ lauterDeadspace_l     WRITE setLauterDeadspace_l     )
   /**
    * \brief The kettle top up in liters.                                       ⮜⮜⮜ Optional in BeerXML.  Not supported in BeerJSON. ⮞⮞⮞
    *        Amount normally added to the boil kettle before the boil.
    */
   Q_PROPERTY(double  topUpKettle_l         READ topUpKettle_l         WRITE setTopUpKettle_l         )
   /**
    * \brief The hop utilization factor. I do not believe this is used.         ⮜⮜⮜ Optional in BeerXML.  Not supported in BeerJSON. ⮞⮞⮞
    *        Large batch hop utilization.  This value should be 100% for batches less than 20 gallons, but may be higher
    *        (200% or more) for very large batch equipment.
    */
   Q_PROPERTY(double hopUtilization_pct    READ hopUtilization_pct    WRITE setHopUtilization_pct    )
   /**
    * \brief The Brew Kettle Notes.
    *
    *        In BeerXML, there is one "kettleNotes" field for the whole equipment record.  In BeerJSON, there is no overall
    *        kettleNotes field, but, instead, each vessel ("HLT", "Mash Tun", etc) has its own kettleNotes field.  To bridge the
    *        gap, we treat the "Brew Kettle" kettleNotes field of BeerJSON as the overall kettleNotes field of BeerXML.
    */
   Q_PROPERTY(QString kettleNotes                READ kettleNotes                 WRITE setKettleNotes                 )
   /**
    * \brief How much water the grains absorb in liters/kg.  NB: Not part of BeerXML (but is part of BeerJSON)
    *
    *        The apparent volume absorbed by grain, typical values are 0.125 qt/lb (1.04 L/kg) for a mashtun,
    *        0.08 gal/lb (0.66 L/kg) for BIAB.
    */
   Q_PROPERTY(double mashTunGrainAbsorption_LKg   READ mashTunGrainAbsorption_LKg   WRITE setMashTunGrainAbsorption_LKg   )
   /**
    * \brief The boiling point of water in Celsius.  NB: Not part of BeerXML or BeerJSON
    */
   Q_PROPERTY(double boilingPoint_c        READ boilingPoint_c        WRITE setBoilingPoint_c        )

   //============================================ "GETTER" MEMBER FUNCTIONS ============================================
   double  boilSize_l           () const;
   double  batchSize_l          () const;
   double  mashTunVolume_l      () const;
   double  mashTunWeight_kg     () const;
   double  mashTunSpecificHeat_calGC() const;
   double  topUpWater_l         () const;
   double  trubChillerLoss_l    () const;
   double  evapRate_pctHr       () const;
   double  kettleEvaporationPerHour_l         () const;
   double  boilTime_min         () const;
   bool    calcBoilVolume       () const;
   double  lauterDeadspace_l    () const;
   double  topUpKettle_l        () const;
   double  hopUtilization_pct   () const;
   QString kettleNotes                () const;
   double  mashTunGrainAbsorption_LKg  () const;
   double  boilingPoint_c       () const;

   //============================================ "SETTER" MEMBER FUNCTIONS ============================================
   void setBoilSize_l           (double  const   val);
   void setBatchSize_l          (double  const   val);
   void setMashTunVolume_l      (double  const   val);
   void setMashTunWeight_kg     (double  const   val);
   void setMashTunSpecificHeat_calGC(double  const   val);
   void setTopUpWater_l         (double  const   val);
   void setTrubChillerLoss_l    (double  const   val);
   void setEvapRate_pctHr       (double  const   val);
   void setKettleEvaporationPerHour_l         (double  const   val);
   void setBoilTime_min         (double  const   val);
   void setCalcBoilVolume       (bool    const   val);
   void setLauterDeadspace_l    (double  const   val);
   void setTopUpKettle_l        (double  const   val);
   void setHopUtilization_pct   (double  const   val);
   void setKettleNotes                (QString const & val);
   void setMashTunGrainAbsorption_LKg  (double  const   val);
   void setBoilingPoint_c       (double  const   val);

   //! \brief Calculate how much wort is left immediately at knockout.
   double wortEndOfBoil_l( double kettleWort_l ) const;

   virtual Recipe * getOwningRecipe();

signals:

protected:
   virtual bool isEqualTo(NamedEntity const & other) const;
   virtual ObjectStore & getObjectStoreTypedInstance() const;

private:
   double  m_boilSize_l           ;
   double  m_batchSize_l          ;
   double  m_mashTunVolume_l          ;
   double  m_mashTunWeight_kg         ;
   double  m_mashTunSpecificHeat_calGC;
   double  m_topUpWater_l         ;
   double  m_trubChillerLoss_l    ;
   double  m_evapRate_pctHr       ;
   double  m_kettleEvaporationPerHour_l         ;
   double  m_boilTime_min         ;
   bool    m_calcBoilVolume       ;
   double  m_lauterDeadspace_l    ;
   double  m_topUpKettle_l        ;
   double  m_hopUtilization_pct   ;
   QString m_kettleNotes                ;
   double  m_mashTunGrainAbsorption_LKg  ;
   double  m_boilingPoint_c       ;

   // Calculate the boil size.
   void doCalculations();
};

Q_DECLARE_METATYPE( Equipment* )

#endif
