/*======================================================================================================================
 * model/Step.h is part of Brewken, and is copyright the following authors 2023-2024:
 *   • Matt Young <mfsy@yahoo.com>
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
#ifndef MODEL_STEP_H
#define MODEL_STEP_H
#pragma once

#include <optional>

#include <QString>

#include "database/ObjectStoreWrapper.h"
#include "model/FolderBase.h"
#include "model/NamedEntity.h"

//======================================================================================================================
//========================================== Start of property name constants ==========================================
// See comment in model/NamedEntity.h
#define AddPropertyName(property) namespace PropertyNames::Step { BtStringConst const property{#property}; }
AddPropertyName(description      )
AddPropertyName(endAcidity_pH    )
AddPropertyName(endTemp_c        )
AddPropertyName(ownerId          )
AddPropertyName(rampTime_mins    )
AddPropertyName(startAcidity_pH  )
AddPropertyName(stepNumber       )
AddPropertyName(stepTime_min     )
#undef AddPropertyName
//=========================================== End of property name constants ===========================================
//======================================================================================================================


/**
 * \brief Common base class for \c MashStep, \c BoilStep, \c FermentationStep
 *
 * \sa \c StepBase
 */
class Step : public NamedEntity {
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

   Step(QString name = "");
   Step(NamedParameterBundle const & namedParameterBundle);
   Step(Step const & other);

   virtual ~Step();

   //=================================================== PROPERTIES ====================================================
   //! \brief The time of the step in min.
   Q_PROPERTY(double                stepTime_min           READ stepTime_min           WRITE setStepTime_min                    )
   //! \brief The target ending temp of the step in C.                       ⮜⮜⮜ Optional in BeerXML & BeerJSON ⮞⮞⮞
   Q_PROPERTY(std::optional<double> endTemp_c              READ endTemp_c              WRITE setEndTemp_c                       )
   /**
    * \brief The time it takes to ramp the temp to the target temp in min - ie the amount of time that passes before
    *        this step begins.                                                           ⮜⮜⮜ Optional in BeerXML & BeerJSON ⮞⮞⮞
    *
    *        Eg for \c MashStep, moving from a mash step (step 1) of 148F, to a new temperature step of 156F (step 2)
    *        may take 8 minutes to heat the mash. Step 2 would have a ramp time of 8 minutes.
    *
    *        Similarly, for a \c BoilStep, moving from a boiling step (step 1) to a whirlpool step (step 2) may take 5
    *        minutes.  Step 2 would have a ramp time of 5 minutes, hop isomerization and bitterness calculations will
    *        need to account for this accordingly.
    *
    *        NOTE: This property is \b not used by \c FermentationStep.  (It is the only property shared by \c MashStep
    *              and \c BoilStep that is not also needed in \c FermentationStep.  We can't really do mix-ins in Qt, so
    *              it's simplest just to not use it in \c FermentationStep.)
    */
   Q_PROPERTY(std::optional<double> rampTime_mins          READ rampTime_mins          WRITE setRampTime_mins                   )
   //! \brief The step number in a sequence of other steps.  Step numbers start from 1.
   Q_PROPERTY(int                   stepNumber             READ stepNumber             WRITE setStepNumber          STORED false)
   //! \brief The Mash, Boil or Fermentation to which this Step belongs
   Q_PROPERTY(int                   ownerId                READ ownerId                WRITE setOwnerId                         )
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   Q_PROPERTY(QString               description            READ description            WRITE setDescription                     )
   Q_PROPERTY(std::optional<double> startAcidity_pH        READ startAcidity_pH        WRITE setStartAcidity_pH                 )
   Q_PROPERTY(std::optional<double>   endAcidity_pH        READ   endAcidity_pH        WRITE   setEndAcidity_pH                 )

   //============================================ "GETTER" MEMBER FUNCTIONS ============================================
   double                stepTime_min          () const;
   std::optional<double> endTemp_c             () const;
   int                   stepNumber            () const;
   int                   ownerId               () const;
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   QString               description           () const;
   std::optional<double> rampTime_mins         () const;
   std::optional<double> startAcidity_pH       () const;
   std::optional<double> endAcidity_pH         () const;

   //============================================ "SETTER" MEMBER FUNCTIONS ============================================
   void setStepTime_min          (double                const   val       );
   void setEndTemp_c             (std::optional<double> const   val       );
   void setStepNumber            (int                   const   stepNumber);
   void setOwnerId               (int                   const   ownerId   );
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   void setDescription           (QString               const & val);
   void setRampTime_mins         (std::optional<double> const   val);
   void setStartAcidity_pH       (std::optional<double> const   val);
   void setEndAcidity_pH         (std::optional<double> const   val);

signals:

protected:
   virtual bool isEqualTo(NamedEntity const & other) const;

protected:
   double                m_stepTime_min          ;
   std::optional<double> m_endTemp_c             ;
   int                   m_stepNumber            ;
   int                   m_ownerId               ;
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   QString               m_description           ;
   std::optional<double> m_rampTime_mins         ;
   std::optional<double> m_startAcidity_pH       ;
   std::optional<double> m_endAcidity_pH         ;

};

#endif
