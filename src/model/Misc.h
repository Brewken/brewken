/*======================================================================================================================
 * model/Misc.h is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Jeff Bailey <skydvr38@verizon.net>
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
#ifndef MODEL_MISC_H
#define MODEL_MISC_H
#pragma once

#include <QString>
#include <QSqlRecord>

#include "measurement/Amount.h"
#include "measurement/ConstrainedAmount.h"
#include "measurement/Unit.h"
#include "model/NamedEntityWithInventory.h"
#include "utils/EnumStringMapping.h"

//======================================================================================================================
//========================================== Start of property name constants ==========================================
// See comment in model/NamedEntity.h
#define AddPropertyName(property) namespace PropertyNames::Misc { BtStringConst const property{#property}; }
AddPropertyName(amount         )
AddPropertyName(amountIsWeight )
AddPropertyName(amountWithUnits)
AddPropertyName(notes          )
AddPropertyName(producer       )
AddPropertyName(productId      )
AddPropertyName(time_min       )
AddPropertyName(type           )
AddPropertyName(useFor         )
AddPropertyName(use            )
#undef AddPropertyName
//=========================================== End of property name constants ===========================================
//======================================================================================================================


/*!
 * \class Misc
 *
 * \brief Model for a misc record in the database.
 */
class Misc : public NamedEntityWithInventory {
   Q_OBJECT
   Q_CLASSINFO("signal", "miscs")

public:

   /**
    * \brief The type of ingredient.
    */
   enum class Type {Spice      ,
                    Fining     ,
                    Water_Agent,
                    Herb       ,
                    Flavor     ,
                    Other      ,
                    // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
                    Wood       ,};
   // This allows us to store the above enum class in a QVariant
   Q_ENUM(Type)

   /*!
    * \brief Mapping between \c Misc::Type and string values suitable for serialisation in DB, BeerJSON, etc (but
    *        \b not BeerXML)
    *
    *        This can also be used to obtain the number of values of \c Type, albeit at run-time rather than
    *        compile-time.  (One day, C++ will have reflection and we won't need to do things this way.)
    */
   static EnumStringMapping const typeStringMapping;

   /*!
    * \brief Localised names of \c Misc::Type values suitable for displaying to the end user
    */
   static EnumStringMapping const typeDisplayNames;

   /**
    * \brief Where the ingredient is used.  NOTE that this is not stored in BeerJSON.
    */
   enum class Use {Boil     ,
                   Mash     ,
                   Primary  ,
                   Secondary,
                   Bottling ,};
   // This allows us to store the above enum class in a QVariant
   Q_ENUM(Use)

   static EnumStringMapping const useStringMapping;
   static EnumStringMapping const useDisplayNames;

   /**
    * \brief Mapping of names to types for the Qt properties of this class.  See \c NamedEntity::typeLookup for more
    *        info.
    */
   static TypeLookup const typeLookup;

   Misc(QString name = "");
   Misc(NamedParameterBundle const & namedParameterBundle);
   Misc(Misc const & other);

   virtual ~Misc();

   //! \brief The \c Type.
   Q_PROPERTY(Type type   READ type   WRITE setType)
   /**
    * \brief The \c Use.  This becomes an optional field with the introduction of BeerJSON.
    *
    *        See comment in \c model/Fermentable.h for \c grainGroup property for why this has to be
    *        \c std::optional<int>, not \c std::optional<Use>
    */
   Q_PROPERTY(std::optional<int> use   READ useAsInt   WRITE setUseAsInt)
   //! \brief The time used in minutes.
   Q_PROPERTY(double  time_min              READ time_min              WRITE setTime_min             )
   //! \brief The amount in either kg or L, depending on \c amountIsWeight().
   Q_PROPERTY( double amount READ amount WRITE setAmount)
   //! \brief Whether the amount is weight (kg), or volume (L).
   Q_PROPERTY( bool amountIsWeight READ amountIsWeight WRITE setAmountIsWeight)
   //! \brief What to use it for.
   Q_PROPERTY( QString useFor READ useFor WRITE setUseFor)
   //! \brief The notes.
   Q_PROPERTY( QString notes READ notes WRITE setNotes)
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   /**
    * \brief Amounts of a \c Misc can be measured by mass or by volume (depending usually on what it is)
    *
    * .:TBD JSON:. Check what else we need to do to tie in to Mixed2PhysicalQuantities, plus look at how we force weight
    * for BeerXML.
    */
   Q_PROPERTY(MassOrVolumeAmt    amountWithUnits   READ amountWithUnits   WRITE setAmountWithUnits)
   Q_PROPERTY(QString            producer          READ producer          WRITE setProducer       )
   Q_PROPERTY(QString            productId         READ productId         WRITE setProductId      )

   //============================================ "GETTER" MEMBER FUNCTIONS ============================================
   Type               type           () const;
   std::optional<Use> use            () const;
   std::optional<int> useAsInt       () const;
   double             amount         () const;
   double             time_min       () const;
   bool               amountIsWeight () const;
   QString            useFor         () const;
   QString            notes          () const;
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   MassOrVolumeAmt    amountWithUnits() const;
   QString            producer       () const;
   QString            productId      () const;

   //============================================ "SETTER" MEMBER FUNCTIONS ============================================
   void setType           (Type               const   val);
   void setUse            (std::optional<Use> const   val);
   void setUseAsInt       (std::optional<int> const   val);
   void setAmount         (double             const   val);
   void setTime_min       (double             const   val);
   void setAmountIsWeight (bool               const   val);
   void setUseFor         (QString            const & val);
   void setNotes          (QString            const & val);
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   void setAmountWithUnits(MassOrVolumeAmt    const   val);
   void setProducer       (QString            const & val);
   void setProductId      (QString            const & val);

   // Insert boiler-plate declarations for inventory
   INVENTORY_COMMON_HEADER_DEFNS

   virtual Recipe * getOwningRecipe();

signals:

   //! \brief Emitted when \c name() changes.
   // Declared in Base Class NamedEntity, should not be overloaded
   //void changedName(QString);

protected:
   virtual bool isEqualTo(NamedEntity const & other) const;
   virtual ObjectStore & getObjectStoreTypedInstance() const;

private:
   Type               m_type          ;
   std::optional<Use> m_use           ;
   double             m_time_min      ;
   double             m_amount        ;
   bool               m_amountIsWeight;
   QString            m_useFor        ;
   QString            m_notes         ;
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   QString            m_producer      ;
   QString            m_productId     ;
};

Q_DECLARE_METATYPE( QList<Misc*> )

#endif
