/*======================================================================================================================
 * trees/TreeNode.h is part of Brewken, and is copyright the following authors 2009-2025:
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
#ifndef TREES_TREENODE_H
#define TREES_TREENODE_H
#pragma once

#include <QDebug>
#include <QIcon>
#include <QList>
#include <QObject>
#include <QModelIndex>
#include <QVariant>

#include "Localization.h"
#include "RecipeFormatter.h"
#include "config.h"
#include "measurement/Measurement.h"
#include "model/BrewNote.h"
#include "model/Equipment.h"
#include "model/Fermentable.h"
#include "model/Folder.h"
#include "model/Hop.h"
#include "model/Misc.h"
#include "model/NamedEntity.h"
#include "model/Recipe.h"
#include "model/Style.h"
#include "model/Water.h"
#include "model/Yeast.h"
#include "utils/CuriouslyRecurringTemplateBase.h"
#include "utils/EnumStringMapping.h"
#include "utils/NoCopy.h"

/**
 * \brief Each tree has one primary type of object that it stores.  However, some trees (eg Recipe, Mash) can hold
 *        secondary items (eg Recipe tree holds Recipes and BrewNotes owned by those Recipes).  It's useful to have a
 *        compile-time mapping from object type to show which class belongs in which tree.  The rule here is that things
 *        belong in their own tree (eg Equipment is in Equipment tree) unless there's a specialisation that says
 *        otherwise.
 */
template <class NE> struct TreeTypeDeducer           { using TreeType = NE    ; };
template<>          struct TreeTypeDeducer<BrewNote> { using TreeType = Recipe; };

class TreeModel;

/**
 * \brief See comment in qtModels/tableModels/TableModelBase.h for why we use a traits class to allow the following attributes
 *        from each \c Derived class to be accessible in \c TreeNodeBase:
 *           - \c ColumnIndex        = class enum for the columns of this node type
 *           - \c NumberOfColumns    = number of entries in the above.  (Yes, it is a bit frustrating that we cannot
 *                                     easily deduce the number of values of a class enum.  Hopefully this will change
 *                                     in future versions of C++.)
 *           - \c NodeClassifier     = \c TreeNodeClassifier for this node type
 *           - \c ParentPtrTypes     = std::variant of raw pointers to valid parent types
 *           - \c ChildPtrTypes      = std::variant of shared_ptrs to valid child types (or
 *                                     std::variant<std::monostate> if no children are allowed).
 *           - \c DragNDropMimeType  = used with drag-and-drop to determine which things can be dropped where.  See
 *                                     \c mimeAccepted properties in \c ui/mainWindow.ui.  Note that this type
 *                                     determines where a dragged item can be dropped.  Broadly:
 *                                       - Recipes, equipment and styles get dropped on the recipe pane
 *                                       - Folders will be handled by themselves
 *                                       - Most other things get dropped on the ingredients pane
 *                                       - TBD what to do about Water
 *                                       - BrewNotes can't be dropped anywhere
 *        We use smart pointers for children and raw pointers for parents because parents own their children (and not
 *        vice versa).  We use std::variant even in trees where all nodes have a single parent type because it
 *        simplifies the generic code.
 */
template<class NE, class TreeType = NE>
struct TreeNodeTraits;

//! \brief See comment on \c TreeNodeBase below for explanation of this
enum class TreeNodeClassifier {
   Folder        = 0,
   PrimaryItem   = 1,
   SecondaryItem = 2,
};
//! \brief Convenience function for logging
template<class S> S & operator<<(S & stream, TreeNodeClassifier const treeNodeClassifier) {
   switch (treeNodeClassifier) {
      case TreeNodeClassifier::Folder       : stream << "Folder"       ; break;
      case TreeNodeClassifier::PrimaryItem  : stream << "PrimaryItem"  ; break;
      case TreeNodeClassifier::SecondaryItem: stream << "SecondaryItem"; break;
   }
   return stream;
}


class TreeNode {
protected:
   TreeNode() = default;

public:
   ~TreeNode() = default;

   /**
    * \brief Derived classes implement this function, which then makes it easy for us to cast from TreeNode * to the
    *        actual type.
    */
   virtual TreeNodeClassifier classifier() const = 0;

   /**
    * \brief Called from \c TreeModelBase::doData to obtain what to show in the specified column for the given role
    *
    *        See https://doc.qt.io/qt-6/qt.html#ItemDataRole-enum for possible values for \c role
    */
   virtual QVariant data(int const column, int const role) const = 0;

   virtual int childCount() const = 0;

   virtual TreeNode * rawChild(int number) const = 0;

   virtual TreeNode * rawParent() const = 0;

   virtual int numberOfChild(void const * childToCheck) = 0;

   virtual int childNumber() const = 0;

   virtual bool removeChildren(int position, int count) = 0;

   /**
    * \brief Class name of whatever type of object is stored in this node (eg "Recipe", "Hop, etc)
    */
   virtual QString className() const = 0;

   /**
    * \brief Localised name of whatever type of object is stored in this node (eg
    *        "Recipe" -> "Recette" / "Rezept" / "Receta" / etc)
    */
   virtual QString localisedClassName() const = 0;

