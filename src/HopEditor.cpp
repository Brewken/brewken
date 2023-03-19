/*======================================================================================================================
 * HopEditor.cpp is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
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
#include "HopEditor.h"

#include <QtGui>
#include <QIcon>
#include <QInputDialog>

#include "BtHorizontalTabs.h"
#include "config.h"
#include "database/ObjectStoreWrapper.h"
#include "measurement/Unit.h"
#include "model/Hop.h"

HopEditor::HopEditor(QWidget * parent) :
   QDialog(parent),
   obsHop(nullptr) {
   setupUi(this);

   this->tabWidget_editor->tabBar()->setStyle(new BtHorizontalTabs);

   //
   // According to https://bugreports.qt.io/browse/QTBUG-50823 it is never going to be possible to specify the data (as
   // opposed to display text) for a combo box via the .ui file.  So we have to do it in code instead.
   // We could use the raw enum values as the data, but it would be a bit painful to debug if we ever had to, so for
   // small extra effort we use the same serialisation strings that we use for BeerJSON and the DB.
   //
   for (auto ii : Hop::allTypes) {
      this->comboBox_hopType->addItem(Hop::typeDisplayNames[ii], Hop::typeStringMapping.enumToString(ii));
   }
   for (auto ii : Hop::allForms) {
      this->comboBox_hopForm->addItem(Hop::formDisplayNames[ii], Hop::formStringMapping.enumToString(ii));
   }
   for (auto ii : Hop::allUses) {
      this->comboBox_hopUse->addItem (Hop::useDisplayNames[ii],  Hop::useStringMapping.enumToString(ii));
   }

   connect(pushButton_new,    &QAbstractButton::clicked, this, &HopEditor::clickedNewHop);
   connect(pushButton_save,   &QAbstractButton::clicked, this, &HopEditor::save);
   connect(pushButton_cancel, &QAbstractButton::clicked, this, &HopEditor::clearAndClose);

   return;
}

HopEditor::~HopEditor() = default;

void HopEditor::setHop(Hop * h) {
   if (obsHop) {
      disconnect(obsHop, nullptr, this, nullptr);
   }

   obsHop = h;
   if (obsHop) {
      connect(obsHop, &NamedEntity::changed, this, &HopEditor::changed);
      showChanges();
   }
   return;
}

void HopEditor::save() {
   if (!this->obsHop) {
      setVisible(false);
      return;
   }

   this->obsHop->setName             (lineEdit_name         ->text());
   this->obsHop->setAlpha_pct        (lineEdit_alpha        ->getValueAs<double>());
   this->obsHop->setTime_min         (lineEdit_time         ->toCanonical().quantity());
   this->obsHop->setBeta_pct         (lineEdit_beta         ->getValueAs<double>());
   this->obsHop->setHsi_pct          (lineEdit_HSI          ->getValueAs<double>());
   this->obsHop->setOrigin           (lineEdit_origin       ->text()           );
   this->obsHop->setHumulene_pct     (lineEdit_humulene     ->getValueAs<double>());
   this->obsHop->setCaryophyllene_pct(lineEdit_caryophyllene->getValueAs<double>());
   this->obsHop->setCohumulone_pct   (lineEdit_cohumulone   ->getValueAs<double>());
   this->obsHop->setMyrcene_pct      (lineEdit_myrcene      ->getValueAs<double>());
   this->obsHop->setSubstitutes      (textEdit_substitutes  ->toPlainText()    );
   this->obsHop->setNotes            (textEdit_notes        ->toPlainText()    );

   //
   // It's a coding error if we don't recognise the values in our own combo boxes, so it's OK that we'd get a
   // std::bad_optional_access exception in such a case
   //
   this->obsHop->setType(Hop::typeStringMapping.stringToEnum<Hop::Type>(comboBox_hopType->currentData().toString()));
   this->obsHop->setForm(Hop::formStringMapping.stringToEnum<Hop::Form>(comboBox_hopForm->currentData().toString()));
   this->obsHop->setUse (Hop::useStringMapping.stringToEnum<Hop::Use>  (comboBox_hopUse->currentData().toString()));

   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞


   if (this->obsHop->key() < 0) {
      ObjectStoreWrapper::insert(*this->obsHop);
   }

   // do this late to make sure we've the row in the inventory table
   this->obsHop->setInventoryAmount(lineEdit_inventory->toCanonical().quantity());
   setVisible(false);
   return;
}

void HopEditor::clearAndClose() {
   setHop(nullptr);
   setVisible(false); // Hide the window.
}

void HopEditor::changed(QMetaProperty prop, QVariant /*val*/) {
   if (sender() == obsHop) {
      showChanges(&prop);
   }
}

