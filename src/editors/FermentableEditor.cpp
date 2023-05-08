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
   EditorBase<Fermentable, FermentableEditor>() {
   setupUi(this);

   this->tabWidget_editor->tabBar()->setStyle(new BtHorizontalTabs);

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

   BT_COMBO_BOX_INIT(FermentableEditor, comboBox_type      , Fermentable, type      );
   BT_COMBO_BOX_INIT(FermentableEditor, comboBox_grainGroup, Fermentable, grainGroup);

   connect(this->pushButton_new,    &QAbstractButton::clicked, this, &FermentableEditor::clickedNew   );
   connect(this->pushButton_save,   &QAbstractButton::clicked, this, &FermentableEditor::save         );
   connect(this->pushButton_cancel, &QAbstractButton::clicked, this, &FermentableEditor::clearAndClose);
   connect(this->checkBox_isWeight, &QCheckBox::toggled,       this, &FermentableEditor::setIsWeight  ); // ⮜⮜⮜ Added for BeerJSON support ⮞⮞⮞
   return;
}

FermentableEditor::~FermentableEditor() = default;

void FermentableEditor::writeFieldsToEditItem() {

   this->m_editItem->setType(this->comboBox_type      ->getNonOptValue<Fermentable::Type      >());

   this->m_editItem->setName                  (this->lineEdit_name          ->text                    ());
   this->m_editItem->setYield_pct             (this->lineEdit_yield         ->getNonOptValueAs<double>());
   this->m_editItem->setColor_srm             (this->lineEdit_color         ->toCanonical().quantity  ());
   this->m_editItem->setAddAfterBoil          (this->checkBox_addAfterBoil  ->checkState() == Qt::Checked);
   this->m_editItem->setOrigin                (this->lineEdit_origin        ->text                    ());
   this->m_editItem->setSupplier              (this->lineEdit_supplier      ->text                    ());
   this->m_editItem->setCoarseFineDiff_pct    (this->lineEdit_coarseFineDiff->getNonOptValueAs<double>());
   this->m_editItem->setMoisture_pct          (this->lineEdit_moisture      ->getNonOptValueAs<double>());
   this->m_editItem->setDiastaticPower_lintner(this->lineEdit_diastaticPower->toCanonical().quantity  ());
   this->m_editItem->setProtein_pct           (this->lineEdit_protein       ->getNonOptValueAs<double>());
   this->m_editItem->setMaxInBatch_pct        (this->lineEdit_maxInBatch    ->getNonOptValueAs<double>());
   this->m_editItem->setRecommendMash         (this->checkBox_recommendMash ->checkState() == Qt::Checked);
   this->m_editItem->setIsMashed              (this->checkBox_isMashed      ->checkState() == Qt::Checked);
   this->m_editItem->setIbuGalPerLb           (this->lineEdit_ibuGalPerLb   ->getNonOptValueAs<double>()); // .:TBD:. No metric measure?
   this->m_editItem->setNotes                 (this->textEdit_notes         ->toPlainText             ());
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   this->m_editItem->setAmountIsWeight        (this->checkBox_isWeight      ->checkState() == Qt::Checked);
   this->m_editItem->setGrainGroup            (this->comboBox_grainGroup->getOptValue   <Fermentable::GrainGroup>());

   return;
}

void FermentableEditor::writeLateFieldsToEditItem() {
   // Since inventory amount isn't really an attribute of the Fermentable, it's best to store it after we know the
   // Fermentable has a DB record.
   this->m_editItem->setInventoryAmount(lineEdit_inventory->toCanonical().quantity());
   return;
}

void FermentableEditor::readFieldsFromEditItem(std::optional<QString> propName) {
   if (!propName || *propName == PropertyNames::Fermentable::type                  ) { this->comboBox_type          ->setValue     (m_editItem->type                 ()); if (propName) { return; } }

   if (!propName || *propName == PropertyNames::NamedEntity::name                  ) { this->lineEdit_name          ->setText      (m_editItem->name()                  ); // Continues to next line
                                                                                      this->lineEdit_name->setCursorPosition(0); tabWidget_editor->setTabText(0, m_editItem->name());                               if (propName) { return; } }
   if (!propName || *propName == PropertyNames::NamedEntityWithInventory::inventory) { this->lineEdit_inventory     ->setAmount    (m_editItem->inventory()             );                                          if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::yield_pct             ) { this->lineEdit_yield         ->setAmount    (m_editItem->yield_pct()             );                                          if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::color_srm             ) { this->lineEdit_color         ->setAmount    (m_editItem->color_srm()             );                                          if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::addAfterBoil          ) { this->checkBox_addAfterBoil  ->setCheckState(m_editItem->addAfterBoil() ? Qt::Checked : Qt::Unchecked);                      if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::origin                ) { this->lineEdit_origin        ->setText      (m_editItem->origin()                ); lineEdit_origin  ->setCursorPosition(0); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::supplier              ) { this->lineEdit_supplier      ->setText      (m_editItem->supplier()              ); lineEdit_supplier->setCursorPosition(0); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::coarseFineDiff_pct    ) { this->lineEdit_coarseFineDiff->setAmount    (m_editItem->coarseFineDiff_pct()    );                                          if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::moisture_pct          ) { this->lineEdit_moisture      ->setAmount    (m_editItem->moisture_pct()          );                                          if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::diastaticPower_lintner) { this->lineEdit_diastaticPower->setAmount    (m_editItem->diastaticPower_lintner());                                          if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::protein_pct           ) { this->lineEdit_protein       ->setAmount    (m_editItem->protein_pct()           );                                          if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::maxInBatch_pct        ) { this->lineEdit_maxInBatch    ->setAmount    (m_editItem->maxInBatch_pct()        );                                          if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::recommendMash         ) { this->checkBox_recommendMash ->setCheckState(m_editItem->recommendMash() ? Qt::Checked : Qt::Unchecked);                     if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::isMashed              ) { this->checkBox_isMashed      ->setCheckState(m_editItem->isMashed()      ? Qt::Checked : Qt::Unchecked);                     if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::ibuGalPerLb           ) { this->lineEdit_ibuGalPerLb   ->setAmount    (m_editItem->ibuGalPerLb()           );                                          if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::notes                 ) { this->textEdit_notes         ->setPlainText (m_editItem->notes()                 );                                          if (propName) { return; } }
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   if (!propName || *propName == PropertyNames::Fermentable::amountIsWeight        ) { this->checkBox_isWeight      ->setCheckState(m_editItem->amountIsWeight() ? Qt::Checked : Qt::Unchecked);                    if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::grainGroup            ) { this->comboBox_grainGroup    ->setValue     (m_editItem->grainGroup            ()); if (propName) { return; } }
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

// Insert the boiler-plate stuff that we cannot do in EditorBase
EDITOR_COMMON_SLOT_DEFINITIONS(FermentableEditor)
