/*======================================================================================================================
 * tableModels/TableModelBase.h is part of Brewken, and is copyright the following authors 2023:
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
#ifndef TABLEMODELS_TABLEMODELBASE_H
#define TABLEMODELS_TABLEMODELBASE_H
#pragma once

#include <QList>
#include <QModelIndex>

#include "measurement/Measurement.h"
#include "tableModels/BtTableModel.h"
#include "utils/MetaTypes.h"

/**
 * \brief We want, eg, \c HopTableModel to inherit from \c TableModelBase<HopTableModel, Hop> and to have its own enum
 *        \c HopTableModel::ColumnIndex.  But we'd also like \c HopTableModel::ColumnIndex to be accessible from within
 *        \c TableModelBase, which normally isn't possible, eg as explained at
 *        https://stackoverflow.com/questions/5534759/c-with-crtp-class-defined-in-the-derived-class-is-not-accessible-in-the-base
 *        Per the same link, the way around this is to use a traits class.  This is another "trick" where we declare
 *        a template for the "traits" class before the base class of the curiously recurring template pattern (CRTP),
 *        but then specialise that "traits" class in the derived class.
 */
template<class Derived>
struct TableModelTraits;

/**
 * \brief See comment in tableModels/BtTableModel.h for more info on inheritance structure
 *
 *        Classes inheriting from this one need to declare the following functions which are then implemented by
 *        including the TABLE_MODEL_COMMON_CODE in the .cpp file:
 *           ColumnInfo const & getColumnInfo(ColumnIndex const columnIndex) const;
 *
 *        Subclasses also need to declare and implement the following functions (with the obvious substitutions for NS):
 *           void added  (std::shared_ptr<NE> item);  // Updates any global info as a result of item being added
 *           void removed(std::shared_ptr<NE> item);  // Updates any global info as a result of item being removed
 *           void removedAll();                       // Updates any global info as a result of all items being removed
 *                                                    // (Better than repeated calls to removed() because avoids
 *                                                    // rounding errors on running totals.)
 */
template<class Derived, class NE>
class TableModelBase {
public:
   // This gets round the fact that we would not be able to access Derived::ColumnIndex directly
   //
   // In theory, in C++20, we don't need the `typename` here, but, per comment in BtTableModel::ColumnInfo, we need to
   // retain it until our Mac build environment is using a more recent version of Clang.
   using ColumnIndex = typename TableModelTraits<Derived>::ColumnIndex;

protected:
   TableModelBase() : m_derived{static_cast<Derived *>(this)}, rows{} {
      return;
   }
   // Need a virtual destructor as we have a virtual member function
   virtual ~TableModelBase() = default;

public:
   /**
    * \brief Casting wrapper for \c BtTableModel::getColumnInfo
    *
    *        Note that, without additional `using` declarations in the derived class we cannot simply call this
    *        \c getColumnInfo as we'd then be trying to have two unconnected base classes participate in the name
    *        resolution (as explained at
    *        https://stackoverflow.com/questions/51690394/overloading-member-function-among-multiple-base-classes).
    */
   BtTableModel::ColumnInfo const & get_ColumnInfo(ColumnIndex const columnIndex) const {
      return m_derived->BtTableModel::getColumnInfo(static_cast<size_t>(columnIndex));
   }

   /**
    * \brief Return the \c i-th row in the model.
    *        Returns \c nullptr on failure.
    */
   std::shared_ptr<NE> getRow(int ii) const {
      if (!(this->rows.isEmpty())) {
         if (ii >= 0 && ii < this->rows.size()) {
            return this->rows[ii];
         }
         qWarning() << Q_FUNC_INFO << "index out of range (" << ii << "/" << this->rows.size() << ")";
      } else {
         qWarning() << Q_FUNC_INFO << "this->rows is empty (" << ii << "/" << this->rows.size() << ")";
      }
      return nullptr;
   }

   /**
    * \brief Remove duplicates and non-displayable items from the supplied list
    */
   QList< std::shared_ptr<NE> > removeDuplicates(QList< std::shared_ptr<NE> > items,
                                                 Recipe const * recipe = nullptr) {
      decltype(items) tmp;

      for (auto ii : items) {
         if (!recipe && (ii->deleted() || !ii->display())) {
               continue;
         }
         if (!this->rows.contains(ii) ) {
            tmp.append(ii);
         }
      }
      return tmp;
   }

