/*======================================================================================================================
 * tableModels/HopTableModel.h is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Jeff Bailey <skydvr38@verizon.net>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Markus Mårtensson <mackan.90@gmail.com>
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
#ifndef TABLEMODELS_HOPTABLEMODEL_H
#define TABLEMODELS_HOPTABLEMODEL_H
#pragma once

#include <QItemDelegate>
#include <QMetaProperty>
#include <QModelIndex>
#include <QVariant>
#include <QWidget>

#include "tableModels/BtTableModelInventory.h"
#include "tableModels/ItemDelegate.h"
#include "tableModels/TableModelBase.h"

class BtStringConst;
class Hop;
class Recipe;

// You have to get the order of everything right with traits classes, but the end result is that we can refer to
// HopTableModel::ColumnIndex::Alpha etc.
class HopTableModel;
template <> struct TableModelTraits<HopTableModel> {
   enum class ColumnIndex {
      Name     ,
      Alpha    ,
      Amount   ,
      Inventory,
      Form     ,
      Use      ,
      Time     ,
   };
};

/*!
 * \class HopTableModel
 *
 * \brief Model class for a list of hops.
 */
class HopTableModel : public BtTableModelInventory, public TableModelBase<HopTableModel, Hop> {
   Q_OBJECT

public:

   HopTableModel(QTableView* parent=nullptr, bool editable=true);
   virtual ~HopTableModel();

   //
   // This block of functions is called from the TableModelBase class
   //
   void added  (std::shared_ptr<Hop> item);
   void removed(std::shared_ptr<Hop> item);
   void removedAll();

   //! \brief Observe a recipe's list of hops.
   void observeRecipe(Recipe* rec);
   //! \brief If true, we model the database's list of hops.
   void observeDatabase(bool val);
   //! \brief Show ibus in the vertical header.
   void setShowIBUs( bool var );
private:
   //! \brief Watch all the \c hops for changes.
   void addHops(QList< std::shared_ptr<Hop> > hops);

public:
   //! \brief Reimplemented from QAbstractTableModel.
   virtual int rowCount(QModelIndex const & parent = QModelIndex()) const;
   //! \brief Reimplemented from QAbstractTableModel.
   virtual QVariant data(QModelIndex const & index, int role = Qt::DisplayRole) const;
   //! \brief Reimplemented from QAbstractTableModel
   virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
   //! \brief Reimplemented from QAbstractTableModel.
   virtual Qt::ItemFlags flags(const QModelIndex& index) const;
   //! \brief Reimplemented from QAbstractTableModel.
   virtual bool setData(QModelIndex const & index, QVariant const & value, int role = Qt::EditRole);

public slots:
   void changed(QMetaProperty, QVariant);
   void changedInventory(int invKey, BtStringConst const & propertyName);
   //! \brief Add a hop to the model.
   void addHop(int hopId);
   void removeHop(int hopId, std::shared_ptr<QObject> object);

private:
   bool showIBUs; // True if you want to show the IBU contributions in the table rows.
};

//=============================================== CLASS HopItemDelegate ================================================

/*!
 *  \class HopItemDelegate
 *
 *  \brief An item delegate for hop tables.
 *  \sa HopTableModel
 */
class HopItemDelegate : public QItemDelegate,
                               public ItemDelegate<HopItemDelegate, HopTableModel> {
   Q_OBJECT

public:
   HopItemDelegate(QTableView * parent, HopTableModel & tableModel);
   virtual ~HopItemDelegate();

   //! \brief Reimplemented from QItemDelegate.
   virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
   //! \brief Reimplemented from QItemDelegate.
   virtual void setEditorData(QWidget *editor, const QModelIndex &index) const;
   //! \brief Reimplemented from QItemDelegate.
   virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
   //! \brief Reimplemented from QItemDelegate.
   virtual void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif
