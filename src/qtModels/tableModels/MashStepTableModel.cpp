/*======================================================================================================================
 * qtModels/tableModels/MashStepTableModel.cpp is part of Brewken, and is copyright the following authors 2009-2025:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Maxime Lavigne <duguigne@gmail.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
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
#include "qtModels/tableModels/MashStepTableModel.h"

#include <QHeaderView>
#include <QIcon>
#include <QMenu>
#include <QModelIndex>
#include <QTableView>
#include <QVariant>
#include <QWidget>

#include "model/MashStep.h"
#include "model/RecipeUtils.h"
#include "qtModels/tableModels/BtTableModel.h"

#ifdef MANUALLY_INCLUDE_MOC
   // Explicitly doing this include reduces potential problems with AUTOMOC when compiling with CMake
   #include "moc_MashStepTableModel.cpp"
#endif

COLUMN_INFOS(
   MashStepTableModel,
   TABLE_MODEL_HEADER(MashStep, Name        , PropertyNames::NamedEntity::name               ), // "Name"
   TABLE_MODEL_HEADER(MashStep, Type        , PropertyNames::MashStep::type                  ), // "Type"
   TABLE_MODEL_HEADER(MashStep, Amount      , PropertyNames::MashStep::amount_l              ), // "Amount"
   TABLE_MODEL_HEADER(MashStep, Temp        , PropertyNames::MashStep::infuseTemp_c          ), // "Infusion Temp"
   TABLE_MODEL_HEADER(MashStep, TargetTemp  , PropertyNames::StepBase::startTemp_c           ), // "Target Temp"
   TABLE_MODEL_HEADER(MashStep, EndTemp     , PropertyNames::Step::endTemp_c                 ), // "End Temp"
   TABLE_MODEL_HEADER(MashStep, RampTime    , PropertyNames::StepBase::rampTime_mins         ), // "Ramp Time"
   TABLE_MODEL_HEADER(MashStep, Time        , PropertyNames::StepBase::stepTime_mins         ), // "Time"
   TABLE_MODEL_HEADER(MashStep, StartAcidity, PropertyNames::Step::startAcidity_pH           ), // "Start pH"
   TABLE_MODEL_HEADER(MashStep,   EndAcidity, PropertyNames::Step::  endAcidity_pH           ), // "End pH"
   TABLE_MODEL_HEADER(MashStep, WaterToGrain, PropertyNames::MashStep::liquorToGristRatio_lKg), // "Water:Grain"
)

MashStepTableModel::MashStepTableModel(QTableView * parent, bool editable) :
   BtTableModel{parent, editable},
   TableModelBase<MashStepTableModel, MashStep>{},
   EnumeratedItemTableModelBase<MashStepTableModel, MashStep, Mash>{} {
   this->setObjectName("mashStepTableModel");

   QHeaderView* headerView = m_parentTableWidget->horizontalHeader();
   connect(headerView, &QWidget::customContextMenuRequested, this, &MashStepTableModel::contextMenu);
   m_parentTableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
   connect(m_parentTableWidget, &QWidget::customContextMenuRequested, this, &MashStepTableModel::cellContextMenu);
   //
   // See comment in qtModels/tableModels/BoilStepTableModel.cpp for why we don't listen directly to signals from
   // ObjectStore.
   //
   return;
}

MashStepTableModel::~MashStepTableModel() = default;

void MashStepTableModel::added  ([[maybe_unused]] std::shared_ptr<MashStep> item) { return; }
void MashStepTableModel::removed([[maybe_unused]] std::shared_ptr<MashStep> item) { return; }
void MashStepTableModel::updateTotals()                                      { return; }

QVariant MashStepTableModel::data(QModelIndex const & index, int role) const {
   //
   // Show a pin icon on amounts and infusion temperatures that are "fixed", ie that will not be changed when
   // calculating the mash (see MashCalculator and MashEditor::calculate).
   //
   if (this->m_itemOwnerObs && (role == Qt::DecorationRole || role == Qt::ToolTipRole) && this->indexOk(index)) {
      auto const columnIndex = static_cast<MashStepTableModel::ColumnIndex>(index.column());
      auto const & row = this->m_rows[index.row()];
      bool const isFixed =
         (columnIndex == MashStepTableModel::ColumnIndex::Amount && row->amountFixed()) ||
         (columnIndex == MashStepTableModel::ColumnIndex::Temp   && row->infuseTempFixed() &&
                                                                    row->type() != MashStep::Type::Decoction);
      if (isFixed) {
         if (role == Qt::DecorationRole) {
            static QIcon const pinIcon{":/images/smallPin.svg"};
            return pinIcon;
         }
         return tr("Fixed: not changed when calculating the mash");
      }
      return QVariant();
   }

   if (!this->m_itemOwnerObs || !this->indexAndRoleOk(index, role)) {
      return QVariant();
   }

   auto const columnIndex = static_cast<MashStepTableModel::ColumnIndex>(index.column());
   if (MashStepTableModel::ColumnIndex::Temp == columnIndex) {
      auto row = this->m_rows[index.row()];
      if (row->type() == MashStep::Type::Decoction) {
         return QVariant("---");
      }
   }

   // No other special handling required for any of our other columns
   return this->readDataFromModel(index, role);
}

void MashStepTableModel::cellContextMenu(QPoint const & point) {
   if (!this->m_itemOwnerObs) {
      return;
   }
   QModelIndex const index = this->m_parentTableWidget->indexAt(point);
   if (!index.isValid() || index.row() >= static_cast<int>(this->m_rows.size())) {
      return;
   }
   auto const columnIndex = static_cast<MashStepTableModel::ColumnIndex>(index.column());
   std::shared_ptr<MashStep> const step = this->m_rows[index.row()];

   QString actionText;
   bool isFixed = false;
   if (columnIndex == MashStepTableModel::ColumnIndex::Amount) {
      actionText = tr("Fixed amount (not changed when calculating the mash)");
      isFixed = step->amountFixed();
   } else if (columnIndex == MashStepTableModel::ColumnIndex::Temp &&
              step->type() != MashStep::Type::Decoction &&
              step->type() != MashStep::Type::Temperature) {
      actionText = tr("Fixed infusion temperature (not changed when calculating the mash)");
      isFixed = step->infuseTempFixed();
   } else {
      // No menu for other cells
      return;
   }

   QMenu menu{this->m_parentTableWidget};
   QAction * fixedAction = menu.addAction(QIcon{":/images/smallPin.svg"}, actionText);
   fixedAction->setCheckable(true);
   fixedAction->setChecked(isFixed);
   // Same rules as for editing the cell
   bool const canChange =
      this->m_editable &&
      RecipeUtils::lockReason(*this->m_itemOwnerObs) == RecipeUtils::LockReason::NotLocked;
   fixedAction->setEnabled(canChange);

   if (menu.exec(this->m_parentTableWidget->viewport()->mapToGlobal(point)) != fixedAction) {
      return;
   }
   if (columnIndex == MashStepTableModel::ColumnIndex::Amount) {
      step->setAmountFixed(!isFixed);
   } else {
      step->setInfuseTempFixed(!isFixed);
   }
   return;
}

bool MashStepTableModel::setData(QModelIndex const & index, QVariant const & value, int role) {
   if (!this->m_itemOwnerObs) {
      return false;
   }
   return this->doSetDataDefault(index, value, role);
}

// Insert the boiler-plate stuff that we cannot do in TableModelBase
TABLE_MODEL_COMMON_CODE(MashStep, mashStep, PropertyNames::Recipe::mashId)
// Insert the boiler-plate stuff that we cannot do in StepTableModelBase
ENUMERATED_ITEM_TABLE_MODEL_COMMON_CODE(MashStep, Mash)
//=============================================== CLASS MashStepItemDelegate ================================================

// Insert the boiler-plate stuff that we cannot do in ItemDelegate
ITEM_DELEGATE_COMMON_CODE(MashStep)