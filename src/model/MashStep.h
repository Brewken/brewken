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
AddPropertyName(description      )
AddPropertyName(endAcidity_pH    )
AddPropertyName(endTemp_c        )
AddPropertyName(infuseAmount_l   ) // Should only be used for BeerXML
AddPropertyName(infuseTemp_c     )
AddPropertyName(liquorToGristRatio_lKg)
AddPropertyName(mashId           )
AddPropertyName(rampTime_min     )
AddPropertyName(startAcidity_pH  )
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
    *
    *        Strictly speaking this is an optional field in BeerJSON, because it's not required for every type of mash
    *        step, but I don't think it's too ambiguous for us to retain 0.0 as the "unspecified / not relevant" value.
    *        This saves us a bit of hassle in adding logic to check whether the field should be set and what default
    *        value to use if it's not etc.
    */
   Q_PROPERTY(double                amount_l               READ amount_l              WRITE setAmount_l                         )
   //! \brief The infusion amount in liters - NB: Retained only for BeerXML; DO NOT USE.
   Q_PROPERTY(double                infuseAmount_l         READ infuseAmount_l       /*WRITE setInfuseAmount_l*/    STORED false)
   //! \brief The decoction amount in liters - NB: Retained only for BeerXML; DO NOT USE.
   Q_PROPERTY(double                decoctionAmount_l      READ decoctionAmount_l    /*WRITE setDecoctionAmount_l*/ STORED false)
   //! \brief The target temperature of this step in C.
   Q_PROPERTY(double                stepTemp_c             READ stepTemp_c             WRITE setStepTemp_c                      )
   //! \brief The time of the step in min.
   Q_PROPERTY(double                stepTime_min           READ stepTime_min           WRITE setStepTime_min                    )
   //! \brief The time it takes to ramp the temp to the target temp in min.  ⮜⮜⮜ Optional in BeerXML & BeerJSON ⮞⮞⮞
   Q_PROPERTY(std::optional<double> rampTime_min           READ rampTime_min           WRITE setRampTime_min                    )
   //! \brief The target ending temp of the step in C.                       ⮜⮜⮜ Optional in BeerXML & BeerJSON ⮞⮞⮞
   Q_PROPERTY(std::optional<double> endTemp_c              READ endTemp_c              WRITE setEndTemp_c                       )
   //! \brief The infusion temp in C.                                        ⮜⮜⮜ Not part of BeerXML; optional in BeerJSON ⮞⮞⮞
   Q_PROPERTY(std::optional<double> infuseTemp_c           READ infuseTemp_c           WRITE setInfuseTemp_c                    )
   //! \brief The step number in a sequence of other steps.  Step numbers start from 1.
   Q_PROPERTY(int                   stepNumber             READ stepNumber             WRITE setStepNumber          STORED false)
   //! \brief The Mash to which this MashStep belongs
   Q_PROPERTY(int                   mashId                 READ getMashId              WRITE setMashId                          )
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   Q_PROPERTY(QString               description            READ description            WRITE setDescription                     )
   /**
    * \brief Mash thickness, aka liquor-to-grist ratio, is the volume of strike water (liters) divided by the mass of
    *        grist (kilograms).  Its practical range is 2 to 4 and most often is around 2.5 to 3.2.
    */
   Q_PROPERTY(std::optional<double> liquorToGristRatio_lKg READ liquorToGristRatio_lKg WRITE setLiquorToGristRatio_lKg          )
   Q_PROPERTY(std::optional<double> startAcidity_pH        READ startAcidity_pH        WRITE setStartAcidity_pH                 )
   Q_PROPERTY(std::optional<double>   endAcidity_pH        READ   endAcidity_pH        WRITE   setEndAcidity_pH                 )

   //============================================ "GETTER" MEMBER FUNCTIONS ============================================
   Type type() const;
   double                amount_l              () const; // ⮜⮜⮜ Added, to replace infuseAmount_l & decoctionAmount_l, for BeerJSON support ⮞⮞⮞
   [[deprecated]] double infuseAmount_l        () const;
   [[deprecated]] double decoctionAmount_l     () const;
   double                stepTemp_c            () const;
   double                stepTime_min          () const;
   std::optional<double> rampTime_min          () const;
   std::optional<double> endTemp_c             () const;
   std::optional<double> infuseTemp_c          () const;
   int                   stepNumber            () const;
   int                   getMashId             () const;
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   QString               description           () const;
   std::optional<double> liquorToGristRatio_lKg() const;
   std::optional<double> startAcidity_pH       () const;
   std::optional<double> endAcidity_pH         () const;

   //============================================ "SETTER" MEMBER FUNCTIONS ============================================
   void setType                  (Type                  const   val);
   void setAmount_l              (double                const   val); // ⮜⮜⮜ Added, to replace setInfuseAmount_l & setDecoctionAmount_l, for BeerJSON support ⮞⮞⮞
   void setStepTemp_c            (double                const   val);
   void setStepTime_min          (double                const   val);
   void setRampTime_min          (std::optional<double> const   val);
   void setEndTemp_c             (std::optional<double> const   val);
   void setInfuseTemp_c          (std::optional<double> const   val);
   void setStepNumber            (int                   const   stepNumber);
   void setMashId                (int                   const   mashId    );
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   void setDescription           (QString               const & val);
   void setLiquorToGristRatio_lKg(std::optional<double> const   val);
   void setStartAcidity_pH       (std::optional<double> const   val);
   void setEndAcidity_pH         (std::optional<double> const   val);

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
   Type                  m_type                  ;
   double                m_amount_l              ;
   double                m_stepTemp_c            ;
   double                m_stepTime_min          ;
   std::optional<double> m_rampTime_min          ;
   std::optional<double> m_endTemp_c             ;
   std::optional<double> m_infuseTemp_c          ;
   int                   m_stepNumber            ;
   int                   m_mashId                ;
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   QString               m_description           ;
   std::optional<double> m_liquorToGristRatio_lKg;
   std::optional<double> m_startAcidity_pH       ;
   std::optional<double> m_endAcidity_pH         ;
};

#endif
