/*======================================================================================================================
 * trees/TreeView.h is part of Brewken, and is copyright the following authors 2009-2025:
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
#ifndef TREES_TREEVIEW_H
#define TREES_TREEVIEW_H
#pragma once

#include <QTreeView>
#include <QWidget>
#include <QPoint>
#include <QMouseEvent>

#include "trees/TreeNode.h"
#include "trees/TreeFilterProxyModel.h"
#include "utils/CuriouslyRecurringTemplateBase.h"
#include "utils/NoCopy.h"

// Forward declarations.
class TreeModel;
class Recipe;
class Equipment;
class Fermentable;
class Hop;
class Mash;
class Misc;
class Yeast;
class BrewNote;
class Style;
class Water;

/*!
 * \class TreeView
 *
 * \brief View class for \c TreeModel.
 */
class TreeView : public QTreeView {
   Q_OBJECT
public:
   //! \brief The standard constructor
   TreeView(QWidget * parent = nullptr /*, TreeModel::TypeMasks mask = TreeModel::TypeMask::Recipe*/);
   //! \brief returns the model associated with this tree
   virtual TreeModel & model() = 0;
///   TreeModel * model() const;
///   //! \b returns the filter associated with this model
///   TreeFilterProxyModel * filter() const;

   //! Called from \c MainWindow::treeActivated
   virtual void activated(QModelIndex const & index) = 0;

   //! \brief returns the context menu associated with the \c selected item
   virtual QMenu * getContextMenu(QModelIndex const & selectedViewIndex) = 0;

   //! \brief Copy the specified items
   virtual void copy(QModelIndexList const & selectedViewIndexes) = 0;
   /**
    * \brief Delete the specified items
    * \return Index of what, if anything,  should now be selected (assuming the deleted items were what were previously
    *         selected).
    */
   virtual std::optional<QModelIndex> deleteItems(QModelIndexList const & selectedViewIndexes) = 0;

   virtual void setSelected(QModelIndex const & index) = 0;

   //! \brief Copy the selected items in this tree
   virtual void copySelected() = 0;
   //! \brief Delete the selected items in this tree
   virtual void deleteSelected() = 0;
   //! \brief Export the selected items in this tree to BeerXML or BeerJSON
   void exportSelected() const;
   //! \brief Import items from BeerXML or BeerJSON
   void importFiles();

   virtual void renameSelected() = 0;

   //! \brief adds a folder to the tree
   virtual void addFolder(QString const & folder) = 0;

   virtual QString folderName(QModelIndex const & viewIndex) const = 0;

protected:
///   virtual QMimeData * mimeData(QModelIndexList const & indexes) = 0;

///»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»
public:
///   //! \brief removes \c index item from the tree returns true if the remove works
///   bool removeRow(const QModelIndex & index);
///   //! \brief returns true if \c parent is the parent of \c child
///   bool isParent(const QModelIndex & parent, const QModelIndex & child);

///   //! \brief returns the parent of \c child
///   QModelIndex parent(const QModelIndex & child);
///   //! \brief returns the first \c type element in the tree
///   QModelIndex first();
///
///   QModelIndex findElement(NamedEntity * thing);

///   /**
///    * \brief returns the item at \c index
///    *        Valid for \c Recipe, \c Equipment, \c Fermentable, \c Hop, \c Misc, \c Yeast, \c Style, \c BrewNote,
///    *        \c Water, \c Folder.
///    */
///   template<class T>
///   T * getItem(QModelIndex const & index) const;

///   //! \brief finds the index of the \c folder in the tree,but does not create
///   QModelIndex findFolder(Folder * folder);
///   //! \brief adds a folder to the tree
///   void addFolder(QString folder);
///   //! \brief renames a folder and all of its subitems
///   void renameFolder(Folder * victim, QString newName);
///   QString folderName(QModelIndex starter);

   //! \return the classifier of the item at \c index, or \c nullopt if \c index is invalid
   std::optional<TreeNodeClassifier> classifier(QModelIndex const & index) const;

///   //! \brief returns true if a recipe and an ingredient (hop, equipment, etc.) are selected at the same time
///   bool multiSelected();

   // Another try at drag and drop
   //! \brief Overrides \c QTreeView::mousePressEvent.  Starts a drag and drop event
   virtual void mousePressEvent(QMouseEvent * event) override;
   //! \brief Overrides \c QTreeView::mouseMoveEvent.  Distinguishes between a move event and a double click
   virtual void mouseMoveEvent(QMouseEvent * event) override;
   //! \brief Overrides \c QTreeView::mouseDoubleClickEvent.  Recognizes a double click event
   virtual void mouseDoubleClickEvent(QMouseEvent * event) override;

