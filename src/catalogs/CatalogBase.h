/*======================================================================================================================
 * catalogs/CatalogBase.h is part of Brewken, and is copyright the following authors 2023:
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
#ifndef CATALOGS_CATALOGBASE_H
#define CATALOGS_CATALOGBASE_H
#pragma once

#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QLineEdit>
#include <QMetaObject>
#include <QPushButton>
#include <QSize>
#include <QSpacerItem>
#include <QStringLiteral>
#include <QTableView>
#include <QVBoxLayout>

#include "database/ObjectStoreWrapper.h"
#include "MainWindow.h"

// TBD: Double-click does different things depending on whether you're looking at list of things in a recipe or
// list of all things.  Propose it should become consistent!

/**
 * \class CatalogBase
 *
 * \brief This is one of the base classes for \c HopCatalog, \c FermentableCatalog, etc.  Essentially each of these
 *        classes is a UI element that shows a list of all model items of a certain type, eg all hops or all
 *        fermentables, etc.
 *
 *        (The classes used to be called \c HopDialog, \c FermentableDialog, etc, which wasn't incorrect, but hopefully
 *        the new names are more descriptive.  In the UI, we also use phrases such as "hop database" for "list of all
 *        types of hop we know about", but that's confusing in the code, where \c Database has a more technical meaning.
 *        So, in the code, we prefer "hop catalog" as a more old-school synonym for "list/directory of all hops" etc.)
 *
 *        See editors/EditorBase.h for the idea behind what we're doing with the class structure here.  These catalog
 *        classes are "simpler" in that they don't have .ui files, but the use of the of the Curiously Recurring
 *        Template Pattern to minimise code duplication is the same.
 *
 *           QObject
 *                \
 *                ...
 *                  \
 *                  QDialog       CatalogBase<HopCatalog, Hop, HopTableModel, HopSortFilterProxyModel, HopEditor>
 *                        \       /
 *                         \     /
 *                        HopCatalog
 *
 *        Because the TableModel classes (\c HopTableModel, \c FermentableTableModel, etc) are doing most of the work,
 *        these Catalog classes are relatively simple.
 *
 *        Classes inheriting from this one need to include the CATALOG_COMMON_DECL macro in their header file and
 *        the CATALOG_COMMON_CODE macro in their .cpp file.
 *
 *        Besides inheriting from \c QDialog, the derived class (eg \c HopCatalog in the example above) needs to
 *        implement the following trivial public slots:
 *
 *           void addItem(QModelIndex const &)          -- should call CatalogBase::add
 *           void removeItem()                          -- should call CatalogBase::remove
 *           void editSelected()                        -- should call CatalogBase::edit
 *           void newItem()                             -- should call CatalogBase::makeNew †
 *           void filterItems(QString searchExpression) -- should call CatalogBase::filter
 *
 *        The following protected function overload is also needed:
 *           virtual void changeEvent(QEvent* event)
 *
 *        the code for the definitions of all these functions is "the same" for all editors, and should be inserted in
 *        the implementation file using the CATALOG_COMMON_CODE macro.  Eg, in HopDialog, we need:
 *
 *          CATALOG_COMMON_CODE(HopDialog)
 *
 *        There is not much to the rest of the derived class (eg HopDialog).
 *
 *        † Not the greatest name, but `new` is a reserved word and `create` is already taken by QWidget
 */
template<class Derived, class NE, class NeTableModel, class NeSortFilterProxyModel, class NeEditor>
class CatalogBase {
public:

   CatalogBase(MainWindow * parent) :
      m_derived               {static_cast<Derived *>(this)             },
      m_parent                {parent                                   },
      m_neEditor              {new NeEditor(m_derived)                  },
      m_verticalLayout        {new QVBoxLayout(m_derived)               },
      m_tableWidget           {new QTableView (m_derived)               },
      m_horizontalLayout      {new QHBoxLayout()                        },
      m_qLineEdit_searchBox   {new QLineEdit()                          },
      m_horizontalSpacer      {new QSpacerItem(40,
                                               20,
                                               QSizePolicy::Expanding,
                                               QSizePolicy::Minimum)    },
      m_pushButton_addToRecipe{this->createAddToRecipeButton()          },
      m_pushButton_new        {new QPushButton(m_derived)               },
      m_pushButton_edit       {new QPushButton(m_derived)               },
      m_pushButton_remove     {new QPushButton(m_derived)               },
      m_neTableModel          {new NeTableModel(m_tableWidget, false)   },
      m_neTableProxy          {new NeSortFilterProxyModel(m_tableWidget)} {

      this->enableEditableInventory();
      m_neTableProxy->setSourceModel(m_neTableModel);

      m_tableWidget->setModel(m_neTableProxy);
      m_tableWidget->setSortingEnabled(true);
      m_tableWidget->sortByColumn(static_cast<int>(NeTableModel::ColumnIndex::Name), Qt::AscendingOrder);
      m_neTableProxy->setDynamicSortFilter(true);
      m_neTableProxy->setFilterKeyColumn(1);

      m_qLineEdit_searchBox->setMaxLength(30);
      m_qLineEdit_searchBox->setPlaceholderText("Enter filter");
      if (m_pushButton_addToRecipe) {
         m_pushButton_addToRecipe->setObjectName(QStringLiteral("pushButton_addToRecipe"));
         m_pushButton_addToRecipe->setAutoDefault(false);
         m_pushButton_addToRecipe->setDefault(true);
      }
      m_pushButton_new->setObjectName(QStringLiteral("pushButton_new"));
      m_pushButton_new->setAutoDefault(false);
      m_pushButton_edit->setObjectName(QStringLiteral("pushButton_edit"));
      QIcon icon;
      icon.addFile(QStringLiteral(":/images/edit.svg"), QSize(), QIcon::Normal, QIcon::Off);
      m_pushButton_edit->setIcon(icon);
      m_pushButton_edit->setAutoDefault(false);
      m_pushButton_remove->setObjectName(QStringLiteral("pushButton_remove"));
      QIcon icon1;
      icon1.addFile(QStringLiteral(":/images/smallMinus.svg"), QSize(), QIcon::Normal, QIcon::Off);
      m_pushButton_remove->setIcon(icon1);
      m_pushButton_remove->setAutoDefault(false);

      // The order we add things to m_horizontalLayout determines their left-to-right order in that layout
      m_horizontalLayout->addWidget(m_qLineEdit_searchBox);
      m_horizontalLayout->addItem(m_horizontalSpacer);
      if (m_pushButton_addToRecipe) {
         m_horizontalLayout->addWidget(m_pushButton_addToRecipe);
      }
      m_horizontalLayout->addWidget(m_pushButton_new);
      m_horizontalLayout->addWidget(m_pushButton_edit);
      m_horizontalLayout->addWidget(m_pushButton_remove);
      m_verticalLayout->addWidget(m_tableWidget);
      m_verticalLayout->addLayout(m_horizontalLayout);


      this->m_derived->resize(800, 300);

      this->retranslateUi();
      QMetaObject::connectSlotsByName(this->m_derived);

      // Note, per https://doc.qt.io/qt-6/signalsandslots-syntaxes.html and
      // https://wiki.qt.io/New_Signal_Slot_Syntax#Default_arguments_in_slot, use of a trivial lambda function to allow
      // a signal with no arguments to connect to a "slot" function with default arguments.
      //
      // We could probably use the same or similar trick to avoid having to declare "public slots" at all in HopCatalog,
      // FermentableCatalog, etc, but I'm not sure it buys us much.
      if (m_pushButton_addToRecipe) {
         m_derived->connect(m_pushButton_addToRecipe, &QAbstractButton::clicked,         m_derived, [this]() { this->add(); return; } );
      }
      m_derived->connect(m_pushButton_edit       , &QAbstractButton::clicked,         m_derived, &Derived::editSelected     );
      m_derived->connect(m_pushButton_remove     , &QAbstractButton::clicked,         m_derived, &Derived::removeItem );
      m_derived->connect(m_pushButton_new        , &QAbstractButton::clicked,         m_derived, &Derived::newItem    );
      m_derived->connect(m_tableWidget           , &QAbstractItemView::doubleClicked, m_derived, &Derived::addItem    );
      m_derived->connect(m_qLineEdit_searchBox   , &QLineEdit::textEdited,            m_derived, &Derived::filterItems);

      m_neTableModel->observeDatabase(true);

      return;
   }
   virtual ~CatalogBase() = default;

   QPushButton * createAddToRecipeButton() requires IsTableModel<NeTableModel> && HasInventory<NeTableModel> {
      return new QPushButton(m_derived);
   }
   QPushButton * createAddToRecipeButton() requires IsTableModel<NeTableModel> && HasNoInventory<NeTableModel> {
      // No-op version
      return nullptr;
   }