   //! \brief Name of individual object stored in this node (eg "Oatmeal Stout")
   virtual QString name() const = 0;

   virtual QString dragAndDropMimeType() const = 0;

   /**
    * \brief For a \c TreeFolderNode, this should return the folder held by the node.
    *        For a \c TreeItemNode, this should return the closest containing folder, or \c nullptr otherwise.
    */
   virtual std::shared_ptr<Folder> folder() const = 0;

   //! \brief flag this node to override display() or not
   void setShowMe(bool val);

   //! \brief does the node want to be shown regardless of display()
   bool showMe() const;

protected:
   bool m_showMe = true;

private:
   // Insert all the usual boilerplate to prevent copy/assignment/move
   // Since a TreeNode owns its children, we don't want nodes to be copied.
   NO_COPY_DECLARATIONS(TreeNode)
};

//! \brief Convenience function for logging
template<class S> S & operator<<(S & stream, TreeNode const & treeNode) {
   stream <<
      treeNode.className() << "TreeNode (" << treeNode.classifier() << "):" << treeNode.name() << "(" <<
      treeNode.childCount() << "children)";
   return stream;
}
template<class S> S & operator<<(S & stream, TreeNode const * treeNode) {
   if (treeNode) {
      stream << *treeNode;
   } else {
      stream << "NULL";
   }
   return stream;
}

template<class NE> class TreeFolderNode;
template<class NE> class TreeItemNode;

template <class NE> struct TreeNodeTraits<Folder, NE> {
   enum class ColumnIndex {
      // TBD: Not sure we need all these columns!
      Name    ,
      Path    ,
      FullPath,
   };
   static constexpr size_t NumberOfColumns = 3;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::Folder;
   using TreeType = NE;
   using ParentPtrTypes = std::variant<TreeFolderNode<NE> *>;
   using ChildPtrTypes = std::variant<std::shared_ptr<TreeFolderNode<NE>>, std::shared_ptr<TreeItemNode<NE>>>;
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-folder";

///   static QString getRootName();

   static QVariant data(Folder const & folder, ColumnIndex const column) {
      switch (column) {
         case ColumnIndex::Name:
            return QVariant(folder.name());
         case ColumnIndex::Path:
            return QVariant(folder.path());
         case ColumnIndex::FullPath:
            return QVariant(folder.fullPath());
      }
      std::unreachable();
   }
};

template<> struct TreeNodeTraits<BrewNote, Recipe> {
   enum class ColumnIndex {
      BrewDate,
   };
   static constexpr size_t NumberOfColumns = 1;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::SecondaryItem;
   using TreeType = Recipe;
   using ParentPtrTypes = std::variant<TreeItemNode<Recipe> *>;
   using ChildPtrTypes = std::variant<std::monostate>;
   // BrewNotes can't be dropped anywhere, so there isn't anywhere in the program that accepts drops with this MIME type
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-brewnote";

   static QVariant data(BrewNote const & brewNote, ColumnIndex const column) {
      // I know this is a bit overkill when we only have one column, but I prefer to keep the same code structure for
      // all node types - in case we decide to add more columns in future.
      switch (column) {
         case ColumnIndex::BrewDate:
            return QVariant(brewNote.brewDate_short());
      }
      std::unreachable();
   }
};

template<> struct TreeNodeTraits<Recipe, Recipe> {
   enum class ColumnIndex {
      Name             ,
      NumberOfAncestors,
      BrewDate         ,
      Style            ,
   };
   static constexpr size_t NumberOfColumns = 4;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::PrimaryItem;
   using TreeType = Recipe;
   using ParentPtrTypes = std::variant<TreeFolderNode<Recipe> *, TreeItemNode<Recipe> *>;
   using ChildPtrTypes = std::variant<std::shared_ptr<TreeItemNode<BrewNote>>, std::shared_ptr<TreeItemNode<Recipe>>>;
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-recipe";

   static QString getRootName() { return Recipe::tr("Recipes"); }

   static QVariant data(Recipe const & recipe, ColumnIndex const column) {
      switch (column) {
         case ColumnIndex::Name:
            return QVariant(recipe.name());
         case ColumnIndex::NumberOfAncestors:
            return QVariant(recipe.ancestors().size());
         case ColumnIndex::BrewDate:
            return recipe.date() ? Localization::displayDateUserFormated(*recipe.date()) : QVariant();
         case ColumnIndex::Style:
            return recipe.style() ? QVariant(recipe.style()->name()) : QVariant();
      }
      std::unreachable();
   }
};

