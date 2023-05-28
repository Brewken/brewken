/*======================================================================================================================
 * tableModels/FermentableTableModel.h is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Jeff Bailey <skydvr38@verizon.net>
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
#ifndef TABLEMODELS_FERMENTABLETABLEMODEL_H
#define TABLEMODELS_FERMENTABLETABLEMODEL_H
#pragma once

#include <memory>

#include <QAbstractItemDelegate>
#include <QItemDelegate>
#include <QList>
#include <QMetaProperty>
#include <QModelIndex>
#include <QVariant>
#include <QWidget>

#include "measurement/Unit.h"
#include "model/Fermentable.h"
#include "tableModels/BtTableModelInventory.h"
#include "tableModels/ItemDelegate.h"
#include "tableModels/TableModelBase.h"

// Forward declarations.
class BtStringConst;
class Fermentable;
class Recipe;

//
// You have to get the order of everything right with traits classes, but the end result is that we can refer to
// FermentableTableModel::ColumnIndex::Color etc.
//
class FermentableTableModel;
template <> struct TableModelTraits<FermentableTableModel> {
   enum class ColumnIndex {
      Name     ,
      Type     ,
      Amount   ,
      Inventory,
      IsWeight ,
      IsMashed ,
      AfterBoil,
      Yield    ,
      Color    ,
   };
};

/*!
 * \class FermentableTableModel
 *
 * \brief A table model for a list of fermentables.
 */
class FermentableTableModel : public BtTableModelInventory, public TableModelBase<FermentableTableModel, Fermentable> {
   Q_OBJECT

public:
   FermentableTableModel(QTableView* parent=nullptr, bool editable=true);
   virtual ~FermentableTableModel();

   //
   // This block of functions is called from the TableModelBase class
   //
   void added  (std::shared_ptr<Fermentable> item);
   void removed(std::shared_ptr<Fermentable> item);
   void removedAll();

   //! \brief Observe a recipe's list of fermentables.
   void observeRecipe(Recipe* rec);
   //! \brief If true, we model the database's list of fermentables.
   void observeDatabase(bool val);
private:
   //! \brief Watch all the \b ferms for changes.
   void addFermentables(QList<std::shared_ptr< Fermentable> > ferms);
public:
   //! \brief True if you want to display percent of each grain in the row header.
   void setDisplayPercentages( bool var );

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
   //! \brief Watch \b ferm for changes.
   void addFermentable(int fermId);

   void removeFermentable(int fermId, std::shared_ptr<QObject> object);

   /**
    * \brief Catch changes to Recipe, Database, and Fermentable.
    *        NB: Needs to be public, not private, as accessed from \c TableModelBase
    */
   void changed(QMetaProperty, QVariant);

private slots:
   //! \brief Catches changes to inventory
   void changedInventory(int invKey, BtStringConst const & propertyName);

private:
   //! \brief Recalculate the total amount of grains in the model.
   void updateTotalGrains();

private:
   bool displayPercentages;

   // .:TODO:.:JSON:.  Now that fermentables can also be measured by volume, we need to rethink this
   double totalFermMass_kg;
};

//=========================================== CLASS FermentableItemDelegate ============================================

/*!
 * \brief An item delegate for Fermentable tables.
 * \sa FermentableTableModel.
 */
class FermentableItemDelegate : public QItemDelegate,
                                public ItemDelegate<FermentableItemDelegate, FermentableTableModel> {
   Q_OBJECT

public:
   FermentableItemDelegate(QTableView * parent, FermentableTableModel & tableModel);
   virtual ~FermentableItemDelegate();

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
