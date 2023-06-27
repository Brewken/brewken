/*======================================================================================================================
 * sortFilterProxyModels/SortFilterProxyModelBase.h is part of Brewken, and is copyright the following authors
 * 2023:
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
#ifndef SORTFILTERPROXYMODELS_SORTFILTERPROXYMODELBASE_H
#define SORTFILTERPROXYMODELS_SORTFILTERPROXYMODELBASE_H
#pragma once

/**
 * \brief Curiously Recurring Template Pattern (CRTP) base class for HopSortFilterProxyModel,
 *        FermentableSortFilterProxyModel, etc
 *
 *           QSortFilterProxyModel     SortFilterProxyModelBase<HopSortFilterProxyModel, HopTableModel>
 *                           \            /
 *                            \          /
 *                          HopSortFilterProxyModel
 *
 *        Derived classes need to implement lessThan to provide the right per-column logic for this.
 */
template<class Derived, class NeTableModel>
class SortFilterProxyModelBase {
public:
   SortFilterProxyModelBase(bool filter) :
      m_filter{filter} {
      return;
   }

protected:
   bool doFilterAcceptsRow(int source_row, QModelIndex const & source_parent) const {
      //
      // Note that sourceModel can be either a subclass of QAbstractListModel (eg StyleListModel) or a subclass of
      // QAbstractTableModel (eg StyleTableModel)
      //
      //                  QAbstractItemModel
      //                     |         |
      //                     |         |
      //      QAbstractListModel      QAbstractTableModel
      //              |                        |
      //              |                        |
      //              |                  BtTableModel   TableModelBase<StyleTableModel, Style>
      //              |                        |         /
      //              |                        |        /
      //              |                       ...      /
      //              |                        |      /
      //              |                        |     /
      //        StyleListModel          StyleTableModel
      //
      // In some cases, we can just treat sourceModel as QAbstractItemModel and rely on virtual member functions, such
      // as index() and data().  In others, we need to cast as:
      //
      //    - getRow() is only in TableModelBase
      //    - at() is only in XxxxListModel
      //
      //
      NeTableModel * model = qobject_cast<NeTableModel *>(this->derived().sourceModel());
      if (model) {
         QModelIndex index = model->index(source_row, 0, source_parent);

         return !m_filter || (model->data(index).toString().contains(this->derived().filterRegExp()) &&
                              model->getRow(source_row)->display());
      }
      // TODO: Fix this!
      Q_ASSERT(false);
   }

   bool doLessThan(QModelIndex const & left, QModelIndex const & right) const {
      QAbstractItemModel * source = this->derived().sourceModel();
      QVariant leftItem, rightItem;
      if (source) {
         leftItem = source->data(left);
         rightItem = source->data(right);
      }

      auto const columnIndex = static_cast<NeTableModel::ColumnIndex>(left.column());
      return this->derived().isLessThan(columnIndex, leftItem, rightItem);
   }

private:
   bool const m_filter;
   Derived const & derived() const {
      return *static_cast<Derived const *>(this);
   }
};


/**
 * \brief Derived classes should include this in their header file, right after Q_OBJECT
 *
 *        Note we have to be careful about comment formats in macro definitions
 */
#define SORT_FILTER_PROXY_MODEL_COMMON_DECL(NeName) \
   /* This allows SortFilterProxyModelBase to call protected and private members of Derived */  \
   friend class SortFilterProxyModelBase<NeName##SortFilterProxyModel,                          \
                                         NeName##TableModel>;                                   \
                                                                                                \
   public:                                                                                      \
      NeName##SortFilterProxyModel(QObject * parent = nullptr, bool filter = true);             \
      virtual ~NeName##SortFilterProxyModel();                                                  \
                                                                                                \
   protected:                                                                                   \
      /* Override QSortFilterProxyModel::filterAcceptsRow                          */           \
      /* Returns true if the item in the row indicated by the given source_row and */           \
      /* source_parent should be included in the model; otherwise returns false.   */           \
      virtual bool filterAcceptsRow(int source_row, QModelIndex const & source_parent) const;   \
      /* Override QSortFilterProxyModel::lessThan                                  */           \
      virtual bool lessThan(QModelIndex const & left, QModelIndex const & right) const;         \
   private:                                                                                     \
      /* Called from lessThan to do the work specific to this class                */           \
      bool isLessThan(NeName##TableModel::ColumnIndex const columnIndex,                        \
                      QVariant const & leftItem,                                                \
                      QVariant const & rightItem) const;                                        \

/**
 * \brief Derived classes should include this in their implementation file
 */
#define SORT_FILTER_PROXY_MODEL_COMMON_CODE(NeName)                                               \
   NeName##SortFilterProxyModel::NeName##SortFilterProxyModel(QObject * parent, bool filter) :    \
      QSortFilterProxyModel{parent},                                                              \
      SortFilterProxyModelBase<NeName##SortFilterProxyModel,                                      \
                                         NeName##TableModel>{filter} {                            \
      return;                                                                                     \
   }                                                                                              \
                                                                                                  \
   NeName##SortFilterProxyModel::~NeName##SortFilterProxyModel() = default;                       \
                                                                                                  \
   bool NeName##SortFilterProxyModel::filterAcceptsRow(int source_row,                            \
                                                       QModelIndex const & source_parent) const { \
      return this->doFilterAcceptsRow(source_row, source_parent);                                 \
   }                                                                                              \
   bool NeName##SortFilterProxyModel::lessThan(QModelIndex const & left,                          \
                                               QModelIndex const & right) const {                 \
      return this->doLessThan(left, right);                                                       \
   }                                                                                              \

#endif