template<> struct TreeNodeTraits<Equipment, Equipment> {
   enum class ColumnIndex {
      Name    ,
      BoilTime,
   };
   static constexpr size_t NumberOfColumns = 2;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::PrimaryItem;
   using TreeType = Equipment;
   using ParentPtrTypes = std::variant<TreeFolderNode<Equipment> *>;
   using ChildPtrTypes = std::variant<std::monostate>;
   //
   // Although it seems odd for Equipment to have a drag-and-drop MIME type of recipe, it is intentional.  This means an
   // Equipment can be dropped on the recipe pane (MainWindow::tabWidget_recipeView).
   //
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-recipe";

   static QString getRootName() { return Equipment::tr("Equipments"); }

   static QVariant data(Equipment const & equipment, ColumnIndex const column) {
      switch (column) {
         case ColumnIndex::Name:
            return QVariant(equipment.name());
         case ColumnIndex::BoilTime:
            return QVariant::fromValue(equipment.boilTime_min());
      }
      std::unreachable();
   }
};

template<> struct TreeNodeTraits<Fermentable, Fermentable> {
   enum class ColumnIndex {
      Name ,
      Type ,
      Color,
   };
   static constexpr size_t NumberOfColumns = 3;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::PrimaryItem;
   using TreeType = Fermentable;
   using ParentPtrTypes = std::variant<TreeFolderNode<Fermentable> *>;
   using ChildPtrTypes = std::variant<std::monostate>;
   // Fermentables and other ingredients can be dropped on MainWindow::tabWidget_ingredients
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-ingredient";

   static QString getRootName() { return Fermentable::tr("Fermentables"); }

   static QVariant data(Fermentable const & fermentable, ColumnIndex const column) {
      switch (column) {
         case ColumnIndex::Name:
            return QVariant(fermentable.name());
         case ColumnIndex::Type:
            return QVariant(Fermentable::typeDisplayNames[fermentable.type()]);
         case ColumnIndex::Color:
            return QVariant(Measurement::displayAmount(Measurement::Amount{fermentable.color_srm(),
                                                                           Measurement::Units::srm}, 0));
      }
      std::unreachable();
   }

};

template<> struct TreeNodeTraits<Hop, Hop> {
   enum class ColumnIndex {
      Name    ,
      Form    ,
      AlphaPct, // % Alpha Acid
      Origin  , // Country of origin
   };
   static constexpr size_t NumberOfColumns = 4;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::PrimaryItem;
   using TreeType = Hop;
   using ParentPtrTypes = std::variant<TreeFolderNode<Hop> *>;
   using ChildPtrTypes = std::variant<std::monostate>;
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-ingredient";

   static QString getRootName() { return Hop::tr("Hops"); }

   static QVariant data(Hop const & hop, ColumnIndex const column) {
      switch (column) {
         case ColumnIndex::Name:
            return QVariant(hop.name());
         case ColumnIndex::Form:
            return QVariant(Hop::formDisplayNames[hop.form()]);
         case ColumnIndex::AlphaPct:
            return QVariant(hop.alpha_pct());
         case ColumnIndex::Origin:
            return QVariant(hop.origin());
      }
      std::unreachable();
   }
};

template<> struct TreeNodeTraits<Misc, Misc> {
   enum class ColumnIndex {
      Name,
      Type,
   };
   static constexpr size_t NumberOfColumns = 2;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::PrimaryItem;
   using TreeType = Misc;
   using ParentPtrTypes = std::variant<TreeFolderNode<Misc> *>;
   using ChildPtrTypes = std::variant<std::monostate>;
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-ingredient";

   static QString getRootName() { return Misc::tr("Miscellaneous"); }

   static QVariant data(Misc const & misc, ColumnIndex const column) {
      switch (column) {
         case ColumnIndex::Name:
            return QVariant(misc.name());
         case ColumnIndex::Type:
            return QVariant(Misc::typeDisplayNames[misc.type()]);
      }
      std::unreachable();
   }
};

template<> struct TreeNodeTraits<Yeast, Yeast> {
   enum class ColumnIndex {
      // It's tempting to put Laboratory first, and have it at the first column, but it messes up the way the folders
      // work if the first column isn't Name
      Name,
      Laboratory,
      ProductId,
      Type,
      Form,
   };
   static constexpr size_t NumberOfColumns = 5;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::PrimaryItem;
   using TreeType = Yeast;
   using ParentPtrTypes = std::variant<TreeFolderNode<Yeast> *>;
   using ChildPtrTypes = std::variant<std::monostate>;
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-ingredient";

   static QString getRootName() { return Yeast::tr("Yeasts"); }

   static QVariant data(Yeast const & yeast, ColumnIndex const column) {
      switch (column) {
         case ColumnIndex::Name:
            return QVariant(yeast.name());
         case ColumnIndex::Laboratory:
            return QVariant(yeast.laboratory());
         case ColumnIndex::ProductId:
            return QVariant(yeast.productId());
         case ColumnIndex::Type:
            return QVariant(Yeast::typeDisplayNames[yeast.type()]);
         case ColumnIndex::Form:
            return QVariant(Yeast::formDisplayNames[yeast.form()]);
      }
      std::unreachable();
   }
};

