/*======================================================================================================================
 * tableModels/MashStepTableModel.h is part of Brewken, and is copyright the following authors 2009-2023:
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

#include <QItemDelegate>
#include <QMetaProperty>
#include <QModelIndex>
#include <QStyleOptionViewItem>
#include <QVariant>
#include <QVector>
#include <QWidget>

#include "measurement/Unit.h"
#include "model/MashStep.h"
#include "model/Mash.h"
#include "tableModels/BtTableModel.h"
#include "tableModels/ItemDelegate.h"
#include "tableModels/TableModelBase.h"

// You have to get the order of everything right with traits classes, but the end result is that we can refer to
// HopTableModel::ColumnIndex::Alpha etc.
class MashStepTableModel;
template <> struct TableModelTraits<MashStepTableModel> {
   enum class ColumnIndex {
      Name      ,
      Type      ,
      Amount    ,
      Temp      ,
      TargetTemp,
      Time      ,
   };
};

/*!
 * \class MashStepTableModel
 *
 * \brief Model for the list of mash steps in a mash.
 */
class MashStepTableModel : public BtTableModel, public TableModelBase<MashStepTableModel, MashStep> {
   Q_OBJECT

   TABLE_MODEL_COMMON_DECL(MashStep)

public:

///   //! \brief Casting wrapper for \c BtTableModel::getColumnInfo
///   ColumnInfo const & getColumnInfo(ColumnIndex const columnIndex) const;

   /**
    * \brief Set the mash whose mash steps we want to model or reload steps from an existing mash after they were
    *        changed.
    */
   void setMash(Mash * m);

   Mash * getMash() const;

///   //! Reimplemented from QAbstractTableModel.
///   virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
///   //! Reimplemented from QAbstractTableModel.
///   virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
///   //! Reimplemented from QAbstractTableModel.
///   virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
///   //! Reimplemented from QAbstractTableModel.
///   virtual Qt::ItemFlags flags(const QModelIndex& index ) const;
///   //! Reimplemented from QAbstractTableModel.
///   virtual bool setData( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole );

   //! \returns true if mashStep is successfully found and removed.
   bool remove(std::shared_ptr<MashStep> MashStep);

public slots:
   void moveStepUp(int i);
   void moveStepDown(int i);
   void mashChanged();
   void mashStepChanged(QMetaProperty,QVariant);

private:
   Mash* mashObs;

   void reorderMashStep(std::shared_ptr<MashStep> step, int current);
};

//============================================ CLASS MashStepItemDelegate ==============================================

/**
 * \class MashStepItemDelegate
 *
 * \brief An item delegate for hop tables.
 * \sa MashStepTableModel
 */
class MashStepItemDelegate : public QItemDelegate,
                               public ItemDelegate<MashStepItemDelegate, MashStepTableModel> {
   Q_OBJECT

   ITEM_DELEGATE_COMMON_DECL(MashStep)
};

#endif
