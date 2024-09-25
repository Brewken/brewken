/*======================================================================================================================
 * editors/EditorBase.h is part of Brewken, and is copyright the following authors 2023-2024:
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
#include <variant>
#include <vector>

#include <QAbstractButton>
#include <QInputDialog>
#include <QString>
#include <QPlainTextEdit>

#include "database/ObjectStoreWrapper.h"
#include "model/NamedEntity.h"
#include "utils/CuriouslyRecurringTemplateBase.h"
#include "widgets/BtBoolComboBox.h"
#include "widgets/BtComboBox.h"

/**
 * \brief Most fields are written together.  However, some are marked 'Late' because they need to be written after
 *        the object is created.
 *
 *        Logically this belongs inside EditorBaseField, but that's templated, so it would get a bit hard to refer to if
 *        we put it there.
 */
enum class WhenToWriteField {
   Normal,
   Late
};

/**
 * \brief Field info for a field of a subclass of \c EditorBase.
 *
 *        Note that we can't put this inside the \c EditorBase class declaration as we also want to use it there, and
 *        we'd get errors about "invalid use of incomplete type ‘class EditorBase<Derived, NE>’".
 *
 *        We template on both label and edit field types.  This is partly so we call the right overload of
 *        \c SmartAmounts::Init, partly because \c QLineEdit and \c QTextEdit don't have a useful common base class, and
 *        partly because we want to access member functions of \c SmartLineEdit that don't exist on \c QLineEdit or
 *        \c QTextEdit.
 *
 *        Note that the member functions are mostly const because they are not modifying this struct -- merely things
 *        referenced by the struct.
 */
template<class LabelType, class EditFieldType>
struct EditorBaseField {

   char const * labelName;
   LabelType * label;
   EditFieldType * editField;
   BtStringConst const & property;
   // This field isn't used for all values of EditFieldType, but we don't try to make it conditional for the same
   // reasons as EditorBase::m_liveEditItem below.
   std::optional<int> precision = std::nullopt;
   WhenToWriteField whenToWrite = WhenToWriteField::Normal;

   /**
    * \brief Constructor for when we don't have a SmartLineEdit or similar
    *
    *        NB: Both \c precision and \c whenToWrite have defaults, but precision is the one that more often needs
    *        something other than default to be specified, so we put it first in the argument list.
    */
   EditorBaseField([[maybe_unused]] char const * const editorClass,
                   char const * const labelName,
                   [[maybe_unused]] char const * const labelFqName,
                   LabelType * label,
                   [[maybe_unused]] char const * const editFieldName,
                   [[maybe_unused]] char const * const editFieldFqName,
                   EditFieldType * editField,
                   BtStringConst const & property,
                   [[maybe_unused]] TypeInfo const & typeInfo,
                   std::optional<int> precision = std::nullopt,
                   WhenToWriteField whenToWrite = WhenToWriteField::Normal)
   requires (!std::same_as<EditFieldType, SmartLineEdit > &&
             !std::same_as<EditFieldType, BtComboBox    > &&
             !std::same_as<EditFieldType, BtBoolComboBox>) :
      labelName  {labelName  },
      label      {label      },
      editField  {editField  },
      property   {property   },
      precision  {precision  },
      whenToWrite{whenToWrite} {
      return;
   }

   //! Constructor for when we have a SmartLineEdit
   EditorBaseField(char const * const editorClass,
                   char const * const labelName,
                   char const * const labelFqName,
                   LabelType * label,
                   char const * const editFieldName,
                   char const * const editFieldFqName,
                   SmartLineEdit * editField,
                   BtStringConst const & property,
                   TypeInfo const & typeInfo,
                   std::optional<int> precision = std::nullopt,
                   WhenToWriteField whenToWrite = WhenToWriteField::Normal)
   requires (std::same_as<EditFieldType, SmartLineEdit>) :
      labelName  {labelName  },
      label      {label      },
      editField  {editField  },
      property   {property   },
      precision  {precision  },
      whenToWrite{whenToWrite} {
      SmartAmounts::Init(editorClass,
                         labelName,
                         labelFqName,
                         *label,
                         editFieldName,
                         editFieldFqName,
                         *editField,
                         typeInfo,
                         precision);
      return;
   }

