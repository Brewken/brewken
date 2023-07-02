/*======================================================================================================================
 * model/Hop.h is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Jeff Bailey <skydvr38@verizon.net>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Samuel Östling <MrOstling@gmail.com>
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
#ifndef MODEL_HOP_H
#define MODEL_HOP_H
#pragma once

#include <array>

#include <QString>
#include <QStringList>
#include <QSqlRecord>

#include "model/NamedEntityWithInventory.h"
#include "utils/EnumStringMapping.h"

//======================================================================================================================
//========================================== Start of property name constants ==========================================
// See comment in model/NamedEntity.h
#define AddPropertyName(property) namespace PropertyNames::Hop { BtStringConst const property{#property}; }
AddPropertyName(alpha_pct            )
AddPropertyName(amount_kg            )
AddPropertyName(beta_pct             )
AddPropertyName(b_pinene_pct         )
AddPropertyName(caryophyllene_pct    )
AddPropertyName(cohumulone_pct       )
AddPropertyName(farnesene_pct        )
AddPropertyName(form                 )
AddPropertyName(geraniol_pct         )
AddPropertyName(hsi_pct              )
AddPropertyName(humulene_pct         )
AddPropertyName(limonene_pct         )
AddPropertyName(linalool_pct         )
AddPropertyName(myrcene_pct          )
AddPropertyName(nerol_pct            )
AddPropertyName(notes                )
AddPropertyName(origin               )
AddPropertyName(pinene_pct           )
AddPropertyName(polyphenols_pct      )
AddPropertyName(producer             )
AddPropertyName(product_id           )
AddPropertyName(substitutes          )
AddPropertyName(time_min             )
AddPropertyName(total_oil_ml_per_100g)
AddPropertyName(type                 )
AddPropertyName(use                  )
AddPropertyName(xanthohumol_pct      )
AddPropertyName(year                 )
#undef AddPropertyName
//=========================================== End of property name constants ===========================================
//======================================================================================================================


/*!
 * \class Hop
 *
 * \brief Model class for a hop record in the database.
 */
class Hop : public NamedEntityWithInventory {
   Q_OBJECT

public:
   /**
    * \brief See comment in model/NamedEntity.h
    */
   static QString const LocalisedName;

   /*!
    * \brief The type of hop, meaning for what properties it is used.  Arguably we should have three binary flags
    *        (aroma, bittering and flavor), but keeping a single enum makes the mappings to/from BeerXML and BeerJSON
    *        easier.
    */
   enum class Type {Bittering,
                    Aroma,
                    AromaAndBittering, // was Both
                    // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
                    Flavor,
                    BitteringAndFlavor,
                    AromaAndFlavor,
                    AromaBitteringAndFlavor};
   // This allows us to store the above enum class in a QVariant
   Q_ENUM(Type)

   /*!
    * \brief Mapping between \c Hop::Type and string values suitable for serialisation in DB, BeerJSON, etc (but \b not
    *        BeerXML)
    *
    *        This can also be used to obtain the number of values of \c Type, albeit at run-time rather than
    *        compile-time.  (One day, C++ will have reflection and we won't need to do things this way.)
    */
   static EnumStringMapping const typeStringMapping;

   /*!
    * \brief Localised names of \c Hop::Type values suitable for displaying to the end user
    */
   static EnumStringMapping const typeDisplayNames;

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

   /*!
    * \brief The way the hop is used.
    *        NOTE that this is not stored in BeerJSON
    */
   enum class Use {Mash,
                   First_Wort,
                   Boil,
                   Aroma,
                   Dry_Hop};
   // This allows us to store the above enum class in a QVariant
   Q_ENUM(Use)

   /*!
    * \brief Mapping between \c Hop::Form and string values suitable for serialisation in DB, BeerXML, etc (but \b not
    *        used in BeerJSON)
    *
    *        This can also be used to obtain the number of values of \c Type, albeit at run-time rather than
    *        compile-time.  (One day, C++ will have reflection and we won't need to do things this way.)
    */
   static EnumStringMapping const useStringMapping;

   /*!
    * \brief Localised names of \c Hop::Use values suitable for displaying to the end user
    */
   static EnumStringMapping const useDisplayNames;

   /**
    * \brief Mapping of names to types for the Qt properties of this class.  See \c NamedEntity::typeLookup for more
    *        info.
    */
   static TypeLookup const typeLookup;

   Hop(QString name = "");
   Hop(NamedParameterBundle const & namedParameterBundle);
   Hop(Hop const & other);

   virtual ~Hop();

