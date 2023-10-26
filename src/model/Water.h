/*======================================================================================================================
 * model/Water.h is part of Brewken, and is copyright the following authors 2009-2023:
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
#ifndef MODEL_WATER_H
#define MODEL_WATER_H
#pragma once

#include <QString>
#include <QSqlRecord>

#include "model/NamedEntity.h"
#include "utils/EnumStringMapping.h"

//======================================================================================================================
//========================================== Start of property name constants ==========================================
// See comment in model/NamedEntity.h
#define AddPropertyName(property) namespace PropertyNames::Water { BtStringConst const property{#property}; }
AddPropertyName(alkalinity_ppm  )
AddPropertyName(alkalinityAsHCO3)
AddPropertyName(amount          )
AddPropertyName(bicarbonate_ppm )
AddPropertyName(calcium_ppm     )
AddPropertyName(chloride_ppm    )
AddPropertyName(magnesium_ppm   )
AddPropertyName(mashRo_pct      )
AddPropertyName(notes           )
AddPropertyName(ph              )
AddPropertyName(sodium_ppm      )
AddPropertyName(spargeRo_pct    )
AddPropertyName(sulfate_ppm     )
AddPropertyName(type            )
#undef AddPropertyName
//=========================================== End of property name constants ===========================================
//======================================================================================================================


/*!
 * \class Water
 *
 * \brief Model for water records in the database.
 */
class Water : public NamedEntity {
   Q_OBJECT

public:
   /**
    * \brief See comment in model/NamedEntity.h
    */
   static QString const LocalisedName;

   /**
    * \brief
    *        NOTE: This does not appear to be part of BeerXML or BeerJSON.
    */
   enum class Type {
      Base  ,
      Target,
   };
   // This allows us to store the above enum class in a QVariant
   Q_ENUM(Type)

   /*!
    * \brief Mapping between \c Water::Type and string values suitable for serialisation in DB
    *
    *        This can also be used to obtain the number of values of \c Type, albeit at run-time rather than
    *        compile-time.  (One day, C++ will have reflection and we won't need to do things this way.)
    */
   static EnumStringMapping const typeStringMapping;

   /*!
    * \brief Localised names of \c Water::Type values suitable for displaying to the end user
    */
   static EnumStringMapping const typeDisplayNames;

   /**
    * \brief
    */
   enum class Ion {
      Ca  ,
      Cl  ,
      HCO3,
      Mg  ,
      Na  ,
      SO4 ,
   };
   // This allows us to store the above enum class in a QVariant
   Q_ENUM(Ion)

   static EnumStringMapping const ionStringMapping;
   static EnumStringMapping const ionDisplayNames;

   /**
    * \brief Mapping of names to types for the Qt properties of this class.  See \c NamedEntity::typeLookup for more
    *        info.
    */
   static TypeLookup const typeLookup;

   Water(QString name = "");
   Water(NamedParameterBundle const & namedParameterBundle);
   Water(Water const & other);

   virtual ~Water();

   // It is useful to be able to assign one Water to another - see eg editors/WaterEditor.cpp
   Water & operator=(Water other);

protected:
   /**
    * \brief Swap the contents of two Water objects - which provides an exception-safe way of implementing operator=
    */
   void swap(Water & other) noexcept;

public:
   // .:TODO:. On a base or target profile, bicarbonate and alkalinity cannot both be used. I'm gonna have fun figuring that out

