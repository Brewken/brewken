/*======================================================================================================================
 * editors/EquipmentEditor.cpp is part of Brewken, and is copyright the following authors 2009-2023:
 *   • A.J. Drobnich <aj.drobnich@gmail.com>
 *   • Brian Rower <brian.rower@gmail.com>
 *   • David Grundberg <individ@acc.umu.se>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mike Evans <mikee@saxicola.co.uk>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Priceless Brewing <shadowchao99@gmail.com>
 *   • Tyler Cipriani <tcipriani@wikimedia.org>
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
#include "editors/EquipmentEditor.h"

#include <QCloseEvent>
#include <QDebug>
#include <QIcon>
#include <QInputDialog>
#include <QMessageBox>

#include "BtHorizontalTabs.h"
#include "database/ObjectStoreWrapper.h"
#include "EquipmentListModel.h"
#include "HeatCalculations.h"
#include "Localization.h"
#include "measurement/Measurement.h"
#include "measurement/Unit.h"
#include "model/Equipment.h"
#include "NamedEntitySortProxyModel.h"
#include "PersistentSettings.h"
#include "PhysicalConstants.h"

//
// TODO: According to https://www.engineersedge.com/materials/specific_heat_capacity_of_metals_13259.htm, the specific
// heat capacity of 304 grade stainless steel is 502.416 J/kg·K = 0.120080 c/g·C.  Would be nice to have a way for the
// user to grab this value (and that of other common materials if we can find them).
//

EquipmentEditor::EquipmentEditor(QWidget* parent/*, bool singleEquipEditor*/) :
   QDialog(parent),
   EditorBase<Equipment, EquipmentEditor>() {
   this->setupUi(this);

///   if (singleEquipEditor) {
///      //horizontalLayout_equipments->setVisible(false);
///      for (int i = 0; i < horizontalLayout_equipments->count(); ++i) {
///         QWidget* w = horizontalLayout_equipments->itemAt(i)->widget();
///         if (w) {
///            w->setVisible(false);
///         }
///      }
///
///   }

   this->tabWidget_editor->tabBar()->setStyle(new BtHorizontalTabs);

///   // Set grain absorption label based on units.
///   Measurement::Unit const * weightUnit = nullptr;
///   Measurement::Unit const * volumeUnit = nullptr;
///   Measurement::getThicknessUnits(&volumeUnit, &weightUnit);
///   label_mashTunGrainAbsorption->setText(tr("Grain absorption (%1/%2)").arg(volumeUnit->name).arg(weightUnit->name));

///   equipmentListModel = new EquipmentListModel(equipmentComboBox);
///   equipmentSortProxyModel = new NamedEntitySortProxyModel(equipmentListModel);
///   equipmentComboBox->setModel(equipmentSortProxyModel);
///
///   obsEquip = nullptr;

   // TODO: Reinstate handling of checkBox_defaultEquipment, pushButton_absorption
   SMART_FIELD_INIT(EquipmentEditor, label_name                    , lineEdit_name                    , Equipment, PropertyNames::NamedEntity::name                     );
   SMART_FIELD_INIT(EquipmentEditor, label_mashTunSpecificHeat     , lineEdit_mashTunSpecificHeat     , Equipment, PropertyNames::Equipment::mashTunSpecificHeat_calGC  );
   SMART_FIELD_INIT(EquipmentEditor, label_mashTunGrainAbsorption  , lineEdit_mashTunGrainAbsorption  , Equipment, PropertyNames::Equipment::mashTunGrainAbsorption_LKg );
   SMART_FIELD_INIT(EquipmentEditor, label_hopUtilization          , lineEdit_hopUtilization          , Equipment, PropertyNames::Equipment::hopUtilization_pct         , 0);
   SMART_FIELD_INIT(EquipmentEditor, label_mashTunWeight           , lineEdit_mashTunWeight           , Equipment, PropertyNames::Equipment::mashTunWeight_kg           );
   SMART_FIELD_INIT(EquipmentEditor, label_boilingPoint            , lineEdit_boilingPoint            , Equipment, PropertyNames::Equipment::boilingPoint_c             , 1);
   SMART_FIELD_INIT(EquipmentEditor, label_boilTime                , lineEdit_boilTime                , Equipment, PropertyNames::Equipment::boilTime_min               );
   SMART_FIELD_INIT(EquipmentEditor, label_fermenterBatchSize      , lineEdit_fermenterBatchSize      , Equipment, PropertyNames::Equipment::fermenterBatchSize_l       );
   SMART_FIELD_INIT(EquipmentEditor, label_kettleBoilSize          , lineEdit_kettleBoilSize          , Equipment, PropertyNames::Equipment::kettleBoilSize_l           );
   SMART_FIELD_INIT(EquipmentEditor, label_kettleEvaporationPerHour, lineEdit_kettleEvaporationPerHour, Equipment, PropertyNames::Equipment::kettleEvaporationPerHour_l );
   SMART_FIELD_INIT(EquipmentEditor, label_lauterDeadspaceLoss     , lineEdit_lauterDeadspaceLoss     , Equipment, PropertyNames::Equipment::lauterDeadspaceLoss_l      );
   SMART_FIELD_INIT(EquipmentEditor, label_topUpKettle             , lineEdit_topUpKettle             , Equipment, PropertyNames::Equipment::topUpKettle_l              );
   SMART_FIELD_INIT(EquipmentEditor, label_topUpWater              , lineEdit_topUpWater              , Equipment, PropertyNames::Equipment::topUpWater_l               );
   SMART_FIELD_INIT(EquipmentEditor, label_kettleTrubChillerLoss   , lineEdit_kettleTrubChillerLoss   , Equipment, PropertyNames::Equipment::kettleTrubChillerLoss_l    );
   SMART_FIELD_INIT(EquipmentEditor, label_mashTunVolume           , lineEdit_mashTunVolume           , Equipment, PropertyNames::Equipment::mashTunVolume_l            );
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   SMART_FIELD_INIT(EquipmentEditor, label_hltType                 , lineEdit_hltType                 , Equipment, PropertyNames::Equipment::hltType                    );
   SMART_FIELD_INIT(EquipmentEditor, label_mashTunType             , lineEdit_mashTunType             , Equipment, PropertyNames::Equipment::mashTunType                );
   SMART_FIELD_INIT(EquipmentEditor, label_lauterTunType           , lineEdit_lauterTunType           , Equipment, PropertyNames::Equipment::lauterTunType              );
   SMART_FIELD_INIT(EquipmentEditor, label_kettleType              , lineEdit_kettleType              , Equipment, PropertyNames::Equipment::kettleType                 );
   SMART_FIELD_INIT(EquipmentEditor, label_fermenterType           , lineEdit_fermenterType           , Equipment, PropertyNames::Equipment::fermenterType              );
   SMART_FIELD_INIT(EquipmentEditor, label_agingVesselType         , lineEdit_agingVesselType         , Equipment, PropertyNames::Equipment::agingVesselType            );
   SMART_FIELD_INIT(EquipmentEditor, label_packagingVesselType     , lineEdit_packagingVesselType     , Equipment, PropertyNames::Equipment::packagingVesselType        );
   SMART_FIELD_INIT(EquipmentEditor, label_hltVolume               , lineEdit_hltVolume               , Equipment, PropertyNames::Equipment::hltVolume_l                );
   SMART_FIELD_INIT(EquipmentEditor, label_lauterTunVolume         , lineEdit_lauterTunVolume         , Equipment, PropertyNames::Equipment::lauterTunVolume_l          );
   SMART_FIELD_INIT(EquipmentEditor, label_agingVesselVolume       , lineEdit_agingVesselVolume       , Equipment, PropertyNames::Equipment::agingVesselVolume_l        );
   SMART_FIELD_INIT(EquipmentEditor, label_packagingVesselVolume   , lineEdit_packagingVesselVolume   , Equipment, PropertyNames::Equipment::packagingVesselVolume_l    );
   SMART_FIELD_INIT(EquipmentEditor, label_hltLoss                 , lineEdit_hltLoss                 , Equipment, PropertyNames::Equipment::hltLoss_l                  );
   SMART_FIELD_INIT(EquipmentEditor, label_mashTunLoss             , lineEdit_mashTunLoss             , Equipment, PropertyNames::Equipment::mashTunLoss_l              );
   SMART_FIELD_INIT(EquipmentEditor, label_fermenterLoss           , lineEdit_fermenterLoss           , Equipment, PropertyNames::Equipment::fermenterLoss_l            );
   SMART_FIELD_INIT(EquipmentEditor, label_agingVesselLoss         , lineEdit_agingVesselLoss         , Equipment, PropertyNames::Equipment::agingVesselLoss_l          );
   SMART_FIELD_INIT(EquipmentEditor, label_packagingVesselLoss     , lineEdit_packagingVesselLoss     , Equipment, PropertyNames::Equipment::packagingVesselLoss_l      );
   SMART_FIELD_INIT(EquipmentEditor, label_kettleOutflowPerMinute  , lineEdit_kettleOutflowPerMinute  , Equipment, PropertyNames::Equipment::kettleOutflowPerMinute_l   );
   SMART_FIELD_INIT(EquipmentEditor, label_hltWeight               , lineEdit_hltWeight               , Equipment, PropertyNames::Equipment::hltWeight_kg               );
   SMART_FIELD_INIT(EquipmentEditor, label_lauterTunWeight         , lineEdit_lauterTunWeight         , Equipment, PropertyNames::Equipment::lauterTunWeight_kg         );
   SMART_FIELD_INIT(EquipmentEditor, label_kettleWeight            , lineEdit_kettleWeight            , Equipment, PropertyNames::Equipment::kettleWeight_kg            );
   SMART_FIELD_INIT(EquipmentEditor, label_hltSpecificHeat         , lineEdit_hltSpecificHeat         , Equipment, PropertyNames::Equipment::hltSpecificHeat_calGC      );
   SMART_FIELD_INIT(EquipmentEditor, label_lauterTunSpecificHeat   , lineEdit_lauterTunSpecificHeat   , Equipment, PropertyNames::Equipment::lauterTunSpecificHeat_calGC);
   SMART_FIELD_INIT(EquipmentEditor, label_kettleSpecificHeat      , lineEdit_kettleSpecificHeat      , Equipment, PropertyNames::Equipment::kettleSpecificHeat_calGC   );

///   // Connect all the edit boxen
///   connect(lineEdit_boilTime,         &SmartLineEdit::textModified,  this, &EquipmentEditor::updateCheckboxRecord     );
///   connect(lineEdit_kettleEvaporationPerHour,  &SmartLineEdit::textModified,  this, &EquipmentEditor::updateCheckboxRecord     );
///   connect(lineEdit_topUpWater,       &SmartLineEdit::textModified,  this, &EquipmentEditor::updateCheckboxRecord     );
///   connect(lineEdit_kettleTrubChillerLoss,  &SmartLineEdit::textModified,  this, &EquipmentEditor::updateCheckboxRecord     );
///   connect(lineEdit_fermenterBatchSize,        &SmartLineEdit::textModified,  this, &EquipmentEditor::updateCheckboxRecord     );
///   // Set up the buttons
///   // Note, per https://wiki.qt.io/New_Signal_Slot_Syntax#Default_arguments_in_slot, the use of a trivial lambda
///   // function to allow use of default argument on newEquipment() slot
///   connect(pushButton_save,           &QAbstractButton::clicked,      this, &EquipmentEditor::save                     );
///   connect(pushButton_new,            &QAbstractButton::clicked,      this, [this]() { this->newEquipment(); return; } );
///   connect(pushButton_cancel,         &QAbstractButton::clicked,      this, &EquipmentEditor::cancel                   );
///   connect(pushButton_remove,         &QAbstractButton::clicked,      this, &EquipmentEditor::removeEquipment          );
///   connect(pushButton_absorption,     &QAbstractButton::clicked,      this, &EquipmentEditor::resetAbsorption          );
///   connect(equipmentComboBox,         &QComboBox::currentTextChanged, this, &EquipmentEditor::equipmentSelected        );
///   // Check boxen
///   connect(checkBox_calcBoilVolume,   &QCheckBox::stateChanged,       this, &EquipmentEditor::updateCheckboxRecord     );
///   connect(checkBox_defaultEquipment, &QCheckBox::stateChanged,       this, &EquipmentEditor::updateDefaultEquipment   );
///
///   // make sure the dialog gets populated the first time it's opened from the menu
///   equipmentSelected();
///   // Ensure correct state of Boil Volume edit box.
///   updateCheckboxRecord();
////   this->infoButton_name->linkWith(this->infoText_name);

   this->connectSignalsAndSlots();
   return;
}

