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

#include "measurement/ConstrainedAmount.h"
#include "model/NamedEntityWithInventory.h"
#include "utils/EnumStringMapping.h"

//======================================================================================================================
//========================================== Start of property name constants ==========================================
// See comment in model/NamedEntity.h
#define AddPropertyName(property) namespace PropertyNames::Yeast { BtStringConst const property{#property}; }
AddPropertyName(addToSecondary           )
AddPropertyName(alcoholTolerance_pct     )
AddPropertyName(amount                   )
AddPropertyName(amountIsWeight           )
AddPropertyName(amountWithUnits          )
AddPropertyName(attenuationMax_pct       )
AddPropertyName(attenuationMin_pct       )
AddPropertyName(attenuation_pct          )
AddPropertyName(bestFor                  )
AddPropertyName(flocculation             )
AddPropertyName(form                     )
AddPropertyName(glucoamylasePositive     )
AddPropertyName(killerNeutral            )
AddPropertyName(killerProducingK1Toxin   )
AddPropertyName(killerProducingK28Toxin  )
AddPropertyName(killerProducingK2Toxin   )
AddPropertyName(killerProducingKlusToxin )
AddPropertyName(laboratory               )
AddPropertyName(maxReuse                 )
AddPropertyName(maxTemperature_c         )
AddPropertyName(minTemperature_c         )
AddPropertyName(notes                    )
AddPropertyName(phenolicOffFlavorPositive)
AddPropertyName(productID                )
AddPropertyName(timesCultured            )
AddPropertyName(type                     )
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