   //=================================================== PROPERTIES ====================================================
   //! \brief The percent alpha acid
   Q_PROPERTY(double  alpha_pct             READ alpha_pct             WRITE setAlpha_pct            )
   //! \brief The amount in kg.
   Q_PROPERTY(double  amount_kg             READ amount_kg             WRITE setAmount_kg            )
   /**
    * \brief The \c Use.  This becomes an optional field with the introduction of BeerJSON.
    *
    *        See comment in \c model/Fermentable.h for \c grainGroup property for why this has to be
    *        \c std::optional<int>, not \c std::optional<Use>
    */
   Q_PROPERTY(std::optional<int> use       READ useAsInt               WRITE setUseAsInt             )
   //! \brief The time in minutes that the hop is used.
   Q_PROPERTY(double  time_min              READ time_min              WRITE setTime_min             )
   //! \brief The notes.
   Q_PROPERTY(QString notes                 READ notes                 WRITE setNotes                )
   //! \brief The \c Type.
   Q_PROPERTY(Type    type                  READ type                  WRITE setType                 )
   //! \brief The \c Form.
   Q_PROPERTY(Form    form                  READ form                  WRITE setForm                 )
   //! \brief The percent of beta acids.
   Q_PROPERTY(double  beta_pct              READ beta_pct              WRITE setBeta_pct             )
   //! \brief The hop stability index in percent.  The Hop Stability Index (HSI) is defined as the percentage of hop
   //         alpha lost in 6 months of storage.  It may be related to the Hop Storage Index...
   Q_PROPERTY(double  hsi_pct               READ hsi_pct               WRITE setHsi_pct              )
   //! \brief The origin.
   Q_PROPERTY(QString origin                READ origin                WRITE setOrigin               )
   //! \brief The list of substitutes.
   Q_PROPERTY(QString substitutes           READ substitutes           WRITE setSubstitutes          )
   //! \brief Humulene as a percentage of total hop oil.
   Q_PROPERTY(double  humulene_pct          READ humulene_pct          WRITE setHumulene_pct         )
   //! \brief Caryophyllene as a percentage of total hop oil.
   Q_PROPERTY(double  caryophyllene_pct     READ caryophyllene_pct     WRITE setCaryophyllene_pct    )
   //! \brief Cohumulone as a percentage of total hop oil.
   Q_PROPERTY(double  cohumulone_pct        READ cohumulone_pct        WRITE setCohumulone_pct       )
   //! \brief Myrcene as a percentage of total hop oil.
   Q_PROPERTY(double  myrcene_pct           READ myrcene_pct           WRITE setMyrcene_pct          )

   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   Q_PROPERTY(QString producer              READ producer              WRITE setProducer             )
   Q_PROPERTY(QString product_id            READ product_id            WRITE setProduct_id           )
   /**
    * \brief It might seem odd to store year as a string rather than, say, std::optional<unsigned int>, but this is
    *        deliberate and for two reasons.  Firstly BeerJSON treats it as a string.  Secondly, we don't want it
    *        formatted as a number when we display it.  Nobody writes "2,023" or "2 023" for the year 2023.
    */
   Q_PROPERTY(QString                     year                  READ year                  WRITE setYear                 )
   Q_PROPERTY(std::optional<double      > total_oil_ml_per_100g READ total_oil_ml_per_100g WRITE setTotal_oil_ml_per_100g)
   Q_PROPERTY(std::optional<double      > farnesene_pct         READ farnesene_pct         WRITE setFarnesene_pct        )
   Q_PROPERTY(std::optional<double      > geraniol_pct          READ geraniol_pct          WRITE setGeraniol_pct         )
   Q_PROPERTY(std::optional<double      > b_pinene_pct          READ b_pinene_pct          WRITE setB_pinene_pct         )
   Q_PROPERTY(std::optional<double      > linalool_pct          READ linalool_pct          WRITE setLinalool_pct         )
   Q_PROPERTY(std::optional<double      > limonene_pct          READ limonene_pct          WRITE setLimonene_pct         )
   Q_PROPERTY(std::optional<double      > nerol_pct             READ nerol_pct             WRITE setNerol_pct            )
   Q_PROPERTY(std::optional<double      > pinene_pct            READ pinene_pct            WRITE setPinene_pct           )
   Q_PROPERTY(std::optional<double      > polyphenols_pct       READ polyphenols_pct       WRITE setPolyphenols_pct      )
   Q_PROPERTY(std::optional<double      > xanthohumol_pct       READ xanthohumol_pct       WRITE setXanthohumol_pct      )