   void retranslateUi() {
      m_derived->setWindowTitle(QString(QObject::tr("%1 Catalog / Database")).arg(NE::LocalisedName));
      if (m_pushButton_addToRecipe) {
         m_pushButton_addToRecipe->setText(QString(QObject::tr("Add to Recipe")));
      }
      m_pushButton_new        ->setText(QString(QObject::tr("New")));
      m_pushButton_edit       ->setText(QString());
      m_pushButton_remove     ->setText(QString());
#ifndef QT_NO_TOOLTIP
      if (m_pushButton_addToRecipe) {
         m_pushButton_addToRecipe->setToolTip(QString(QObject::tr("Add selected %1 to recipe")).arg(NE::LocalisedName));
      }
      m_pushButton_new        ->setToolTip(QString(QObject::tr("Create new %1")).arg(NE::LocalisedName));
      m_pushButton_edit       ->setToolTip(QString(QObject::tr("Edit selected %1")).arg(NE::LocalisedName));
      m_pushButton_remove     ->setToolTip(QString(QObject::tr("Remove selected %1")).arg(NE::LocalisedName));
#endif
      return;
   }

   void setEnableAddToRecipe(bool enabled) {
      if (m_pushButton_addToRecipe) {
         m_pushButton_addToRecipe->setEnabled(enabled);
      }
      return;
   }

   void enableEditableInventory() requires IsTableModel<NeTableModel> && HasInventory<NeTableModel> {
      m_neTableModel->setInventoryEditable(true);
      return;
   }
   void enableEditableInventory() requires IsTableModel<NeTableModel> && HasNoInventory<NeTableModel> {
      // No-op version
      return;
   }

   /**
    * \brief Subclass should call this from its \c addItem slot
    *
    *        If \b index is the default, will add the selected ingredient to list. Otherwise, will add the ingredient
    *        at the specified index.
    */
   void add(QModelIndex const & index = QModelIndex()) requires IsTableModel<NeTableModel> && ObservesRecipe<NeTableModel> {
      qDebug() << Q_FUNC_INFO << "Index: " << index;
      QModelIndex translated;

      // If there is no provided index, get the selected index.
      if (!index.isValid()) {
         QModelIndexList selected = m_tableWidget->selectionModel()->selectedIndexes();

         int size = selected.size();
         if (size == 0) {
            return;
         }

         // Make sure only one row is selected.
         int row = selected[0].row();
         for (int i = 1; i < size; ++i) {
            if (selected[i].row() != row) {
               return;
            }
         }

         translated = m_neTableProxy->mapToSource(selected[0]);
      } else {
         // Only respond if the name is selected.  Since we connect to double-click signal, this keeps us from adding
         // something to the recipe when we just want to edit one of the other fields.
         if (index.column() == static_cast<int>(NeTableModel::ColumnIndex::Name)) {
            translated = m_neTableProxy->mapToSource(index);
         } else {
            return;
         }
      }

      qDebug() << Q_FUNC_INFO << "translated.row(): " << translated.row();
      m_parent->addToRecipe(m_neTableModel->getRow(translated.row()));

      return;
   }
   void add([[maybe_unused]] QModelIndex const & index = QModelIndex()) requires IsTableModel<NeTableModel> && DoesNotObserveRecipe<NeTableModel> {
      qDebug() << Q_FUNC_INFO << "No-op";
      // No-op version
      return;
   }

   /**
    * \brief Subclass should call this from its \c removeItem slot
    */
   void remove() {
      QModelIndexList selected = m_tableWidget->selectionModel()->selectedIndexes();

      int size = selected.size();
      if (size == 0) {
         return;
      }

      // Make sure only one row is selected.
      int row = selected[0].row();
      for (int i = 1; i < size; ++i) {
         if (selected[i].row() != row) {
            return;
         }
      }

      QModelIndex translated = m_neTableProxy->mapToSource(selected[0]);
      auto ingredient = m_neTableModel->getRow(translated.row());
      ObjectStoreWrapper::softDelete(*ingredient);
      return;
   }

   /**
    * \brief Subclass should call this from its \c editItem slot
    */
   void edit() {
      QModelIndexList selected = m_tableWidget->selectionModel()->selectedIndexes();

      int size = selected.size();
      if (size == 0) {
         return;
      }

      // Make sure only one row is selected.
      int row = selected[0].row();
      for (int i = 1; i < size; ++i) {
         if (selected[i].row() != row) {
            return;
         }
      }

      QModelIndex translated = m_neTableProxy->mapToSource(selected[0]);
      auto ingredient = m_neTableModel->getRow(translated.row());
      m_neEditor->setEditItem(ingredient);
      m_neEditor->show();
      return;
   }

