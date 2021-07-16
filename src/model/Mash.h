/**
 * model/Mash.h is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Jeff Bailey <skydvr38@verizon.net>
 *   • Kregg Kemper <gigatropolis@yahoo.com>
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
#ifndef MODEL_MASH_H
#define MODEL_MASH_H
#pragma once

#include <QSqlRecord>
#include <QVector>

#include "model/NamedEntity.h"
#define AddPropertyName(property) \
namespace PropertyNames::Mash {static char const * const property = #property; }
AddPropertyName(equipAdjust)
AddPropertyName(grainTemp_c)
AddPropertyName(mashSteps)
AddPropertyName(notes)
AddPropertyName(ph)
AddPropertyName(spargeTemp_c)
AddPropertyName(tunSpecificHeat_calGC)
AddPropertyName(tunTemp_c)
AddPropertyName(tunWeight_kg)
#undef AddPropertyName


// Forward declarations.
class MashStep;

/*!
 * \class Mash
 *
 * \brief Model class for a mash record in the database.
 */
class Mash : public NamedEntity {
   Q_OBJECT
   Q_CLASSINFO("signal", "mashs")


   friend class MashDesigner;
   friend class MashEditor;
public:
   Mash(QString name = "", bool cache = true);
   Mash(NamedParameterBundle const & namedParameterBundle);
   Mash(Mash const& other);

   virtual ~Mash() = default;

   //! \brief The initial grain temp in Celsius.
   Q_PROPERTY( double grainTemp_c READ grainTemp_c WRITE setGrainTemp_c /*NOTIFY changed*/ /*changedGrainTemp_c*/ )
   //! \brief The notes.
   Q_PROPERTY( QString notes READ notes WRITE setNotes /*NOTIFY changed*/ /*changedNotes*/ )
   //! \brief The initial tun temp in Celsius.
   Q_PROPERTY( double tunTemp_c READ tunTemp_c WRITE setTunTemp_c /*NOTIFY changed*/ /*changedTunTemp_c*/ )
   //! \brief The sparge temp in C.
   Q_PROPERTY( double spargeTemp_c READ spargeTemp_c WRITE setSpargeTemp_c /*NOTIFY changed*/ /*changedSpargeTemp_c*/ )
   //! \brief The pH.
   Q_PROPERTY( double ph READ ph WRITE setPh /*NOTIFY changed*/ /*changedPh*/ )
   //! \brief The mass of the tun in kg.
   Q_PROPERTY( double tunWeight_kg READ tunWeight_kg WRITE setTunWeight_kg /*NOTIFY changed*/ /*changedTunWeight_kg*/ )
   //! \brief The tun's specific heat in kcal/(g*C).
   Q_PROPERTY( double tunSpecificHeat_calGC READ tunSpecificHeat_calGC WRITE setTunSpecificHeat_calGC /*NOTIFY changed*/ /*changedTunSpecificHeat_calGC*/ )
   //! \brief Whether to adjust strike temperatures to account for the tun.
   Q_PROPERTY( bool equipAdjust READ equipAdjust WRITE setEquipAdjust /*NOTIFY changed*/ /*changedEquipAdjust*/ )
   //! \brief The total water that went into the mash in liters. Calculated.
   Q_PROPERTY( double totalMashWater_l READ totalMashWater_l /*WRITE*/ /*NOTIFY changed*/ /*changedTotalMashWater_l*/ STORED false )
   //! \brief The total mash time in minutes. Calculated.
   Q_PROPERTY( double totalTime READ totalTime /*NOTIFY changed*/ /*changedTotalTime*/ STORED false )
  // Q_PROPERTY( double tunMass_kg READ tunMass_kg  WRITE setTunMass_kg /*NOTIFY changed*/ /*changedTotalTime*/ )
   //! \brief The individual mash steps.
   Q_PROPERTY( QList<MashStep*> mashSteps  READ mashSteps /*WRITE*/ /*NOTIFY changed*/ /*changedTotalTime*/ STORED false )

   /**
    * \brief Connect MashStep changed signals to their parent Mashes.
    *
    *        Needs to be called \b after all the calls to ObjectStoreTyped<FooBar>::getInstance().loadAll()
    */
   static void connectSignals();

   virtual void setKey(int key);

   // Setters
   void setGrainTemp_c( double var );
   void setNotes( const QString &var );
   void setTunTemp_c( double var );
   void setSpargeTemp_c( double var );
   void setPh( double var );
   void setTunWeight_kg( double var );
   void setTunSpecificHeat_calGC( double var );
   void setEquipAdjust( bool var );

   // Getters
   double grainTemp_c() const;
   unsigned int numMashSteps() const;
   QString notes() const;
   double tunTemp_c() const;
   double spargeTemp_c() const;
   double ph() const;
   double tunWeight_kg() const;
   double tunSpecificHeat_calGC() const;
   bool equipAdjust() const;

   // Calculated getters
   //! \brief all the mash water, sparge and strike
   double totalMashWater_l();
   //! \brief all the infusion water, excluding sparge
   double totalInfusionAmount_l() const;
   //! \brief all the sparge water
   double totalSpargeAmount_l() const;
   double totalTime();

   bool hasSparge() const;

   // Relational getters
   QList<MashStep*> mashSteps() const;

   /*!
    * \brief Swap MashSteps \c ms1 and \c ms2
    */
   void swapMashSteps(MashStep & ms1, MashStep & ms2);

   void removeAllMashSteps();

   virtual Recipe * getOwningRecipe();

public slots:
   void acceptMashStepChange(QMetaProperty, QVariant);
   MashStep * addMashStep(MashStep * mashStep);
   MashStep * removeMashStep(MashStep * mashStep);

signals:
   // Emitted when the number of steps change, or when you should call mashSteps() again.
   void mashStepsChanged();

protected:
   virtual bool isEqualTo(NamedEntity const & other) const;
   virtual ObjectStore & getObjectStoreTypedInstance() const;

private:
   double m_grainTemp_c;
   QString m_notes;
   double m_tunTemp_c;
   double m_spargeTemp_c;
   double m_ph;
   double m_tunWeight_kg;
   double m_tunSpecificHeat_calGC;
   bool m_equipAdjust;

//   QList<MashStep*> m_mashSteps;
   QVector<int> mashStepIds;

};

Q_DECLARE_METATYPE( Mash* )

#endif