template<> struct TreeNodeTraits<Style, Style> {
   enum class ColumnIndex {
      Name          ,
      Category      ,
      CategoryNumber,
      CategoryLetter,
      StyleGuide    ,
   };
   static constexpr size_t NumberOfColumns = 5;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::PrimaryItem;
   using TreeType = Style;
   using ParentPtrTypes = std::variant<TreeFolderNode<Style> *>;
   using ChildPtrTypes = std::variant<std::monostate>;
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-recipe";

   static QString getRootName() { return Style::tr("Styles"); }

   static QVariant data(Style const & style, ColumnIndex const column) {
      switch (column) {
         case ColumnIndex::Name:
            return QVariant(style.name());
         case ColumnIndex::Category:
            return QVariant(style.category());
         case ColumnIndex::CategoryNumber:
            return QVariant(style.categoryNumber());
         case ColumnIndex::CategoryLetter:
            return QVariant(style.styleLetter());
         case ColumnIndex::StyleGuide:
            return QVariant(style.styleGuide());
      }
      std::unreachable();
   }
};

template<> struct TreeNodeTraits<Water, Water> {
   enum class ColumnIndex {
      Name       ,
      Calcium    ,
      Bicarbonate,
      Sulfate    ,
      Chloride   ,
      Sodium     ,
      Magnesium  ,
      pH         ,
   };
   static constexpr size_t NumberOfColumns = 8;
   static constexpr TreeNodeClassifier NodeClassifier = TreeNodeClassifier::PrimaryItem;
   using TreeType = Water;
   using ParentPtrTypes = std::variant<TreeFolderNode<Water> *>;
   using ChildPtrTypes = std::variant<std::monostate>;
   static constexpr char const * DragNDropMimeType = DEF_CONFIG_MIME_PREFIX "-ingredient";

   static QString getRootName() { return Water::tr("Waters"); }

   static QVariant data(Water const & water, ColumnIndex const column) {
      switch (static_cast<ColumnIndex>(column)) {
         case ColumnIndex::Name:
            return QVariant(water.name());
         case ColumnIndex::Calcium:
            return QVariant(water.calcium_ppm());
         case ColumnIndex::Bicarbonate:
            return QVariant(water.bicarbonate_ppm());
         case ColumnIndex::Sulfate:
            return QVariant(water.sulfate_ppm());
         case ColumnIndex::Chloride:
            return QVariant(water.chloride_ppm());
         case ColumnIndex::Sodium:
            return QVariant(water.sodium_ppm());
         case ColumnIndex::Magnesium:
            return QVariant(water.magnesium_ppm());
         case ColumnIndex::pH:
            return water.ph() ? QVariant(*water.ph()) : QVariant();
      }
      std::unreachable();
   }
};

/**
 * \class TreeNodeBase Curiously Recurring Template Base for NewTreeNode subclasses
 *
 *        NOTE: This is still mostly an idea at the moment - would require a rework of TreeView and TreeModel to be
 *              useful.  For now we just use ColumnIndex and Info.
 *
 *        Class structure:
 *        ----------------
 *                                           TreeNode
 *                                              |
 *                                         TreeNodeBase
 *                                          /        \ .
 *                             TreeFolderNode<NE>    TreeItemNode<NE>
 *
 *        Note that we have a simpler structure here than in a lot of places where we use CRTP.  This is because these
 *        classes do not need to inherit from QObject, so we don't need to jump around to ensure the Q_OBJECT pseudo
 *        macro works etc.
 *
 *        Tree structure:
 *        ---------------
 *
 *           TreeModel<Recipe>
 *             │
 *           TreeFolderNode<Recipe>
 *             ├── TreeFolderNode<Recipe>
 *             │   ├── TreeItemNode<Recipe>
 *             │   │   └── TreeItemNode<BrewNote>
 *             │   └── TreeItemNode<Recipe>
 *             ├── TreeFolderNode<Recipe>
 *             │   └── TreeItemNode<Recipe>
 *             ├── TreeItemNode<Recipe>
 *             └── TreeItemNode<Recipe>
 *
 *        A folder node in a Hop tree can contain only hop nodes or other Hop Folder nodes.  A Hop node cannot contain
 *        other nodes.
 *
 *        In a Recipe tree it's a bit more complicated:
 *           - A Folder node can contain only Recipe nodes or other Recipe Folder nodes
 *           - A Recipe node can contain only BrewNote nodes or Recipe nodes (when using ancestor versioning)
 *           - A BrewNote node cannot contain other nodes
 *
 *        So, in general, depending on the type of node, it can contain:
 *           - No other nodes
 *           - Nodes of one other type
 *           - Nodes of its own type and nodes of one other type
 *
 *        This means, depending on the type of node, its parent can be:
 *           - A node of its own type
 *           - A node of one other type
 *           - Either of the above
 *
 *        And, similarly, in a given tree, there are either two or three types of node:
 *           - Folders
 *           - Primary item - eg Recipe - which is also the type of the tree
 *           - Secondary item - eg BrewNote in the Recipe tree, but not present in the Hop tree
 *        This is a helpful classification for code that is traversing or manipulating the tree, so we have an enum for
 *        it: NodeClassifier
 *
 */