EquipmentEditor::~EquipmentEditor() = default;

///void EquipmentEditor::setEquipment(Equipment* e) {
///   if (e) {
///      obsEquip = e;
///
///      // Make sure the combo box gets set to the right place.
///      QModelIndex modelIndex(equipmentListModel->find(e));
///      QModelIndex viewIndex(equipmentSortProxyModel->mapFromSource(modelIndex));
///      if (viewIndex.isValid()) {
///         equipmentComboBox->setCurrentIndex(viewIndex.row());
///      }
///
///      showChanges();
///   }
///   return;
///}
///
///void EquipmentEditor::removeEquipment() {
///   if( this->obsEquip ) {
///      ObjectStoreWrapper::softDelete(*this->obsEquip);
///   }
///
///   equipmentComboBox->setCurrentIndex(-1);
///   setEquipment(nullptr);
///   return;
///}
///
///void EquipmentEditor::clear() {
///   lineEdit_name           ->setText(QString(""));
///   lineEdit_name           ->setCursorPosition(0);
///   lineEdit_kettleBoilSize       ->setText(QString(""));
///   checkBox_calcBoilVolume ->setCheckState(Qt::Unchecked);
///   lineEdit_fermenterBatchSize      ->setText(QString(""));
///   lineEdit_mashTunVolume      ->setText(QString(""));
///   lineEdit_mashTunWeight      ->setText(QString(""));
///   lineEdit_mashTunSpecificHeat->setText(QString(""));
///   lineEdit_boilTime       ->setText(QString(""));
///   lineEdit_kettleEvaporationPerHour->setText(QString(""));
///   lineEdit_topUpKettle    ->setText(QString(""));
///   lineEdit_topUpWater     ->setText(QString(""));
///   lineEdit_kettleTrubChillerLoss->setText(QString(""));
///   lineEdit_lauterDeadspaceLoss->setText(QString(""));
///   lineEdit_hopUtilization ->setText(QString(""));
///   textEdit_kettleNotes          ->setText(QString(""));
///   lineEdit_mashTunGrainAbsorption->setText(QString(""));
///   return;
///}