   //! \brief Overrides \c QTreeView::keyPressEvent.  Catches a key stroke in a tree
   virtual void keyPressEvent(QKeyEvent * event) override;

///   //! \brief creates a context menu based on the type of tree
///   void setupContextMenu(QWidget * top, QWidget * editor);

///   //! \brief sets a new filter
///   void setFilter(TreeFilterProxyModel * newFilter);

///   /**
///    * \brief Delete the selected nodes and return the closest node that could become the new selected one.  Eg, if nodes
///    *        [n, m] are to be deleted then, if node m+1 exists, it will become node n after the deletion and thus the
///    *        new current selection.  If there is no node m+1, then node n-1, if it exists, will become the selected one.
///    *        (The actual logic is a bit more complicated than this because we have to account for folders, but this
///    *        gives the idea.)
///    */
///   QModelIndex deleteSelected(QModelIndexList selected);
///
///   void copySelected(QModelIndexList selected);
///   // Friend classes. For the most part, the children don't do much beyond
///   // contructors and context menus. So far :/
///   friend class RecipeTreeView;
///   friend class EquipmentTreeView;
///   friend class FermentableTreeView;
///   friend class HopTreeView;
///   friend class MiscTreeView;
///   friend class YeastTreeView;
///   friend class StyleTreeView;
///   friend class WaterTreeView;

///public slots:
///   void newNamedEntity();

///private slots:
///   void expandFolder(TreeModel::TypeMasks kindaThing, QModelIndex fIdx);



protected:
///   TreeFilterProxyModel * m_filter;
///   TreeModel::TypeMasks m_type;
///   QMenu * m_contextMenu;
///   QMenu * subMenu;
///   QMenu * m_versionMenu;
///   QMenu * m_exportMenu;
///   QAction *       m_deleteAction;
///   QAction *         m_copyAction;
///   QAction *       m_brewItAction;
   QPoint dragStart;
///   QWidget * m_editor;

   bool doubleClick;

///   int verifyDelete(int confirmDelete, QString tag, QString name);
///   QString verifyCopy(QString tag, QString name, bool * abort);

private:
   // Insert all the usual boilerplate to prevent copy/assignment/move
   NO_COPY_DECLARATIONS(TreeView)
};

/////======================================================================================================================
///template<class Derived> class TreeViewPhantom;
///template<class Derived, class NE>
///class TreeViewBase : public CuriouslyRecurringTemplateBase<TreeViewPhantom, Derived> {
///public:
///   TreeViewBase() {
///      return;
///   }
///   ~TreeViewBase() = default;
///
///   void doConnections() requires std::same_as<Recipe, Derived> {
///      this->derived().connect(this->derived().m_model, &TreeModel::recipeSpawn, &this->derived(), &TreeView::versionedRecipe);
///      return;
///   }
///
///   void doConnections() requires (!std::same_as<Recipe, Derived>) {
///      return;
///   }
///
///
///};
/////======================================================================================================================
///#define TREE_VIEW_COMMON_DECL(NeName)                                                ¥
///class NeName##TreeView : public TreeView,                                            ¥
///                         public TreeViewBase<NeName##TreeView, NeName> {             ¥
///   Q_OBJECT                                                                          ¥
///                                                                                     ¥
///   /* This allows TreeViewBase to call protected and private members of Derived */   ¥
///   friend class TreeViewBase<NeName##TreeView, NeName>;                              ¥
///                                                                                     ¥
///   public:                                                                           ¥
///      /* Constructs the tree view, sets up the filter proxy and sets a */            ¥
///      /* few options on the tree that can only be set after the model  */            ¥
///      NeName##TreeView(QWidget * parent = nullptr);                                  ¥
///      virtual ~NeName##TreeView();                                                   ¥
///};                                                                                   ¥
///
///TREE_VIEW_COMMON_DECL(Recipe)
///TREE_VIEW_COMMON_DECL(Equipment)
///TREE_VIEW_COMMON_DECL(Fermentable)
///TREE_VIEW_COMMON_DECL(Hop)
///TREE_VIEW_COMMON_DECL(Misc)
///TREE_VIEW_COMMON_DECL(Yeast)
///TREE_VIEW_COMMON_DECL(Style)
///TREE_VIEW_COMMON_DECL(Water)
///TREE_VIEW_COMMON_DECL(Mash)
///
///#define TREE_VIEW_COMMON_CODE(NeName)                     ¥
///   NeName##TreeView::NeName##TreeView(QWidget * parent) : ¥
///      TreeView(parent, TreeModel::TypeMask::NeName),      ¥
///      TreeViewBase<NeName##TreeView, NeName>() {          ¥
///      this->doConnections();                              ¥
///      return;                                             ¥
///   }                                                      ¥
///                                                          ¥
///   NeName##TreeView::~NeName##TreeView() = default;       ¥
///

#endif
