/*======================================================================================================================
 * model/Mash.h is part of Brewken, and is copyright the following authors 2009-2023:
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
 =====================================================================================================================*/
#ifndef MODEL_MASH_H
#define MODEL_MASH_H
#pragma once

#include <memory> // For PImpl

#include <QList>
#include <QMetaProperty>
#include <QSqlRecord>
#include <QString>
#include <QVariant>
#include <QVector>

#include "model/NamedEntity.h"

//======================================================================================================================
//========================================== Start of property name constants ==========================================
// See comment in model/NamedEntity.h
#define AddPropertyName(property) namespace PropertyNames::Mash { BtStringConst const property{#property}; }
AddPropertyName(equipAdjust              )
AddPropertyName(grainTemp_c              )
AddPropertyName(mashSteps                )
AddPropertyName(mashStepsDowncast        )
AddPropertyName(mashTunSpecificHeat_calGC)
AddPropertyName(mashTunWeight_kg         )
AddPropertyName(notes                    )
AddPropertyName(ph                       )
AddPropertyName(spargeTemp_c             )
AddPropertyName(totalMashWater_l         )
AddPropertyName(totalTime                )
AddPropertyName(tunTemp_c                )
#undef AddPropertyName
//=========================================== End of property name constants ===========================================
//======================================================================================================================


// Forward declarations.
class MashStep;

/*!
 * \class Mash
 *
 * \brief Model class for a mash record in the database.
 *
 *        .:TBD:. Mashes have a freestanding existence and can, in principle, be shared between Recipes but the UI does
 *        not currently enforce them having non-empty names.
 */
class Mash : public NamedEntity {
   Q_OBJECT

public:
   /**
    * \brief See comment in model/NamedEntity.h
    */
   static QString const LocalisedName;

   /**
    * \brief Mapping of names to types for the Qt properties of this class.  See \c NamedEntity::typeLookup for more
    *        info.
    */
   static TypeLookup const typeLookup;

   Mash(QString name = "");
   Mash(NamedParameterBundle const & namedParameterBundle);
   Mash(Mash const & other);

   virtual ~Mash();

   //! \brief The initial grain temp in Celsius.
   Q_PROPERTY(double grainTemp_c READ grainTemp_c WRITE setGrainTemp_c  )
   //! \brief The notes.
   Q_PROPERTY(QString notes READ notes WRITE setNotes  )
   //! \brief The initial tun temp in Celsius.  ⮜⮜⮜ Optional in BeerXML.  Not part of BeerJSON. ⮞⮞⮞
   Q_PROPERTY(std::optional<double> tunTemp_c READ tunTemp_c WRITE setTunTemp_c  )
   //! \brief The sparge temp in C.             ⮜⮜⮜ Optional in BeerXML.  Not part of BeerJSON. ⮞⮞⮞
   Q_PROPERTY(std::optional<double> spargeTemp_c READ spargeTemp_c WRITE setSpargeTemp_c  )
   //! \brief The pH of the sparge.             ⮜⮜⮜ Optional in BeerXML.  Not part of BeerJSON. ⮞⮞⮞
   Q_PROPERTY(std::optional<double> ph READ ph WRITE setPh  )
   //! \brief The mass of the tun in kg.        ⮜⮜⮜ Optional in BeerXML.  Not part of BeerJSON. ⮞⮞⮞
   Q_PROPERTY(std::optional<double> mashTunWeight_kg READ mashTunWeight_kg WRITE setTunWeight_kg  )
   //! \brief The tun's specific heat in kcal/(g*C).   ⮜⮜⮜ Optional in BeerXML.  Not part of BeerJSON. ⮞⮞⮞
   Q_PROPERTY(std::optional<double> mashTunSpecificHeat_calGC READ mashTunSpecificHeat_calGC WRITE setMashTunSpecificHeat_calGC  )
   //! \brief Whether to adjust strike temperatures to account for the tun.   ⮜⮜⮜ Optional in BeerXML.  Not part of BeerJSON. ⮞⮞⮞
   Q_PROPERTY(bool equipAdjust READ equipAdjust WRITE setEquipAdjust  )
   //! \brief The total water that went into the mash in liters. Calculated.
   Q_PROPERTY(double totalMashWater_l READ totalMashWater_l  STORED false )
   //! \brief The total mash time in minutes. Calculated.
   Q_PROPERTY(double totalTime READ totalTime  STORED false )
   //! \brief The individual mash steps.
   Q_PROPERTY(QList< std::shared_ptr<MashStep> > mashSteps  READ mashSteps  STORED false )
   //! \brief The individual mash steps downcast as pointers to \c NamedEntity, which is used for BeerJSON processing.
   Q_PROPERTY(QList<std::shared_ptr<NamedEntity>> mashStepsDowncast READ mashStepsDowncast WRITE setMashStepsDowncast STORED false )