public:
   /**
    * \brief See comment in model/NamedEntity.h
    */
   static QString const LocalisedName;

   /**
    * \brief Attenuation figure we use in several places where we wouldn't otherwise have a figure
    *
    *        Currently it's 75%, which is a slightly arbitrary figure mentioned at
    *        https://en.wikipedia.org/wiki/Attenuation_(brewing) as a quote from a 1956 book called "The Book Of Beer".
    */
   static double constexpr DefaultAttenuation_pct = 75.0;

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
   Q_PROPERTY(Type                   type                      READ type                      WRITE setType                     )
   //! \brief The \c Form.
   Q_PROPERTY(Form                   form                      READ form                      WRITE setForm                     )
   //! \brief The amount in either liters or kg depending on \c amountIsWeight().
   Q_PROPERTY(double                 amount                    READ amount                    WRITE setAmount                   )
   /**
    * \brief Whether the \c amount() is weight (kg) or volume (liters).
    *
    * .:TBD:. For BeerJSON at least, we should enforce mass for Dry and volume for all other cases.
    */
   Q_PROPERTY(bool                   amountIsWeight            READ amountIsWeight            WRITE setAmountIsWeight           )
   //! \brief The lab from which it came.
   Q_PROPERTY(QString                laboratory                READ laboratory                WRITE setLaboratory               )
   //! \brief The product ID.
   Q_PROPERTY(QString                productID                 READ productID                 WRITE setProductID                )
   //! \brief The minimum fermenting temperature.                  ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   Q_PROPERTY(std::optional<double>  minTemperature_c          READ minTemperature_c          WRITE setMinTemperature_c         )
   //! \brief The maximum fermenting temperature.                  ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   Q_PROPERTY(std::optional<double>  maxTemperature_c          READ maxTemperature_c          WRITE setMaxTemperature_c         )
   /**
    * \brief The \c Flocculation.                                  ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
    *        See comment on \c grainGroup in \c Fermentable for why type has to be std::optional<int> in the interface
    *        rather than std::optional<Flocculation>.
    */
   Q_PROPERTY(std::optional<int>     flocculation              READ flocculationAsInt         WRITE setFlocculationAsInt        )
   /**
    * \brief The apparent attenuation in percent.                 ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
    *
    *        .:TBD:. Maybe this should ultimately be read-only returning the average of Min and Max
    */
   Q_PROPERTY(std::optional<double>  attenuation_pct           READ attenuation_pct           WRITE setAttenuation_pct          )
   //! \brief The notes.
   Q_PROPERTY(QString                notes                     READ notes                     WRITE setNotes                    )
   //! \brief What styles the strain is best for.
   Q_PROPERTY(QString                bestFor                   READ bestFor                   WRITE setBestFor                  )
   //! \brief The number of times recultured.                      ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   Q_PROPERTY(std::optional<int>     timesCultured             READ timesCultured             WRITE setTimesCultured            )
   //! \brief The maximum recommended number of reculturings.      ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   Q_PROPERTY(std::optional<int>     maxReuse                  READ maxReuse                  WRITE setMaxReuse                 )
   //! \brief Whether the yeast is added to secondary or primary.  ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   Q_PROPERTY(std::optional<bool>    addToSecondary            READ addToSecondary            WRITE setAddToSecondary           )

   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞

   //! \brief Amounts of a \c Yeast can be measured by mass or by volume (depending usually on its \c Form)
   Q_PROPERTY(Measurement::Amount        amountWithUnits           READ amountWithUnits           WRITE setAmountWithUnits          )
   //! \brief The recommended limit of abv by the culture producer before attenuation stops.
   Q_PROPERTY(std::optional<double>  alcoholTolerance_pct      READ alcoholTolerance_pct      WRITE setAlcoholTolerance_pct     )
   Q_PROPERTY(std::optional<double>  attenuationMin_pct        READ attenuationMin_pct        WRITE setAttenuationMin_pct       )
   Q_PROPERTY(std::optional<double>  attenuationMax_pct        READ attenuationMax_pct        WRITE setAttenuationMax_pct       )
   //! \brief aka POF+
   Q_PROPERTY(std::optional<bool>    phenolicOffFlavorPositive READ phenolicOffFlavorPositive WRITE setPhenolicOffFlavorPositive)
   /**
    * \brief A glucoamylase positive culture is capable of producing glucoamylase, the enzyme produced through
    *        expression of the diastatic gene, which allows yeast to attenuate dextrins and starches leading to a very
    *        low FG.  This is positive in some saison/brett yeasts as well as the new gulo hybrid by Omega yeast labs.
    */
   Q_PROPERTY(std::optional<bool>    glucoamylasePositive      READ glucoamylasePositive      WRITE setGlucoamylasePositive     )
   /**
    * \brief See https://www.milkthefunk.com/wiki/Saccharomyces#Killer_Wine_Yeast for more on "killer" yeasts and
    *        "killer neutral" yeasts.  BeerJSON calls these killer yeast properties "zymocide", but AFAICT "killer" is
    *        still the more widely used term, at least in relation to brewing.
    *
    *        Technically, \c killerNeutral being \c true implies all the other \c killerProducingXxxToxin properties are
    *        false, because "neutral strains do not produce toxins, nor are they killed by them".  But, for now at
    *        least, we do not enforce that logic.
    */
   Q_PROPERTY(std::optional<bool>    killerProducingK1Toxin    READ killerProducingK1Toxin     WRITE setKillerProducingK1Toxin  )
   Q_PROPERTY(std::optional<bool>    killerProducingK2Toxin    READ killerProducingK2Toxin     WRITE setKillerProducingK2Toxin  )
   Q_PROPERTY(std::optional<bool>    killerProducingK28Toxin   READ killerProducingK28Toxin    WRITE setKillerProducingK28Toxin )
   Q_PROPERTY(std::optional<bool>    killerProducingKlusToxin  READ killerProducingKlusToxin   WRITE setKillerProducingKlusToxin)
   Q_PROPERTY(std::optional<bool>    killerNeutral             READ killerNeutral              WRITE setKillerNeutral           )

   //============================================ "GETTER" MEMBER FUNCTIONS ============================================
   Type                        type                     () const;
   Form                        form                     () const;
   double                      amount                   () const;
   bool                        amountIsWeight           () const;
   QString                     laboratory               () const;
   QString                     productID                () const;
   std::optional<double>       minTemperature_c         () const; // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   std::optional<double>       maxTemperature_c         () const; // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   std::optional<Flocculation> flocculation             () const; // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   std::optional<int>          flocculationAsInt        () const; // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   std::optional<double>       attenuation_pct          () const; // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   QString                     notes                    () const;
   QString                     bestFor                  () const;
   std::optional<int>          timesCultured            () const; // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   std::optional<int>          maxReuse                 () const; // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   std::optional<bool>         addToSecondary           () const; // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   MassOrVolumeAmt             amountWithUnits          () const;
   std::optional<double>       alcoholTolerance_pct     () const;
   std::optional<double>       attenuationMin_pct       () const;
   std::optional<double>       attenuationMax_pct       () const;
   std::optional<bool>         phenolicOffFlavorPositive() const;
   std::optional<bool>         glucoamylasePositive     () const;
   std::optional<bool>         killerProducingK1Toxin   () const;
   std::optional<bool>         killerProducingK2Toxin   () const;
   std::optional<bool>         killerProducingK28Toxin  () const;
   std::optional<bool>         killerProducingKlusToxin () const;
   std::optional<bool>         killerNeutral            () const;

   //============================================ "SETTER" MEMBER FUNCTIONS ============================================
   void setType                     (Type                        const   val);
   void setForm                     (Form                        const   val);
   void setAmount                   (double                      const   val);
   void setAmountIsWeight           (bool                        const   val);
   void setLaboratory               (QString                     const & val);
   void setProductID                (QString                     const & val);
   void setMinTemperature_c         (std::optional<double>       const   val); // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   void setMaxTemperature_c         (std::optional<double>       const   val); // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   void setFlocculation             (std::optional<Flocculation> const   val); // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   void setFlocculationAsInt        (std::optional<int>          const   val); // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   void setAttenuation_pct          (std::optional<double>       const   val); // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   void setNotes                    (QString                     const & val);
   void setBestFor                  (QString                     const & val);
   void setTimesCultured            (std::optional<int>          const   val); // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   void setMaxReuse                 (std::optional<int>          const   val); // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   void setAddToSecondary           (std::optional<bool>         const   val); // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   void setAmountWithUnits          (MassOrVolumeAmt             const   val);
   void setAlcoholTolerance_pct     (std::optional<double>       const   val);
   void setAttenuationMin_pct       (std::optional<double>       const   val);
   void setAttenuationMax_pct       (std::optional<double>       const   val);
   void setPhenolicOffFlavorPositive(std::optional<bool>         const   val);
   void setGlucoamylasePositive     (std::optional<bool>         const   val);
   void setKillerProducingK1Toxin   (std::optional<bool>         const   val);
   void setKillerProducingK2Toxin   (std::optional<bool>         const   val);
   void setKillerProducingK28Toxin  (std::optional<bool>         const   val);
   void setKillerProducingKlusToxin (std::optional<bool>         const   val);
   void setKillerNeutral            (std::optional<bool>         const   val);

   // .:TBD:. I'm not wild about using "quanta" here (presumably to mean number of packets or number of cultures)
   //         Storing an int in a double is safe, so, for now, just leave this in place but as a wrapper around the more
   //         generic setInventoryAmount().
   void setInventoryQuanta (int             const   val);

   // Insert boiler-plate declarations for inventory
   INVENTORY_COMMON_HEADER_DECLS

   /**
    * \brief Get the best attenuation figure to use for this yeast.
    *
    *        If \c attenuation_pct is set, returns that.  Otherwise, if \c attenuationMin_pct and \c attenuationMax_pct
    *        are set, return the mean of those two figures.  Otherwise returns \c Yeast::DefaultAttenuation_pct
    */
   double getTypicalAttenuation_pct() const;

   virtual Recipe * getOwningRecipe() const;