void HopEditor::showChanges(QMetaProperty * prop) {
   if (!this->obsHop) {
      return;
   }

   bool updateAll = false;
   QString propName = "";

   if (prop == nullptr) {
      updateAll = true;
   } else {
      propName = prop->name();
   }

   if (updateAll || propName == PropertyNames::Hop::use) {
      // As above, it's a coding error if there isn't a combo box entry corresponding to the Hop type
      comboBox_hopUse->setCurrentIndex(
         comboBox_hopUse->findData(Hop::useStringMapping.enumToString(obsHop->use()))
      );
      if (!updateAll) {
         return;
      }
   }
   if (updateAll || propName == PropertyNames::Hop::type) {
      // As above, it's a coding error if there isn't a combo box entry corresponding to the Hop type
      comboBox_hopType->setCurrentIndex(
         comboBox_hopType->findData(Hop::typeStringMapping.enumToString(obsHop->type()))
      );
      if (!updateAll) {
         return;
      }
   }
   if (updateAll || propName == PropertyNames::Hop::form) {
      // As above, it's a coding error if there isn't a combo box entry corresponding to the Hop form
      comboBox_hopForm->setCurrentIndex(
         comboBox_hopForm->findData(Hop::formStringMapping.enumToString(obsHop->form()))
      );
      if (!updateAll) {
         return;
      }
   }
   if (updateAll || propName == PropertyNames::NamedEntity::name                  ) { lineEdit_name                ->setText(obsHop->name                 ()); // Continues to next line
                                                                                      lineEdit_name  ->setCursorPosition(0); tabWidget_editor->setTabText(0, obsHop->name());                         if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Hop::origin                        ) { lineEdit_origin              ->setText(obsHop->origin               ()); lineEdit_origin->setCursorPosition(0); if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Hop::alpha_pct                     ) { lineEdit_alpha               ->setText(obsHop->alpha_pct            ());                                        if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Hop::time_min                      ) { lineEdit_time                ->setText(obsHop->time_min             ());                                        if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Hop::beta_pct                      ) { lineEdit_beta                ->setText(obsHop->beta_pct             ());                                        if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Hop::hsi_pct                       ) { lineEdit_HSI                 ->setText(obsHop->hsi_pct              ());                                        if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Hop::humulene_pct                  ) { lineEdit_humulene            ->setText(obsHop->humulene_pct         ());                                        if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Hop::caryophyllene_pct             ) { lineEdit_caryophyllene       ->setText(obsHop->caryophyllene_pct    ());                                        if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Hop::cohumulone_pct                ) { lineEdit_cohumulone          ->setText(obsHop->cohumulone_pct       ());                                        if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Hop::myrcene_pct                   ) { lineEdit_myrcene             ->setText(obsHop->myrcene_pct          ());                                        if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Hop::substitutes                   ) { textEdit_substitutes         ->setPlainText(obsHop->substitutes     ());                                        if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Hop::notes                         ) { textEdit_notes               ->setPlainText(obsHop->notes           ());                                        if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::NamedEntityWithInventory::inventory) { lineEdit_inventory           ->setText(obsHop->inventory            ());                                        if (!updateAll) { return; } }
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   if (updateAll || propName == PropertyNames::Hop::producer                      ) {lineEdit_producer             ->setText(obsHop->producer             ());                                        if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Hop::product_id                    ) {lineEdit_product_id           ->setText(obsHop->product_id           ());                                        if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Hop::year                          ) {lineEdit_year                 ->setText(obsHop->year                 ());                                        if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Hop::total_oil_ml_per_100g         ) {lineEdit_total_oil_ml_per_100g->setText(obsHop->total_oil_ml_per_100g());                                        if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Hop::farnesene_pct                 ) {lineEdit_farnesene            ->setText(obsHop->farnesene_pct        ());                                        if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Hop::geraniol_pct                  ) {lineEdit_geraniol             ->setText(obsHop->geraniol_pct         ());                                        if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Hop::b_pinene_pct                  ) {lineEdit_b_pinene             ->setText(obsHop->b_pinene_pct         ());                                        if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Hop::linalool_pct                  ) {lineEdit_linalool             ->setText(obsHop->linalool_pct         ());                                        if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Hop::limonene_pct                  ) {lineEdit_limonene             ->setText(obsHop->limonene_pct         ());                                        if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Hop::nerol_pct                     ) {lineEdit_nerol                ->setText(obsHop->nerol_pct            ());                                        if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Hop::pinene_pct                    ) {lineEdit_pinene               ->setText(obsHop->pinene_pct           ());                                        if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Hop::polyphenols_pct               ) {lineEdit_polyphenols          ->setText(obsHop->polyphenols_pct      ());                                        if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Hop::xanthohumol_pct               ) {lineEdit_xanthohumol          ->setText(obsHop->xanthohumol_pct      ());                                        if (!updateAll) { return; } }

   return;
}


void HopEditor::newHop(QString folder) {
   QString name = QInputDialog::getText(this, tr("Hop name"), tr("Hop name:"));
   if (name.isEmpty()) {
      return;
   }

   // .:TODO:. Change this to shared_ptr as currently results in memory leak in clearAndClose()
   Hop * h = new Hop(name);

   if (! folder.isEmpty()) {
      h->setFolder(folder);
   }

   setHop(h);
   show();
   return;
}

void HopEditor::clickedNewHop() {
   newHop(QString());
   return;
}