template<class Derived> class TreeNodeBasePhantom;
template<class Derived, class NE, class TreeType>
class TreeNodeBase : public TreeNode, public CuriouslyRecurringTemplateBase<TreeNodeBasePhantom, Derived> {
public:
   using ColumnIndex        = typename TreeNodeTraits<NE, TreeType>::ColumnIndex;
   // We _could_ use size_t for NumberOfColumns, since it's obviously never negative.  However, various Qt functions for
   // column number use int (and -1 means "invalid"), so we can spare ourselves compiler warnings about comparing signed
   // and unsigned types by sticking to int ourselves.
   static constexpr int                NumberOfColumns = TreeNodeTraits<NE, TreeType>::NumberOfColumns;
   static constexpr TreeNodeClassifier NodeClassifier  = TreeNodeTraits<NE, TreeType>::NodeClassifier;
///   using TreeType           = typename TreeNodeTraits<NE, TreeType>::TreeType;
   using ParentPtrTypes     = typename TreeNodeTraits<NE, TreeType>::ParentPtrTypes;
   using ChildPtrTypes      = typename TreeNodeTraits<NE, TreeType>::ChildPtrTypes;
   static constexpr char const *       DragNDropMimeType = TreeNodeTraits<NE, TreeType>::DragNDropMimeType;

   TreeNodeBase(ParentPtrTypes parent = nullptr,
                std::shared_ptr<NE> underlyingItem = nullptr) :
      m_parent{parent},
      m_underlyingItem{underlyingItem} {
      return;
   }
   TreeNodeBase(TreeNode * parent,
                std::shared_ptr<NE> underlyingItem) :
      m_parent{
         [parent]() -> ParentPtrTypes {
            //
            // Because we've made everything strongly typed (yay), there are some things we _have_ to do at compile time
            // to avoid asking the compiler to generate meaningless code.
            //
            if constexpr (NodeClassifier == TreeNodeClassifier::Folder) {
               // Folder can only have folder as parent
               return static_cast<TreeFolderNode<TreeType> *>(parent);
            } else if constexpr (NodeClassifier == TreeNodeClassifier::PrimaryItem) {
               if constexpr (std::variant_size_v<ParentPtrTypes> == 1) {
                  // If there's only one possibility for parent type, then it will be folder
                  return static_cast<TreeFolderNode<TreeType> *>(parent);
               } else {
                  //
                  // This is the only case where we have to decide at run-time -- ie where a primary item could have either
                  // a folder or another primary item as parent.  At the moment, it's only needed in the Recipe tree (to
                  // handle Recipe versioning).
                  //
                  if (!parent || parent->classifier() == TreeNodeClassifier::Folder) {
                     return static_cast<TreeFolderNode<TreeType> *>(parent);
                  }
                  return static_cast<TreeItemNode<TreeType> *>(parent);
               }
            } else {
               static_assert(NodeClassifier == TreeNodeClassifier::SecondaryItem);
               // Secondary Item (eg BrewNote) can only have primary item (eg Recipe) as parent
               return static_cast<TreeItemNode<TreeType> *>(parent);
            }
         }()
      },
      m_underlyingItem{underlyingItem} {
      return;
   }
   virtual ~TreeNodeBase() = default;

   virtual TreeNodeClassifier classifier() const override {
      return NodeClassifier;
   }

   virtual QVariant data(int const column, int const role) const override {
      if (column < 0 || column >= NumberOfColumns) {
         return QVariant{};
      }

      // Check above means this cast is valid
      auto const typedColumn = static_cast<ColumnIndex>(column);

      switch (role) {
         case Qt::ToolTipRole:
            if (this->m_underlyingItem) {
               if constexpr (NodeClassifier == TreeNodeClassifier::Folder) {
                  // Tooltip for folders is just the name of the tree - eg "Recipes" for the Recipe tree
                  return QVariant(TreeNodeTraits<TreeType>::getRootName());
               } else {
                  return this->derived().getToolTip();
               }
            }
            break;

         case Qt::DisplayRole:
            if (this->m_underlyingItem) {
               return TreeNodeTraits<NE, TreeType>::data(*this->m_underlyingItem, typedColumn);
            }
            // Special handling for the root node
            if (!this->rawParent()) {
               // For the root node, we display the name of the tree in the first column
               // Root node is always a folder
               if constexpr (NodeClassifier == TreeNodeClassifier::Folder) {
                  if (typedColumn == ColumnIndex::Name) {
                     return QVariant(TreeNodeTraits<TreeType>::getRootName());
                  }
               }
               return QVariant{};
            }
            break;

         case Qt::DecorationRole:
            if (column == 0 && NodeClassifier == TreeNodeClassifier::Folder) {
               return QIcon(":images/folder.png");
            }
            break;

         default:
            break;
      }

      return QVariant();
   }

   static QVariant header(size_t const section) {
      if (/*section < 0 || */section >= NumberOfColumns) {
         return QVariant();
      }
      return QVariant(Derived::columnDisplayNames[section]);
   }

   static bool lessThan(TreeModel const & model,
                        QModelIndex const & left,
                        QModelIndex const & right,
                        NE const & lhs,
                        NE const & rhs) {
      return Derived::isLessThan(model, left, right, static_cast<ColumnIndex>(left.column()), lhs, rhs);
   }

