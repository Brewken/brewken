/*======================================================================================================================
 * tableModels/ItemDelegate.h is part of Brewken, and is copyright the following authors 2023:
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
#ifndef TABLEMODELS_ITEMDELEGATE_H
#define TABLEMODELS_ITEMDELEGATE_H
#pragma once

#include <QLineEdit>

#include "utils/NoCopy.h"
#include "widgets/BtBoolComboBox.h"
#include "widgets/BtComboBox.h"

/**
 * \class ItemDelegate
 *
 * \brief Used by \c BtTableModel subclasses
 *
 *           QObject
 *                \
 *                ...
 *                  \
 *                  QItemDelegate       ItemDelegate<HopItemDelegate, HopTableModel>
 *                              \       /
 *                               \     /
 *                           HopItemDelegate
 *
 *        Derived classes (eg \c HopItemDelegate in this example) need to implement the following boilerplate member
 *        functions that override \c QItemDelegate:
 *           createEditor         -- calls ItemDelegate::getEditWidget
 *           setEditorData        -- calls ItemDelegate::readDataFromModel
 *           setModelData         -- calls ItemDelegate::writeDataToModel
 *           updateEditorGeometry -- calls setGeometry on its first parameter
 *
 *        The code for the definitions of all these functions is "the same" for all delegates and should be inserted in
 *        the implementation file using the ITEM_DELEGATE_COMMON_CODE macro.  Eg, in HopItemDelegate, we need:
 *
 *          ITEM_DELEGATE_COMMON_CODE(Hop)
 *
 *
 */
template<class Derived, class NeTableModel>
class ItemDelegate {

public:
   ItemDelegate(NeTableModel & tableModel) : m_derived{static_cast<Derived *>(this)}, m_tableModel{tableModel} {
      return;
   }

   ~ItemDelegate() = default;

private:
   BtTableModel::ColumnInfo const & getColumnInfo(QModelIndex const & index) const {
      auto const columnIndex = static_cast<NeTableModel::ColumnIndex>(index.column());
      //
      // In theory we can get `QAbstractItemModel const *` from `index.model()` and downcast it via QAbstractTableModel
      // to NeTableModel (ie HopTableModel, FermentableTableModel, etc).  In practice, this is a bit painful, eg
      // qobject_cast will return nullptr because it cannot handle the multiple inheritance of the HopTableModel etc
      // classes, and there are problems with using other casts that I didn't get to the bottom of.  Since the table
      // model is known at object construction time, and is not going to change, it's simpler and safer to just grab it
      // in the constructor.
      //
      BtTableModel::ColumnInfo const & columnInfo = m_tableModel.get_ColumnInfo(columnIndex);
      Q_ASSERT(index.column() >= 0);
      Q_ASSERT(columnInfo.index == static_cast<size_t>(index.column()));
      return columnInfo;
   }

public:

