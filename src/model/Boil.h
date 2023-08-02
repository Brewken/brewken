/*======================================================================================================================
 * model/Boil.h is part of Brewken, and is copyright the following authors 2023:
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
#ifndef MODEL_BOIL_H
#define MODEL_BOIL_H
#pragma once

#include <memory>
#include <optional>

#include <QList>
#include <QString>

#include "model/BoilStep.h"
#include "model/NamedEntity.h"
#include "model/StepOwnerBase.h"

//======================================================================================================================
//========================================== Start of property name constants ==========================================
// See comment in model/NamedEntity.h
#define AddPropertyName(property) namespace PropertyNames::Boil { BtStringConst const property{#property}; }
AddPropertyName(description      )
AddPropertyName(notes            )
AddPropertyName(preBoilSize_l    )
AddPropertyName(boilTime_mins    )
AddPropertyName(boilSteps        )
AddPropertyName(boilStepsDowncast)
#undef AddPropertyName
//=========================================== End of property name constants ===========================================
//======================================================================================================================



/**
 * \class Boil is a collection of steps providing process information for common boil procedures.  It is introduced as
 *             part of BeerJSON.  It shares a number of characteristics with \c Mash.
 *
 *             A \c Boil with no \c BoilSteps is the same as a standard single step boil.
 *
 *             Note that, although it seems like rather a lot of pain to move this info out of the \c Recipe object,
 *             there is a long-term merit in it for two reasons:
 *               - A boil profile will likely be shared across a lot of recipes;
 *               - Some "raw ale" recipes do not have a boil (see eg https://byo.com/article/raw-ale/)
 *             We don't yet implement/support either of these features, it should be easier to introduce them in future
 *             with this boil-as-a-separate-object structure.
 *
 *             Additionally, there is a short-term benefit, which is that we can share a lot of the logic between
 *             MashStep and BoilStep, which saves us duplicating code.
 */
class Boil : public NamedEntity, public StepOwnerBase<Boil, BoilStep> {
   Q_OBJECT

   STEP_OWNER_COMMON_DECL(Boil, boil)

public:
   /**
    * \brief See comment in model/NamedEntity.h
    */
   static QString const LocalisedName;

   /**
    * \brief Mapping of names to types for the Qt properties of this class.  See \c NamedEntity::typeLookup for more
    *        info.
    */
   static TypeLookup const typeLookup;

   Boil(QString name = "");
   Boil(NamedParameterBundle const & namedParameterBundle);
   Boil(Boil const & other);

   virtual ~Boil();

   /**
    * \brief In some parts of the code, we need to know if a particular part of the recipe counts as part of the boil
    *        proper.  We allow the user to specify the boiling point of water that they want to use for their brewing,
    *        in case they are brewing at high altitude.
    *
    *        According to https://en.wikipedia.org/wiki/High-altitude_cooking, the boiling point of water is a bit over
    *        84.5°C at 4500 meters altitude, which is already higher than the vast majority of settlements, per
    *        https://en.wikipedia.org/wiki/List_of_highest_settlements.  If you were at 5100 meters, the height of the
    *        highest human settlement in the world, then the boiling point of water would be about 82.5°C.
    *
    *        Per https://en.wikipedia.org/wiki/Lautering, mashout temperature is 77°C.
    *
    *        So, we take 81°C as a sensible dividing line.  If the wort is not above this temperature, we can't be in
    *        the boil proper (though we might be ramping up to, or down from, the boil).
    *
    *        Put another way, we're assuming 81°C is higher than any mash would end and lower than the temperature of
    *        any boil.
    */
   static constexpr double minimumBoilTemperature_c{81.0};

   //=================================================== PROPERTIES ====================================================
   Q_PROPERTY(QString               description            READ description     WRITE setDescription   )
   Q_PROPERTY(QString               notes                  READ notes           WRITE setNotes         )
   Q_PROPERTY(std::optional<double> preBoilSize_l          READ preBoilSize_l   WRITE setPreBoilSize_l )
   Q_PROPERTY(double                boilTime_mins          READ boilTime_mins   WRITE setBoilTime_mins )
   //! \brief The individual boil steps.
   Q_PROPERTY(QList<std::shared_ptr<BoilStep>>    boilSteps         READ boilSteps  STORED false )
   //! \brief The individual boil steps downcast as pointers to \c NamedEntity, which is used for BeerJSON processing.
   Q_PROPERTY(QList<std::shared_ptr<NamedEntity>> boilStepsDowncast READ boilStepsDowncast WRITE setBoilStepsDowncast STORED false )

   //============================================ "GETTER" MEMBER FUNCTIONS ============================================
   QString               description  () const;
   QString               notes        () const;
   std::optional<double> preBoilSize_l() const;
   double                boilTime_mins() const;

   //============================================ "SETTER" MEMBER FUNCTIONS ============================================
   void setDescription  (QString               const & val);
   void setNotes        (QString               const & val);
   void setPreBoilSize_l(std::optional<double> const   val);
   void setBoilTime_mins(double                const   val);

///   // Relational getters and setters
///   QList<std::shared_ptr<BoilStep>> boilSteps() const;
///   QList<std::shared_ptr<NamedEntity>> boilStepsDowncast() const;
///   void setBoilStepsDowncast(QList<std::shared_ptr<NamedEntity>> const & val);

///   virtual Recipe * getOwningRecipe() const;
///
///   /**
///    * \brief A Boil owns its BoilSteps so needs to delete those if it itself is being deleted
///    */
///   virtual void hardDeleteOwnedEntities();

public slots:
   void acceptStepChange(QMetaProperty, QVariant);

signals:
   // Emitted when the number of steps change, or when you should call boilSteps() again.
   void stepsChanged();

protected:
   virtual bool isEqualTo(NamedEntity const & other) const;
   virtual ObjectStore & getObjectStoreTypedInstance() const;

private:
   QString               m_description  ;
   QString               m_notes        ;
   std::optional<double> m_preBoilSize_l;
   double                m_boilTime_mins;
};

Q_DECLARE_METATYPE(Boil *)

#endif
