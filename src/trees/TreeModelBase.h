/*======================================================================================================================
 * trees/TreeModelBase.h is part of Brewken, and is copyright the following authors 2024:
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
#ifndef TREES_TREEMODELBASE_H
#define TREES_TREEMODELBASE_H
#pragma once

#include <memory>
#include <type_traits>
#include <variant>

#include "trees/TreeNode.h"
#include "utils/CuriouslyRecurringTemplateBase.h"
#include "utils/TypeTraits.h"

//
// In the code below, we're using a parameter pack for a single optional type (the secondary object type), if any, shown
// in the tree.  It's useful to be able to have different versions of some functions depending on whether this type is
// present.
//
template <class... ParameterPack> struct has_0Entries : public std::integral_constant<bool, (sizeof...(ParameterPack) == 0)>{};
template <class... ParameterPack> struct has_1Entry   : public std::integral_constant<bool, (sizeof...(ParameterPack) == 1)>{};
// See comment in utils/TypeTraits.h for definition of CONCEPT_FIX_UP (and why, for now, we need it)
template <class... ParameterPack> concept CONCEPT_FIX_UP Has0Entries = has_0Entries<ParameterPack...>::value;
template <class... ParameterPack> concept CONCEPT_FIX_UP Has1Entry   = has_1Entry  <ParameterPack...>::value;

//
// In C++26, we'll be able to write T...[0] when T is a class, but, for now, this is a way to get the first class
// parameter in a parameter pack.
//
template <class P0, class... ParameterPack> struct Parm0Of : std::type_identity<P0> {};

/**
 * \brief CRTP base for TreeModel classes
 *
 * \param Derived - The derived class
 * \param NE   - The primary \c NamedEntity subclass (besides \c Folder) shown in this tree (eg \c Recipe for
 *               \c RecipeTreeModel)
 * \param SNEs - The optional secondary \c NamedEntity subclass shown in this tree (eg \c BrewNote for
 *               \c RecipeTreeModel, or \c MashStep for \c MashTreeModel).  This class must have:
 *                 • an \c owner() member function that does the obvious thing (eg \c BrewNote::owner() returns a
 *                   \c Recipe; \c MashStep::owner returns a \c Mash);
 *                 • a static \c ownedBy() member function that returns all the \c BrewNote objects owned by a given
 *                   \c Recipe or all the \c MashStep objects owned by a given \c Mash, etc.
 */
template<class Derived> class TreeModelPhantom;
template<class Derived, class NE, class... SNEs>
class TreeModelBase : public CuriouslyRecurringTemplateBase<TreeModelPhantom, Derived> {
   friend Derived;
private:
   TreeModelBase() :
   m_rootNode {std::make_unique<TreeFolderNode<NE>>()} {
      return;
   }
public:
   ~TreeModelBase() = default;

   //! No-op version
   void connectSecondarySignalsAndSlots() requires Has0Entries<SNEs...> { return; }
   //! Substantive version
   void connectSecondarySignalsAndSlots() requires Has1Entry<SNEs...> {
      // For the moment at least, we don't support more than one secondary subclass
      connect(&ObjectStoreTyped<typename Parm0Of<SNEs...>::type>::getInstance(),
              &ObjectStoreTyped<typename Parm0Of<SNEs...>::type>::signalObjectInserted,
              &this->derived(),
              &Derived::secondaryElementAdded  );
      connect(&ObjectStoreTyped<typename Parm0Of<SNEs...>::type>::getInstance(),
              &ObjectStoreTyped<typename Parm0Of<SNEs...>::type>::signalObjectDeleted ,
              &this->derived(),
              &Derived::secondaryElementRemoved);
      return;
   }

   /**
    * \brief Call this at the end of derived class's constructor.
    */
   void connectSignalsAndSlots() {
      //
      // We want to know about additions or deletions of objects of the type(s) used in our tree
      //
      this->derived().connect(&ObjectStoreTyped<NE>::getInstance(), &ObjectStoreTyped<NE>::signalObjectInserted, &this->derived(), &Derived::elementAdded  );
      this->derived().connect(&ObjectStoreTyped<NE>::getInstance(), &ObjectStoreTyped<NE>::signalObjectDeleted , &this->derived(), &Derived::elementRemoved);
      this->connectSecondarySignalsAndSlots();
      return;
   }

   void observeElement(std::shared_ptr<NE> observed) {
      if (observed) {
         this->derived().connect(observed.get(), &NamedEntity::changedName  , &this->derived(), &Derived::elementChanged);
         this->derived().connect(observed.get(), &NamedEntity::changedFolder, &this->derived(), &Derived::folderChanged );
      }
      return;
   }