   //! Constructor for when we have a BtComboBox
   EditorBaseField(char const * const editorClass,
                   char const * const labelName,
                   char const * const labelFqName,
                   LabelType * label,
                   char const * const editFieldName,
                   char const * const editFieldFqName,
                   BtComboBox * editField,
                   BtStringConst const & property,
                   TypeInfo const & typeInfo,
                   EnumStringMapping const & nameMapping,
                   EnumStringMapping const & displayNameMapping,
                   std::vector<int>  const * restrictTo = nullptr,
                   SmartLineEdit *           controlledField = nullptr,
                   WhenToWriteField whenToWrite = WhenToWriteField::Normal)
   requires (std::same_as<EditFieldType, BtComboBox>) :
      labelName  {labelName  },
      label      {label      },
      editField  {editField  },
      property   {property   },
      precision  {std::nullopt},
      whenToWrite{whenToWrite} {
      editField->init(editorClass,
                      editFieldName,
                      editFieldFqName,
                      nameMapping,
                      displayNameMapping,
                      typeInfo,
                      restrictTo,
                      controlledField);
      return;
   }

   //! Constructor for when we have a BtBoolComboBox
   EditorBaseField(char const * const editorClass,
                   char const * const labelName,
                   [[maybe_unused]] char const * const labelFqName,
                   LabelType * label,
                   char const * const editFieldName,
                   char const * const editFieldFqName,
                   BtBoolComboBox * editField,
                   BtStringConst const & property,
                   TypeInfo const & typeInfo,
                   QString const & unsetDisplay = QObject::tr("No"),
                   QString const & setDisplay   = QObject::tr("Yes"),
                   WhenToWriteField whenToWrite = WhenToWriteField::Normal)
   requires (std::same_as<EditFieldType, BtBoolComboBox>) :
      labelName  {labelName  },
      label      {label      },
      editField  {editField  },
      property   {property   },
      precision  {std::nullopt},
      whenToWrite{whenToWrite} {
      // We could use BT_BOOL_COMBO_BOX_INIT here, but we'd be repeating a bunch of work we already did in EDITOR_FIELD
      editField->init(editorClass,
                      editFieldName,
                      editFieldFqName,
                      unsetDisplay,
                      setDisplay,
                      typeInfo);
      return;
   }

   //
   // You might think that in these connectFieldChanged, it would suffice to use QObject * as the type of context, but
   // this gave an error about "invalid conversion from ‘QObject*’ to
   // ‘const QtPrivate::FunctionPointer<void (WaterEditor::*)()>::Object*’ {aka ‘const WaterEditor*’} [-fpermissive]".
   // Rather than fight this, we just add another template parameter.
   //

   //! Simple case - the field tells us editing finished via the \c editingFinished signal
   template <typename Derived, typename Functor>
   void connectFieldChanged(Derived * context, Functor functor) const
   requires (std::same_as<EditFieldType, QLineEdit>) {
      // We ignore the defaulted parameter on connect, and its return value, as we don't need them
      context->connect(this->editField, &EditFieldType::editingFinished, context, functor, Qt::AutoConnection);
      return;
   }

   //! I don't know \c QPlainTextEdit does not have an \c editingFinished signal
   template <typename Derived, typename Functor>
   void connectFieldChanged(Derived * context, Functor functor) const
   requires (std::same_as<EditFieldType, QTextEdit> ||
             std::same_as<EditFieldType, QPlainTextEdit>) {
      context->connect(this->editField, &EditFieldType::textChanged, context, functor, Qt::AutoConnection);
      return;
   }

   //! \c SmartLineEdit uses \c editingFinished itself, and subsequently emits \c textModified after text corrections
   template <typename Derived, typename Functor>
   void connectFieldChanged(Derived * context, Functor functor) const
   requires (std::same_as<EditFieldType, SmartLineEdit>) {
      context->connect(this->editField, &EditFieldType::textModified, context, functor, Qt::AutoConnection);
      return;
   }

