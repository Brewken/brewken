/**
 * EquipmentListModel.h is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Matt Young <mfsy@yahoo.com>
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
 */
#ifndef EQUIPMENTLISTMODEL_H
#define EQUIPMENTLISTMODEL_H
#pragma once

#include <memory>

#include <QAbstractListModel>
#include <QList>
#include <QMetaProperty>
#include <QModelIndex>
#include <QVariant>

// Forward declarations.
class Equipment;
class Recipe;

/*!
 * \class EquipmentListModel
 *
 * \brief Model for a list of equipments.
 */
class EquipmentListModel : public QAbstractListModel
{
   Q_OBJECT

public:
   EquipmentListModel(QWidget* parent = 0);

   //! \brief Reimplemented from QAbstractListModel.
   virtual int rowCount( QModelIndex const& parent = QModelIndex() ) const;
   //! \brief Reimplemented from QAbstractListModel.
   virtual QVariant data( QModelIndex const& index, int role = Qt::DisplayRole ) const;
   //! \brief Reimplemented from QAbstractListModel.
   virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

   void observeRecipe(Recipe* rec);
   //! \brief Add many equipments to the list.
   void addEquipments(QList<Equipment*> equips);
   //! \brief Remove all equipments from the list.
   void removeAll();

   //! \brief Return the equipment at the index in the list.
   Equipment* at(int ndx);
   //! \brief Return the index of a particular equipment. DEPRECATED.
   int indexOf(Equipment* e);
   //! \brief Return the index of a particular equipment.
   QModelIndex find(Equipment* e);

public slots:
   void recChanged(QMetaProperty,QVariant);
   void equipChanged(QMetaProperty,QVariant);

   //! Add an equipment to the list.
   void addEquipment(int equipmentId);
   //! Remove an equipment from the list.
   void removeEquipment(int equipmentId, std::shared_ptr<QObject> object);

private:
   QList<Equipment*> equipments;
   Recipe* recipe;

   void repopulateList();
};

#endif
