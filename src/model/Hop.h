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
   Q_CLASSINFO("signal", "hops")

public:

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

   /**
    * \brief Array of all possible values of \c Hop::Type.  NB: This is \b not guaranteed to be in the same order as the
    *        values of the enum.
    *
    *        This is the least ugly way I could think of to allow other parts of the code to iterate over all values
    *        of enum class \c Type.   Hopefully, if Reflection makes it into C++23, then this will ultimately be
    *        unnecessary.
    */
   static std::array<Type, 7> const allTypes;

   /*!
    * \brief Mapping between \c Hop::Type and string values suitable for serialisation in DB, BeerJSON, etc (but \b not
    *        BeerXML)
    */
   static EnumStringMapping const typeStringMapping;

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

   /**
    * \brief Array of all possible values of \c Hop::Form.  NB: This is \b not guaranteed to be in the same order as the
    *        values of the enum.
    */
   static std::array<Form, 6> const allForms;

   /*!
    * \brief Mapping between \c Hop::Form and string values suitable for serialisation in DB, BeerJSON, etc (but \b not
    *        BeerXML)
    */
   static EnumStringMapping const formStringMapping;

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

   /**
    * \brief Array of all possible values of \c Hop::Use.  NB: This is \b not guaranteed to be in the same order as the
    *        values of the enum.
    */
   static std::array<Use, 5> const allUses;

   /*!
    * \brief Mapping between \c Hop::Form and string values suitable for serialisation in DB, BeerXML, etc (but \b not
    *        used in BeerJSON)
    */
   static EnumStringMapping const useStringMapping;

   /*!
    * \brief Localised names of \c Hop::Type values suitable for displaying to the end user
    */
   static QMap<Hop::Type, QString> const typeDisplayNames;

   /*!
    * \brief Localised names of \c Hop::Form values suitable for displaying to the end user
    */
   static QMap<Hop::Form, QString> const formDisplayNames;

   /*!
    * \brief Localised names of \c Hop::Use values suitable for displaying to the end user
    */
   static QMap<Hop::Use, QString> const useDisplayNames;

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
   //! \brief The \c Use.
   Q_PROPERTY(Use     use                   READ use                   WRITE setUse                  )
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
   // .:TODO JSON:. Some of these should be optional
   Q_PROPERTY(QString producer              READ producer              WRITE setProducer             )
   Q_PROPERTY(QString product_id            READ product_id            WRITE setProduct_id           )
   Q_PROPERTY(std::optional<int>     year                  READ year                  WRITE setYear                 )
   Q_PROPERTY(double  total_oil_ml_per_100g READ total_oil_ml_per_100g WRITE setTotal_oil_ml_per_100g)
   Q_PROPERTY(double  farnesene_pct         READ farnesene_pct         WRITE setFarnesene_pct        )
   Q_PROPERTY(double  geraniol_pct          READ geraniol_pct          WRITE setGeraniol_pct         )
   Q_PROPERTY(double  b_pinene_pct          READ b_pinene_pct          WRITE setB_pinene_pct         )
   Q_PROPERTY(double  linalool_pct          READ linalool_pct          WRITE setLinalool_pct         )
   Q_PROPERTY(double  limonene_pct          READ limonene_pct          WRITE setLimonene_pct         )
   Q_PROPERTY(double  nerol_pct             READ nerol_pct             WRITE setNerol_pct            )
   Q_PROPERTY(double  pinene_pct            READ pinene_pct            WRITE setPinene_pct           )
   Q_PROPERTY(double  polyphenols_pct       READ polyphenols_pct       WRITE setPolyphenols_pct      )
   Q_PROPERTY(double  xanthohumol_pct       READ xanthohumol_pct       WRITE setXanthohumol_pct      )

   //============================================ "GETTER" MEMBER FUNCTIONS ============================================
   double  alpha_pct            () const;
   double  amount_kg            () const;
   Use     use                  () const;
   double  time_min             () const;
   QString notes                () const;
   Type    type                 () const;
   Form    form                 () const;
   double  beta_pct             () const;
   double  hsi_pct              () const;
   QString origin               () const;
   QString substitutes          () const;
   double  humulene_pct         () const;
   double  caryophyllene_pct    () const;
   double  cohumulone_pct       () const;
   double  myrcene_pct          () const;
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   QString producer             () const;
   QString product_id           () const;
   std::optional<int>     year                 () const;
   double  total_oil_ml_per_100g() const;
   double  farnesene_pct        () const;
   double  geraniol_pct         () const;
   double  b_pinene_pct         () const;
   double  linalool_pct         () const;
   double  limonene_pct         () const;
   double  nerol_pct            () const;
   double  pinene_pct           () const;
   double  polyphenols_pct      () const;
   double  xanthohumol_pct      () const;

   virtual double inventory() const;

   //============================================ "SETTER" MEMBER FUNCTIONS ============================================
   void setAlpha_pct            (double  const   val);
   void setAmount_kg            (double  const   val);
   void setUse                  (Use     const   val);
   void setTime_min             (double  const   val);
   void setNotes                (QString const & val);
   void setType                 (Type    const   val);
   void setForm                 (Form    const   val);
   void setBeta_pct             (double  const   val);
   void setHsi_pct              (double  const   val);
   void setOrigin               (QString const & val);
   void setSubstitutes          (QString const & val);
   void setHumulene_pct         (double  const   val);
   void setCaryophyllene_pct    (double  const   val);
   void setCohumulone_pct       (double  const   val);
   void setMyrcene_pct          (double  const   val);
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   void setProducer             (QString const & val);
   void setProduct_id           (QString const & val);
   void setYear                 (std::optional<int>     const   val);
   void setTotal_oil_ml_per_100g(double  const   val);
   void setFarnesene_pct        (double  const   val);
   void setGeraniol_pct         (double  const   val);
   void setB_pinene_pct         (double  const   val);
   void setLinalool_pct         (double  const   val);
   void setLimonene_pct         (double  const   val);
   void setNerol_pct            (double  const   val);
   void setPinene_pct           (double  const   val);
   void setPolyphenols_pct      (double  const   val);
   void setXanthohumol_pct      (double  const   val);

   virtual void setInventoryAmount(double const val);

   virtual Recipe * getOwningRecipe();

protected:
   virtual bool isEqualTo(NamedEntity const & other) const;
   virtual ObjectStore & getObjectStoreTypedInstance() const;

private:
   Use     m_use;
   Type    m_type;
   Form    m_form;
   double  m_alpha_pct;
   double  m_amount_kg;
   double  m_time_min;
   QString m_notes;
   double  m_beta_pct;
   double  m_hsi_pct;
   QString m_origin;
   QString m_substitutes;
   double  m_humulene_pct;
   double  m_caryophyllene_pct;
   double  m_cohumulone_pct;
   double  m_myrcene_pct;
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   QString m_producer;
   QString m_product_id;
   std::optional<int>     m_year;
   double  m_total_oil_ml_per_100g;
   double  m_farnesene_pct;
   double  m_geraniol_pct;
   double  m_b_pinene_pct;
   double  m_linalool_pct;
   double  m_limonene_pct;
   double  m_nerol_pct;
   double  m_pinene_pct;
   double  m_polyphenols_pct;
   double  m_xanthohumol_pct;

   void setDefaults();
};

Q_DECLARE_METATYPE( QList<Hop*> )

/**
 * \brief This function is used (as a parameter to std::sort) for sorting in the recipe formatter
 */
bool hopLessThanByTime(Hop const * const lhs, Hop const * const rhs);

#endif