   //! Combo boxes are slightly different
   template <typename Derived, typename Functor>
   void connectFieldChanged(Derived * context, Functor functor) const
   requires (std::same_as<EditFieldType, BtComboBox> ||
             std::same_as<EditFieldType, BtBoolComboBox>) {
      // QOverload is needed on next line because the signal currentIndexChanged is overloaded in QComboBox - see
      // https://doc.qt.io/qt-5/qcombobox.html#currentIndexChanged
      context->connect(this->editField, QOverload<int>::of(&QComboBox::currentIndexChanged), context, functor, Qt::AutoConnection);
      return;
   }

   QVariant getFieldValue() const requires (std::same_as<EditFieldType, QTextEdit     > ||
                                            std::same_as<EditFieldType, QPlainTextEdit>) {
      return this->editField->toPlainText();
   }

   QVariant getFieldValue() const requires (std::same_as<EditFieldType, QLineEdit>) {
      return this->editField->text();
   }

   QVariant getFieldValue() const requires (std::same_as<EditFieldType, SmartLineEdit > ||
                                            std::same_as<EditFieldType, BtComboBox    > ||
                                            std::same_as<EditFieldType, BtBoolComboBox>) {
      // Through the magic of templates, and naming conventions, one line suffices for all three types
      return this->editField->getAsVariant();
   }

   /**
    * \brief Set property on supplied object from edit field
    */
   void setPropertyFromEditField(QObject & object) const {
      object.setProperty(*property, this->getFieldValue());
      return;
   }

   void setEditFieldText(QString const & val) const requires (std::same_as<EditFieldType, QTextEdit     > ||
                                                              std::same_as<EditFieldType, QPlainTextEdit>) {
      this->editField->setPlainText(val);
      return;
   }

   void setEditFieldText(QString const & val) const requires (std::same_as<EditFieldType, QLineEdit    > ||
                                                              std::same_as<EditFieldType, SmartLineEdit>) {
      this->editField->setText(val);
      return;
   }

   void setEditField(QVariant const & val) const requires (std::same_as<EditFieldType, QTextEdit     > ||
                                                           std::same_as<EditFieldType, QPlainTextEdit> ||
                                                           std::same_as<EditFieldType, QLineEdit     >) {
      this->setEditFieldText(val.toString());
      return;
   }

   void setEditField(QVariant const & val) const requires (std::same_as<EditFieldType, SmartLineEdit > ||
                                                           std::same_as<EditFieldType, BtComboBox    > ||
                                                           std::same_as<EditFieldType, BtBoolComboBox>) {
      this->editField->setFromVariant(val);
      return;
   }

   //! This clears the field, or sets it to the default value
   void clearEditField()  const requires (std::same_as<EditFieldType, QTextEdit     > ||
                                          std::same_as<EditFieldType, QPlainTextEdit> ||
                                          std::same_as<EditFieldType, QLineEdit     >) {
      this->setEditFieldText("");
      return;
   }
   void clearEditField() const requires (std::same_as<EditFieldType, SmartLineEdit > ||
                                         std::same_as<EditFieldType, BtComboBox    > ||
                                         std::same_as<EditFieldType, BtBoolComboBox>) {
      this->editField->setDefault();
      return;
   }

   /**
    * \brief Set edit field from property on supplied object
    */
   void setEditFieldFromProperty(QObject & object) const {
      this->setEditField(object.property(*property));
      return;
   }

};

using EditorBaseFieldVariant = std::variant<
   // Not all permutations are valid, hence why some are commented out
   EditorBaseField<QLabel, QLineEdit     >,
   EditorBaseField<QLabel, QTextEdit     >,
   EditorBaseField<QLabel, QPlainTextEdit>,
   EditorBaseField<QLabel, SmartLineEdit >,
   EditorBaseField<QLabel, BtComboBox    >,
   EditorBaseField<QLabel, BtBoolComboBox>,
   EditorBaseField<SmartLabel, QLineEdit     >,
//   EditorBaseField<SmartLabel, QTextEdit     >,
//   EditorBaseField<SmartLabel, QPlainTextEdit>,
   EditorBaseField<SmartLabel, SmartLineEdit >,
   EditorBaseField<SmartLabel, BtComboBox    >,
   EditorBaseField<SmartLabel, BtBoolComboBox>
>;

