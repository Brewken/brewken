/*======================================================================================================================
 * ingredientDialogs/IngredientDialog.h is part of Brewken, and is copyright the following authors 2023:
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
#ifndef INGREDIENTDIALOGS_INGREDIENTDIALOG_H
#define INGREDIENTDIALOGS_INGREDIENTDIALOG_H
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

/**
 * \class IngredientDialog
 *
 * \brief See editors/EditorBase.h for the idea behind what we're doing with the class structure here.  The ingredient
 *        dialog classes are "simpler" in that they don't have .ui files, but the use of the of the Curiously Recurring
 *        Template Pattern to minimise code duplication is the same.
 *
 *           QObject
 *                \
 *                ...
 *                  \
 *                  QDialog       IngredientDialog<Hop, HopDialog, HopTableModel, HopSortFilterProxyModel, HopEditor>
 *                        \       /
 *                         \     /
 *                        HopDialog
 *
 *        Besides inheriting from \c QDialog, the derived class (eg \c HopDialog in the example above) needs to
 *        implement the following trivial public slots:
 *
 *           void addIngredient(QModelIndex const &)          -- should call IngredientDialog::add
 *           void removeIngredient()                          -- should call IngredientDialog::remove
 *           void editSelected()                              -- should call IngredientDialog::edit
 *           void newIngredient()                             -- should call IngredientDialog::newItem
 *           void filterIngredients(QString searchExpression) -- should call IngredientDialog::filter
 *
 *        The following protected function overload is also needed:
 *           virtual void changeEvent(QEvent* event)
 *
 *        The code for the definitions of all these functions is "the same" for all editors) can be inserted in
 *        the implementation file using the INGREDIENT_DIALOG_COMMON_CODE macro.  Eg, in HopDialog, we need:
 *
 *          INGREDIENT_DIALOG_COMMON_CODE(HopDialog)
 *
 *        There is not much to the rest of the derived class (eg HopDialog).
 */
template<class NE, class Derived, class NeTableModel, class NeSortFilterProxyModel, class NeEditor>
class IngredientDialog {
public:

   IngredientDialog(MainWindow * parent) :
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
      m_pushButton_addToRecipe{new QPushButton(m_derived)               },
      m_pushButton_new        {new QPushButton(m_derived)               },
      m_pushButton_edit       {new QPushButton(m_derived)               },
      m_pushButton_remove     {new QPushButton(m_derived)               },
      m_neTableModel          {new NeTableModel(m_tableWidget, false)   },
      m_neTableProxy          {new NeSortFilterProxyModel(m_tableWidget)} {

      m_neTableModel->setInventoryEditable(true);
      m_neTableProxy->setSourceModel(m_neTableModel);

      m_tableWidget->setModel(m_neTableProxy);
      m_tableWidget->setSortingEnabled(true);
      m_tableWidget->sortByColumn(static_cast<int>(NeTableModel::ColumnIndex::Name), Qt::AscendingOrder);
      m_neTableProxy->setDynamicSortFilter(true);
      m_neTableProxy->setFilterKeyColumn(1);

      m_qLineEdit_searchBox->setMaxLength(30);
      m_qLineEdit_searchBox->setPlaceholderText("Enter filter");
      m_pushButton_addToRecipe->setObjectName(QStringLiteral("pushButton_addToRecipe"));
      m_pushButton_addToRecipe->setAutoDefault(false);
      m_pushButton_addToRecipe->setDefault(true);
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

      m_horizontalLayout->addWidget(m_qLineEdit_searchBox);
      m_horizontalLayout->addItem(m_horizontalSpacer);
      m_horizontalLayout->addWidget(m_pushButton_addToRecipe);
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
      // We could probably use the same or similar trick to avoid having to declare "public slots" at all in HopDialog,
      // FermentableDialog, etc, but I'm not sure it buys us much.
      m_derived->connect(m_pushButton_addToRecipe, &QAbstractButton::clicked,         m_derived, [this]() { this->add(); return; } );
      m_derived->connect(m_pushButton_edit       , &QAbstractButton::clicked,         m_derived, &Derived::editSelected     );
      m_derived->connect(m_pushButton_remove     , &QAbstractButton::clicked,         m_derived, &Derived::removeIngredient );
      m_derived->connect(m_pushButton_new        , &QAbstractButton::clicked,         m_derived, &Derived::newIngredient    );
      m_derived->connect(m_tableWidget           , &QAbstractItemView::doubleClicked, m_derived, &Derived::addIngredient    );
      m_derived->connect(m_qLineEdit_searchBox   , &QLineEdit::textEdited,            m_derived, &Derived::filterIngredients);

      m_neTableModel->observeDatabase(true);

      return;
   }
   virtual ~IngredientDialog() = default;

