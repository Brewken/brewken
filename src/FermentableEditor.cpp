/*======================================================================================================================
 * FermentableEditor.cpp is part of Brewken, and is copyright the following authors 2009-2023:
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
#include "FermentableEditor.h"

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

   // See comment in HopEditor.cpp about combo box setup in Qt
   for (auto ii : Fermentable::allTypes) {
      this->comboBox_fermentableType->addItem(Fermentable::typeDisplayNames[ii],
                                              Fermentable::typeStringMapping.enumToString(ii));
   }

   SMART_LINE_EDIT_INIT(FermentableEditor, Fermentable, lineEdit_name          , PropertyNames::NamedEntity::name                                                  );
   SMART_LINE_EDIT_INIT(FermentableEditor, Fermentable, lineEdit_color         , PropertyNames::Fermentable::color_srm             , *this->label_color         , 0);
   SMART_LINE_EDIT_INIT(FermentableEditor, Fermentable, lineEdit_diastaticPower, PropertyNames::Fermentable::diastaticPower_lintner, *this->label_diastaticPower   );
   SMART_LINE_EDIT_INIT(FermentableEditor, Fermentable, lineEdit_coarseFineDiff, PropertyNames::Fermentable::coarseFineDiff_pct                                 , 0);
   SMART_LINE_EDIT_INIT(FermentableEditor, Fermentable, lineEdit_ibuGalPerLb   , PropertyNames::Fermentable::ibuGalPerLb                                        , 0);
   SMART_LINE_EDIT_INIT(FermentableEditor, Fermentable, lineEdit_maxInBatch    , PropertyNames::Fermentable::maxInBatch_pct                                     , 0);
   SMART_LINE_EDIT_INIT(FermentableEditor, Fermentable, lineEdit_moisture      , PropertyNames::Fermentable::moisture_pct                                       , 0);
   SMART_LINE_EDIT_INIT(FermentableEditor, Fermentable, lineEdit_protein       , PropertyNames::Fermentable::protein_pct                                        , 0);
   SMART_LINE_EDIT_INIT(FermentableEditor, Fermentable, lineEdit_yield         , PropertyNames::Fermentable::yield_pct                                          , 1);
   SMART_LINE_EDIT_INIT(FermentableEditor, Fermentable, lineEdit_inventory     , PropertyNames::Fermentable::amount                , *this->label_inventory        );
   SMART_LINE_EDIT_INIT(FermentableEditor, Fermentable, lineEdit_origin        , PropertyNames::Fermentable::origin                                                );
   SMART_LINE_EDIT_INIT(FermentableEditor, Fermentable, lineEdit_supplier      , PropertyNames::Fermentable::supplier                                              );

   connect(pushButton_new,    &QAbstractButton::clicked, this, &FermentableEditor::clickedNewFermentable);
   connect(pushButton_save,   &QAbstractButton::clicked, this, &FermentableEditor::save);
   connect(pushButton_cancel, &QAbstractButton::clicked, this, &FermentableEditor::clearAndClose);
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

   obsFerm->setYield_pct             (lineEdit_yield         ->getValueAs<double>());
   obsFerm->setColor_srm             (lineEdit_color         ->toCanonical().quantity());
   obsFerm->setAddAfterBoil          (checkBox_addAfterBoil  ->checkState() == Qt::Checked);
   obsFerm->setOrigin                (lineEdit_origin        ->text());
   obsFerm->setSupplier              (lineEdit_supplier      ->text());
   obsFerm->setCoarseFineDiff_pct    (lineEdit_coarseFineDiff->getValueAs<double>());
   obsFerm->setMoisture_pct          (lineEdit_moisture      ->getValueAs<double>());
   obsFerm->setDiastaticPower_lintner(lineEdit_diastaticPower->toCanonical().quantity());
   obsFerm->setProtein_pct           (lineEdit_protein       ->getValueAs<double>());
   obsFerm->setMaxInBatch_pct        (lineEdit_maxInBatch    ->getValueAs<double>());
   obsFerm->setRecommendMash         (checkBox_recommendMash ->checkState() == Qt::Checked);
   obsFerm->setIsMashed              (checkBox_isMashed      ->checkState() == Qt::Checked);
   obsFerm->setIbuGalPerLb           (lineEdit_ibuGalPerLb   ->getValueAs<double>()); // .:TBD:. No metric measure?
   obsFerm->setNotes                 (textEdit_notes         ->toPlainText());

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
      // As above, it's a coding error if there isn't a combo box entry corresponding to the Hop type
      comboBox_fermentableType->setCurrentIndex(
         comboBox_fermentableType->findData(Fermentable::typeStringMapping.enumToString(obsFerm->type()))
      );
      if (!updateAll) {
         return;
      }
   }
   if (updateAll || propName == PropertyNames::NamedEntity::name                  ) { lineEdit_name          ->setText      (obsFerm->name()                  ); // Continues to next line
                                                                                      lineEdit_name->setCursorPosition(0); tabWidget_editor->setTabText(0, obsFerm->name());                              if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::NamedEntityWithInventory::inventory) { lineEdit_inventory     ->setAmount    (obsFerm->inventory()             );                                          if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::yield_pct             ) { lineEdit_yield         ->setAmount    (obsFerm->yield_pct()             );                                          if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::color_srm             ) { lineEdit_color         ->setAmount    (obsFerm->color_srm()             );                                          if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::addAfterBoil          ) { checkBox_addAfterBoil  ->setCheckState(obsFerm->addAfterBoil() ? Qt::Checked : Qt::Unchecked);                      if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::origin                ) { lineEdit_origin        ->setText      (obsFerm->origin()                ); lineEdit_origin  ->setCursorPosition(0); if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::supplier              ) { lineEdit_supplier      ->setText      (obsFerm->supplier()              ); lineEdit_supplier->setCursorPosition(0); if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::coarseFineDiff_pct    ) { lineEdit_coarseFineDiff->setAmount    (obsFerm->coarseFineDiff_pct()    );                                          if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::moisture_pct          ) { lineEdit_moisture      ->setAmount    (obsFerm->moisture_pct()          );                                          if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::diastaticPower_lintner) { lineEdit_diastaticPower->setAmount    (obsFerm->diastaticPower_lintner());                                          if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::protein_pct           ) { lineEdit_protein       ->setAmount    (obsFerm->protein_pct()           );                                          if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::maxInBatch_pct        ) { lineEdit_maxInBatch    ->setAmount    (obsFerm->maxInBatch_pct()        );                                          if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::recommendMash         ) { checkBox_recommendMash ->setCheckState(obsFerm->recommendMash() ? Qt::Checked : Qt::Unchecked);                     if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::isMashed              ) { checkBox_isMashed      ->setCheckState(obsFerm->isMashed()      ? Qt::Checked : Qt::Unchecked);                     if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::ibuGalPerLb           ) { lineEdit_ibuGalPerLb   ->setAmount    (obsFerm->ibuGalPerLb()           );                                          if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Fermentable::notes                 ) { textEdit_notes         ->setPlainText (obsFerm->notes()                 );                                          if (!updateAll) { return; } }
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
