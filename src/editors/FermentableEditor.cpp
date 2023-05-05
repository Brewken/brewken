/*======================================================================================================================
 * editors/FermentableEditor.cpp is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Daniel Pettersson <pettson81@gmail.com>
 *   • Kregg Kemper <gigatropolis@yahoo.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Samuel Östling <MrOstling@gmail.com>
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
#include "editors/FermentableEditor.h"

#include <QIcon>
#include <QInputDialog>

#include "BtHorizontalTabs.h"
#include "database/ObjectStoreWrapper.h"
#include "measurement/Unit.h"
#include "model/Fermentable.h"

FermentableEditor::FermentableEditor(QWidget* parent) :
   QDialog(parent),
   obsFerm(nullptr) {
   setupUi(this);

   this->tabWidget_editor->tabBar()->setStyle(new BtHorizontalTabs);

   // See comment in editors/HopEditor.cpp about combo box setup in Qt
   for (auto ii : Fermentable::allTypes) {
      this->comboBox_fermentableType->addItem(Fermentable::typeDisplayNames[ii],
                                              Fermentable::typeStringMapping.enumToString(ii));
   }

   SMART_FIELD_INIT(FermentableEditor, label_name          , lineEdit_name          , Fermentable, PropertyNames::NamedEntity::name                     );
   SMART_FIELD_INIT(FermentableEditor, label_color         , lineEdit_color         , Fermentable, PropertyNames::Fermentable::color_srm             , 0);
   SMART_FIELD_INIT(FermentableEditor, label_diastaticPower, lineEdit_diastaticPower, Fermentable, PropertyNames::Fermentable::diastaticPower_lintner   );
   SMART_FIELD_INIT(FermentableEditor, label_coarseFineDiff, lineEdit_coarseFineDiff, Fermentable, PropertyNames::Fermentable::coarseFineDiff_pct    , 0);
   SMART_FIELD_INIT(FermentableEditor, label_ibuGalPerLb   , lineEdit_ibuGalPerLb   , Fermentable, PropertyNames::Fermentable::ibuGalPerLb           , 0);
   SMART_FIELD_INIT(FermentableEditor, label_maxInBatch    , lineEdit_maxInBatch    , Fermentable, PropertyNames::Fermentable::maxInBatch_pct        , 0);
   SMART_FIELD_INIT(FermentableEditor, label_moisture      , lineEdit_moisture      , Fermentable, PropertyNames::Fermentable::moisture_pct          , 0);
   SMART_FIELD_INIT(FermentableEditor, label_protein       , lineEdit_protein       , Fermentable, PropertyNames::Fermentable::protein_pct           , 0);
   SMART_FIELD_INIT(FermentableEditor, label_yield         , lineEdit_yield         , Fermentable, PropertyNames::Fermentable::yield_pct             , 1);
   SMART_FIELD_INIT(FermentableEditor, label_inventory     , lineEdit_inventory     , Fermentable, PropertyNames::Fermentable::amount                   );
   SMART_FIELD_INIT(FermentableEditor, label_origin        , lineEdit_origin        , Fermentable, PropertyNames::Fermentable::origin                   );
   SMART_FIELD_INIT(FermentableEditor, label_supplier      , lineEdit_supplier      , Fermentable, PropertyNames::Fermentable::supplier                 );

   connect(this->pushButton_new,    &QAbstractButton::clicked, this, &FermentableEditor::clickedNewFermentable);
   connect(this->pushButton_save,   &QAbstractButton::clicked, this, &FermentableEditor::save                 );
   connect(this->pushButton_cancel, &QAbstractButton::clicked, this, &FermentableEditor::clearAndClose        );
   connect(this->checkBox_isWeight, &QCheckBox::toggled,       this, &FermentableEditor::setIsWeight          ); // ⮜⮜⮜ Added for BeerJSON support ⮞⮞⮞
   return;
}

FermentableEditor::~FermentableEditor() = default;

void FermentableEditor::setFermentable(Fermentable * newFerm) {
   if (newFerm) {
      obsFerm = newFerm;
      showChanges();
   }
   return;
}

void FermentableEditor::save() {
   if (!obsFerm) {
      setVisible(false);
      return;
   }

   obsFerm->setName(lineEdit_name->text());

   //
   // It's a coding error if we don't recognise the values in our own combo boxes, so it's OK that we'd get a
   // std::bad_optional_access exception in such a case
   //
   this->obsFerm->setType(
      Fermentable::typeStringMapping.stringToEnum<Fermentable::Type>(comboBox_fermentableType->currentData().toString())
   );

   this->obsFerm->setYield_pct             (this->lineEdit_yield         ->getNonOptValueAs<double>());
   this->obsFerm->setColor_srm             (this->lineEdit_color         ->toCanonical().quantity());
   this->obsFerm->setAddAfterBoil          (this->checkBox_addAfterBoil  ->checkState() == Qt::Checked);
   this->obsFerm->setOrigin                (this->lineEdit_origin        ->text());
   this->obsFerm->setSupplier              (this->lineEdit_supplier      ->text());
   this->obsFerm->setCoarseFineDiff_pct    (this->lineEdit_coarseFineDiff->getNonOptValueAs<double>());
   this->obsFerm->setMoisture_pct          (this->lineEdit_moisture      ->getNonOptValueAs<double>());
   this->obsFerm->setDiastaticPower_lintner(this->lineEdit_diastaticPower->toCanonical().quantity());
   this->obsFerm->setProtein_pct           (this->lineEdit_protein       ->getNonOptValueAs<double>());
   this->obsFerm->setMaxInBatch_pct        (this->lineEdit_maxInBatch    ->getNonOptValueAs<double>());
   this->obsFerm->setRecommendMash         (this->checkBox_recommendMash ->checkState() == Qt::Checked);
   this->obsFerm->setIsMashed              (this->checkBox_isMashed      ->checkState() == Qt::Checked);
   this->obsFerm->setIbuGalPerLb           (this->lineEdit_ibuGalPerLb   ->getNonOptValueAs<double>()); // .:TBD:. No metric measure?
   this->obsFerm->setNotes                 (this->textEdit_notes         ->toPlainText());
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   this->obsFerm->setAmountIsWeight        (this->checkBox_isWeight->checkState() == Qt::Checked);

   if (this->obsFerm->key() < 0) {
      ObjectStoreWrapper::insert(*this->obsFerm);
   }

   // Since inventory amount isn't really an attribute of the Fermentable, it's best to store it after we know the
   // Fermentable has a DB record.
   this->obsFerm->setInventoryAmount(lineEdit_inventory->toCanonical().quantity());

   setVisible(false);
   return;
}

void FermentableEditor::clearAndClose() {
   setFermentable(nullptr);
   setVisible(false); // Hide the window.
   return;
}

void FermentableEditor::showChanges(QMetaProperty* metaProp) {
   if (!this->obsFerm) {
      return;
   }

   QString propName;
   bool updateAll = false;
   if (metaProp == nullptr) {
      updateAll = true;
   } else {
      propName = metaProp->name();
   }

   if (propName == PropertyNames::Fermentable::type || updateAll) {
      // As above, it's a coding error if there isn't a combo box entry corresponding to the Fermentable type
      comboBox_fermentableType->setCurrentIndex(
         comboBox_fermentableType->findData(Fermentable::typeStringMapping.enumToString(obsFerm->type()))
      );
      if (!updateAll) {
         return;
      }
   }
   if (updateAll || propName == PropertyNames::NamedEntity::name                  ) { this->lineEdit_name          ->setText      (obsFerm->name()                  ); // Continues to next line
                                                                                      this->lineEdit_name->setCursorPosition(0); tabWidget_editor->setTabText(0, obsFerm->name());                              if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::NamedEntityWithInventory::inventory) { this->lineEdit_inventory     ->setAmount    (obsFerm->inventory()             );                                          if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::yield_pct             ) { this->lineEdit_yield         ->setAmount    (obsFerm->yield_pct()             );                                          if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::color_srm             ) { this->lineEdit_color         ->setAmount    (obsFerm->color_srm()             );                                          if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::addAfterBoil          ) { this->checkBox_addAfterBoil  ->setCheckState(obsFerm->addAfterBoil() ? Qt::Checked : Qt::Unchecked);                      if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::origin                ) { this->lineEdit_origin        ->setText      (obsFerm->origin()                ); lineEdit_origin  ->setCursorPosition(0); if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::supplier              ) { this->lineEdit_supplier      ->setText      (obsFerm->supplier()              ); lineEdit_supplier->setCursorPosition(0); if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::coarseFineDiff_pct    ) { this->lineEdit_coarseFineDiff->setAmount    (obsFerm->coarseFineDiff_pct()    );                                          if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::moisture_pct          ) { this->lineEdit_moisture      ->setAmount    (obsFerm->moisture_pct()          );                                          if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::diastaticPower_lintner) { this->lineEdit_diastaticPower->setAmount    (obsFerm->diastaticPower_lintner());                                          if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::protein_pct           ) { this->lineEdit_protein       ->setAmount    (obsFerm->protein_pct()           );                                          if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::maxInBatch_pct        ) { this->lineEdit_maxInBatch    ->setAmount    (obsFerm->maxInBatch_pct()        );                                          if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::recommendMash         ) { this->checkBox_recommendMash ->setCheckState(obsFerm->recommendMash() ? Qt::Checked : Qt::Unchecked);                     if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::isMashed              ) { this->checkBox_isMashed      ->setCheckState(obsFerm->isMashed()      ? Qt::Checked : Qt::Unchecked);                     if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::ibuGalPerLb           ) { this->lineEdit_ibuGalPerLb   ->setAmount    (obsFerm->ibuGalPerLb()           );                                          if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::notes                 ) { this->textEdit_notes         ->setPlainText (obsFerm->notes()                 );                                          if (!updateAll) { return; } }
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   if (updateAll || propName == PropertyNames::Fermentable::amountIsWeight        ) { this->checkBox_isWeight      ->setCheckState  (obsFerm->amountIsWeight() ? // Continues to next line
                                                                                                                                Qt::Checked : Qt::Unchecked            ); if (!updateAll) { return; } }

   return;
}

void FermentableEditor::newFermentable(QString folder)  {
   QString name = QInputDialog::getText(this, tr("Fermentable name"), tr("Fermentable name:"));
   if (name.isEmpty()) {
      return;
   }

   // .:TODO:. Change to shared_ptr as currently leads to memory leak in clearAndClose()
   Fermentable * f = new Fermentable(name);

   if (! folder.isEmpty()) {
      f->setFolder(folder);
   }

   setFermentable(f);
   show();
   return;
}

void FermentableEditor::clickedNewFermentable() {
   newFermentable(QString());
   return;
}

void FermentableEditor::setIsWeight(bool const state) {
   qDebug() << Q_FUNC_INFO << "state is" << state;
   // But you have to admit, this is clever
   this->lineEdit_inventory->selectPhysicalQuantity(
      state ? Measurement::PhysicalQuantity::Mass : Measurement::PhysicalQuantity::Volume
   );

   // maybe? My head hurts now
   this->lineEdit_inventory->onLineChanged();

   // Strictly, if we change a Fermentable to be measured by mass instead of volume (or vice versa) we should also somehow tell
   // any other bit of the UI that is showing that Fermentable (eg a RecipeEditor or MainWindow) to redisplay the relevant
   // field.  Currently we don't do this, on the assumption that it's rare you will change how a Fermentable is measured after
   // you started using it in recipes.
   return;
}
