/*======================================================================================================================
 * model/Fermentable.h is part of Brewken, and is copyright the following authors 2009-2022:
 *   • Blair Bonnett <blair.bonnett@gmail.com>
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Jeff Bailey <skydvr38@verizon.net>
 *   • Kregg Kemper <gigatropolis@yahoo.com>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Samuel Östling <MrOstling@gmail.com>
 *   • Théophane Martin <theophane.m@gmail.com>
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
#ifndef MODEL_FERMENTABLE_H
#define MODEL_FERMENTABLE_H
#pragma once

#include <QStringList>
#include <QString>
#include <QSqlRecord>

#include "measurement/Unit.h"
#include "model/NamedEntityWithInventory.h"
#include "utils/EnumStringMapping.h"

//======================================================================================================================
//========================================== Start of property name constants ==========================================
#define AddPropertyName(property) namespace PropertyNames::Fermentable { BtStringConst const property{#property}; }
AddPropertyName(addAfterBoil)
AddPropertyName(additionMethod)
AddPropertyName(additionTime)
AddPropertyName(amount_kg)
AddPropertyName(coarseFineDiff_pct)
AddPropertyName(color_srm)
AddPropertyName(diastaticPower_lintner)
AddPropertyName(ibuGalPerLb)
AddPropertyName(isMashed)
AddPropertyName(maxInBatch_pct)
AddPropertyName(moisture_pct)
AddPropertyName(notes)
AddPropertyName(origin)
AddPropertyName(protein_pct)
AddPropertyName(recommendMash)
AddPropertyName(supplier)
AddPropertyName(type)
AddPropertyName(yield_pct)
#undef AddPropertyName
//=========================================== End of property name constants ===========================================
//======================================================================================================================

/*!
 * \class Fermentable
 *
 * \brief Model for a fermentable record in the database.
 */
class Fermentable : public NamedEntityWithInventory {
   Q_OBJECT
   Q_CLASSINFO("signal", "fermentables")

public:

   /**
    * \brief The type of Fermentable.
    */
   enum class Type {Grain,
                    Sugar,
                    Extract,
                    Dry_Extract,
                    Other_Adjunct,     // Was Adjunct.  Corresponds to "other" in BeerJSON
                    // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
                    Fruit,
                    Juice,
                    Honey};
   /*!
    * \brief Mapping between \c Fermentable::Type and string values suitable for serialisation in DB, BeerJSON, etc (but
    *        \b not BeerXML)
    */
   static EnumStringMapping const typeStringMapping;



   /**
    * \brief The addition method.
    *
    *        This approximately corresponds to BeerXML boolean attribute RECOMMEND_MASH which is 'TRUE if it is
    *        recommended the grain be mashed, FALSE if it can be steeped.  A value of TRUE is only appropriate for a
    *        "Grain" or "Adjunct" types.  The default value is FALSE.  Note that this does NOT indicate whether the
    *        grain is mashed or not – it is only a recommendation used in recipe formulation.'
    *
    *        In BeerJSON, recommend_mash is defined as "True if the fermentable must be mashed, false if it can be steeped."
    */
   enum class AdditionMethod {Mashed, Steeped, Not_Mashed};

   /*!
    * \brief Localised names of \c Fermentable::Type values suitable for displaying to the end user
    */
   static QMap<Fermentable::Type, QString> const typeDisplayNames;

   Fermentable(QString name = "");
   Fermentable(NamedParameterBundle const & namedParameterBundle);
   Fermentable(Fermentable const & other);

   virtual ~Fermentable();

