/*======================================================================================================================
 * tableModels/BtTableModel.h is part of Brewken, and is copyright the following authors 2021-2023:
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
#ifndef TABLEMODELS_BTTABLEMODEL_H
#define TABLEMODELS_BTTABLEMODEL_H
#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <QAbstractTableModel>
#include <QDebug>
#include <QHeaderView>
#include <QMap>
#include <QMenu>
#include <QPoint>
#include <QTableView>

#include "BtFieldType.h"
#include "measurement/UnitSystem.h"
#include "model/NamedEntity.h"
#include "utils/BtStringConst.h"
#include "utils/EnumStringMapping.h"

class Recipe;

/**
 * \class BtTableModelData
 *
 * \brief Unfortunately we can't template \c BtTableModel because it inherits from a \c QObject and the Qt meta-object
 *        compiler (moc) can't handle templated classes in QObject-derived classes (though it is fine with templated
 *        member functions in such classes, as long as they are not signals or slots).  We might one day look at
 *        https://github.com/woboq/verdigris, which overcomes these limitations, but, for now, we live within Qt's
 *        constraints and try to pull out as much common code as possible using a limited form of multiple inheritance
 *        and the Curiously Recurring Template Pattern.
 *
 *              QObject
 *                   \
 *                   ...
 *                     \
 *              QAbstractTableModel
 *                           \
 *                            \
 *                          BtTableModel               TableModelBase<NE, xxxTableModel>
 *                                /   \                /     /     /
 *                               /     \              /     /     /
 *                              /      MashStepTableModel  /     /
 *                             /                          /     /
 *                            /                          /     /
 *                         BtTableModelRecipeObserver   /     /
 *                              \       \              /     /
 *                               \       \            /     /
 *                                \      SaltTableModel    /
 *                                 \    WaterTableModel   /
 *                                  \                    /
 *                                   \                  /
 *                         BtTableModelInventory       /
 *                                    \               /
 *                                     \             /
 *                                FermentableTableModel
 *                                    HopTableModel
 *                                   MiscTableModel
 *                                   YeastTableModel
 *
 *        Eg HopTableModel inherits from BtTableModelInventory and TableModelBase<Hop, HopTableModel>
 *
 *        The \c BtTableModelRecipeObserver class adds a pointer to a \c Recipe to \c BtTableModel.
 *        The \c BtTableModelInventory class adds a boolean \c inventoryEditable flag to \c BtTableModelRecipeObserver.
 *
 *        TBD: I did start trying to do something clever with a common base class to try to expose functions from
 *        \c BtTableModelData to the implementation of \c BtTableModel / \c BtTableModelRecipeObserver /
 *        \c BtTableModelInventory, but it quickly gets more complicated than it's worth IMHO because (a)
 *        \c BtTableModelData is templated but a common base class cannot be and (b) templated functions cannot be
 *        virtual.  Maybe we can do something the other way around with the curiously recurring template pattern.
 */