   static bool lessThan(TreeModel const & model,
                        QModelIndex const & left,
                        QModelIndex const & right,
                        Derived const & lhs,
                        Derived const & rhs) {
      return lessThan(model, left, right, *lhs.underlyingItem(), *rhs.underlyingItem());
   }

   /**
    * \brief returns item's parent
    */
   ParentPtrTypes parent() const {
      return this->m_parent;
   }

   virtual TreeNode * rawParent() const override {
      // Every substantive member of ParentPtrTypes is always a raw pointer to some subclass of TreeNode, so this
      // generic lambda suffices to obtain whatever member the variant holds.
      auto theParent{this->parent()};
      return std::visit([](auto&& arg) { return static_cast<TreeNode *>(arg); }, theParent);
   }

   std::shared_ptr<NE> underlyingItem() const {
      return this->m_underlyingItem;
   }

   void setUnderlyingItem(std::shared_ptr<NE> val) {
      this->m_underlyingItem = val;
      return;
   }

   /**
    * \brief inserts a new item at \c position
    *
    * \return \c true if succeeded, \c false otherwise
    */
   bool insertChild(std::size_t position, ChildPtrTypes child) requires IsSubstantiveVariant<ChildPtrTypes> {
      if (position > this->m_children.size()) {
         // This is probably a coding error, but we can probably recover by just not doing the insert
         qWarning() << Q_FUNC_INFO << "Position" << position << "outside range (0, " << this->m_children.size() << ")";
         return false;
      }

      this->m_children.insert(this->m_children.begin() + position, child);

      return true;
   }

   /**
    * \brief inserts \c count new empty items starting at \c position  TBD Do we need this?
    *
    * \return \c true if succeeded, \c false otherwise
    */
   bool insertChildren(std::size_t position, int count) requires IsSubstantiveVariant<ChildPtrTypes> {
      if (position > this->m_children.size()) {
         // This is probably a coding error, but we can probably recover by just not doing the insert
         qWarning() << Q_FUNC_INFO << "Position" << position << "outside range (0, " << this->m_children.size() << ")";
         return false;
      }

      for (int row = 0; row < count; ++row) {
         this->m_children.emplace(this->m_children.begin() + position + row);
      }

      return true;
   }

//   bool insertChildren(std::size_t position, int count) requires IsNullVariant<ChildPtrTypes> {
//      qWarning() << Q_FUNC_INFO << "Should not be called!";
//      return false;
//   }

   /**
    * \brief Removes \c count items starting at \c position.  NB: This just removes the nodes from the tree structure;
    *        it does not delete the contents of the nodes (m_underlyingItem).  Similarly, it is not recursive, so it is
    *        the caller's responsibility to do any processing of children's children etc.
    *
    * \return \c true if succeeded, \c false otherwise
    */
   virtual bool removeChildren(int position, int count) override {
      if constexpr (IsSubstantiveVariant<ChildPtrTypes>) {
         if (position < 0  || position > static_cast<int>(this->m_children.size())) {
            // This is probably a coding error, but we can probably recover by just not doing the remove
            qWarning() <<
               Q_FUNC_INFO << "Position" << position << "outside range (0, " << this->m_children.size() << ")";
            return false;
         }

         // The range for erase is inclusive of the first element, and exclusive of the last, so the second parameter is
         // one beyond where we want to erase (and can legitimately be cend()).
         this->m_children.erase(this->m_children.cbegin() + position, this->m_children.cbegin() + position + count);
         return true;
      }

      // Shouldn't ever get here
      qWarning() << Q_FUNC_INFO << "This function should not be called on nodes that cannot have children!";
      return false;
   }

   //! \brief returns the number of children of the folder (or recipe)
   virtual int childCount() const override {
      if constexpr (IsSubstantiveVariant<ChildPtrTypes>) {
         return this->m_children.size();
      } else {
         return 0;
      }
   }

   /**
    * \brief Return specified child.
    *
    *        TODO: One day it would be neat to have an iterator for looping over children etc.
    */
   ChildPtrTypes child(int number) const {
      if constexpr (IsSubstantiveVariant<ChildPtrTypes>) {
         return this->m_children.at(number);
      } else {
         // For the moment, it is simpler to allow calls to this function even when there can be no children.  We just
         // always return the null variant in such cases.
         return std::variant<std::monostate>{};
      }
   }

   virtual TreeNode * rawChild(int number) const override {
      // If this node type does not support children, there are never any to return.  (It is in fact unlikely we'd get
      // called in such circumstances, but we can't have a requires clause on a virtual function, so it's easier just to
      // cover the case here.)
      if constexpr (IsNullVariant<ChildPtrTypes>) {
         return nullptr;
      } else {
         // Every substantive member of ChildPtrTypes is always a shared pointer to some subclass of TreeNode, so this
         // generic lambda suffices to give us a raw pointer that can be cast to TreeNode *.
         auto theChild{this->child(number)};
         return std::visit([](auto&& arg) { return static_cast<TreeNode *>(arg.get()); }, theChild);
      }
   }

