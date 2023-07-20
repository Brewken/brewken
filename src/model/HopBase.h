/*======================================================================================================================
 * model/HopBase.h is part of Brewken, and is copyright the following authors 2023:
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
#ifndef MODEL_HOPBASE_H
#define MODEL_HOPBASE_H
#pragma once

#include <QObject>

#include "model/NamedEntity.h"
#include "utils/BtStringConst.h"
#include "utils/EnumStringMapping.h"

//======================================================================================================================
//========================================== Start of property name constants ==========================================
// See comment in model/NamedEntity.h
#define AddPropertyName(property) namespace PropertyNames::HopBase { BtStringConst const property{#property}; }
AddPropertyName(alpha_pct )
AddPropertyName(beta_pct  )
AddPropertyName(form      )
AddPropertyName(origin    )
AddPropertyName(producer  )
AddPropertyName(product_id)
AddPropertyName(year      )
#undef AddPropertyName
//=========================================== End of property name constants ===========================================
//======================================================================================================================


/**
 * \brief Contains the common parts of \c Hop and \c RecipeAdditionHop
 */
class HopBase : public NamedEntity {
Q_OBJECT
public:
   /**
    * \brief See comment in model/NamedEntity.h
    */
   static QString const LocalisedName;

   /*!
    * \brief The form of the hop.
    */
   enum class Form {Leaf,
                    Pellet,
                    Plug,
                    // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
                    Extract,
                    WetLeaf,
                    Powder};
   // This allows us to store the above enum class in a QVariant
   Q_ENUM(Form)

   /*!
    * \brief Mapping between \c Hop::Form and string values suitable for serialisation in DB, BeerJSON, etc (but \b not
    *        BeerXML)
    *
    *        This can also be used to obtain the number of values of \c Type, albeit at run-time rather than
    *        compile-time.  (One day, C++ will have reflection and we won't need to do things this way.)
    */
   static EnumStringMapping const formStringMapping;

   /*!
    * \brief Localised names of \c Hop::Form values suitable for displaying to the end user
    */
   static EnumStringMapping const formDisplayNames;

   /**
    * \brief Mapping of names to types for the Qt properties of this class.  See \c NamedEntity::typeLookup for more
    *        info.
    */
   static TypeLookup const typeLookup;

   HopBase(QString name = "");
   HopBase(NamedParameterBundle const & namedParameterBundle);
   HopBase(HopBase const & other);

   virtual ~HopBase();

   //=================================================== PROPERTIES ====================================================
   //! \brief The percent alpha acid
   Q_PROPERTY(double                alpha_pct    READ alpha_pct    WRITE setAlpha_pct            )
   /**
    * \brief The \c Form.                ⮜⮜⮜ Optional in BeerJSON and BeerXML ⮞⮞⮞
    *
    *        See comment in \c model/Fermentable.h for \c grainGroup property for why this has to be
    *        \c std::optional<int>, not \c std::optional<Use>
    */
   Q_PROPERTY(std::optional<int>    form         READ formAsInt    WRITE setFormAsInt            )
   //! \brief The percent of beta acids.  ⮜⮜⮜ Optional in BeerJSON and BeerXML ⮞⮞⮞
   Q_PROPERTY(std::optional<double> beta_pct     READ beta_pct     WRITE setBeta_pct             )
   //! \brief The origin.
   Q_PROPERTY(QString               origin       READ origin       WRITE setOrigin               )
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   Q_PROPERTY(QString               producer     READ producer     WRITE setProducer             )
   Q_PROPERTY(QString               product_id   READ product_id   WRITE setProduct_id           )
   /**
    * \brief It might seem odd to store year as a string rather than, say, std::optional<unsigned int>, but this is
    *        deliberate and for two reasons.  Firstly BeerJSON treats it as a string.  Secondly, we don't want it
    *        formatted as a number when we display it.  Nobody writes "2,023" or "2 023" for the year 2023.
    */
   Q_PROPERTY(QString               year         READ year         WRITE setYear                 )

   //============================================ "GETTER" MEMBER FUNCTIONS ============================================
   double                alpha_pct () const;
   std::optional<Form  > form      () const;
   std::optional<int   > formAsInt () const;
   std::optional<double> beta_pct  () const;
   QString               origin    () const;
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   QString               producer  () const;
   QString               product_id() const;
   QString               year      () const;

   //============================================ "SETTER" MEMBER FUNCTIONS ============================================
   void setAlpha_pct (double                const   val);
   void setForm      (std::optional<Form  > const   val);
   void setFormAsInt (std::optional<int   > const   val);
   void setBeta_pct  (std::optional<double> const   val);
   void setOrigin    (QString               const & val);
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   void setProducer  (QString               const & val);
   void setProduct_id(QString               const & val);
   void setYear      (QString               const   val);

protected:
   virtual bool isEqualTo(NamedEntity const & other) const;
   // Note that this is an abstract base class, so it doesn't have its own ObjectStore
   virtual ObjectStore & getObjectStoreTypedInstance() const = 0;

private:
   double                m_alpha_pct ;
   std::optional<Form  > m_form      ;
   std::optional<double> m_beta_pct  ;
   QString               m_origin    ;
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   QString               m_producer  ;
   QString               m_product_id;
   QString               m_year      ;

};

#endif