template<class NE>
class BtTableModelData {
protected:
   BtTableModelData() : rows{} {
      return;
   }
   // Need a virtual destructor as we have a virtual member function
   virtual ~BtTableModelData() = default;
public:
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
   QList< std::shared_ptr<NE> > removeDuplicates(QList< std::shared_ptr<NE> > items, Recipe const * recipe = nullptr) {
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
   QList< std::shared_ptr<NE> > removeDuplicatesIgnoreDisplay(QList< std::shared_ptr<NE> > items, Recipe const * recipe = nullptr) {
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

protected:
   virtual std::shared_ptr<NamedEntity> getRowAsNamedEntity(int ii) {
      return std::static_pointer_cast<NamedEntity>(this->getRow(ii));
   }

   QList< std::shared_ptr<NE> > rows;
};

/*!
 * \class BtTableModel
 *
 * \brief Shared interface & code for all the table models we use
 */
class BtTableModel : public QAbstractTableModel {
   Q_OBJECT
public:
   /**
    * \brief Extra info stored in \c ColumnInfo (see below) for enum types
    */
   struct EnumInfo {
      /**
       * \brief Values to store in combo box
       */
      EnumStringMapping const & stringMapping;

      /**
       * \brief Localised display names to show on combo box
       */
      EnumStringMapping const & displayNames;
   };

   /**
    * \brief Extra info stored in \c ColumnInfo (see below) for boolean types
    *
    *        For most boolean types, we show a combo box (eg "Not mashed" / "Mashed"; "Weight" / "Volume")
    *
    *        These are QString rather than reference to QString as typically initialised with an rvalue (the result of
    *        calling \c QObject::tr).
    */
   struct BoolInfo {
      QString const unsetDisplay;
      QString const setDisplay;
   };

   /**
    * \brief I know we don't need a struct for this, but it's more consistent to use one
    */
   struct PrecisionInfo {
      unsigned int const precision;
   };

   /**
    * \brief This per-column struct / mini-class holds basic info about each column in the table.  It also plays a
    *        slightly similar role as \c SmartLabel.  However, there are several important differences, including that
    *        \c ColumnInfo is \b not a \c QWidget and therefore not a signal emitter.  (As mentioned below, it is
    *        \c QHeaderView that sends us the signal about the user having right-clicked on a column header.  We then
    *        act on the pop-up menu selections directly, rather than \c SmartLabel sending a signal that
    *        \c SmartLineEdit (and sometimes others) pick up.
    *
    *        NOTE that you usually want to use the SMART_COLUMN_HEADER_INIT macro when constructing
    */
   struct ColumnInfo {
      /**
       * \brief By analogy with \c editorName in \c SmartLabel and \c SmartLineEdit
       */
      char const * const tableModelName;

      /**
       * \brief By analogy with \c labelName in \c SmartLabel and \c lineEditName in \c SmartLineEdit
       */
      char const * const columnName;

      /**
       * \brief By analogy with \c labelFqName in \c SmartLabel and \c lineEditFqName in \c SmartLineEdit
       */
      char const * const columnFqName;

      /**
       * \brief Each subclass should normally declare its own \c enum \c class \c ColumnIndex to identify its columns.
       *        We store the column index here as a cross-check that we've got everything in the right order.
       */
      size_t const index;

      /**
       * \brief The localised text to display in this column header
       */
      QString const label;

      /**
       * \brief
       */
      BtStringConst const & propertyName;

      /**
       * \brief What type of data is shown in this column
       */
      TypeInfo const & typeInfo;

      /**
       * \brief Extra info, dependent on the type of value in the column
       */
      std::optional<std::variant<EnumInfo, BoolInfo, PrecisionInfo>> extras = std::nullopt;

      // Stuff for setting display units and scales -- per column
      // I know it looks odd to have const setters, but they are const because they do not change the data in the struct
      void setForcedSystemOfMeasurement(std::optional<Measurement::SystemOfMeasurement> forcedSystemOfMeasurement) const;
      void setForcedRelativeScale(std::optional<Measurement::UnitSystem::RelativeScale> forcedScale) const;
      std::optional<Measurement::SystemOfMeasurement> getForcedSystemOfMeasurement() const;
      std::optional<Measurement::UnitSystem::RelativeScale> getForcedRelativeScale() const;
   };

   /**
    * \brief
    *
    * \param parent
    * \param editable
    * \param columnInfos Needs to be in order
    */
   BtTableModel(QTableView * parent,
                bool editable,
                std::initializer_list<ColumnInfo> columnInfos);
   virtual ~BtTableModel();

   ColumnInfo const & getColumnInfo(size_t const columnIndex) const;

   //! \brief Called from \c headerData()
   QVariant getColumnLabel(size_t const columnIndex) const;

   // Per https://doc.qt.io/qt-5/qabstracttablemodel.html, when subclassing QAbstractTableModel, you must implement
   // rowCount(), columnCount(), and data(). Default implementations of the index() and parent() functions are provided
   // by QAbstractTableModel. Well behaved models will also implement headerData().

   //! \brief Reimplemented from QAbstractTableModel
   virtual int columnCount(QModelIndex const & parent = QModelIndex()) const;

   // These are protected member functions of QAbstractItemModel that we need to make public so that
   // TableModelBase::removeAll() can access them
   using QAbstractItemModel::beginInsertRows;
   using QAbstractItemModel::endInsertRows;
   using QAbstractItemModel::beginRemoveRows;
   using QAbstractItemModel::endRemoveRows;

///private:
///   void doContextMenu(QPoint const & point, QHeaderView * hView, QMenu * menu, int selected);

public slots:
   //! \brief Receives the \c QWidget::customContextMenuRequested signal from \c QHeaderView to pops the context menu
   // for changing units and scales
   void contextMenu(QPoint const & point);

protected:
   QTableView* parentTableWidget;
   bool editable;
private:
   /**
    * \brief We're using a \c std::vector here because it's easier for constant lists.  (With \c QVector, at last in
    *        Qt 5, the items stored even in a const instance still need to be default constructable and copyable.)
    */
   std::vector<ColumnInfo> const m_columnInfos;
};

class BtTableModelRecipeObserver : public BtTableModel {
public:
   BtTableModelRecipeObserver(QTableView * parent,
                              bool editable,
                              std::initializer_list<ColumnInfo> columnInfos);
   ~BtTableModelRecipeObserver();

   // Normally this would be protected, but it needs to be public for TableModelBase to access
   Recipe * recObs;
};

/**
 * \brief This macro saves a bit of copy-and-paste when calling the \c BtTableModel::ColumnInfo constructor.  Eg instead
 *        of writing:
 *
 *           BtTableModel::ColumnInfo{"HopTableModel",
 *                                    "Alpha",
 *                                    "HopTableModel::ColumnIndex::Alpha",
 *                                    static_cast<size_t>(HopTableModel::ColumnIndex::Alpha),
 *                                    tr("Alpha %"),
 *                                    Hop::typeLookup.getType(PropertyNames::Hop::alpha)}
 *
 *        you write:
 *
 *           SMART_COLUMN_HEADER_DEFN(HopTableModel, Alpha, tr("Alpha %"), Hop, PropertyNames::Hop::alpha);
 *
 *        Arguments not specified below are passed in to initialise \c BtTableModel::ColumnInfo::extra.
 *
 * \param tableModelClass The class name of the class holding the field we're initialising, eg \c HopTableModel.
 * \param columnName
 * \param labelText
 * \param modelClass The subclass of \c NamedEntity that we're editing.  Eg in \c HopEditor, this will be \c Hop
 * \param propertyName The name of the property to which this field relates, eg in \c HopEditor, this could be
 *                     \c PropertyNames::NamedEntity::name, \c PropertyNames::Hop::alpha_pct, etc.  (Note, as per
 *                     comments in \c widgets/SmartAmounts.h, we intentionally do \b not automatically insert the
 *                     \c PropertyNames:: prefix.)
 */
#define SMART_COLUMN_HEADER_DEFN(tableModelClass, columnName, labelText, modelClass, propertyName, ...) \
   BtTableModel::ColumnInfo{#tableModelClass, \
                            #columnName, \
                            #tableModelClass "::ColumnIndex::" #columnName, \
                            static_cast<size_t>(tableModelClass::ColumnIndex::columnName), \
                            labelText, \
                            propertyName, \
                            modelClass::typeLookup.getType(propertyName) \
                            __VA_OPT__(, __VA_ARGS__)}
#endif
