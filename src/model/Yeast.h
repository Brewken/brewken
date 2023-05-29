/*======================================================================================================================
 * model/Yeast.h is part of Brewken, and is copyright the following authors 2009-2023:
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
#ifndef MODEL_YEAST_H
#define MODEL_YEAST_H
#pragma once

#include <QSqlRecord>
#include <QString>
#include <QStringList>

#include "model/NamedEntityWithInventory.h"
#include "utils/EnumStringMapping.h"

//======================================================================================================================
//========================================== Start of property name constants ==========================================
// See comment in model/NamedEntity.h
#define AddPropertyName(property) namespace PropertyNames::Yeast { BtStringConst const property{#property}; }
AddPropertyName(addToSecondary  )
AddPropertyName(amount          )
AddPropertyName(amountIsWeight  )
AddPropertyName(amountWithUnits )
AddPropertyName(attenuation_pct )
AddPropertyName(bestFor         )
AddPropertyName(flocculation    )
AddPropertyName(form            )
AddPropertyName(laboratory      )
AddPropertyName(maxReuse        )
AddPropertyName(maxTemperature_c)
AddPropertyName(minTemperature_c)
AddPropertyName(notes           )
AddPropertyName(productID       )
AddPropertyName(timesCultured   )
AddPropertyName(type            )
#undef AddPropertyName
//=========================================== End of property name constants ===========================================
//======================================================================================================================


/*!
 * \class Yeast
 *
 * \brief Model for yeast records in the database.
 *
 *        Since BeerJSON, this is expanded to include other microbes used in brewing.   For the moment, we retain the
 *        name Yeast however.
 */
class Yeast : public NamedEntityWithInventory {
   Q_OBJECT
   Q_CLASSINFO("signal", "yeasts")

public:
   /**
    * \brief What type of yeast or other culture this is.
    *
    *        NB: This is a slightly loose classification, with overlap between some of the categories.  BeerJSON has
    *        somewhat expanded this list of types, and corrected what is arguably an error of having a Wheat category.
    */
   enum class Type {
      Ale         , // Saccharomyces cerevisiae strains used for beer
      Lager       , // Saccharomyces pastorianus - https://en.wikipedia.org/wiki/Saccharomyces_pastorianus
      Other       , // Was Wheat.  In BeerXML, there was a "Wheat" yeast type, for the subset of Ale yeasts used in Wheat beers.  In BeerJSON, this category doesn't exist, so we subsume it into Other.
      Wine        , // Typically Saccharomyces cerevisiae and/or Saccharomyces bayanus
      Champagne   , // Wine yeast strains used for sparkling wines
      // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
      Bacteria    , // Catch-all for all brewing bacteria
      Brett       , // Brettanomyces yeast - https://www.milkthefunk.com/wiki/Brettanomyces
      Kveik       , // Kveik yeast - https://www.milkthefunk.com/wiki/Kveik
      Lacto       , // Lactobacillus bacteria - https://www.milkthefunk.com/wiki/Lactobacillus
      Malolactic  , // Lactic acid bacteria such as Oenococcus oeni used for Malolactic fermentation - https://en.wikipedia.org/wiki/Malolactic_fermentation
      MixedCulture, // A blend of 2 or more - https://www.milkthefunk.com/wiki/Mixed_Cultures
      Pedio       , // Pediococcus bacteria - https://www.milkthefunk.com/wiki/Pediococcus
      Spontaneous , // Local ambient microbes - https://www.milkthefunk.com/wiki/Spontaneous_Fermentation
   };
   // This allows us to store the above enum class in a QVariant
   Q_ENUM(Type)

   /*!
    * \brief Mapping between \c Yeast::Type and string values suitable for serialisation in DB, BeerJSON, etc (but
    *        \b not BeerXML)
    */
   static EnumStringMapping const typeStringMapping;

   /*!
    * \brief Localised names of \c Yeast::Type values suitable for displaying to the end user
    */
   static EnumStringMapping const typeDisplayNames;

   //! \brief What form the yeast comes in.
   enum class Form {
      Liquid ,
      Dry    ,
      Slant  ,
      Culture,
      // ⮜⮜⮜ Below added for BeerJSON support ⮞⮞⮞
      Dregs  ,
   };
   // This allows us to store the above enum class in a QVariant
   Q_ENUM(Form)