   /**
    * \brief Subclass should call this from its \c newItem slot.
    *
    *        Note that the \c newItem slot doesn't take a parameter and always relies on the default folder
    *        parameter here, whereas direct callers can specify a folder.
    *
    * TODO: This duplicates EditorBase::newEditItem.  We should just call that instead.
    *
    * \param folder
    */
   void makeNew(QString folder = "") {
      QString name = QInputDialog::getText(this->m_derived,
                                           QString(QObject::tr("%1 name")).arg(NE::staticMetaObject.className()),
                                           QString(QObject::tr("%1 name:")).arg(NE::staticMetaObject.className()));
      if (name.isEmpty()) {
         return;
      }

      auto ingredient = std::make_shared<NE>(name);
      if (!folder.isEmpty()) {
         ingredient->setFolder(folder);
      }

      m_neEditor->setEditItem(ingredient);
      m_neEditor->show();
      return;
   }

   /**
    * \brief Subclass should call this from its \c filterItems slot
    */
   void filter(QString searchExpression) {
      m_neTableProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
      m_neTableProxy->setFilterFixedString(searchExpression);
      return;
   }

   //================================================ Member Variables =================================================

   /**
    * \brief This is the 'this' pointer downcast to the derived class, which allows us to call non-virtual member
    *        functions in the derived class from this templated base class.
    */
   Derived * m_derived;

   // Arguably we don't need to store this pointer as MainWindow is a singleton.  However, we get given it at
   // construction, so, why not...
   MainWindow * m_parent;

   NeEditor *   m_neEditor;

   //! \name Public UI Variables
   //! @{
   QVBoxLayout * m_verticalLayout;
   QTableView *  m_tableWidget;
   QHBoxLayout * m_horizontalLayout;
   QLineEdit *   m_qLineEdit_searchBox;
   QSpacerItem * m_horizontalSpacer;
   QPushButton * m_pushButton_addToRecipe;
   QPushButton * m_pushButton_new;
   QPushButton * m_pushButton_edit;
   QPushButton * m_pushButton_remove;
   //! @}

   NeTableModel *           m_neTableModel;
   NeSortFilterProxyModel * m_neTableProxy;
};

/**
 * \brief Derived classes should include this in their header file, right after Q_OBJECT
 *
 *        Note we have to be careful about comment formats in macro definitions
 */
#define CATALOG_COMMON_DECL(NeName) \
   /* This allows CatalogBase to call protected and private members of Derived */  \
   friend class CatalogBase<NeName##Catalog,                                       \
                            NeName,                                                \
                            NeName##TableModel,                                    \
                            NeName##SortFilterProxyModel,                          \
                            NeName##Editor>;                                       \
                                                                                   \
   public:                                                                         \
      NeName##Catalog(MainWindow * parent);                                        \
      virtual ~NeName##Catalog();                                                  \
                                                                                   \
   public slots:                                                                   \
      void addItem(QModelIndex const & index);                                     \
      void removeItem();                                                           \
      void editSelected();                                                         \
      void newItem();                                                              \
      void filterItems(QString searchExpression);                                  \
                                                                                   \
   protected:                                                                      \
      virtual void changeEvent(QEvent* event);                                     \


/**
 * \brief Derived classes should include this in their implementation file
 *
 *        Note that we cannot implement changeEvent in the base class (\c CatalogBase) because it needs access to
 *        \c QDialog::changeEvent, which is \c protected.
 */
#define CATALOG_COMMON_CODE(NeName) \
   NeName##Catalog::NeName##Catalog(MainWindow* parent) : \
      QDialog(parent),                                    \
      CatalogBase<NeName##Catalog,                        \
                  NeName,                                 \
                  NeName##TableModel,                     \
                  NeName##SortFilterProxyModel,           \
                  NeName##Editor>(parent) {               \
      return;                                             \
   }                                                      \
                                                          \
   NeName##Catalog::~NeName##Catalog() = default;         \
                                                          \
   void NeName##Catalog::addItem(QModelIndex const & index)    { this->add(index);               return; } \
   void NeName##Catalog::removeItem()                          { this->remove();                 return; } \
   void NeName##Catalog::editSelected()                        { this->edit  ();                 return; } \
   void NeName##Catalog::newItem()                             { this->makeNew();                return; } \
   void NeName##Catalog::filterItems(QString searchExpression) { this->filter(searchExpression); return; } \
   void NeName##Catalog::changeEvent(QEvent* event) { \
      if (event->type() == QEvent::LanguageChange) {  \
         this->retranslateUi();                       \
      }                                               \
      this->QDialog::changeEvent(event);              \
      return;                                         \
   }

#endif