   /**
    * \brief Subclass should call this from its override of \c QItemDelegate::createEditor.
    *        Returns the widget used to edit the item specified by index for editing.
    */
   QWidget * getEditWidget(QWidget * parent,
                           [[maybe_unused]] QStyleOptionViewItem const & option,
                           QModelIndex const & index) const {
      BtTableModel::ColumnInfo const & columnInfo = this->getColumnInfo(index);
      TypeInfo const & typeInfo = columnInfo.typeInfo;

      if (std::holds_alternative<NonPhysicalQuantity>(*typeInfo.fieldType)) {
         auto const fieldType = std::get<NonPhysicalQuantity>(*typeInfo.fieldType);

         if (fieldType == NonPhysicalQuantity::Enum) {
            Q_ASSERT(columnInfo.extras);
            Q_ASSERT(std::holds_alternative<BtTableModel::EnumInfo>(*columnInfo.extras));
            BtTableModel::EnumInfo const & enumInfo = std::get<BtTableModel::EnumInfo>(*columnInfo.extras);

            BtComboBox * comboBox = new BtComboBox(parent);
            comboBox->init(columnInfo.tableModelName,
                           columnInfo.columnName,
                           columnInfo.columnFqName,
                           enumInfo.stringMapping,
                           enumInfo.displayNames,
                           typeInfo);
            comboBox->setMinimumWidth(comboBox->minimumSizeHint().width());
            comboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
            comboBox->setFocusPolicy(Qt::StrongFocus);

            return comboBox;
         }

         if (fieldType == NonPhysicalQuantity::Bool) {
            Q_ASSERT(columnInfo.extras);
            Q_ASSERT(std::holds_alternative<BtTableModel::BoolInfo>(*columnInfo.extras));
            BtTableModel::BoolInfo const & boolInfo = std::get<BtTableModel::BoolInfo>(*columnInfo.extras);

            BtBoolComboBox * boolComboBox = new BtBoolComboBox(parent);
            boolComboBox->init(columnInfo.tableModelName,
                               columnInfo.columnName,
                               columnInfo.columnFqName,
                               boolInfo.unsetDisplay,
                               boolInfo.setDisplay,
                               typeInfo);
            boolComboBox->setMinimumWidth(boolComboBox->minimumSizeHint().width());
            boolComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
            boolComboBox->setFocusPolicy(Qt::StrongFocus);

            return boolComboBox;
         }

      }

      return new QLineEdit(parent);
   }

   /**
    * \brief Subclass should call this from its override of \c QItemDelegate::setEditorData.
    *        Sets the data to be displayed and edited by the editor from the data model item specified by the model
    *        index.
    */
   void readDataFromModel(QWidget * editor,
                          QModelIndex const & index) const {
      BtTableModel::ColumnInfo const & columnInfo = this->getColumnInfo(index);
      TypeInfo const & typeInfo = columnInfo.typeInfo;

      // Because index is a run-time value, we need to pull the model data out in a QVariant.  We can use the Qt
      // Property system as we do elsewhere.
      QVariant modelData = m_tableModel.data(index, Qt::EditRole);

      if (std::holds_alternative<NonPhysicalQuantity>(*typeInfo.fieldType)) {
         auto const fieldType = std::get<NonPhysicalQuantity>(*typeInfo.fieldType);

         if (fieldType == NonPhysicalQuantity::Enum) {
            BtComboBox * comboBox = qobject_cast<BtComboBox *>(editor);
            if (typeInfo.isOptional()) {
               bool hasValue = false;
               Optional::removeOptionalWrapper<int>(modelData, &hasValue);
               if (!hasValue) {
                  comboBox->setNull();
                  return;
               }
            }
            comboBox->setValue(modelData.toInt());

            return;
         }

         if (fieldType == NonPhysicalQuantity::Bool) {
            BtBoolComboBox * boolComboBox = qobject_cast<BtBoolComboBox *>(editor);
            if (typeInfo.isOptional()) {
               bool hasValue = false;
               Optional::removeOptionalWrapper<bool>(modelData, &hasValue);
               if (!hasValue) {
                  boolComboBox->setNull();
                  return;
               }
            }
            boolComboBox->setValue(modelData.toBool());
            return;
         }
      }

      // For everything else, apart from handling optional, we can trust QVariant to know how to convert things to text
      QLineEdit * line = qobject_cast<QLineEdit *>(editor);
      if (typeInfo.isOptional()) {
         bool hasValue = false;
         Optional::removeOptionalWrapper(modelData, typeInfo, &hasValue);
         if (!hasValue) {
            line->setText("");
            return;
         }
      }
      line->setText(modelData.toString());
      return;

   }