   /*!
    * \brief Mapping between \c Yeast::Form and string values suitable for serialisation in DB, BeerJSON, etc (but
    *        \b not BeerXML)
    */
   static EnumStringMapping const formStringMapping;

   /*!
    * \brief Localised names of \c Yeast::Form values suitable for displaying to the end user
    */
   static EnumStringMapping const formDisplayNames;

   //! \brief How flocculant the strain is.
   enum class Flocculation {
      // BeerJSON has an entire type called QualitativeRangeType, but it's only used for this field, so, for now, we
      // treat it as an enum
      VeryLow   , // ⮜⮜⮜ Added for BeerJSON support ⮞⮞⮞
      Low       ,
      MediumLow , // ⮜⮜⮜ Added for BeerJSON support ⮞⮞⮞
      Medium    ,
      MediumHigh, // ⮜⮜⮜ Added for BeerJSON support ⮞⮞⮞
      High      ,
      VeryHigh  ,
   };
   // This allows us to store the above enum class in a QVariant
   Q_ENUM(Flocculation)

   /*!
    * \brief Mapping between \c Yeast::Flocculation and string values suitable for serialisation in DB, BeerJSON, etc
    *        (but \b not BeerXML)
    */
   static EnumStringMapping const flocculationStringMapping;

   /*!
    * \brief Localised names of \c Yeast::Flocculation values suitable for displaying to the end user
    */
   static EnumStringMapping const flocculationDisplayNames;

   /**
    * \brief Mapping of names to types for the Qt properties of this class.  See \c NamedEntity::typeLookup for more
    *        info.
    */
   static TypeLookup const typeLookup;

   Yeast(QString name = "");
   Yeast(NamedParameterBundle const & namedParameterBundle);
   Yeast(Yeast const & other);

   virtual ~Yeast();

   //! \brief The \c Type.
   Q_PROPERTY( Type type READ type WRITE setType /*NOTIFY changed*/ /*changedType*/ )
///   //! \brief The \c Type string.
///   Q_PROPERTY( QString typeString READ typeString )
///   //! \brief The translated \c Type string.
///   Q_PROPERTY( QString typeStringTr READ typeStringTr )
   //! \brief The \c Form.
   Q_PROPERTY( Form form READ form WRITE setForm /*NOTIFY changed*/ /*changedForm*/ )
///   //! \brief The \c Form string.
///   Q_PROPERTY( QString formString READ formString )
///   //! \brief The translated \c Form string.
///   Q_PROPERTY( QString formStringTr READ formStringTr )
   //! \brief The amount in either liters or kg depending on \c amountIsWeight().
   Q_PROPERTY( double amount READ amount WRITE setAmount /*NOTIFY changed*/ /*changedAmount*/ )
   //! \brief Whether the \c amount() is weight (kg) or volume (liters).
   Q_PROPERTY( bool amountIsWeight READ amountIsWeight WRITE setAmountIsWeight /*NOTIFY changed*/ /*changedAmountIsWeight*/ )
   //! \brief The lab from which it came.
   Q_PROPERTY( QString laboratory READ laboratory WRITE setLaboratory /*NOTIFY changed*/ /*changedLaboratory*/ )
   //! \brief The product ID.
   Q_PROPERTY( QString productID READ productID WRITE setProductID /*NOTIFY changed*/ /*changedProductID*/ )
   //! \brief The minimum fermenting temperature.
   Q_PROPERTY( double minTemperature_c READ minTemperature_c WRITE setMinTemperature_c /*NOTIFY changed*/ /*changedMinTemperature_c*/ )
   //! \brief The maximum fermenting temperature.
   Q_PROPERTY( double maxTemperature_c READ maxTemperature_c WRITE setMaxTemperature_c /*NOTIFY changed*/ /*changedMaxTemperature_c*/ )
   //! \brief The \c Flocculation.
   Q_PROPERTY( Flocculation flocculation READ flocculation WRITE setFlocculation /*NOTIFY changed*/ /*changedFlocculation*/ )
///   //! \brief The \c Flocculation string.
///   Q_PROPERTY( QString flocculationString READ flocculationString )
///   //! \brief The translated \c Flocculation string.
///   Q_PROPERTY( QString flocculationStringTr READ flocculationStringTr )
///   //! \brief The apparent attenuation in percent.
   Q_PROPERTY( double attenuation_pct READ attenuation_pct WRITE setAttenuation_pct /*NOTIFY changed*/ /*changedAttenuation_pct*/ )
   //! \brief The notes.
   Q_PROPERTY( QString notes READ notes WRITE setNotes /*NOTIFY changed*/ /*changedNotes*/ )
   //! \brief What styles the strain is best for.
   Q_PROPERTY( QString bestFor READ bestFor WRITE setBestFor /*NOTIFY changed*/ /*changedBestFor*/ )
   //! \brief The number of times recultured.
   Q_PROPERTY( int timesCultured READ timesCultured WRITE setTimesCultured /*NOTIFY changed*/ /*changedTimesCultured*/ )
   //! \brief The maximum recommended number of reculturings.
   Q_PROPERTY( int maxReuse READ maxReuse WRITE setMaxReuse /*NOTIFY changed*/ /*changedMaxReuse*/ )
   //! \brief Whether the yeast is added to secondary or primary.
   Q_PROPERTY( bool addToSecondary READ addToSecondary WRITE setAddToSecondary /*NOTIFY changed*/ /*changedAddToSecondary*/ )
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   /**
    * \brief Amounts of a \c Yeast can be measured by mass or by volume (depending usually on its \c Form)
    */
   Q_PROPERTY(MassOrVolumeAmt    amountWithUnits   READ amountWithUnits   WRITE setAmountWithUnits)

