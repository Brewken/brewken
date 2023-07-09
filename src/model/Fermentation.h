/*======================================================================================================================
 * model/Fermentation.h is part of Brewken, and is copyright the following authors 2023:
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
#ifndef MODEL_FERMENTATION_H
#define MODEL_FERMENTATION_H
#pragma once

#include <memory>
#include <optional>

#include <QList>
#include <QMetaProperty>
#include <QString>
#include <QVariant>

#include "model/FermentationStep.h"
#include "model/NamedEntity.h"
#include "model/StepOwnerBase.h"

//======================================================================================================================
//========================================== Start of property name constants ==========================================
// See comment in model/NamedEntity.h
#define AddPropertyName(property) namespace PropertyNames::Fermentation { BtStringConst const property{#property}; }
AddPropertyName(description              )
AddPropertyName(notes                    )
AddPropertyName(fermentationSteps        )
AddPropertyName(fermentationStepsDowncast)
#undef AddPropertyName
//=========================================== End of property name constants ===========================================
//======================================================================================================================

/**
 * \class Fermentation is a collection of steps providing process information for common fermentation procedures.  It is
 *        introduced as part of BeerJSON.  It shares a number of characteristics with \c Mash and \c Boil
 */
class Fermentation : public NamedEntity, public StepOwnerBase<Fermentation, FermentationStep> {
   Q_OBJECT

   STEP_OWNER_COMMON_DECL(Fermentation, fermentation)

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

   Fermentation(QString name = "");
   Fermentation(NamedParameterBundle const & namedParameterBundle);
   Fermentation(Fermentation const & other);

   virtual ~Fermentation();

   //=================================================== PROPERTIES ====================================================
   Q_PROPERTY(QString               description            READ description     WRITE setDescription   )
   Q_PROPERTY(QString               notes                  READ notes           WRITE setNotes         )
   //! \brief The individual fermentation steps.
   Q_PROPERTY(QList<std::shared_ptr<FermentationStep>>    fermentationSteps         READ fermentationSteps  STORED false )
   //! \brief The individual fermentation steps downcast as pointers to \c NamedEntity, which is used for BeerJSON processing.
   Q_PROPERTY(QList<std::shared_ptr<NamedEntity>> fermentationStepsDowncast READ fermentationStepsDowncast WRITE setFermentationStepsDowncast STORED false )

   //============================================ "GETTER" MEMBER FUNCTIONS ============================================
   QString               description  () const;
   QString               notes        () const;

   //============================================ "SETTER" MEMBER FUNCTIONS ============================================
   void setDescription  (QString               const & val);
   void setNotes        (QString               const & val);

public slots:
   void acceptStepChange(QMetaProperty, QVariant);

signals:
   // Emitted when the number of steps change, or when you should call fermentationSteps() again.
   void stepsChanged();

protected:
   virtual bool isEqualTo(NamedEntity const & other) const;
   virtual ObjectStore & getObjectStoreTypedInstance() const;

private:
   QString               m_description  ;
   QString               m_notes        ;
};

Q_DECLARE_METATYPE(Fermentation *)

#endif