/**
 * \brief This macro is similar to SMART_FIELD_INIT, but allows us to pass the EditorBaseField constructor.
 *
 *        We assume that, where Foo is some subclass of NamedEntity, then the editor class for Foo is always called
 *        FooEditor.
 *
 *        Note that we can't just write decltype(*label) because (as explained at
 *        https://stackoverflow.com/questions/34231547/decltype-a-dereferenced-pointer-in-c), *label is actually a
 *        reference, and we can't have a member of EditorBaseField be a pointer to a reference.  Fortunately
 *        std::remove_pointer does what we want.
 */
#define EDITOR_FIELD(modelClass, label, editField, property, ...) \
   EditorBaseFieldVariant{ \
      EditorBaseField<std::remove_pointer<decltype(label)>::type, std::remove_pointer<decltype(editField)>::type>{\
         #modelClass "Editor", \
         #label, \
         #modelClass "Editor->" #label, \
         label, \
         #editField, \
         #modelClass "Editor->" #editField, \
         editField, \
         property, \
         modelClass ::typeLookup.getType(property) \
         __VA_OPT__(, __VA_ARGS__) \
      } \
   }

/**
 * \brief Trivial enum flag used by \c EditorBase to make instantiations more self-explanatory
 */
enum class LiveEditItem {
   Disabled,
   Enabled
};

template <LiveEditItem LEI> struct has_LiveEditItem : public std::false_type{};
template <>                 struct has_LiveEditItem<LiveEditItem::Enabled> : public std::true_type{};
// See comment in utils/TypeTraits.h for definition of CONCEPT_FIX_UP (and why, for now, we need it)
template <LiveEditItem LEI> concept CONCEPT_FIX_UP HasLiveEditItem = has_LiveEditItem<LEI>::value;

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
 *                           HopEditor
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
 *        the implementation file using the EDITOR_COMMON_CODE macro.  Eg, in HopEditor, we need:
 *
 *          (HopEditor)
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
 *
 *        Additionally, derived class needs to have the following QPushButton members (typically defined in the .ui file):
 *           pushButton_new, pushButton_save, pushButton_cancel
 *
 *        The LiveEditItem template parameter determines whether we keep a "live" copy of whatever is being edited (ie
 *        a copy object to which edits will be applied in real time).  This is useful to show fields calculated by the
 *        NE object itself or (as in the case of \c WaterEditor) to feed data to a chart.  Subclasses that set
 *        LEI = LiveEditItem::Enabled need the following additional private member functions:
 *           - \c void \c postInputFieldModified -- Update any chart following input field modification.
 */
template<class Derived> class EditorPhantom;
template<class Derived, class NE, LiveEditItem LEI = LiveEditItem::Disabled>
class EditorBase : public CuriouslyRecurringTemplateBase<EditorPhantom, Derived> {
public:
   /**
    * \brief Constructor
    *
    *        Often with CRTP it's good to make the constructor private and Derived a friend, so that only Derived can
    *        call the CRTP base constructor.  This stops errors with incorrect inheritance - eg makes a compile error if
    *        we write `class FooEditor : ... public EditorBase<BarEditor, Bar>` instead of
    *        `class FooEditor : ... public EditorBase<FooEditor, Foo>`.  However, since we want EditorWithRecipeBase to
    *        inherit from EditorBase, we can't do that trick here.
    *
    *        Note that we cannot initialise this->m_fields here, as the parameters themselves won't get constructed
    *        until Derived calls setupUi().
    */
   EditorBase(QString const editorName) :
      m_editorName{editorName},
      m_fields{nullptr},
      m_editItem{nullptr},
      m_liveEditItem{nullptr} {
      return;
   }
   ~EditorBase() = default;

   /**
    * \brief Derived should call this after calling setupUi
    */
   void postSetupUiInit(std::vector<EditorBaseFieldVariant> fields) {
      this->m_fields = std::make_unique<std::vector<EditorBaseFieldVariant>>(fields);
      this->connectSignalsAndSlots();
      return;
   }

   //! \brief No-op version
   void connectLiveEditSignalsAndSlots() requires (!HasLiveEditItem<LEI>) {
      return;
   }

