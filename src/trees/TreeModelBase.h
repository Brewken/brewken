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

#include "trees/TreeNode.h"
#include "utils/CuriouslyRecurringTemplateBase.h"

template<class Derived> class TreeModelPhantom;
template<class Derived, class NE>
class TreeModelBase : public CuriouslyRecurringTemplateBase<TreeModelPhantom, Derived> {
   friend Derived;
private:
   TreeModelBase() :
   m_rootNode {std::make_unique<TreeFolderNode<NE>>()} {
      return;
   }
public:
   ~TreeModelBase() = default;

   /**
    * \brief Call this at the end of derived class's constructor.
    */
   void connectSignalsAndSlots() {
      //
      // We want to know about additions or deletions of objects of the type(s) used in our tree
      //
      connect(&ObjectStoreTyped<NE>::getInstance(), &ObjectStoreTyped<NE>::signalObjectInserted, &this->derived(), &Derived::elementAdded  );
      connect(&ObjectStoreTyped<NE>::getInstance(), &ObjectStoreTyped<NE>::signalObjectDeleted , &this->derived(), &Derived::elementRemoved);
      return;
   }
   void doElementAdded  (int elementId) {
      auto element = ObjectStoreWrapper::getById<NE>(elementId);
      return;

   }
   void doElementRemoved(int elementId) { return; }

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
      void elementAdded  (int victimId);                                                 \
      void elementRemoved(int victimId);                                                 \



/**
 * \brief Derived classes should include this in their .cpp file
 *
 *        Note we have to be careful about comment formats in macro definitions
 *
 *        NB: Mostly I have tried to make these macro-included function bodies trivial.  Macros are a bit clunky, so we
 *            only really want to use them for the things that are hard to do other ways.
 */
#define TREE_MODEL_COMMON_CODE(NeName) \
   void NeName##TreeModel::elementAdded  (int elementId) { this-doElementAdded  (elementId); return; }   \
   void NeName##TreeModel::elementRemoved(int elementId) { this-doElementRemoved(elementId); return; }   \

#endif