   /**
    * \brief Remove duplicates, ignoring if the item is displayed
    */
   QList< std::shared_ptr<NE> > removeDuplicatesIgnoreDisplay(QList< std::shared_ptr<NE> > items,
                                                              Recipe const * recipe = nullptr) {
      decltype(items) tmp;

      for (auto ii : items) {
         if (!recipe && ii->deleted() ) {
            continue;
         }
         if (!this->rows.contains(ii) ) {
            tmp.append(ii);
         }
      }
      return tmp;
   }

   /**
    * \brief Given a raw pointer, find the index of the corresponding shared pointer in \c this->rows
    *
    *        This is useful because the Qt signals and slots framework allows the slot receiving a signal to get a raw
    *        pointer to the object that sent the signal, and we often want to find the corresponding shared pointer in
    *        our list.
    *
    *        Note that using this function is a lot safer than, say, calling ObjectStoreWrapper::getSharedFromRaw(), as
    *        that only works for objects that are already stored in the database, something which is not guaranteed to
    *        be the case with our rows.  (Eg in SaltTableModel, new Salts are only stored in the DB when the window is
    *        closed with OK.)
    *
    *        Function name is for consistency with \c QList::indexOf
    *
    * \param object  what to search for
    * \return index of object in this->rows or -1 if it's not found
    */
   int findIndexOf(NE const * object) const {
      for (int index = 0; index < this->rows.size(); ++index) {
         if (this->rows.at(index).get() == object) {
            return index;
         }
      }
      return -1;
   }

   void add(std::shared_ptr<NE> item) {
      int size = this->rows.size();
      m_derived->beginInsertRows(QModelIndex(), size, size);
      this->rows.append(item);
      m_derived->connect(item.get(), &NamedEntity::changed, m_derived, &Derived::changed);
      m_derived->added(item);
      //reset(); // Tell everybody that the table has changed.
      m_derived->endInsertRows();
      return;
   }

   //! \returns true if \c item is successfully found and removed.
   bool remove(std::shared_ptr<NE> item) {
      int rowNum = this->rows.indexOf(item);
      if (rowNum >= 0)  {
         m_derived->beginRemoveRows(QModelIndex(), rowNum, rowNum);
         m_derived->disconnect(item.get(), nullptr, m_derived, nullptr);
         this->rows.removeAt(rowNum);

         m_derived->removed(item);

         //reset(); // Tell everybody the table has changed.
         m_derived->endRemoveRows();

         return true;
      }

      return false;
   }

protected:

   /**
    * \brief Clear the model.
    */
   void removeAll() {
      int const size = this->rows.size();
      if (size > 0) {
         m_derived->beginRemoveRows(QModelIndex(), 0, size - 1);
         while (!this->rows.empty()) {
            auto item = this->rows.takeLast();
            m_derived->disconnect(item.get(), nullptr, m_derived, nullptr);
            //m_derived->removed(item); // Shouldn't be necessary as we call removedAll() below
         }
         m_derived->endRemoveRows();
         m_derived->removedAll();
      }
      return;
   }

   virtual std::shared_ptr<NamedEntity> getRowAsNamedEntity(int ii) {
      return std::static_pointer_cast<NamedEntity>(this->getRow(ii));
   }

   /**
    * \brief Check supplied index is within bounds
    */
   bool isIndexOk(QModelIndex const & index) const {
      if (index.row() >= static_cast<int>(this->rows.size())) {
         qCritical() << Q_FUNC_INFO << "Bad model index. row = " << index.row() << "; max row = " << this->rows.size();
         return false;
      }

      auto row = this->rows[index.row()];
      if (!row) {
         // This is almost certainly a coding error
         qCritical() << Q_FUNC_INFO << "Null pointer at row" << index.row() << "of" << this->rows.size();
         return false;
      }
      return true;
   }