   /**
    * \brief Return a raw pointer to specified child, suitable for call to \c QAbstractItemModel::createIndex
    */
   void const * voidChild(std::size_t number) const requires IsSubstantiveVariant<ChildPtrTypes> {
      if (number < this->m_children.size()) {
         auto const & theChild = this->m_children.at(number);
         return std::visit([](auto visited){ return static_cast<void *>(visited.get()); }, theChild);
      }
      return nullptr;
   }

   /**
    * \brief If the supplied parameter is a pointer to one of the children of this node, then return the number of
    *        that child in our list.  Otherwise, return -1.
    */
   virtual int numberOfChild(void const * childToCheck) {
      // Comment from rawChild above applies equally here
      if constexpr (!IsNullVariant<ChildPtrTypes>) {
         for (int childNumber = 0; childNumber < static_cast<int>(this->m_children.size()); ++childNumber) {
            auto const & currentChild = this->m_children.at(childNumber);
            if (std::visit([&](auto visited){ return (visited.get() == childToCheck); }, currentChild)) {
               return childNumber;
            }
         }
      }

      // Usually it's a coding error if we get here
      qCritical() << Q_FUNC_INFO << "Unable to find child";
      return -1;
   }

   /**
    * \brief returns the index of the item in its parent's list.  This is needed for constructing \c QModelIndex
    *        objects.
    */
   virtual int childNumber() const override {
      TreeNode * rawParent = this->rawParent();
      if (!rawParent) {
         return 0;
      }

      return rawParent->numberOfChild(this);
   }

   virtual QString className() const override {
      return NE::staticMetaObject.className();
   }

   virtual QString localisedClassName() const override {
      return NE::localisedName();
   }

   virtual QString name() const override {
      if (!this->m_underlyingItem) {
         return QObject::tr("None!");
      }
      return this->m_underlyingItem->name();
   }

   virtual QString dragAndDropMimeType() const override {
      return QString{DragNDropMimeType};
   }

   //================================================ Member variables =================================================
   ParentPtrTypes m_parent;
   //
   // Although it's easy to have conditional member functions (ie ones that only exist when certain template constraints
   // are satisfied), there isn't (yet) a straightforward way to do the equivalent for member variables.  About as close
   // as we can get is using [[no_unique_address]] here, which allows the compiler to optimise away the variable storage
   // when it's an empty class type
   //
   struct Empty { };
   [[no_unique_address]] std::conditional_t<IsSubstantiveVariant<ChildPtrTypes>,
                                            std::vector<ChildPtrTypes>,
                                            Empty>  m_children;
   //
   // The underlying item stored in this tree node -- eg the Recipe object stored in a particular TreeItemNode<Recipe>
   // object.
   //
   std::shared_ptr<NE> m_underlyingItem = nullptr;
};

//=================================================== TreeFolderNode ===================================================

/**
 * \brief Besides other folders of the same type, a given type of folders can only only contain one type of thing (eg
 *        FermentableTreeItem, HopTreeItem, etc).
 */
template<class NE>
class TreeFolderNode : public TreeNodeBase<TreeFolderNode<NE>, Folder, NE> {
public:
   using TreeNodeBase<TreeFolderNode<NE>, Folder, NE>::TreeNodeBase;
   ~TreeFolderNode() = default;

   static EnumStringMapping const columnDisplayNames;
   static bool isLessThan([[maybe_unused]] TreeModel const & model,
                          [[maybe_unused]] QModelIndex const & left,
                          [[maybe_unused]] QModelIndex const & right,
                          TreeNodeTraits<Folder, NE>::ColumnIndex section,
                          Folder const & lhs,
                          Folder const & rhs) {
      switch (section) {
         case TreeNodeTraits<Folder, NE>::ColumnIndex::Name     : return lhs.name    () < rhs.name    ();
         case TreeNodeTraits<Folder, NE>::ColumnIndex::Path     : return lhs.path    () < rhs.path    ();
         case TreeNodeTraits<Folder, NE>::ColumnIndex::FullPath : return lhs.fullPath() < rhs.fullPath();
      }
      std::unreachable();
   }

   // Have to override the version in \c TreeNodeBase as that will give Folder::staticMetaObject.className() rather
   // than NE::staticMetaObject.className()
   virtual QString className() const override {
      return NE::staticMetaObject.className();
   }

   virtual std::shared_ptr<Folder> folder() const override {
      return this->underlyingItem();
   }
};

//==================================================== TreeItemNode ====================================================

template<class NE>
class TreeItemNode : public TreeNodeBase<TreeItemNode<NE>, NE, typename TreeTypeDeducer<NE>::TreeType> {
public:
   using TreeNodeBase<TreeItemNode<NE>, NE, typename TreeTypeDeducer<NE>::TreeType>::TreeNodeBase;
   virtual ~TreeItemNode() = default;

   static EnumStringMapping const columnDisplayNames;
   static bool isLessThan(TreeModel const & model,
                          QModelIndex const & left,
                          QModelIndex const & right,
                          TreeNodeTraits<NE, typename TreeTypeDeducer<NE>::TreeType>::ColumnIndex section,
                          NE const & lhs,
                          NE const & rhs);