   /**
    * \brief When we have a live edit item, we want to know each time an input field has been modified so that we can
    *        update the corresponding property of the live edit item.  We connect the relevant signal to
    *        Derived::inputFieldModified, which then calls doInputFieldModified
    */
   void connectLiveEditSignalsAndSlots() requires HasLiveEditItem<LEI> {
      if (this->m_fields) {
         for (auto const & field : *this->m_fields) {
            // Using std::visit and a lambda allows us to do generic things on std::variant
            std::visit(
               [this](auto&& fieldInfo) {
                  fieldInfo.connectFieldChanged(&this->derived(), &Derived::inputFieldModified);
               },
               field
            );
         }
      }
      return;
   }

   //! \brief No-op version
   void doInputFieldModified([[maybe_unused]] QObject const * const signalSender) requires (!HasLiveEditItem<LEI>) {
      return;
   }

   //! \brief Substantive version
   void doInputFieldModified(QObject const * const signalSender) requires (HasLiveEditItem<LEI>) {
      if (this->m_fields && this->m_liveEditItem && signalSender && signalSender->parent() == &this->derived()) {
         bool foundMatch = false;
         for (auto const & field : *this->m_fields) {
            // Using std::visit and a lambda allows us to do generic things on std::variant
            if (std::visit(
               // Lambda returns true if we matched the signal sender to this EditorBaseField, false otherwise
               [this, signalSender](auto&& fieldInfo) {
                  if (signalSender == fieldInfo.editField) {
                     fieldInfo.setPropertyFromEditField(*this->m_liveEditItem);
                     return true;
                  }
                  return false;
               },
               field
            )) {
               foundMatch = true;
               break;
            }
         }

         if (!foundMatch) {
            // If we get here, it's probably a coding error but there's no harm in soldiering on
            qWarning() << Q_FUNC_INFO << "Unrecognised signal sender";
            return;
         }

         this->derived().postInputFieldModified();
      }
      return;
   }

   /**
    * \brief Call this at the end of derived class's constructor (in particular, after the call to \c setupUi).
    *
    *        NOTE: This relies on derived classes having \c public, not the usual \c private, inheritance from the Ui
    *              base class (eg \c Ui::hopEditor in the example above), as otherwise \c pushButton_new etc would be
    *              inaccessible from this function.
    */
   void connectSignalsAndSlots() {
      // Standard editor slot connections
      this->derived().connect(this->derived().pushButton_new   , &QAbstractButton::clicked, &this->derived(), &Derived::clickedNew   );
      this->derived().connect(this->derived().pushButton_save  , &QAbstractButton::clicked, &this->derived(), &Derived::saveAndClose );
      this->derived().connect(this->derived().pushButton_cancel, &QAbstractButton::clicked, &this->derived(), &Derived::clearAndClose);
      //
      this->connectLiveEditSignalsAndSlots();
      return;
   }

   //! \brief No-op version
   void makeLiveEditItem() requires (!HasLiveEditItem<LEI>) {
      return;
   }

   void makeLiveEditItem() requires HasLiveEditItem<LEI> {
      this->m_liveEditItem = std::make_unique<NE>(*this->m_editItem);
      return;
   }

   /**
    * \brief Edit the given Hop, Fermentable, etc.
    *
    *        Calling with no parameter clears the current item.
    */
   void setEditItem(std::shared_ptr<NE> editItem = nullptr) {
      if (this->m_editItem) {
         this->derived().disconnect(this->m_editItem.get(), nullptr, &this->derived(), nullptr);
      }
      this->m_editItem = editItem;
      if (this->m_editItem) {
         this->derived().connect(this->m_editItem.get(), &NamedEntity::changed, &this->derived(), &Derived::changed);
         this->readFromEditItem(std::nullopt);
      }

      this->makeLiveEditItem();

      // Comment below about calling this->derived().validateBeforeSave() also applies here
      this->derived().postSetEditItem();
      return;
   }

   /**
    * \brief \c Derived can override this if there is additional processing to do at the end of \c setEditItem
    *
    *        This is used, eg, in \c WaterEditor to set up the \c RadarChart
    */
   void postSetEditItem() {
      return;
   }

   /**
    * \brief We don't want the compiler automatically constructing a shared_ptr for us if we accidentally call
    *        \c setEditItem with, say, a raw pointer, so this template trick ensures it can't.
    */
   template <typename D> void setEditItem(D) = delete;