signals:

protected:
   virtual bool isEqualTo(NamedEntity const & other) const;
   virtual ObjectStore & getObjectStoreTypedInstance() const;

private:
   Type                        m_type                     ;
   Form                        m_form                     ;
   double                      m_amount                   ;
   bool                        m_amountIsWeight           ;
   QString                     m_laboratory               ;
   QString                     m_productID                ;
   std::optional<double>       m_minTemperature_c         ; // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   std::optional<double>       m_maxTemperature_c         ; // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   std::optional<Flocculation> m_flocculation             ; // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   std::optional<double>       m_attenuation_pct          ; // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   QString                     m_notes                    ;
   QString                     m_bestFor                  ;
   std::optional<int>          m_timesCultured            ; // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   std::optional<int>          m_maxReuse                 ; // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   std::optional<bool>         m_addToSecondary           ; // ⮜⮜⮜ Optional in BeerXML ⮞⮞⮞
   int                         m_inventory_id             ;
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   std::optional<double>       m_alcoholTolerance_pct     ;
   std::optional<double>       m_attenuationMin_pct       ;
   std::optional<double>       m_attenuationMax_pct       ;
   std::optional<bool>         m_phenolicOffFlavorPositive;
   std::optional<bool>         m_glucoamylasePositive     ;
   std::optional<bool>         m_killerProducingK1Toxin   ;
   std::optional<bool>         m_killerProducingK2Toxin   ;
   std::optional<bool>         m_killerProducingK28Toxin  ;
   std::optional<bool>         m_killerProducingKlusToxin ;
   std::optional<bool>         m_killerNeutral            ;
};

Q_DECLARE_METATYPE( QList<Yeast*> )

#endif