   /**
    * \brief Subclass should call this from its override of \c QItemDelegate::setModelData.
    *        Gets data from the editor widget and stores it in the specified model at the item index.
    */
   void writeDataToModel(QWidget * editor,
                         QAbstractItemModel * model,
                         QModelIndex const & index) const {
      BtTableModel::ColumnInfo const & columnInfo = this->getColumnInfo(index);
      TypeInfo const & typeInfo = columnInfo.typeInfo;

      // .:TBD:. For the moment, for enums and bools we don't check whether the combo box was changed before calling
      //         model->setData.  We could grab model->data(index, Qt::UserRole) and check it first, if it turns out
      //         this is a problem.
      if (std::holds_alternative<NonPhysicalQuantity>(*typeInfo.fieldType)) {
         auto const fieldType = std::get<NonPhysicalQuantity>(*typeInfo.fieldType);

         if (fieldType == NonPhysicalQuantity::Enum) {
            BtComboBox * comboBox = qobject_cast<BtComboBox *>(editor);
            if (typeInfo.isOptional()) {
               model->setData(index, QVariant::fromValue(comboBox->getOptIntValue()), Qt::EditRole);
            } else {
               model->setData(index, QVariant::fromValue(comboBox->getNonOptIntValue()), Qt::EditRole);
            }
            return;
         }

         if (fieldType == NonPhysicalQuantity::Bool) {
            BtBoolComboBox * boolComboBox = qobject_cast<BtBoolComboBox *>(editor);
            if (typeInfo.isOptional()) {
               model->setData(index, QVariant::fromValue(boolComboBox->getOptBoolValue()), Qt::EditRole);
            } else {
               model->setData(index, QVariant::fromValue(boolComboBox->getNonOptBoolValue()), Qt::EditRole);
            }
            return;
         }
      }

      // Note that we handle any conversions to and from canonical amounts in the table model class, as we sometimes
      // need to look at multiple columns (eg "amount" and "is weight").  This means we also handle optional values
      // there too.  All that we do here is pass in the raw text.
      QLineEdit* line = qobject_cast<QLineEdit*>(editor);
      if (line->isModified()) {
         model->setData(index, line->text(), Qt::EditRole);
      }

      return;
   }

   //================================================ Member Variables =================================================

   /**
    * \brief This is the 'this' pointer downcast to the derived class, which allows us to call non-virtual member
    *        functions in the derived class from this templated base class.
    */
   Derived * m_derived;

   NeTableModel & m_tableModel;

   // Insert all the usual boilerplate to prevent copy/assignment/move
   NO_COPY_DECLARATIONS(ItemDelegate)
};

/**
 * \brief Derived classes should include this in their implementation file
 */
#define ITEM_DELEGATE_COMMON_CODE(NeName) \
   NeName##ItemDelegate::NeName##ItemDelegate(QTableView * parent,               \
                                              NeName##TableModel & tableModel) : \
      QItemDelegate(parent),                                               \
      ItemDelegate<NeName##ItemDelegate, NeName##TableModel>(tableModel) { \
      return; \
   }          \
   NeName##ItemDelegate::~NeName##ItemDelegate() = default; \
   QWidget * NeName##ItemDelegate::createEditor(QWidget * parent,                    \
                                                QStyleOptionViewItem const & option, \
                                                QModelIndex const & index) const {   \
      return this->getEditWidget(parent, option, index);                       \
   }                                                                           \
   void NeName##ItemDelegate::setEditorData(QWidget * editor,                  \
                                            QModelIndex const & index) const { \
      this->readDataFromModel(editor, index); \
      return;                                 \
   }                                          \
   void NeName##ItemDelegate::setModelData(QWidget * editor,                  \
                                           QAbstractItemModel * model,        \
                                           QModelIndex const & index) const { \
      this->writeDataToModel(editor, model, index); \
      return;                                       \
   }                                                \
   void NeName##ItemDelegate::updateEditorGeometry(QWidget * editor,                                   \
                                                   QStyleOptionViewItem const & option,                \
                                                   [[maybe_unused]] QModelIndex const & index) const { \
      editor->setGeometry(option.rect);                                                                \
      return;                                                                                          \
   }                                                                                                   \


#endif