   void setFolder(std::shared_ptr<NE> ne, QString const & folder) requires HasFolder<NE> {
      if (!folder.isEmpty()) {
         ne->setFolder(folder);
      }
      return;
   }

   void setFolder([[maybe_unused]] std::shared_ptr<NE> ne, [[maybe_unused]] QString const & folder) requires HasNoFolder<NE> {
      return;
   }

   /**
    * \brief Create a new Hop, Fermentable, etc.
    *
    *        This is also called from \c TreeView::newNamedEntity.
    */
   void newEditItem(QString folder = "") {
      QString name = QInputDialog::getText(&this->derived(),
                                           QString(QObject::tr("%1 name")).arg(NE::staticMetaObject.className()),
                                           QString(QObject::tr("%1 name:")).arg(NE::staticMetaObject.className()));
      if (name.isEmpty()) {
         return;
      }

      auto ne = std::make_shared<NE>(name);
      this->setFolder(ne, folder);

      this->setEditItem(ne);
      this->derived().show();
      return;
   }

   /**
    * \brief Subclass should override this if it needs to validate the form before saving happens.
    *
    * \return \c true if validation succeeded, \c false if it did not (and save should therefore be aborted)
    */
   bool validateBeforeSave() {
      return true;
   }

   /**
    * \brief Subclass should call this from its \c save slot
    */
   void doSaveAndClose() {
      if (!this->m_editItem) {
         this->derived().setVisible(false);
         return;
      }
      // Note that we have to call this->derived().validateBeforeSave(), not just this->validateBeforeSave(), in order
      // to allow the derived class to override validateBeforeSave().  But, because of the magic of the CRTP, there is
      // no need to make validateBeforeSave() virtual.
      if (!this->derived().validateBeforeSave()) {
         return;
      }

      this->writeNormalFields();
      if (this->m_editItem->key() < 0) {
         ObjectStoreWrapper::insert(this->m_editItem);
      }
      this->writeLateFields();

      this->derived().setVisible(false);
      return;
   }

   /**
    * \brief Subclass should call this from its \c clearAndClose slot
    */
   void doClearAndClose() {
      this->setEditItem();
      this->derived().setVisible(false); // Hide the window.
      return;
   }

   /**
    * \brief Read either one field (if \c propName specified) or all (if it is \c std::nullopt) into the UI from the
    *        model item.
    */
   void readFromEditItem(std::optional<QString> propName) {
      if (this->m_editItem && this->m_fields) {
         for (auto const & field : *this->m_fields) {
            if (std::visit(
               // This lambda returns true if we should stop subsequent loop processing, or false if we should carry on
               // looking at subsequent fields
               [this, &propName](auto&& fieldInfo) {
                  if (!propName || *propName == fieldInfo.property) {
                     fieldInfo.setEditFieldFromProperty(*this->m_editItem);
                     if (propName) {
                        // We want to break out here if we were only updating one property, but we can't do that from
                        // inside a lambda, so we need to tell the calling code to break out of the loop.
                        return true;
                     }
                  }
                  return false;
               },
               field
            )) {
               break;
            }
         }
      }
      // TODO: For the moment, we still do this call, but ultimately we'll eliminate it.
      this->derived().readFieldsFromEditItem(propName);
      return;
   }

   /**
    * \brief Subclass should call this from its \c changed slot
    *
    *        Note that \c QObject::sender has \c protected access specifier, so we can't call it from here, not even
    *        via the derived class pointer.  Therefore we have derived class call it and pass us the result.
    */
   virtual void doChanged(QObject * sender, QMetaProperty prop, [[maybe_unused]] QVariant val) {
      if (this->m_editItem && sender == this->m_editItem.get()) {
         this->readFromEditItem(prop.name());
      }
      return;
   }

   void doClearFields() {
      if (this->m_fields) {
         for (auto const & field : *this->m_fields) {
            std::visit(
               [](auto&& fieldInfo) {
                  fieldInfo.clearEditField();
               },
               field
            );
         }
      }
      return;
   }

   void readAllFields() {
      if (this->m_editItem) {
         this->readFromEditItem(std::nullopt);
      } else {
         this->doClearFields();
      }
      return;
   }