   void retranslateUi() {
      m_derived->setWindowTitle(QString(QObject::tr("%1 Database")).arg(NE::staticMetaObject.className()));
      m_pushButton_addToRecipe->setText(QObject::tr("Add to Recipe"));
      m_pushButton_new        ->setText(QObject::tr("New"));
      m_pushButton_edit       ->setText(QString());
      m_pushButton_remove     ->setText(QString());
#ifndef QT_NO_TOOLTIP
      m_pushButton_addToRecipe->setToolTip(QObject::tr("Add selected ingredient to recipe"));
      m_pushButton_new        ->setToolTip(QObject::tr("Create new ingredient"));
      m_pushButton_edit       ->setToolTip(QObject::tr("Edit selected ingredient"));
      m_pushButton_remove     ->setToolTip(QObject::tr("Remove selected ingredient"));
#endif
      return;
   }


   void setEnableAddToRecipe(bool enabled) {
      m_pushButton_addToRecipe->setEnabled(enabled);
      return;
   }

   /**
    * \brief Subclass should call this from its \c addIngredient slot
    *
    *        If \b index is the default, will add the selected ingredient to list. Otherwise, will add the ingredient
    *        at the specified index.
    */
   void add(QModelIndex const & index = QModelIndex()) {
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

      m_parent->addToRecipe(m_neTableModel->getRow(translated.row()));

      return;

   }

   /**
    * \brief Subclass should call this from its \c removeIngredient slot
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
    * \brief Subclass should call this from its \c editIngredient slot
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
    * \brief Subclass should call this from its \c newIngredient slot.  This is also called directly, eg from
    *        \c BtTreeView::newNamedEntity.
    *
    *        Note that the \c newIngredient slot doesn't take a parameter and always relies on the default folder
    *        parameter here, whereas direct callers can specify a folder.
    *
    * \param folder
    */
   void newItem(QString folder = "") {
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
    * \brief Subclass should call this from its \c filterIngredients slot
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
 * \brief Derived classes should include this in their implementation file
 *
 *        Note that we cannot implement changeEvent in the base class (\c IngredientDialog) because it needs access to
 *        \c QDialog::changeEvent, which is \c protected.
 *
 *        With a bit of name concatenation, we could also do the constructor and destructor for the derived class in
 *        this macro.  But, for the moment, I don't think it's worth the extra complexity.
 */
#define INGREDIENT_DIALOG_COMMON_CODE(EditorName) \
   void EditorName::addIngredient(QModelIndex const & index)    { this->add(index);               return; } \
   void EditorName::removeIngredient()                          { this->remove();                 return; } \
   void EditorName::editSelected()                              { this->edit  ();                 return; } \
   void EditorName::newIngredient()                             { this->newItem();                return; } \
   void EditorName::filterIngredients(QString searchExpression) { this->filter(searchExpression); return; } \
   void EditorName::changeEvent(QEvent* event) {     \
      if (event->type() == QEvent::LanguageChange) { \
         this->retranslateUi();                      \
      }                                              \
      this->QDialog::changeEvent(event);             \
      return;                                        \
   }

#endif
