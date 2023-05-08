/*======================================================================================================================
 * editors/EditorBase.h is part of Brewken, and is copyright the following authors 2023:
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
#ifndef EDITORS_EDITORBASE_H
#define EDITORS_EDITORBASE_H
#pragma once

#include <memory>

#include <QInputDialog>
#include <QString>

#include "database/ObjectStoreWrapper.h"
#include "model/NamedEntity.h"

/**
 * \class EditorBase
 *
 * \brief As in other places where we want to use class templating, we have to use multiple inheritance because we can't
 *        template a class that ultimately inherits from \c QObject.  However, with the magic of the Curiously Recurring
 *        Template Pattern, we can get past some of the limitations and avoid too much copy-and-paste code duplication.
 *        Eg:
 *
 *              QObject     Ui::hopEditor
 *                   \           |
 *                   ...         |
 *                     \         |     EditorBase<Hop, HopEditor>
 *                     QDialog   |    /
 *                           \   |   /
 *                            \  |  /
 *                          HopEditor
 *
 *        Besides inheriting from \c QDialog, the derived class (eg \c HopEditor in the example above) needs to
 *        implement the following trivial slots:
 *
 *          - \c void \c save() public slot, which should call \c EditorBase::doSave
 *          - \c void \c clearAndClose() public slot, which should call \c EditorBase::doClearAndClose
 *          - \c void \c changed(\c QMetaProperty, \c QVariant) public slot, which should call \c EditorBase::doChanged
 *          - \c void \c clickedNew() public slot, which should call \c EditorBase::newEditItem
 *
 *        The code for the definition of these slot functions (which is "the same" for all editors) can be inserted in
 *        the implementation file using the EDITOR_COMMON_SLOT_DEFINITIONS macro.  Eg, in HopEditor, we need:
 *
 *          EDITOR_COMMON_SLOT_DEFINITIONS(HopEditor)
 *
 *        Note that we cannot do the equivalent for the header file declarations because the Qt MOC does not expand
 *        non-Qt macros.
 *
 *        The derived class also needs to implement the following substantive member functions that \c EditorBase will
 *        call:
 *          - \c void \c writeFieldsToEditItem -- Writes most fields from the editor GUI fields into the object being
 *                                                edited
 *          - \c void \c writeLateFieldsToEditItem -- Writes any fields that must wait until the object definitely
 *                                                    exists in the DB
 *          - \c void \c readFieldsFromEditItem -- (Re)read one or all fields from the object into the relevant GUI
 *                                                 field(s).
 */
template<class NE, class Derived>
class EditorBase {
public:
   EditorBase() :
      m_derived{static_cast<Derived *>(this)},
      m_editItem{nullptr} {
      return;
   }
   virtual ~EditorBase() = default;

   /**
    * \brief Edit the given Hop, Fermentable, etc.
    *
    *        Calling with no parameter clears the current item.
    */
   void setEditItem(std::shared_ptr<NE> editItem = nullptr) {
      if (this->m_editItem) {
         this->m_derived->disconnect(this->m_editItem.get(), nullptr, this->m_derived, nullptr);
      }
      this->m_editItem = editItem;
      if (this->m_editItem) {
         this->m_derived->connect(this->m_editItem.get(), &NamedEntity::changed, this->m_derived, &Derived::changed);
         this->m_derived->readFieldsFromEditItem(std::nullopt);
      }
      return;
   }

   /**
    * \brief We don't want the compiler automatically constructing a shared_ptr for us if we accidentally call
    *        \c setEditItem with, say, a raw pointer, so this template trick ensures it can't.
    */
   template <typename D> void setEditItem(D) = delete;

   /**
    * \brief Create a new Hop, Fermentable, etc.
    */
   void newEditItem(QString folder = "") {
      QString name = QInputDialog::getText(this->m_derived,
                                           QString(QObject::tr("%1 name")).arg(NE::staticMetaObject.className()),
                                           QString(QObject::tr("%1 name:")).arg(NE::staticMetaObject.className()));
      if (name.isEmpty()) {
         return;
      }

      auto ne = std::make_shared<NE>(name);
      if (!folder.isEmpty()) {
         ne->setFolder(folder);
      }

      this->setEditItem(ne);
      this->m_derived->show();
      return;
   }

   /**
    * \brief Subclass should call this from its \c save slot
    */
   void doSave() {
      if (!this->m_editItem) {
         this->m_derived->setVisible(false);
         return;
      }
      this->m_derived->writeFieldsToEditItem();
      if (this->m_editItem->key() < 0) {
         ObjectStoreWrapper::insert(this->m_editItem);
      }
      this->m_derived->writeLateFieldsToEditItem();

      this->m_derived->setVisible(false);
      return;
   }

   /**
    * \brief Subclass should call this from its \c clearAndClose slot
    */
   void doClearAndClose() {
      this->setEditItem();
      this->m_derived->setVisible(false); // Hide the window.
      return;
   }

   /**
    * \brief Subclass should call this from its \c changed slot
    *
    *        Note that \c QObject::sender has \c protected access specifier, so we can't call it from here, not even
    *        via the derived class pointer.  Therefore we have derived class call it and pass us the result.
    */
   void doChanged(QObject * sender, QMetaProperty prop, [[maybe_unused]] QVariant val) {
      if (this->m_editItem && sender == this->m_editItem.get()) {
         this->m_derived->readFieldsFromEditItem(prop.name());
      }
      return;
   }

protected:
   /**
    * \brief This is the 'this' pointer downcast to the derived class, which allows us to call non-virtual member
    *        functions in the derived class from this templated base class.
    */
   Derived * m_derived;

   /**
    * \brief This is the \c NamedEntity subclass object we are creating or editing.  We are also "observing" it in the
    *        sense that, if any other part of the code changes its data, we'll get a signal so we can update our
    *        display.  Historically therefore this member variable was called \c obsHop, \c obsFermentable, etc in each
    *        of the editor classes.
    */
   std::shared_ptr<NE> m_editItem;

};

/**
 * \brief Derived classes should include this in their implementation file
 */
#define EDITOR_COMMON_SLOT_DEFINITIONS(EditorName) \
   void EditorName::save() { this->doSave(); return; } \
   void EditorName::clearAndClose() { this->doClearAndClose(); return; } \
   void EditorName::changed(QMetaProperty prop, QVariant val) { this->doChanged(this->sender(), prop, val); return; } \
   void EditorName::clickedNew() { this->newEditItem(); return;}

#endif