///void EquipmentEditor::equipmentSelected() {
///   QModelIndex viewIndex(
///      equipmentComboBox->model()->index(equipmentComboBox->currentIndex(),0)
///   );
///
///   QModelIndex modelIndex = equipmentSortProxyModel->mapToSource(viewIndex);
///
///   setEquipment( equipmentListModel->at(modelIndex.row()) );
///   return;
///}

///void EquipmentEditor::save() {
///   if (obsEquip == nullptr) {
///      setVisible(false);
///      return;
///   }
///
///   Measurement::Unit const * weightUnit = nullptr;
///   Measurement::Unit const * volumeUnit = nullptr;
///   Measurement::getThicknessUnits(&volumeUnit, &weightUnit);
///
///   double grainAbs = Localization::toDouble(lineEdit_mashTunGrainAbsorption->text(), Q_FUNC_INFO);
///
///   double ga_LKg = grainAbs * volumeUnit->toCanonical(1.0).quantity() * weightUnit->fromCanonical(1.0);

void EquipmentEditor::writeFieldsToEditItem() {
//   QString describe;
   bool problems=false;

   // Do some prewarning things. I would prefer to do this only on change, but
   // we need to be worried about new equipment too.
   QString message = tr("This equipment profile may break Brewken's maths");
   QString inform = QString("%1%2").arg(tr("The following values are not set:")).arg(QString("<ul>"));
   if ( qFuzzyCompare(lineEdit_mashTunVolume->toCanonical().quantity(), 0.0) ) {
      problems = true;
      inform = inform + QString("<li>%1</li>").arg(tr("mash tun volume (all-grain and BIAB only)"));
   }

   if ( qFuzzyCompare(lineEdit_fermenterBatchSize->toCanonical().quantity(), 0.0) ) {
      problems = true;
      inform = inform + QString("<li>%1</li>").arg(tr("batch size"));
   }

   if ( qFuzzyCompare(lineEdit_hopUtilization->getNonOptValue<double>(), 0.0) ) {
      problems = true;
      inform = inform + QString("<li>%1</li>").arg(tr("hop utilization"));
   }
   inform = inform + QString("</ul>");

   if (problems) {
      QMessageBox theQuestion;
      theQuestion.setWindowTitle( tr("Calculation Warnings") );
      theQuestion.setText( message );
      theQuestion.setInformativeText( inform );
      theQuestion.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
      theQuestion.setDefaultButton(QMessageBox::Save);
      theQuestion.setIcon(QMessageBox::Warning);
      if (theQuestion.exec() == QMessageBox::Cancel) {
         return;
      }
   }

///   m_editItem->setMashTunGrainAbsorption_LKg  (ga_LKg );

   m_editItem->setName                       (lineEdit_name                    ->text                 ());
   m_editItem->setKettleBoilSize_l           (lineEdit_kettleBoilSize          ->getNonOptCanonicalQty());
   m_editItem->setFermenterBatchSize_l       (lineEdit_fermenterBatchSize      ->getNonOptCanonicalQty());
   m_editItem->setMashTunVolume_l            (lineEdit_mashTunVolume           ->getNonOptCanonicalQty());
   m_editItem->setMashTunWeight_kg           (lineEdit_mashTunWeight           ->getOptCanonicalQty   ());
   m_editItem->setMashTunSpecificHeat_calGC  (lineEdit_mashTunSpecificHeat     ->getOptCanonicalQty   ());
   m_editItem->setBoilTime_min               (lineEdit_boilTime                ->getOptCanonicalQty   ());
   m_editItem->setKettleEvaporationPerHour_l (lineEdit_kettleEvaporationPerHour->getOptCanonicalQty   ());
   m_editItem->setTopUpKettle_l              (lineEdit_topUpKettle             ->getOptCanonicalQty   ());
   m_editItem->setTopUpWater_l               (lineEdit_topUpWater              ->getOptCanonicalQty   ());
   m_editItem->setKettleTrubChillerLoss_l    (lineEdit_kettleTrubChillerLoss   ->getNonOptCanonicalQty());
   m_editItem->setLauterDeadspaceLoss_l      (lineEdit_lauterDeadspaceLoss     ->getNonOptCanonicalQty());
   m_editItem->setMashTunGrainAbsorption_LKg (lineEdit_mashTunGrainAbsorption  ->getOptCanonicalQty   ());
   m_editItem->setBoilingPoint_c             (lineEdit_boilingPoint            ->getNonOptCanonicalQty());
   m_editItem->setHopUtilization_pct         (lineEdit_hopUtilization          ->getOptValue<double>  ());
   m_editItem->setKettleNotes                (textEdit_kettleNotes             ->toPlainText          ());
   m_editItem->setCalcBoilVolume             (checkBox_calcBoilVolume          ->checkState() == Qt::Checked);
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   m_editItem->setHltType                    (lineEdit_hltType                 ->text                 ());
   m_editItem->setMashTunType                (lineEdit_mashTunType             ->text                 ());
   m_editItem->setLauterTunType              (lineEdit_lauterTunType           ->text                 ());
   m_editItem->setKettleType                 (lineEdit_kettleType              ->text                 ());
   m_editItem->setFermenterType              (lineEdit_fermenterType           ->text                 ());
   m_editItem->setAgingVesselType            (lineEdit_agingVesselType         ->text                 ());
   m_editItem->setPackagingVesselType        (lineEdit_packagingVesselType     ->text                 ());
   m_editItem->setHltVolume_l                (lineEdit_hltVolume               ->getNonOptCanonicalQty());
   m_editItem->setLauterTunVolume_l          (lineEdit_lauterTunVolume         ->getNonOptCanonicalQty());
   m_editItem->setAgingVesselVolume_l        (lineEdit_agingVesselVolume       ->getNonOptCanonicalQty());
   m_editItem->setPackagingVesselVolume_l    (lineEdit_packagingVesselVolume   ->getNonOptCanonicalQty());
   m_editItem->setHltLoss_l                  (lineEdit_hltLoss                 ->getNonOptCanonicalQty());
   m_editItem->setMashTunLoss_l              (lineEdit_mashTunLoss             ->getNonOptCanonicalQty());
   m_editItem->setFermenterLoss_l            (lineEdit_fermenterLoss           ->getNonOptCanonicalQty());
   m_editItem->setAgingVesselLoss_l          (lineEdit_agingVesselLoss         ->getNonOptCanonicalQty());
   m_editItem->setPackagingVesselLoss_l      (lineEdit_packagingVesselLoss     ->getNonOptCanonicalQty());
   m_editItem->setKettleOutflowPerMinute_l   (lineEdit_kettleOutflowPerMinute  ->getOptCanonicalQty   ());
   m_editItem->setHltWeight_kg               (lineEdit_hltWeight               ->getOptCanonicalQty   ());
   m_editItem->setLauterTunWeight_kg         (lineEdit_lauterTunWeight         ->getOptCanonicalQty   ());
   m_editItem->setKettleWeight_kg            (lineEdit_kettleWeight            ->getOptCanonicalQty   ());
   m_editItem->setHltSpecificHeat_calGC      (lineEdit_hltSpecificHeat         ->getOptCanonicalQty   ());
   m_editItem->setLauterTunSpecificHeat_calGC(lineEdit_lauterTunSpecificHeat   ->getOptCanonicalQty   ());
   m_editItem->setKettleSpecificHeat_calGC   (lineEdit_kettleSpecificHeat      ->getOptCanonicalQty   ());
   m_editItem->setHltNotes                   (textEdit_hltNotes                ->toPlainText          ());
   m_editItem->setMashTunNotes               (textEdit_mashTunNotes            ->toPlainText          ());
   m_editItem->setLauterTunNotes             (textEdit_lauterTunNotes          ->toPlainText          ());
   m_editItem->setFermenterNotes             (textEdit_fermenterNotes          ->toPlainText          ());
   m_editItem->setAgingVesselNotes           (textEdit_agingVesselNotes        ->toPlainText          ());
   m_editItem->setPackagingVesselNotes       (textEdit_packagingVesselNotes    ->toPlainText          ());

///   if (this->obsEquip->key() < 0) {
///      ObjectStoreWrapper::insert(*obsEquip);
///   }
///   setVisible(false);
   return;
}