   /**
    * \brief Find the given \c NE (eg given \c Recipe) in the tree.  Primary elements can only be inside folders, but
    *        folders can also contain other folders.  Caller can provide a starting folder, otherwise we'll start at the
    *        root of the tree.
    *
    * \param ne If not null, this is the primary element (eg \c Recipe) we are looking for.  Otherwise, we are looking
    *           for a place to put a new primary element.
    */
   QModelIndex findElement(std::shared_ptr<NE> const ne) {
      // No element supplied, so make a new entry at the top of the starting folder
      if (!ne) {
         return this->derived().createIndex(0, 0, this->m_rootNode.get());
      }

      //
      // We do a breadth-first search of the tree.  It seems as good as anything, given we don't have any a priori
      // reason to prefer one search order over another.  An obvious alternative would be a depth-first search using
      // recursion.
      //
      QQueue<TreeFolderNode<NE> *> queue;
      queue.enqueue(this->m_rootNode.get());

      while (!queue.isEmpty()) {
         auto folder = queue.dequeue();
         qDebug() << Q_FUNC_INFO << "Find" << *ne << "in" << folder->name();
         for (int ii = 0; ii < folder->childCount(); ++ii) {
            auto child = folder->child(ii);
            if (std::holds_alternative<std::shared_ptr<TreeItemNode<NE>>>(child)) {
               auto itemNode = std::get<std::shared_ptr<TreeItemNode<NE>>>(child);
               if (itemNode->modelItem() == ne) {
                  // We found what we were looking for
                  qDebug() << Q_FUNC_INFO << "Found at" << ii;
                  return this->derived().createIndex(ii, 0, itemNode);
               }
            } else if (std::holds_alternative<std::shared_ptr<TreeFolderNode<NE>>>(child)) {
               // We found another folder to look in.  Add it to the list.
               auto folderNode = std::get<std::shared_ptr<TreeFolderNode<NE>>>(child);
               queue.enqueue(child.get());
            }
         }
      }

      // If we got here, we didn't find a match
      return QModelIndex();
   }

   // TODO Write findElement for SNEs, if necessary

   void doElementAdded(int elementId) {
      auto element = ObjectStoreWrapper::getById<NE>(elementId);
      if (!element->display()) {
         return;
      }

      QModelIndex const pIdx = this->derived().createIndex(0, 0, m_rootNode->voidChild(0));
      if (!pIdx.isValid()) {
         return;
      }

      int breadth = this->derived().rowCount(pIdx);
      if (!this->derived().insertRow(breadth, pIdx, element.get())) {
         return;
      }

      //
      // If this tree can have secondary elements (eg BrewNote items on RecipeTreeModel) then we need to check whether
      // the newly-added primary one has any.
      //
      this->addSecondariesForPrimary(element);

      this->observeElement(element);
      return;

   }

   //! No-op version
   void addSecondariesForPrimary([[maybe_unused]] std::shared_ptr<NE> element) requires Has0Entries<SNEs...> {
      return;
   }
   //! Substantive version
   void addSecondariesForPrimary(std::shared_ptr<NE> element) requires Has1Entry<SNEs...> {
      auto secondaries = Parm0Of<SNEs...>::type::ownedBy(element);
      if (!secondaries.empty()) {
         QModelIndex pIdx = this->findElement(element);
         int row = 0;
         for (auto secondary : secondaries) {
            this->derived().insertRow(row++, pIdx, secondary.get());
         }
      }
      return;
   }

   //! No-op version
   void doSecondaryElementAdded([[maybe_unused]] int elementId) requires Has0Entries<SNEs...> {
      // It's a coding error if this ever gets called!
      Q_ASSERT(false);
      return;
   }
   //! Substantive version
   void doSecondaryElementAdded(int elementId) requires Has1Entry<SNEs...> {
      auto element = ObjectStoreWrapper::getById<typename Parm0Of<SNEs...>::type>(elementId);
      if (!element->display()) {
         return;
      }

      std::shared_ptr<NE> owner = element->owner();
      QModelIndex pIdx = this->findElement(owner);
      if (!pIdx.isValid()) {
         return;
      }

      int breadth = this->derived().rowCount(pIdx);
      if (!this->derived().insertRow(breadth, pIdx, element.get())) {
         return;
      }

      this->observeElement(element);
      return;
   }
   void doElementRemoved(int elementId) {
      // TODO FINISH THIS!
      return;
   }
   void doElementChanged() {
      // TODO FINISH THIS!
      return;
   }
   void doFolderChanged() {
      // TODO FINISH THIS!
      return;
   }

   //================================================ Member Variables =================================================
   std::unique_ptr<TreeFolderNode<NE>> m_rootNode;
};

/**
 * \brief Derived classes should include this in their header file, right after Q_OBJECT
 *
 *        Note we have to be careful about comment formats in macro definitions
 */
#define TREE_MODEL_COMMON_DECL(NeName) \
   /* This allows TableModelBase to call protected and private members of Derived */     \
   friend class TreeModelBase<NeName##TreeModel, NeName>;                                \
                                                                                         \
   public:                                                                               \
      NeName##TreeModel(TreeView * parent = nullptr);                                    \
      virtual ~NeName##TreeModel();                                                      \
                                                                                         \
   private slots:                                                                        \
      void elementAdded  (int elementId);                                                \
      void elementRemoved(int elementId);                                                \
      void elementChanged();                                                             \
      void folderChanged ();                                                             \

/**
 * \brief Derived classes should include this in their .cpp file
 *
 *        Note we have to be careful about comment formats in macro definitions
 *
 *        NB: Mostly I have tried to make these macro-included function bodies trivial.  Macros are a bit clunky, so we
 *            only really want to use them for the things that are hard to do other ways.
 */
#define TREE_MODEL_COMMON_CODE(NeName) \
   void NeName##TreeModel::elementAdded  (int elementId) { this->doElementAdded  (elementId); return; }   \
   void NeName##TreeModel::elementRemoved(int elementId) { this->doElementRemoved(elementId); return; }   \
   void NeName##TreeModel::elementChanged()              { this->doElementChanged()         ; return; }   \
   void NeName##TreeModel::folderChanged ()              { this->doFolderChanged ()         ; return; }   \

#endif
