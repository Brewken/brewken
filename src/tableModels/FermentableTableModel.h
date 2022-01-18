   /*======================================================================================================================
 * tableModels/FermentableTableModel.h is part of Brewken, and is copyright the following authors 2009-2022:
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
#include "tableModels/BtTableModel.h"

// Forward declarations.
class BtStringConst;
class Fermentable;
class Recipe;
class FermentableItemDelegate;

enum{FERMNAMECOL, FERMTYPECOL, FERMAMOUNTCOL, FERMINVENTORYCOL, FERMISMASHEDCOL, FERMAFTERBOIL, FERMYIELDCOL, FERMCOLORCOL, FERMNUMCOLS /*This one MUST be last*/};

/*!
 * \class FermentableTableModel
 *
 * \brief A table model for a list of fermentables.
 */
class FermentableTableModel : public BtTableModel {
   Q_OBJECT

public:
   FermentableTableModel(QTableView* parent=nullptr, bool editable=true);
   virtual ~FermentableTableModel();

   //! \brief Observe a recipe's list of fermentables.
   void observeRecipe(Recipe* rec);
   //! \brief If true, we model the database's list of fermentables.
   void observeDatabase(bool val);
   //! \brief Watch all the \b ferms for changes.
   void addFermentables(QList<Fermentable*> ferms);
   //! \brief Clear the model.
   void removeAll();
   //! \brief Return the \c i-th fermentable in the model.
   Fermentable* getFermentable(unsigned int i);
   //! \brief True if you want to display percent of each grain in the row header.
   void setDisplayPercentages( bool var );
   /*!
    * \brief True if the inventory column should be editable, false otherwise.
    *
    * The default is that the inventory column is not editable
    */
   void setInventoryEditable( bool var ) { _inventoryEditable = var; }

   //! \brief Reimplemented from QAbstractTableModel.
   virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
   //! \brief Reimplemented from QAbstractTableModel.
   virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
   //! \brief Reimplemented from QAbstractTableModel
   virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
   //! \brief Reimplemented from QAbstractTableModel.
   virtual Qt::ItemFlags flags(const QModelIndex& index ) const;
   //! \brief Reimplemented from QAbstractTableModel.
   virtual bool setData( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole );

   //! \returns true if "ferm" is successfully found and removed.
   bool remove(Fermentable * ferm);

public slots:
   //! \brief Watch \b ferm for changes.
   void addFermentable(int fermId);

   void removeFermentable(int fermId, std::shared_ptr<QObject> object);

private slots:
   //! \brief Catch changes to Recipe, Database, and Fermentable.
   void changed(QMetaProperty, QVariant);
   //! \brief Catches changes to inventory
   void changedInventory(int invKey, BtStringConst const & propertyName);

private:
   //! \brief Recalculate the total amount of grains in the model.
   void updateTotalGrains();

private:
   bool _inventoryEditable;
   QList<Fermentable*> fermObs;
   Recipe* recObs;
   bool displayPercentages;
   double totalFermMass_kg;

};

/*!
 * \brief An item delegate for Fermentable tables.
 * \sa FermentableTableModel.
 */
class FermentableItemDelegate : public QItemDelegate {
   Q_OBJECT

public:
   FermentableItemDelegate(QObject* parent = nullptr);

   //! \brief Reimplemented from QItemDelegate.
   virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
   //! \brief Reimplemented from QItemDelegate.
   virtual void setEditorData(QWidget *editor, const QModelIndex &index) const;
   //! \brief Reimplemented from QItemDelegate.
   virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
   //! \brief Reimplemented from QItemDelegate.
   virtual void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
   //virtual void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;

//public slots:
//   void destroyWidget(QWidget* widget, QAbstractItemDelegate::EndEditHint hint);
private:
};

#endif