void EquipmentEditor::writeLateFieldsToEditItem() {
   // Nothing to do here for Equipment
   return;
}

//void EquipmentEditor::newEquipment() {
//   newEquipment(QString());
//   return;
//}

///void EquipmentEditor::newEquipment(QString folder) {
///   QString name = QInputDialog::getText(this, tr("Equipment name"),
///                                          tr("Equipment name:"));
///   if( name.isEmpty() )
///      return;
///
///   // .:TODO:. Change to shared_ptr as currently leads to memory leak
///   Equipment* e = new Equipment(name);
///
///   if ( ! folder.isEmpty() )
///      e->setFolder(folder);
///
///   setEquipment(e);
///   show();
///}
///
///void EquipmentEditor::cancel() {
///   setEquipment(obsEquip);
///
///   setVisible(false);
///   return;
///}
///
///void EquipmentEditor::resetAbsorption() {
///   if( obsEquip == nullptr )
///      return;
///
///   // Get weight and volume units for grain absorption.
///   Measurement::Unit const * weightUnit = nullptr;
///   Measurement::Unit const * volumeUnit = nullptr;
///   Measurement::getThicknessUnits(&volumeUnit, &weightUnit);
///   double gaCustomUnits = PhysicalConstants::grainAbsorption_Lkg * volumeUnit->fromCanonical(1.0) * weightUnit->toCanonical(1.0).quantity();
///
///   lineEdit_mashTunGrainAbsorption->setAmount(gaCustomUnits);
///   return;
///}
///
///void EquipmentEditor::changed(QMetaProperty /*prop*/, QVariant /*val*/) {
///   if( sender() == obsEquip ) {
///      showChanges();
///   }
///   return;
///}