   //=================================================== PROPERTIES ====================================================
   //! \brief The \c Type.
   Q_PROPERTY(Type           type                   READ type                   WRITE setType                               )
   //! \brief The \c addition method.
   Q_PROPERTY(AdditionMethod additionMethod         READ additionMethod         WRITE setAdditionMethod         STORED false)
   //! \brief The translated \c Method string.
   Q_PROPERTY(QString        additionMethodStringTr READ additionMethodStringTr /*WRITE*/                       STORED false)
   //! \brief The amount in kg.
   Q_PROPERTY(double         amount_kg              READ amount_kg              WRITE setAmount_kg                          )
   //! \brief The yield (when finely milled) as a percentage of equivalent glucose.
   Q_PROPERTY(double         yield_pct              READ yield_pct              WRITE setYield_pct                          )
   //! \brief The color in SRM.
   Q_PROPERTY(double         color_srm              READ color_srm              WRITE setColor_srm                          )
   /**
    * \brief Whether to add after the boil: \c true if this Fermentable is normally added after the boil.  The default
    *        value is \c false since most grains are added during the mash or boil.
    *
    *        Note that this is stored in BeerXML but not in BeerJSON.
    */
   Q_PROPERTY(bool           addAfterBoil           READ addAfterBoil           WRITE setAddAfterBoil                       )
   //! \brief The origin.
   Q_PROPERTY(QString        origin                 READ origin                 WRITE setOrigin                             )
   //! \brief The supplier.
   Q_PROPERTY(QString        supplier               READ supplier               WRITE setSupplier                           )
   //! \brief The notes.
   Q_PROPERTY(QString        notes                  READ notes                  WRITE setNotes                              )
   //! \brief The difference in yield between coarsely milled and finely milled grain.
   Q_PROPERTY(double         coarseFineDiff_pct     READ coarseFineDiff_pct     WRITE setCoarseFineDiff_pct                 )
   //! \brief The moisture in pct.
   Q_PROPERTY(double         moisture_pct           READ moisture_pct           WRITE setMoisture_pct                       )
   //! \brief The diastatic power in Lintner.
   Q_PROPERTY(double         diastaticPower_lintner READ diastaticPower_lintner WRITE setDiastaticPower_lintner             )
   //! \brief The percent protein.
   Q_PROPERTY(double         protein_pct            READ protein_pct            WRITE setProtein_pct                        )
   //! \brief The maximum recommended amount in a batch, as a percentage of the total grains.
   Q_PROPERTY(double         maxInBatch_pct         READ maxInBatch_pct         WRITE setMaxInBatch_pct                     )
   /**
    * \brief Whether a mash is recommended. \c true means \c Fermentable must be mashed, \c false means if it can be
    *        steeped.  Note that this does NOT indicate whether the \c Fermentable is mashed or not – it is only a
    *        recommendation used in recipe formulation.
    */
   Q_PROPERTY(bool           recommendMash          READ recommendMash          WRITE setRecommendMash                      )
   //! \brief The IBUs per gal/lb if this is a liquid extract.
   Q_PROPERTY(double         ibuGalPerLb            READ ibuGalPerLb            WRITE setIbuGalPerLb                        )
   //! \brief The maximum kg of equivalent glucose that will come from this Fermentable.
   Q_PROPERTY(double         equivSucrose_kg        READ equivSucrose_kg        /*WRITE*/                       STORED false)
   //! \brief Whether the grains actually is mashed.
   Q_PROPERTY(bool           isMashed               READ isMashed               WRITE setIsMashed                           )
   //! \brief Whether this fermentable is an extract.
   Q_PROPERTY(bool           isExtract              READ isExtract                                              STORED false)
   //! \brief Whether this fermentable is a sugar. Somewhat redundant, but it makes for nice symmetry elsewhere
   Q_PROPERTY(bool           isSugar                READ isSugar                                                STORED false)

   //============================================ "GETTER" MEMBER FUNCTIONS ============================================
   Type    type                  () const;
   double  amount_kg             () const;
   double  yield_pct             () const;
   double  color_srm             () const;
   bool    addAfterBoil          () const;
   QString origin                () const;
   QString supplier              () const;
   QString notes                 () const;
   double  coarseFineDiff_pct    () const;
   double  moisture_pct          () const;
   double  diastaticPower_lintner() const;
   double  protein_pct           () const;
   double  maxInBatch_pct        () const;
   bool    recommendMash         () const;
   double  ibuGalPerLb           () const;
   bool    isMashed              () const;

   AdditionMethod additionMethod () const;
   //! Returns a translated addition method string.
   QString additionMethodStringTr() const;
   // Calculated getters.
   double  equivSucrose_kg       () const;
   bool    isExtract             () const;
   bool    isSugar               () const;

   virtual double inventory() const;

   //============================================ "SETTER" MEMBER FUNCTIONS ============================================
   void setType                  (Type           const   val);
   void setAdditionMethod        (AdditionMethod const   val);
   void setAmount_kg             (double         const   val);
   void setYield_pct             (double         const   val);
   void setColor_srm             (double         const   val);
   void setAddAfterBoil          (bool           const   val);
   void setOrigin                (QString        const & val);
   void setSupplier              (QString        const & val);
   void setNotes                 (QString        const & val);
   void setCoarseFineDiff_pct    (double         const   val);
   void setMoisture_pct          (double         const   val);
   void setDiastaticPower_lintner(double         const   val);
   void setProtein_pct           (double         const   val);
   void setMaxInBatch_pct        (double         const   val);
   void setRecommendMash         (bool           const   val);
   void setIbuGalPerLb           (double         const   val);
   void setIsMashed              (bool           const   val);

   virtual void setInventoryAmount(double amount);

   void save();

   virtual Recipe * getOwningRecipe();

protected:
   virtual bool isEqualTo(NamedEntity const & other) const;
   virtual ObjectStore & getObjectStoreTypedInstance() const;

private:
   QString m_typeStr;
   Type    m_type;
   double  m_amountKg;       // Primarily valid in "Use Of" instance
   double  m_yieldPct;
   double  m_colorSrm;
   bool    m_isAfterBoil;    // Primarily valid in "Use Of" instance
   QString m_origin;
   QString m_supplier;
   QString m_notes;
   double  m_coarseFineDiff;
   double  m_moisturePct;
   double  m_diastaticPower;
   double  m_proteinPct;
   double  m_maxInBatchPct;
   bool    m_recommendMash;
   double  m_ibuGalPerLb;
   bool    m_isMashed;       // Primarily valid in "Use Of" instance
};

Q_DECLARE_METATYPE( QList<Fermentable*> )

/**
 * \brief This function is used (as a parameter to std::sort) for sorting in the recipe formatter
 */
bool fermentablesLessThanByWeight(Fermentable const * const lhs, Fermentable const * const rhs);

#endif