   /**
    * \brief Child classes should call this from their \c data() member function (overriding
    *        \c QAbstractTableModel::data()) to read data for any column that does not require special handling
    */
   QVariant readDataFromModel(QModelIndex const & index, int const role) const {
      //
      // We assume we are always being called from the Derived::data() member function (eg HopTableModel::data(), etc).
      //
      // Per https://doc.qt.io/qt-6/qt.html#ItemDataRole-enum, there are a dozen or so different "roles" that we can
      // get called for, mostly from the Qt framework itself.  If we don't have anything special to say for a particular
      // role, eg if we don't want to return a custom QFont when requested with Qt::FontRole, then
      // https://doc.qt.io/qt-6/qabstractitemmodel.html#data says we just need to return "an invalid (default-
      // constructed) QVariant".
      //
      if (role != Qt::DisplayRole && role != Qt::EditRole) {
         return QVariant{};
      }

      auto row = this->rows[index.row()];
      auto const columnIndex = static_cast<ColumnIndex>(index.column());
      auto const & columnInfo = this->get_ColumnInfo(columnIndex);
///      qDebug().noquote() << Q_FUNC_INFO << "role = " << role << Logging::getStackTrace();

      QVariant modelData = row->property(*columnInfo.propertyName);
      if (!modelData.isValid()) {
         // It's a programming error if we couldn't read a property modelData
         qCritical() <<
            Q_FUNC_INFO << "Unable to read" << row->metaObject()->className() << "property" <<
            columnInfo.propertyName;
         Q_ASSERT(false); // Stop here on debug builds
      }

      //
      // Unlike in an editor, in the table model, the edit control is only shown when you are actually editing a field.
      // Normally there's a separate control flow for just displaying the modelData otherwise.  We'll get called in both
      // cases, but the modelData of `role` will be different.
      //
      // For Qt::EditRole, we're being called from ItemDelegate::readDataFromModel, which will handle any special
      // display requirements for enums and bools (where, in both cases, we show combo boxes), because it is feeding
      // directly into the appropriate editor widget.  For other types, we want to hand back something that can be
      // converted to QString.
      //
      // For Qt::DisplayRole, we're typically being called from QSortFilterProxyModel::data which is, in turn, called by
      // QItemDelegate::paint.  We don't want to override QItemDelegate::paint in ItemDelegate, because it would be
      // overkill.  So, here, we just need to make sure we're returning something that can sensibly be converted to
      // QString.
      //
      TypeInfo const & typeInfo = columnInfo.typeInfo;
///      qDebug() << Q_FUNC_INFO << columnInfo.columnFqName << "TypeInfo is" << typeInfo;

      bool hasValue = false;
      if (typeInfo.isOptional()) {
         Optional::removeOptionalWrapper(modelData, typeInfo, &hasValue);
         if (!hasValue) {
            return QString{""};
         }
      }

      if (std::holds_alternative<NonPhysicalQuantity>(*typeInfo.fieldType)) {
         auto const fieldType = std::get<NonPhysicalQuantity>(*typeInfo.fieldType);
         if (fieldType == NonPhysicalQuantity::Enum) {
            if (role != Qt::DisplayRole) {
               return modelData;
            }

            Q_ASSERT(columnInfo.extras);
            Q_ASSERT(std::holds_alternative<BtTableModel::EnumInfo>(*columnInfo.extras));
            BtTableModel::EnumInfo const & enumInfo = std::get<BtTableModel::EnumInfo>(*columnInfo.extras);
            Q_ASSERT(modelData.canConvert<int>());
            std::optional<QString> displayText = enumInfo.displayNames.enumAsIntToString(modelData.toInt());
            // It's a coding error if we couldn't find something to display!
            Q_ASSERT(displayText);
            return *displayText;
         }

         if (fieldType == NonPhysicalQuantity::Bool) {
            if (role != Qt::DisplayRole) {
               return modelData;
            }

            Q_ASSERT(columnInfo.extras);
            Q_ASSERT(std::holds_alternative<BtTableModel::BoolInfo>(*columnInfo.extras));
            BtTableModel::BoolInfo const & boolInfo = std::get<BtTableModel::BoolInfo>(*columnInfo.extras);
            Q_ASSERT(modelData.canConvert<bool>());
            return modelData.toBool() ? boolInfo.setDisplay : boolInfo.unsetDisplay;
         }

         if (fieldType == NonPhysicalQuantity::Percentage) {
            unsigned int precision = 3;
            if (columnInfo.extras) {
               Q_ASSERT(std::holds_alternative<BtTableModel::PrecisionInfo>(*columnInfo.extras));
               BtTableModel::PrecisionInfo const & boolInfo = std::get<BtTableModel::PrecisionInfo>(*columnInfo.extras);
               precision = boolInfo.precision;
            }
            // We assert that percentages are numbers and therefore either are double or convertible to double
            Q_ASSERT(modelData.canConvert<double>());
            return QVariant(Measurement::displayQuantity(modelData.toDouble(), precision));
         }
      } else {
         // Most of the handling for Measurement::Mixed2PhysicalQuantities and Measurement::PhysicalQuantity is the
         // same.
         unsigned int precision = 3;
         if (columnInfo.extras) {
            Q_ASSERT(std::holds_alternative<BtTableModel::PrecisionInfo>(*columnInfo.extras));
            BtTableModel::PrecisionInfo const & boolInfo = std::get<BtTableModel::PrecisionInfo>(*columnInfo.extras);
            precision = boolInfo.precision;
         }

         if (typeInfo.typeIndex == typeid(double)) {
            Q_ASSERT(modelData.canConvert<double>());
            double rawValue = modelData.value<double>();
            // This is one of the points where it's important that NamedEntity classes always store data in canonical
            // units.  For any properties where that's _not_ the case, we need to ensure we're passing
            // Measurement::Amount, ie the units are always included.
            Q_ASSERT(std::holds_alternative<Measurement::PhysicalQuantity>(*typeInfo.fieldType));
            auto const physicalQuantity = std::get<Measurement::PhysicalQuantity>(*typeInfo.fieldType);
            Measurement::Amount amount{rawValue, Measurement::Unit::getCanonicalUnit(physicalQuantity)};
            return QVariant(
               Measurement::displayAmount(amount,
                                          precision,
                                          columnInfo.getForcedSystemOfMeasurement(),
                                          columnInfo.getForcedRelativeScale())
            );
         } else if (std::holds_alternative<Measurement::Mixed2PhysicalQuantities>(*typeInfo.fieldType) ||
                    typeInfo.typeIndex == typeid(Measurement::Amount)) {
            //
            // This is pretty useful for handling mass-or-volume amounts etc
            //
            // Note that, although we can downcast MassOrVolumeAmt to Measurement::Amount, QVariant doesn't know about
            // this.  So a QVariant holding MassOrVolumeAmt will return false from canConvert<Measurement::Amount>().
            //
            Measurement::Amount amount;
            if (typeInfo.typeIndex == typeid(MassOrVolumeAmt)) {
               Q_ASSERT(modelData.canConvert<MassOrVolumeAmt>());
               amount = modelData.value<MassOrVolumeAmt>();
            } else if (typeInfo.typeIndex == typeid(MassOrVolumeConcentrationAmt)) {
               Q_ASSERT(modelData.canConvert<MassOrVolumeConcentrationAmt>());
               amount = modelData.value<MassOrVolumeConcentrationAmt>();
            } else if (typeInfo.typeIndex == typeid(Measurement::Amount)) {
               Q_ASSERT(modelData.canConvert<Measurement::Amount>());
               amount = modelData.value<Measurement::Amount>();
            } else {
               // It's a coding error if we get here
               qCritical() <<
                  Q_FUNC_INFO << columnInfo.columnFqName << "Don't know how to parse" << columnInfo.propertyName <<
                  "TypeInfo:" << typeInfo << ", modelData:" << modelData;
               Q_ASSERT(false);
            }
            return QVariant(
               Measurement::displayAmount(amount,
                                          precision,
                                          columnInfo.getForcedSystemOfMeasurement(),
                                          columnInfo.getForcedRelativeScale())
            );
         }
      }

      // If we got here, there's no special handling required - ie the data has no units or special formatting
      // requirements, so we can just return as-is.
      return modelData;
   }

   //================================================ Member Variables =================================================

   /**
    * \brief This is the 'this' pointer downcast to the derived class, which allows us to call non-virtual member
    *        functions in the derived class from this templated base class.
    */
   Derived * m_derived;

   QList< std::shared_ptr<NE> > rows;
};

/**
 * .:TODO:. Can we bin this?
 * \brief Derived classes should include this in their implementation file
 *
 *        We can't define getColumnInfo in TableModelBase because it needs access to Derived::ColumnIndex, which is not
 *        yet declared when the TableModelBase template is instantiated.  There are limits to CRTP!
 */
#define TABLE_MODEL_COMMON_CODE(NeName) \


#endif