   void writeFields(WhenToWriteField const normalOrLate) {
      if (this->m_editItem && this->m_fields) {
         for (auto const & field : *this->m_fields) {
            std::visit(
               [this, normalOrLate](auto&& fieldInfo) {
                  if (normalOrLate == fieldInfo.whenToWrite) {
                     fieldInfo.setPropertyFromEditField(*this->m_editItem);
                  }
               },
               field
            );
         }
      }
      return;

   }

   void writeNormalFields() {
      this->writeFields(WhenToWriteField::Normal);
      // TODO: For the moment, we still do this call, but ultimately we'll eliminate it.
      this->derived().writeFieldsToEditItem();
      return;
   }

   void writeLateFields() {
      this->writeFields(WhenToWriteField::Late);
      // TODO: For the moment, we still do this call, but ultimately we'll eliminate it.
      this->derived().writeLateFieldsToEditItem();
      return;
   }

protected:
   /**
    * \brief Optionally an editor can have a "name" to add some context.  Eg for the Water editor, the water chemistry
    *        dialog allows you to have two of them open at once -- one "Base" and one "Target".
    */
   QString const m_editorName;

   /**
    * \brief Info about fields in this editor
    */
   std::unique_ptr<std::vector<EditorBaseFieldVariant>> m_fields;

   /**
    * \brief This is the \c NamedEntity subclass object we are creating or editing.  We are also "observing" it in the
    *        sense that, if any other part of the code changes its data, we'll get a signal so we can update our
    *        display.  Historically therefore this member variable was called \c obsHop, \c obsFermentable, etc in each
    *        of the editor classes.
    */
   std::shared_ptr<NE> m_editItem;

   /**
    * \brief Optionally, an editor can create a temporary copy of \c m_editItem to which to apply edits immediately.
    *        This is useful if we want to be able to show calculated values or if (as in the case of \c WaterEditor) we
    *        want to use a copy object to as input to a chart or graph showing live edits.  This object is discarded
    *        when the user clicks Save or Cancel.  (In the former case, the form values are applied to \c m_editItem; in
    *        the latter they are not.
    *
    *        There are various tricks where we could make the existence or type of this member variable depend on the
    *        LEI template parameter (see https://brevzin.github.io/c++/2021/11/21/conditional-members/) but it's
    *        currently a bit complicated, and should become easier with future reflection features.  So, for now, we
    *        we don't worry about the overhead of unnecessarily having this member when LEI is LiveEditItem::Disabled.
    */
   std::unique_ptr<NE> m_liveEditItem;
};

/**
 * \brief Derived classes should include this in their header file, right after Q_OBJECT
 *
 *        Note we have to be careful about comment formats in macro definitions
 */
#define EDITOR_COMMON_DECL(NeName, ...)                                                   \
   /* This allows EditorBase to call protected and private members of Derived */     \
   friend class EditorBase<NeName##Editor, NeName __VA_OPT__(, __VA_ARGS__)>;        \
                                                                                     \
   public:                                                                           \
      NeName##Editor(QWidget * parent = nullptr, QString const editorName = "");     \
      virtual ~NeName##Editor();                                                     \
                                                                                     \
      void writeFieldsToEditItem();                                                  \
      void writeLateFieldsToEditItem();                                              \
      void readFieldsFromEditItem(std::optional<QString> propName);                  \
                                                                                     \
   public slots:                                                                     \
      /* Standard editor slots */                                                    \
      void saveAndClose();                                                           \
      void clearAndClose();                                                          \
      void changed(QMetaProperty, QVariant);                                         \
      void clickedNew();                                                             \
      void inputFieldModified();                                                     \

/**
 * \brief Derived classes should include this in their implementation file
 */
#define EDITOR_COMMON_CODE(EditorName) \
   void EditorName::saveAndClose() { this->doSaveAndClose(); return; }                                                \
   void EditorName::clearAndClose() { this->doClearAndClose(); return; }                                              \
   void EditorName::changed(QMetaProperty prop, QVariant val) { this->doChanged(this->sender(), prop, val); return; } \
   void EditorName::clickedNew() { this->newEditItem(); return; }                                                     \
   void EditorName::inputFieldModified() { this->doInputFieldModified(this->sender()); return; };                     \

#endif
