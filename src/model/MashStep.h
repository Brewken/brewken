/*======================================================================================================================
 * model/MashStep.h is part of Brewken, and is copyright the following authors 2009-2023:
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
#ifndef MODEL_MASHSTEP_H
#define MODEL_MASHSTEP_H
#pragma once

#include <QString>
#include <QStringList>

#include "model/Mash.h"
#include "model/NamedEntity.h"
#include "utils/EnumStringMapping.h"

//======================================================================================================================
//========================================== Start of property name constants ==========================================
// See comment in model/NamedEntity.h
#define AddPropertyName(property) namespace PropertyNames::MashStep { BtStringConst const property{#property}; }
AddPropertyName(amount_l         )
AddPropertyName(decoctionAmount_l) // Should only be used for BeerXML
AddPropertyName(endTemp_c        )
AddPropertyName(infuseAmount_l   ) // Should only be used for BeerXML
AddPropertyName(infuseTemp_c     )
AddPropertyName(mashId           )
AddPropertyName(rampTime_min     )
AddPropertyName(stepNumber       )
AddPropertyName(stepTemp_c       )
AddPropertyName(stepTime_min     )
AddPropertyName(type             )
#undef AddPropertyName
//=========================================== End of property name constants ===========================================
//======================================================================================================================


/*!
 * \class MashStep
 *
 * \brief Model for a mash step record in the database.
 */
class MashStep : public NamedEntity {
   Q_OBJECT

public:
//   static QString const LocalisedName;

   //! \brief The type of step.
   enum class Type {Infusion   ,
                    Temperature,
                    Decoction  ,
                    FlySparge  , // In BeerJSON this is simply "sparge" (because it's the "normal" method)
                    BatchSparge, // In BeerJSON this is "drain mash tun"
                    // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
                    SouringMash,
                    SouringWort};
   // This allows us to store the above enum class in a QVariant
   Q_ENUM(Type)

   /*!
    * \brief Mapping between \c MashStep::Type and string values suitable for serialisation in DB, BeerJSON, etc (but
    *        \b not BeerXML)
    *
    *        This can also be used to obtain the number of values of \c Type, albeit at run-time rather than
    *        compile-time.  (One day, C++ will have reflection and we won't need to do things this way.)
    */
   static EnumStringMapping const typeStringMapping;

   /*!
    * \brief Localised names of \c MashStep::Type values suitable for displaying to the end user
    */
   static EnumStringMapping const typeDisplayNames;

   /**
    * \brief Mapping of names to types for the Qt properties of this class.  See \c NamedEntity::typeLookup for more
    *        info.
    */
   static TypeLookup const typeLookup;

   MashStep(QString name = "");
   MashStep(NamedParameterBundle const & namedParameterBundle);
   MashStep(MashStep const & other);

   virtual ~MashStep();

   //=================================================== PROPERTIES ====================================================
   //! \brief The \c Type.
   Q_PROPERTY(Type   type READ type WRITE setType )

   /**
    * \brief The infusion or decoction amount for this step.
    *
    *        Historically, we had two separate amount properties -- \c infuseAmount_l and \c decoctionAmount_l.  This
    *        is because BeerXML only allows an infusion amount to be specified and prohibits the use of this field for
    *        a decoction amount, whilst not providing any mechanism to specify the decoction amount.  We then added a
    *        decoction amount field to our BeerXML records as an extension tag.
    *
    *        With BeerJSON, this nonsense is corrected and there is one amount field whose meaning (infusion amount or
    *        decoction amount) can be determined from the type field.
    *
    *        We retain the \c infuseAmount_l and \c decoctionAmount_l fields for ease of reading from / writing to
    *        BeerXML, but their use is @deprecated in other contexts as the underlying storage is now one amount field.
    *        (Because of the way we do serialisation, we only need the getter functions for these legacy attributes.
    *        When we are reading from BeerXML, they get put in a \c NamedParameterBundle.)
    */
   Q_PROPERTY(double amount_l READ amount_l WRITE setAmount_l          )

   //! \brief The infusion amount in liters.
   Q_PROPERTY(double infuseAmount_l READ infuseAmount_l /*WRITE setInfuseAmount_l*/ STORED false)
   //! \brief The target temperature of this step in C.
   Q_PROPERTY(double stepTemp_c READ stepTemp_c WRITE setStepTemp_c                      )
   //! \brief The time of the step in min.
   Q_PROPERTY(double stepTime_min READ stepTime_min WRITE setStepTime_min                )
   //! \brief The time it takes to ramp the temp to the target temp in min.  ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   Q_PROPERTY(double rampTime_min READ rampTime_min WRITE setRampTime_min                )
   //! \brief The target ending temp of the step in C.                       ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   Q_PROPERTY(double endTemp_c READ endTemp_c WRITE setEndTemp_c                         )
   //! \brief The infusion temp in C.
   Q_PROPERTY(double infuseTemp_c READ infuseTemp_c WRITE setInfuseTemp_c                )
   //! \brief The decoction amount in liters.
   Q_PROPERTY(double decoctionAmount_l READ decoctionAmount_l /*WRITE setDecoctionAmount_l*/ STORED false)
   //! \brief The step number in a sequence of other steps.  Step numbers start from 1.
   Q_PROPERTY(int    stepNumber READ stepNumber WRITE setStepNumber /*NOTIFY changed*/ STORED false )
   //! \brief The Mash to which this MashStep belongs
   Q_PROPERTY(int    mashId READ getMashId WRITE setMashId )

   //============================================ "GETTER" MEMBER FUNCTIONS ============================================
   Type type() const;
   double amount_l() const; // ⮜⮜⮜ Added, to replace infuseAmount_l & decoctionAmount_l, for BeerJSON support ⮞⮞⮞
   [[deprecated]] double infuseAmount_l() const;
   double stepTemp_c() const;
   double stepTime_min() const;
   double rampTime_min() const;
   double endTemp_c() const;
   double infuseTemp_c() const;
   [[deprecated]] double decoctionAmount_l() const;
   int getMashId() const;

   //! What number this step is in the mash.
   int stepNumber() const;

   //============================================ "SETTER" MEMBER FUNCTIONS ============================================
   void setType(Type t);
   void setAmount_l(double var); // ⮜⮜⮜ Added, to replace setInfuseAmount_l & setDecoctionAmount_l, for BeerJSON support ⮞⮞⮞
//   void setInfuseAmount_l(double var);
   void setStepTemp_c(double var);
   void setStepTime_min(double var);
   void setRampTime_min(double var);
   void setEndTemp_c(double var);
   void setInfuseTemp_c(double var);
//   void setDecoctionAmount_l(double var);
   void setStepNumber(int stepNumber);
   void setMashId(int mashId);


   //! some convenience methods
   bool isInfusion() const;
   bool isSparge() const;
   bool isTemperature() const;
   bool isDecoction() const;

   virtual Recipe * getOwningRecipe();

signals:

protected:
   virtual bool isEqualTo(NamedEntity const & other) const;
   virtual ObjectStore & getObjectStoreTypedInstance() const;

private:
   Type   m_type;
   double m_amount_l;
   double m_stepTemp_c;
   double m_stepTime_min;
   double m_rampTime_min;
   double m_endTemp_c;
   double m_infuseTemp_c;
   int    m_stepNumber;
   int    m_mashId;
};

#endif