   //============================================ "GETTER" MEMBER FUNCTIONS ============================================
   double                alpha_pct            () const;
   double                amount_kg            () const;
   std::optional<Use>    use                  () const; // ⮜⮜⮜ Modified for BeerJSON support ⮞⮞⮞
   std::optional<int>    useAsInt             () const; // ⮜⮜⮜ Modified for BeerJSON support ⮞⮞⮞
   double                time_min             () const;
   QString               notes                () const;
   Type                  type                 () const;
   Form                  form                 () const;
   double                beta_pct             () const;
   double                hsi_pct              () const;
   QString               origin               () const;
   QString               substitutes          () const;
   double                humulene_pct         () const;
   double                caryophyllene_pct    () const;
   double                cohumulone_pct       () const;
   double                myrcene_pct          () const;
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   QString               producer             () const;
   QString               product_id           () const;
   QString               year                 () const;
   std::optional<double> total_oil_ml_per_100g() const;
   std::optional<double> farnesene_pct        () const;
   std::optional<double> geraniol_pct         () const;
   std::optional<double> b_pinene_pct         () const;
   std::optional<double> linalool_pct         () const;
   std::optional<double> limonene_pct         () const;
   std::optional<double> nerol_pct            () const;
   std::optional<double> pinene_pct           () const;
   std::optional<double> polyphenols_pct      () const;
   std::optional<double> xanthohumol_pct      () const;

   //============================================ "SETTER" MEMBER FUNCTIONS ============================================
   void setAlpha_pct            (double             const   val);
   void setAmount_kg            (double             const   val);
   void setUse                  (std::optional<Use> const   val); // ⮜⮜⮜ Modified for BeerJSON support ⮞⮞⮞
   void setUseAsInt             (std::optional<int> const   val); // ⮜⮜⮜ Modified for BeerJSON support ⮞⮞⮞
   void setTime_min             (double             const   val);
   void setNotes                (QString            const & val);
   void setType                 (Type               const   val);
   void setForm                 (Form               const   val);
   void setBeta_pct             (double             const   val);
   void setHsi_pct              (double             const   val);
   void setOrigin               (QString            const & val);
   void setSubstitutes          (QString            const & val);
   void setHumulene_pct         (double             const   val);
   void setCaryophyllene_pct    (double             const   val);
   void setCohumulone_pct       (double             const   val);
   void setMyrcene_pct          (double             const   val);
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   void setProducer             (QString                      const & val);
   void setProduct_id           (QString                      const & val);
   void setYear                 (QString                      const   val);
   void setTotal_oil_ml_per_100g(std::optional<double      >  const   val);
   void setFarnesene_pct        (std::optional<double      >  const   val);
   void setGeraniol_pct         (std::optional<double      >  const   val);
   void setB_pinene_pct         (std::optional<double      >  const   val);
   void setLinalool_pct         (std::optional<double      >  const   val);
   void setLimonene_pct         (std::optional<double      >  const   val);
   void setNerol_pct            (std::optional<double      >  const   val);
   void setPinene_pct           (std::optional<double      >  const   val);
   void setPolyphenols_pct      (std::optional<double      >  const   val);
   void setXanthohumol_pct      (std::optional<double      >  const   val);

   // Insert boiler-plate declarations for inventory
   INVENTORY_COMMON_HEADER_DECLS

   virtual Recipe * getOwningRecipe();

protected:
   virtual bool isEqualTo(NamedEntity const & other) const;
   virtual ObjectStore & getObjectStoreTypedInstance() const;

private:
   std::optional<Use>    m_use                  ; // ⮜⮜⮜ Modified for BeerJSON support ⮞⮞⮞
   Type                  m_type                 ;
   Form                  m_form                 ;
   double                m_alpha_pct            ;
   double                m_amount_kg            ;
   double                m_time_min             ;
   QString               m_notes                ;
   double                m_beta_pct             ;
   double                m_hsi_pct              ;
   QString               m_origin               ;
   QString               m_substitutes          ;
   double                m_humulene_pct         ;
   double                m_caryophyllene_pct    ;
   double                m_cohumulone_pct       ;
   double                m_myrcene_pct          ;
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   QString               m_producer             ;
   QString               m_product_id           ;
   QString               m_year                 ;
   std::optional<double> m_total_oil_ml_per_100g;
   std::optional<double> m_farnesene_pct        ;
   std::optional<double> m_geraniol_pct         ;
   std::optional<double> m_b_pinene_pct         ;
   std::optional<double> m_linalool_pct         ;
   std::optional<double> m_limonene_pct         ;
   std::optional<double> m_nerol_pct            ;
   std::optional<double> m_pinene_pct           ;
   std::optional<double> m_polyphenols_pct      ;
   std::optional<double> m_xanthohumol_pct      ;

   void setDefaults();
};

Q_DECLARE_METATYPE( QList<Hop*> )

/**
 * \brief This function is used (as a parameter to std::sort) for sorting in the recipe formatter
 */
bool hopLessThanByTime(Hop const * const lhs, Hop const * const rhs);

#endif
