/*======================================================================================================================
 * qtModels/tableModels/MashStepTableModel.h is part of Brewken, and is copyright the following authors 2009-2025:
 *   • Jeff Bailey <skydvr38@verizon.net>
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
#ifndef TABLEMODELS_MASHSTEPTABLEMODEL_H
#define TABLEMODELS_MASHSTEPTABLEMODEL_H
#pragma once

#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QStringList>
#include <QStyledItemDelegate>

#include "model/MashStep.h"
#include "model/Mash.h"
#include "qtModels/tableModels/BtTableModel.h"
#include "qtModels/tableModels/EnumeratedItemTableModelBase.h"
#include "qtModels/tableModels/ItemDelegate.h"
#include "qtModels/tableModels/TableModelBase.h"

class Recipe;

// Define the columns on this table
COLUMN_NAMES(MashStepTableModel, Name        ,
                                 Type        ,
                                 Amount      ,
                                 Temp        ,
                                 TargetTemp  ,
                                 EndTemp     ,
                                 RampTime    ,
                                 Time        ,
                                 StartAcidity,
                                 EndAcidity  ,
                                 WaterToGrain,)

/*!
 * \class MashStepTableModel
 *
 * \brief Model for the list of mash steps in a mash.
 */
class MashStepTableModel : public BtTableModel,
                           public TableModelBase<MashStepTableModel, MashStep>,
                           public EnumeratedItemTableModelBase<MashStepTableModel, MashStep, Mash> {
   Q_OBJECT

   TABLE_MODEL_COMMON_DECL(MashStep)
   ENUMERATED_ITEM_TABLE_MODEL_COMMON_DECL(MashStep, Mash)

public slots:
   /**
    * \brief Context menu for a cell in the table.  For the amount and infusion temperature columns, this allows the user
    *        to toggle whether the value is "fixed", ie not changed when calculating the mash (see \c MashCalculator).
    */
   void cellContextMenu(QPoint const & point);

   /**
    * \brief Re-check the steps (see \c validate) the next time we get back to the event loop.  Multiple calls before
    *        then result in only one check.
    */
   void scheduleValidation();

signals:
   //! Emitted when the results of checking the steps (see \c validate) change
   void validationChanged();

public:
   /**
    * \return \c true if the steps are being checked, ie the mash is used by exactly one recipe
    */
   bool isValidating() const;

   //! \return \c true if checking the steps found any problems, either with individual steps or the mash as a whole
   bool hasProblems() const;

   /**
    * \return Description of all the problems found when checking the steps, one per line, plus any overall problems
    *         with the mash, for showing to the user
    */
   [[nodiscard]] QString allProblemsDisplayable() const;

   //! \return Problems found with the mash as a whole (eg the water not adding up to what the recipe needs)
   [[nodiscard]] QStringList const & mashOverallProblems() const;

private:
   /**
    * \brief If the mash is used by exactly one recipe, check whether its steps make sense for that recipe (see
    *        \c MashCalculator::check), so we can highlight the ones that don't.  (If the mash is used by more than one
    *        recipe, it might make sense for some and not others.)
    *
    *        We re-check whenever the steps, the mash, the recipe or the recipe's equipment change.
    */
   void validate();

   struct StepProblems {
      bool        isError  = false;
      QStringList messages = {};
      bool operator==(StepProblems const & other) const = default;
   };
   QHash<MashStep const *, StepProblems> m_stepProblems;
   QStringList                           m_mashProblems;
   Recipe *                              m_validationRecipe    = nullptr;
   bool                                  m_validationScheduled = false;
   QList<QMetaObject::Connection>        m_validationConnections;
};

//============================================ CLASS MashStepItemDelegate ==============================================

/**
 * \class MashStepItemDelegate
 *
 * \brief An item delegate for hop tables.
 * \sa MashStepTableModel
 */
class MashStepItemDelegate : public QStyledItemDelegate,
                             public ItemDelegate<MashStepItemDelegate, MashStepTableModel> {
   Q_OBJECT

   ITEM_DELEGATE_COMMON_DECL(MashStep)
};

#endif