///void EquipmentEditor::showChanges() {
///   if( this->obsEquip == nullptr ) {
///      clear();
///      return;
///   }
///
///   // Get weight and volume units for grain absorption.
///   Measurement::Unit const * weightUnit = nullptr;
///   Measurement::Unit const * volumeUnit = nullptr;
///   Measurement::getThicknessUnits( &volumeUnit, &weightUnit );
///   this->label_mashTunGrainAbsorption->setText(tr("Grain absorption (%1/%2)").arg(volumeUnit->name).arg(weightUnit->name));
///
///   //equipmentComboBox->setIndexByEquipment(this->obsEquip);

void EquipmentEditor::readFieldsFromEditItem(std::optional<QString> propName) {
///   this->lineEdit_name            ->setText(this->obsEquip->name());
///   this->lineEdit_name            ->setCursorPosition(0);
///   this->tabWidget_editor         ->setTabText(0, this->obsEquip->name());
///   this->lineEdit_kettleBoilSize        ->setAmount(this->obsEquip->kettleBoilSize_l());

///   this->checkBox_calcBoilVolume  ->blockSignals(true); // Keep next line from emitting a signal and changing this->obsEquip.
///   this->checkBox_calcBoilVolume  ->setCheckState( (this->obsEquip->calcBoilVolume())? Qt::Checked : Qt::Unchecked );
///   this->checkBox_calcBoilVolume  ->blockSignals(false);

///   this->lineEdit_fermenterBatchSize       ->setAmount(this->obsEquip->fermenterBatchSize_l               ());
///   this->lineEdit_mashTunVolume       ->setAmount(this->obsEquip->mashTunVolume_l           ());
///   this->lineEdit_mashTunWeight       ->setAmount(this->obsEquip->mashTunWeight_kg          ());
///   this->lineEdit_mashTunSpecificHeat ->setAmount(this->obsEquip->mashTunSpecificHeat_calGC ());
///   this->lineEdit_boilTime        ->setAmount(this->obsEquip->boilTime_min              ());
///   this->lineEdit_kettleEvaporationPerHour ->setAmount(this->obsEquip->kettleEvaporationPerHour_l());
///   this->lineEdit_topUpKettle     ->setAmount(this->obsEquip->topUpKettle_l             ());
///   this->lineEdit_topUpWater      ->setAmount(this->obsEquip->topUpWater_l              ());
///   this->lineEdit_kettleTrubChillerLoss ->setAmount(this->obsEquip->kettleTrubChillerLoss_l         ());
///   this->lineEdit_lauterDeadspaceLoss ->setAmount(this->obsEquip->lauterDeadspaceLoss_l         ());
///   this->textEdit_kettleNotes           ->setText  (this->obsEquip->kettleNotes               ());
///
///   double gaCustomUnits = this->obsEquip->mashTunGrainAbsorption_LKg().value_or(Equipment::default_mashTunGrainAbsorption_LKg) * volumeUnit->fromCanonical(1.0) * weightUnit->toCanonical(1.0).quantity();
///   this->lineEdit_mashTunGrainAbsorption ->setAmount(gaCustomUnits);

   if (!propName || *propName == PropertyNames::NamedEntity::name    ) { this->lineEdit_name          ->setTextCursor(m_editItem->name          ()); // Continues to next line
                                                                         /* this->tabWidget_editor->setTabText(0, m_editItem->name()); */                 if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::kettleBoilSize_l           ) { this->lineEdit_kettleBoilSize          ->setAmount    (m_editItem->kettleBoilSize_l           ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::fermenterBatchSize_l       ) { this->lineEdit_fermenterBatchSize      ->setAmount    (m_editItem->fermenterBatchSize_l       ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::mashTunVolume_l            ) { this->lineEdit_mashTunVolume           ->setAmount    (m_editItem->mashTunVolume_l            ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::mashTunWeight_kg           ) { this->lineEdit_mashTunWeight           ->setAmount    (m_editItem->mashTunWeight_kg           ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::mashTunSpecificHeat_calGC  ) { this->lineEdit_mashTunSpecificHeat     ->setAmount    (m_editItem->mashTunSpecificHeat_calGC  ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::boilTime_min               ) { this->lineEdit_boilTime                ->setAmount    (m_editItem->boilTime_min               ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::kettleEvaporationPerHour_l ) { this->lineEdit_kettleEvaporationPerHour->setAmount    (m_editItem->kettleEvaporationPerHour_l ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::topUpKettle_l              ) { this->lineEdit_topUpKettle             ->setAmount    (m_editItem->topUpKettle_l              ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::topUpWater_l               ) { this->lineEdit_topUpWater              ->setAmount    (m_editItem->topUpWater_l               ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::kettleTrubChillerLoss_l    ) { this->lineEdit_kettleTrubChillerLoss   ->setAmount    (m_editItem->kettleTrubChillerLoss_l    ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::lauterDeadspaceLoss_l      ) { this->lineEdit_lauterDeadspaceLoss     ->setAmount    (m_editItem->lauterDeadspaceLoss_l      ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::kettleNotes                ) { this->textEdit_kettleNotes             ->setText      (m_editItem->kettleNotes                ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::mashTunGrainAbsorption_LKg ) { this->lineEdit_mashTunGrainAbsorption  ->setAmount    (m_editItem->mashTunGrainAbsorption_LKg ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::boilingPoint_c             ) { this->lineEdit_boilingPoint            ->setAmount    (m_editItem->boilingPoint_c             ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::hopUtilization_pct         ) { this->lineEdit_hopUtilization          ->setAmount    (m_editItem->hopUtilization_pct         ()); if (propName) { return; } }
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   if (!propName || *propName == PropertyNames::Equipment::hltType                    ) { this->lineEdit_hltType                 ->setTextCursor(m_editItem->hltType                    ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::mashTunType                ) { this->lineEdit_mashTunType             ->setTextCursor(m_editItem->mashTunType                ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::lauterTunType              ) { this->lineEdit_lauterTunType           ->setTextCursor(m_editItem->lauterTunType              ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::kettleType                 ) { this->lineEdit_kettleType              ->setTextCursor(m_editItem->kettleType                 ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::fermenterType              ) { this->lineEdit_fermenterType           ->setTextCursor(m_editItem->fermenterType              ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::agingVesselType            ) { this->lineEdit_agingVesselType         ->setTextCursor(m_editItem->agingVesselType            ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::packagingVesselType        ) { this->lineEdit_packagingVesselType     ->setTextCursor(m_editItem->packagingVesselType        ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::hltVolume_l                ) { this->lineEdit_hltVolume               ->setAmount    (m_editItem->hltVolume_l                ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::lauterTunVolume_l          ) { this->lineEdit_lauterTunVolume         ->setAmount    (m_editItem->lauterTunVolume_l          ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::agingVesselVolume_l        ) { this->lineEdit_agingVesselVolume       ->setAmount    (m_editItem->agingVesselVolume_l        ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::packagingVesselVolume_l    ) { this->lineEdit_packagingVesselVolume   ->setAmount    (m_editItem->packagingVesselVolume_l    ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::hltLoss_l                  ) { this->lineEdit_hltLoss                 ->setAmount    (m_editItem->hltLoss_l                  ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::mashTunLoss_l              ) { this->lineEdit_mashTunLoss             ->setAmount    (m_editItem->mashTunLoss_l              ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::fermenterLoss_l            ) { this->lineEdit_fermenterLoss           ->setAmount    (m_editItem->fermenterLoss_l            ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::agingVesselLoss_l          ) { this->lineEdit_agingVesselLoss         ->setAmount    (m_editItem->agingVesselLoss_l          ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::packagingVesselLoss_l      ) { this->lineEdit_packagingVesselLoss     ->setAmount    (m_editItem->packagingVesselLoss_l      ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::kettleOutflowPerMinute_l   ) { this->lineEdit_kettleOutflowPerMinute  ->setAmount    (m_editItem->kettleOutflowPerMinute_l   ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::hltWeight_kg               ) { this->lineEdit_hltWeight               ->setAmount    (m_editItem->hltWeight_kg               ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::lauterTunWeight_kg         ) { this->lineEdit_lauterTunWeight         ->setAmount    (m_editItem->lauterTunWeight_kg         ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::kettleWeight_kg            ) { this->lineEdit_kettleWeight            ->setAmount    (m_editItem->kettleWeight_kg            ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::hltSpecificHeat_calGC      ) { this->lineEdit_hltSpecificHeat         ->setAmount    (m_editItem->hltSpecificHeat_calGC      ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::lauterTunSpecificHeat_calGC) { this->lineEdit_lauterTunSpecificHeat   ->setAmount    (m_editItem->lauterTunSpecificHeat_calGC()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::kettleSpecificHeat_calGC   ) { this->lineEdit_kettleSpecificHeat      ->setAmount    (m_editItem->kettleSpecificHeat_calGC   ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::hltNotes                   ) { this->textEdit_hltNotes                ->setText      (m_editItem->hltNotes                   ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::mashTunNotes               ) { this->textEdit_mashTunNotes            ->setText      (m_editItem->mashTunNotes               ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::lauterTunNotes             ) { this->textEdit_lauterTunNotes          ->setText      (m_editItem->lauterTunNotes             ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::fermenterNotes             ) { this->textEdit_fermenterNotes          ->setText      (m_editItem->fermenterNotes             ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::agingVesselNotes           ) { this->textEdit_agingVesselNotes        ->setText      (m_editItem->agingVesselNotes           ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Equipment::packagingVesselNotes       ) { this->textEdit_packagingVesselNotes    ->setText      (m_editItem->packagingVesselNotes       ()); if (propName) { return; } }


///   this->lineEdit_boilingPoint    ->setAmount(this->obsEquip->boilingPoint_c    ());
///   this->lineEdit_hopUtilization  ->setAmount(this->obsEquip->hopUtilization_pct());
///   this->checkBox_defaultEquipment->blockSignals(true);
///   if (PersistentSettings::value(PersistentSettings::Names::defaultEquipmentKey, -1) == this->obsEquip->key()) {
///      checkBox_defaultEquipment->setCheckState(Qt::Checked);
///   } else {
///      checkBox_defaultEquipment->setCheckState(Qt::Unchecked);
///   }
///   checkBox_defaultEquipment->blockSignals(false);

   return;
}

///void EquipmentEditor::updateCheckboxRecord() {
///   if (Qt::Checked == this->checkBox_calcBoilVolume->checkState()) {
///      this->lineEdit_kettleBoilSize->setAmount(this->calcBatchSize());
///      this->lineEdit_kettleBoilSize->setEnabled(false);
///   } else {
///      this->lineEdit_kettleBoilSize->setAmount(this->lineEdit_fermenterBatchSize->toCanonical().quantity());
///      this->lineEdit_kettleBoilSize->setEnabled(true);
///   }
///   return;
///}
///
///double EquipmentEditor::calcBatchSize() {
///   double size     = lineEdit_fermenterBatchSize      ->toCanonical().quantity();
///   double topUp    = lineEdit_topUpWater     ->toCanonical().quantity();
///   double trubLoss = lineEdit_kettleTrubChillerLoss->toCanonical().quantity();
///   double evapRate = lineEdit_kettleEvaporationPerHour->toCanonical().quantity();
///   double time     = lineEdit_boilTime       ->toCanonical().quantity();
///
///   return size - topUp + trubLoss + (time/60.0)*evapRate;
///}
///
///void EquipmentEditor::updateDefaultEquipment(int state) {
///   QVariant currentDefault = PersistentSettings::value(PersistentSettings::Names::defaultEquipmentKey, -1);
///   if ( state == Qt::Checked ) {
///      PersistentSettings::insert(PersistentSettings::Names::defaultEquipmentKey, obsEquip->key());
///   } else if ( currentDefault == obsEquip->key() ) {
///      PersistentSettings::insert(PersistentSettings::Names::defaultEquipmentKey, -1);
///   }
///   return;
///}
///
///void EquipmentEditor::closeEvent(QCloseEvent *event) {
///   cancel();
///   event->accept();
///   return;
///}

// Insert the boiler-plate stuff that we cannot do in EditorBase
EDITOR_COMMON_SLOT_DEFINITIONS(EquipmentEditor)