   //! \brief The amount in liters.
   // .:TBD:. (MY 2020-01-03) In Hop we have amount_kg, so might be more consistent here to have amount_l or similar
   Q_PROPERTY(double amount          READ amount          WRITE setAmount)
   //! \brief The ppm of calcium.  Required in BeerXML and BeerJSON.
   Q_PROPERTY(double calcium_ppm     READ calcium_ppm     WRITE setCalcium_ppm)
   //! \brief The ppm of bicarbonate.  Required in BeerXML and BeerJSON.
   Q_PROPERTY(double bicarbonate_ppm READ bicarbonate_ppm WRITE setBicarbonate_ppm)
   //! \brief The ppm of sulfate.  Required in BeerXML and BeerJSON.
   Q_PROPERTY(double sulfate_ppm     READ sulfate_ppm     WRITE setSulfate_ppm)
   //! \brief The ppm of chloride.  Required in BeerXML and BeerJSON.
   Q_PROPERTY(double chloride_ppm    READ chloride_ppm    WRITE setChloride_ppm)
   //! \brief The ppm of sodium.  Required in BeerXML and BeerJSON.
   Q_PROPERTY(double sodium_ppm      READ sodium_ppm      WRITE setSodium_ppm)
   //! \brief The ppm of magnesium.  Required in BeerXML and BeerJSON.
   Q_PROPERTY(double magnesium_ppm   READ magnesium_ppm   WRITE setMagnesium_ppm)
   //! \brief The pH.  ⮜⮜⮜ TODO: Optional in both BeerXML and BeerJSON. ⮞⮞⮞
   Q_PROPERTY(double ph              READ ph              WRITE setPh)
   //! \brief The residual alkalinity.  Units are ppm.  NB: Not part of BeerXML or BeerJSON.
   Q_PROPERTY(double alkalinity_ppm  READ alkalinity_ppm  WRITE setAlkalinity_ppm)
   //! \brief The notes.
   Q_PROPERTY(QString notes          READ notes           WRITE setNotes)
   /**
    * \brief What kind of water is this.  NB: Not part of BeerXML or BeerJSON.
    *
    *        See comment in \c model/Fermentable.h for \c grainGroup property for why this has to be
    *        \c std::optional<int>, not \c std::optional<Type>
    */
   Q_PROPERTY(std::optional<int> type READ typeAsInt        WRITE setTypeAsInt)
   //! \brief percent of the mash water that is RO (reverse osmosis)
   Q_PROPERTY(double mashRo_pct       READ mashRo_pct       WRITE setMashRo_pct)
   //! \brief percent of the sparge water that is RO (reverse osmosis)
   Q_PROPERTY(double spargeRo_pct     READ spargeRo_pct     WRITE setSpargeRo_pct)
   //! \brief is the alkalinity measured as HCO3 (bicarbonate) or CO3 (carbonate)?
   Q_PROPERTY(bool   alkalinityAsHCO3 READ alkalinityAsHCO3 WRITE setAlkalinityAsHCO3)

   //============================================ "GETTER" MEMBER FUNCTIONS ============================================
   double              amount          () const;
   double              calcium_ppm     () const;
   double              bicarbonate_ppm () const;
   double              sulfate_ppm     () const;
   double              chloride_ppm    () const;
   double              sodium_ppm      () const;
   double              magnesium_ppm   () const;
   double              ph              () const;
   double              alkalinity_ppm  () const;
   QString             notes           () const;
   std::optional<Type> type            () const;
   std::optional<int>  typeAsInt       () const;
   double              mashRo_pct      () const;
   double              spargeRo_pct    () const;
   bool                alkalinityAsHCO3() const;

   double       ppm(Water::Ion const ion) const;

   //============================================ "SETTER" MEMBER FUNCTIONS ============================================
   void setAmount          (double              const   val);
   void setCalcium_ppm     (double              const   val);
   void setSulfate_ppm     (double              const   val);
   void setBicarbonate_ppm (double              const   val);
   void setChloride_ppm    (double              const   val);
   void setSodium_ppm      (double              const   val);
   void setMagnesium_ppm   (double              const   val);
   void setPh              (double              const   val);
   void setAlkalinity_ppm  (double              const   val);
   void setNotes           (QString             const & val);
   void setType            (std::optional<Type> const   val);
   void setTypeAsInt       (std::optional<int>  const   val);
   void setMashRo_pct      (double              const   val);
   void setSpargeRo_pct    (double              const   val);
   void setAlkalinityAsHCO3(bool                const   val);

   virtual Recipe * getOwningRecipe() const;

signals:

protected:
   virtual bool isEqualTo(NamedEntity const & other) const;
   virtual ObjectStore & getObjectStoreTypedInstance() const;

private:
   double              m_amount            ;
   double              m_calcium_ppm       ;
   double              m_bicarbonate_ppm   ;
   double              m_sulfate_ppm       ;
   double              m_chloride_ppm      ;
   double              m_sodium_ppm        ;
   double              m_magnesium_ppm     ;
   double              m_ph                ;
   double              m_alkalinity_ppm    ;
   QString             m_notes             ;
   std::optional<Type> m_type              ;
   double              m_mashRo_pct        ;
   double              m_spargeRo_pct      ;
   bool                m_alkalinity_as_hco3;
};

Q_DECLARE_METATYPE(QList<std::shared_ptr<Water> >)

#endif
