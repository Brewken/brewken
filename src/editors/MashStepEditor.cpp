/*======================================================================================================================
 * editors/MashStepEditor.cpp is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
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
#include "editors/MashStepEditor.h"

#include "MainWindow.h"
#include "measurement/Unit.h"
#include "model/MashStep.h"

MashStepEditor::MashStepEditor(QWidget* parent) :
   QDialog{parent},
   EditorBase<MashStep, MashStepEditor>() {
   this->setupUi(this);

///   this->comboBox_mashStepType->setCurrentIndex(-1);

   SMART_FIELD_INIT(MashStepEditor, label_name           , lineEdit_name           , MashStep, PropertyNames::NamedEntity::name          );
   SMART_FIELD_INIT(MashStepEditor, label_stepTemp       , lineEdit_stepTemp       , MashStep, PropertyNames::MashStep::stepTemp_c       , 1);
   SMART_FIELD_INIT(MashStepEditor, label_amount         , lineEdit_amount         , MashStep, PropertyNames::MashStep::amount_l         );
   SMART_FIELD_INIT(MashStepEditor, label_infuseTemp     , lineEdit_infuseTemp     , MashStep, PropertyNames::MashStep::infuseTemp_c     , 1);
   SMART_FIELD_INIT(MashStepEditor, label_stepTime       , lineEdit_stepTime       , MashStep, PropertyNames::MashStep::stepTime_min     , 0);
   SMART_FIELD_INIT(MashStepEditor, label_rampTime       , lineEdit_rampTime       , MashStep, PropertyNames::MashStep::rampTime_min     , 0);
   SMART_FIELD_INIT(MashStepEditor, label_endTemp        , lineEdit_endTemp        , MashStep, PropertyNames::MashStep::endTemp_c        , 1);

   BT_COMBO_BOX_INIT(MashStepEditor, comboBox_mashStepType, MashStep, type);

///   connect(this->buttonBox,     &QDialogButtonBox::accepted,    this, &MashStepEditor::saveAndClose);
///   connect(this->buttonBox,     &QDialogButtonBox::rejected,    this, &MashStepEditor::close       );
///   connect(this->comboBox_mashStepType, &QComboBox::currentTextChanged, this, &MashStepEditor::grayOutStuff);
   this->connectSignalsAndSlots();
   return;
}

MashStepEditor::~MashStepEditor() = default;

///void MashStepEditor::showChanges(QMetaProperty* metaProp) {
///   if (!this->obs) {
///      this->clear();
///      return;
///   }
///
///   QString propName;
///   QVariant value;
///   bool updateAll = false;
///
///   if (metaProp == nullptr) {
///      updateAll = true;
///   } else {
///      propName = metaProp->name();
///      value = metaProp->read(this->obs.get());
///   }
void MashStepEditor::readFieldsFromEditItem(std::optional<QString> propName) {
   if (!propName || *propName == PropertyNames::NamedEntity::name          ) { this->lineEdit_name        ->setTextCursor(m_editItem->name        ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::MashStep::type             ) { this->comboBox_mashStepType->setValue     (m_editItem->type        ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::MashStep::infuseAmount_l   ) { this->lineEdit_amount      ->setAmount    (m_editItem->amount_l    ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::MashStep::infuseTemp_c     ) { this->lineEdit_infuseTemp  ->setAmount    (m_editItem->infuseTemp_c()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::MashStep::stepTemp_c       ) { this->lineEdit_stepTemp    ->setAmount    (m_editItem->stepTemp_c  ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::MashStep::stepTime_min     ) { this->lineEdit_stepTime    ->setAmount    (m_editItem->stepTime_min()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::MashStep::rampTime_min     ) { this->lineEdit_rampTime    ->setAmount    (m_editItem->rampTime_min()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::MashStep::endTemp_c        ) { this->lineEdit_endTemp     ->setAmount    (m_editItem->endTemp_c   ()); if (propName) { return; } }
   return;
}

///void MashStepEditor::clear() {
///   this->lineEdit_name           ->setText(QString(""));
///   this->comboBox_mashStepType           ->setCurrentIndex(0);
///   this->lineEdit_infuseAmount   ->setText(QString(""));
///   this->lineEdit_infuseTemp     ->setText(QString(""));
///   this->lineEdit_decoctionAmount->setText(QString(""));
///   this->lineEdit_stepTemp       ->setText(QString(""));
///   this->lineEdit_stepTime       ->setText(QString(""));
///   this->lineEdit_rampTime       ->setText(QString(""));
///   this->lineEdit_endTemp        ->setText(QString(""));
///   return;
///}
///
///void MashStepEditor::close() {
///   setVisible(false);
///   return;
///}
///
///void MashStepEditor::changed(QMetaProperty prop, QVariant /*val*/) {
///   if (sender() != this->obs.get()) {
///      return;
///   }
///
///   showChanges(&prop);
///   return;
///}

///void MashStepEditor::setMashStep(std::shared_ptr<MashStep> step) {
///   if (this->obs) {
///      disconnect(this->obs.get(), nullptr, this, nullptr);
///   }
///
///   this->obs = step;
///
///   if (this->obs) {
///      connect(this->obs.get(), &MashStep::changed, this, &MashStepEditor::changed);
///      showChanges();
///   }
///   return;
///}

void MashStepEditor::writeFieldsToEditItem() {
   this->m_editItem->setName             (this->lineEdit_name->text());
   this->m_editItem->setType             (this->comboBox_mashStepType   ->getNonOptValue<MashStep::Type>());
   this->m_editItem->setAmount_l         (this->lineEdit_amount         ->getNonOptCanonicalQty());
   this->m_editItem->setInfuseTemp_c     (this->lineEdit_infuseTemp     ->getNonOptCanonicalQty());
   this->m_editItem->setStepTemp_c       (this->lineEdit_stepTemp       ->getNonOptCanonicalQty());
   this->m_editItem->setStepTime_min     (this->lineEdit_stepTime       ->getNonOptCanonicalQty());
   this->m_editItem->setRampTime_min     (this->lineEdit_rampTime       ->getNonOptCanonicalQty());
   this->m_editItem->setEndTemp_c        (this->lineEdit_endTemp        ->getNonOptCanonicalQty());

///   if (this->obs->key() < 0) {
///      // This is a new MashStep, so we need to store it.
///      // We'll ask MainWindow to do this for us, because then it can be an undoable action.
///      //
///      // The Mash of this MashStep should already have been set by the caller
///      MainWindow::instance().addMashStepToMash(this->obs);
///   }
///
///   setVisible(false);
   return;
}

void MashStepEditor::writeLateFieldsToEditItem() {
   // Nothing to do here
   return;
}

void MashStepEditor::grayOutStuff(QString const & text) {
   if (text == "Infusion") {
//      lineEdit_infuseAmount->setEnabled(true);
      lineEdit_infuseTemp->setEnabled(true);
//      lineEdit_decoctionAmount->setEnabled(false);
   } else if (text == "Decoction") {
//      lineEdit_infuseAmount->setEnabled(false);
      lineEdit_infuseTemp->setEnabled(false);
//      lineEdit_decoctionAmount->setEnabled(true);
   } else if (text == "Temperature") {
//      lineEdit_infuseAmount->setEnabled(false);
      lineEdit_infuseTemp->setEnabled(false);
//      lineEdit_decoctionAmount->setEnabled(false);
   } else {
//      lineEdit_infuseAmount->setEnabled(true);
      lineEdit_infuseTemp->setEnabled(true);
//      lineEdit_decoctionAmount->setEnabled(true);
   }
   return;
}

// Insert the boiler-plate stuff that we cannot do in EditorBase
EDITOR_COMMON_SLOT_DEFINITIONS(MashStepEditor)