   QString getToolTip() const;

   virtual std::shared_ptr<Folder> folder() const override {

      if constexpr (TreeNodeTraits<NE, typename TreeTypeDeducer<NE>::TreeType>::NodeClassifier == TreeNodeClassifier::PrimaryItem) {
         //
         // We are assuming here that all PrimaryItem nodes hold subclasses of NamedEntity that also inherit from
         // FolderBase.  This saves us chasing up the node tree to try to find a TreeFolderNode.
         //
         // TODO: This is a temporary hack to return a Folder object!
         return std::make_shared<Folder>(this->underlyingItem()->folderPath());
      } else {
         //
         // For a SecondaryItem node, it must, by definition, have a parent node, so we just defer to that.
         //
         static_assert (TreeNodeTraits<NE, typename TreeTypeDeducer<NE>::TreeType>::NodeClassifier == TreeNodeClassifier::SecondaryItem);
         return this->rawParent()->folder();
      }
   }

};

//
// Check the concepts we use above are working as we intend
//
static_assert(IsSubstantiveVariant<TreeFolderNode<Equipment>::ChildPtrTypes>);
static_assert(IsSubstantiveVariant<TreeFolderNode<Style>::ChildPtrTypes>);
static_assert(IsNullVariant<TreeItemNode<Equipment>::ChildPtrTypes>);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////*!
/// * \class TreeNode
/// *
/// * \brief Model for an item in a tree.
/// *
/// * This provides a generic item from which the trees are built. Since most of
/// * the actions required are the same regardless of the item being stored (e.g.
/// * hop or equipment), this class considers them all the same.
/// *
/// * It does assume that everything being stored can be cast into a QObject.
/// */
///class TreeNode {
///
///public:
///
///   /*!
///    * This enum lists the different things that we can store in an item
///    */
///   enum class Type {
///      Recipe,
///      Equipment,
///      Fermentable,
///      Hop,
///      Misc,
///      Yeast,
///      BrewNote,
///      Style,
///      Folder,
///      Water
///   };
///
///   /**
///    * \brief This templated function will convert a class to its \c TreeNode::Type. Eg \c typeOf<Hop>() returns
///    *        \c TreeNode::Type::Hop
///    */
///   template<class T>
///   static TreeNode::Type typeOf();
///
///   friend bool operator==(TreeNode & lhs, TreeNode & rhs);
///
///   //! \brief A constructor that sets the \c type of the TreeNode and
///   // the \c parent
///   TreeNode(TreeNode::Type nodeType = TreeNode::Type::Folder, TreeNode * parent = nullptr);
///   virtual ~TreeNode();
///
///   //! \brief returns the child at \c number
///   TreeNode * child(int number);
///   //! \brief returns item's parent
///   TreeNode * parent();
///
///   //! \brief returns item's type
///   TreeNode::Type type() const;
///   //! \brief returns the number of the item's children
///   int childCount() const;
///   //! \brief returns number of columns associated with the item's \c type
///   int columnCount(TreeNode::Type nodeType) const;
///   //! \brief returns the data of the item of \c type at \c column
///   QVariant data(/*TreeNode::Type nodeType, */int column);
///   //! \brief returns the index of the item in it's parents list
///   int childNumber() const;
///
///   //! \brief sets the \c t type of the object and the \c d data
///   void setData(TreeNode::Type t, QObject * d);
///
///   //! \brief returns the data as a T
///   template<class T> T * getData();
///
///   //! \brief returns the data as a NamedEntity
///   NamedEntity * thing();
///
///   //! \brief inserts \c count new items of \c type, starting at \c position
///   bool insertChildren(int position, int count, TreeNode::Type nodeType = TreeNode::Type::Recipe);
///   //! \brief removes \c count items starting at \c position
///   bool removeChildren(int position, int count);
///
///   //! \brief returns the name.
///   QString name();
///   //! \brief flag this node to override display() or not
///   void setShowMe(bool val);
///   //! \brief does the node want to be shown regardless of display()
///   bool showMe() const;
///
///private:
///   /*!  Keep a pointer to the parent tree item. */
///   TreeNode * parentItem;
///   /*!  The list of children associated with this item */
///   QList<TreeNode *> childItems;
///
///   /*! the type of this item */
///   TreeNode::Type nodeType;
///
///   /*! the data associated with this item */
///   QObject * m_thing;
///   //! \b overrides the display()
///   bool m_showMe;
///
///   /*! helper functions to get the information from the item */
///   QVariant dataRecipe(int column);
///   QVariant dataEquipment(int column);
///   QVariant dataFermentable(int column);
///   QVariant dataHop(int column);
///   QVariant dataMisc(int column);
///   QVariant dataYeast(int column);
///   QVariant dataBrewNote(int column);
///   QVariant dataStyle(int column);
///   QVariant dataFolder(int column);
///   QVariant dataWater(int column);
///};
///
////**
/// * \brief Convenience function for logging
/// */
///template<class S>
///S & operator<<(S & stream, TreeNode::Type const treeNodeType);

#endif