   // ⮜⮜⮜ BeerJSON support does not require any additional properties on this class! ⮞⮞⮞

   /**
    * \brief Connect MashStep changed signals to their parent Mashes.
    *
    *        Needs to be called \b after all the calls to ObjectStoreTyped<FooBar>::getInstance().loadAll()
    */
   static void connectSignals();

   virtual void setKey(int key);

   //============================================ "SETTER" MEMBER FUNCTIONS ============================================
   void setGrainTemp_c              (double                const   val);
   void setNotes                    (QString               const & val);
   void setTunTemp_c                (std::optional<double> const   val);
   void setSpargeTemp_c             (std::optional<double> const   val);
   void setPh                       (std::optional<double> const   val);
   void setTunWeight_kg             (std::optional<double> const   val);
   void setMashTunSpecificHeat_calGC(std::optional<double> const   val);
   void setEquipAdjust              (bool                  const   val);

   //============================================ "GETTER" MEMBER FUNCTIONS ============================================
   double                grainTemp_c              () const;
   QString               notes                    () const;
   std::optional<double> tunTemp_c                () const;
   std::optional<double> spargeTemp_c             () const;
   std::optional<double> ph                       () const;
   std::optional<double> mashTunWeight_kg         () const;
   std::optional<double> mashTunSpecificHeat_calGC() const;
   bool                  equipAdjust              () const;

   // Calculated getters
   unsigned int numMashSteps() const;
   //! \brief all the mash water, sparge and strike
   double totalMashWater_l();
   //! \brief all the infusion water, excluding sparge
   double totalInfusionAmount_l() const;
   //! \brief all the sparge water
   double totalSpargeAmount_l() const;
   double totalTime();

   bool hasSparge() const;

   // Relational getters and setters
   QList< std::shared_ptr<MashStep>> mashSteps() const;
   QList<std::shared_ptr<NamedEntity>> mashStepsDowncast() const;
   void setMashStepsDowncast(QList<std::shared_ptr<NamedEntity>> const & val);

   /*!
    * \brief Swap MashSteps \c ms1 and \c ms2
    */
   void swapMashSteps(MashStep & ms1, MashStep & ms2);

   void removeAllMashSteps();

   virtual Recipe * getOwningRecipe();

   /**
    * \brief A Mash owns its MashSteps so needs to delete those if it itself is being deleted
    */
   virtual void hardDeleteOwnedEntities();

   std::shared_ptr<MashStep> addMashStep(std::shared_ptr<MashStep> mashStep);
   std::shared_ptr<MashStep> removeMashStep(std::shared_ptr<MashStep> mashStep);

public slots:
   void acceptMashStepChange(QMetaProperty, QVariant);

signals:
   // Emitted when the number of steps change, or when you should call mashSteps() again.
   void mashStepsChanged();

protected:
   virtual bool isEqualTo(NamedEntity const & other) const;
   virtual ObjectStore & getObjectStoreTypedInstance() const;

private:
   // Private implementation details - see https://herbsutter.com/gotw/_100/
   class impl;
   std::unique_ptr<impl> pimpl;

   double                m_grainTemp_c              ;
   QString               m_notes                    ;
   std::optional<double> m_tunTemp_c                ;
   std::optional<double> m_spargeTemp_c             ;
   std::optional<double> m_ph                       ;
   std::optional<double> m_mashTunWeight_kg         ;
   std::optional<double> m_mashTunSpecificHeat_calGC;
   bool                  m_equipAdjust              ;
};

Q_DECLARE_METATYPE(Mash *)

#endif