   //============================================ "GETTER" MEMBER FUNCTIONS ============================================
///   const QString typeString() const;
///   const QString typeStringTr() const;
///   const QString formString() const;
///   const QString formStringTr() const;
///   const QString flocculationString() const;
///   const QString flocculationStringTr() const;
   Type            type            () const;
   Form            form            () const;
   double          amount          () const;
   bool            amountIsWeight  () const;
   QString         laboratory      () const;
   QString         productID       () const;
   double          minTemperature_c() const;
   double          maxTemperature_c() const;
   Flocculation    flocculation    () const;
   double          attenuation_pct () const;
   QString         notes           () const;
   QString         bestFor         () const;
   int             timesCultured   () const;
   int             maxReuse        () const;
   bool            addToSecondary  () const;
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   MassOrVolumeAmt amountWithUnits () const;

   //============================================ "SETTER" MEMBER FUNCTIONS ============================================
   void setType            (Type            const   val);
   void setForm            (Form            const   val);
   void setAmount          (double          const   val);
   void setAmountIsWeight  (bool            const   val);
   void setLaboratory      (QString         const & val);
   void setProductID       (QString         const & val);
   void setMinTemperature_c(double          const   val);
   void setMaxTemperature_c(double          const   val);
   void setFlocculation    (Flocculation    const   val);
   void setAttenuation_pct (double          const   val);
   void setNotes           (QString         const & val);
   void setBestFor         (QString         const & val);
   void setTimesCultured   (int             const   val);
   void setMaxReuse        (int             const   val);
   void setAddToSecondary  (bool            const   val);
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   void setAmountWithUnits (MassOrVolumeAmt const   val);

   // .:TBD:. I'm not wild about using "quanta" here (presumably to mean number of packets or number of cultures)
   //         Storing an int in a double is safe, so, for now, just leave this in place but as a wrapper around the more
   //         generic setInventoryAmount().
   void setInventoryQuanta (int             const   val);

   // Insert boiler-plate declarations for inventory
   INVENTORY_COMMON_HEADER_DECLS

   virtual Recipe * getOwningRecipe();

signals:

protected:
   virtual bool isEqualTo(NamedEntity const & other) const;
   virtual ObjectStore & getObjectStoreTypedInstance() const;

private:
   Type         m_type            ;
   Form         m_form            ;
   Flocculation m_flocculation    ;
   double       m_amount          ;
   bool         m_amountIsWeight  ;
   QString      m_laboratory      ;
   QString      m_productID       ;
   double       m_minTemperature_c;
   double       m_maxTemperature_c;
   double       m_attenuation_pct ;
   QString      m_notes           ;
   QString      m_bestFor         ;
   int          m_timesCultured   ;
   int          m_maxReuse        ;
   bool         m_addToSecondary  ;
   int          m_inventory_id    ;
};

Q_DECLARE_METATYPE( QList<Yeast*> )

#endif
