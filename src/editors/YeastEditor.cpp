/*======================================================================================================================
 * editors/YeastEditor.cpp is part of Brewken, and is copyright the following authors 2009-2023:
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
#include "editors/YeastEditor.h"

#include <cmath>

#include <QInputDialog>

#include "BtHorizontalTabs.h"
#include "config.h"
#include "database/ObjectStoreWrapper.h"
#include "measurement/Unit.h"
#include "model/Yeast.h"

YeastEditor::YeastEditor(QWidget * parent) :
   QDialog(parent),
   EditorBase<Yeast, YeastEditor>() {
   setupUi(this);

   this->tabWidget_editor->tabBar()->setStyle(new BtHorizontalTabs);

   SMART_FIELD_INIT(YeastEditor, label_name          , lineEdit_name          , Yeast, PropertyNames::NamedEntity::name         );
   SMART_FIELD_INIT(YeastEditor, label_laboratory    , lineEdit_laboratory    , Yeast, PropertyNames::Yeast::laboratory         );
   SMART_FIELD_INIT(YeastEditor, label_inventory     , lineEdit_inventory     , Yeast, PropertyNames::Yeast::amount          , 0);
   SMART_FIELD_INIT(YeastEditor, label_productID     , lineEdit_productID     , Yeast, PropertyNames::Yeast::productID          );
   SMART_FIELD_INIT(YeastEditor, label_minTemperature, lineEdit_minTemperature, Yeast, PropertyNames::Yeast::minTemperature_c, 1);
   SMART_FIELD_INIT(YeastEditor, label_attenuation   , lineEdit_attenuation   , Yeast, PropertyNames::Yeast::attenuation_pct , 0);
   SMART_FIELD_INIT(YeastEditor, label_maxTemperature, lineEdit_maxTemperature, Yeast, PropertyNames::Yeast::maxTemperature_c, 1);
   SMART_FIELD_INIT(YeastEditor, label_timesCultured , lineEdit_timesCultured , Yeast, PropertyNames::Yeast::timesCultured   , 0);
   SMART_FIELD_INIT(YeastEditor, label_maxReuse      , lineEdit_maxReuse      , Yeast, PropertyNames::Yeast::maxReuse        , 0);

   this->connectSignalsAndSlots();
   return;
}

YeastEditor::~YeastEditor() = default;

void YeastEditor::writeFieldsToEditItem() {

   this->m_editItem->setName            (lineEdit_name          ->text()                                        );
   this->m_editItem->setType            (static_cast<Yeast::Type>(comboBox_type->currentIndex())                );
   this->m_editItem->setForm            (static_cast<Yeast::Form>(comboBox_form->currentIndex())                );
   this->m_editItem->setAmountIsWeight  (checkBox_amountIsWeight->checkState() == Qt::Checked                   );
   this->m_editItem->setLaboratory      (lineEdit_laboratory    ->text()                                        );
   this->m_editItem->setProductID       (lineEdit_productID     ->text()                                        );
   this->m_editItem->setMinTemperature_c(lineEdit_minTemperature->toCanonical().quantity()                      );
   this->m_editItem->setMaxTemperature_c(lineEdit_maxTemperature->toCanonical().quantity()                      );
   this->m_editItem->setFlocculation    (static_cast<Yeast::Flocculation>(comboBox_flocculation->currentIndex()));
   this->m_editItem->setAttenuation_pct (lineEdit_attenuation   ->getNonOptValue<double>()                    );
   this->m_editItem->setTimesCultured   (lineEdit_timesCultured ->getNonOptValue<int>()                       );
   this->m_editItem->setMaxReuse        (lineEdit_maxReuse      ->getNonOptValue<int>()                       );
   this->m_editItem->setAddToSecondary  (checkBox_addToSecondary->checkState() == Qt::Checked                   );
   this->m_editItem->setBestFor         (textEdit_bestFor       ->toPlainText()                                 );
   this->m_editItem->setNotes           (textEdit_notes         ->toPlainText()                                 );
   return;
}

void YeastEditor::writeLateFieldsToEditItem() {
   // do this late to make sure we've the row in the inventory table
   this->m_editItem->setInventoryQuanta(lineEdit_inventory->text().toInt());
   return;
}

void YeastEditor::readFieldsFromEditItem(std::optional<QString> propName) {
   if (!propName || *propName == PropertyNames::NamedEntity::name                  ) { this->lineEdit_name          ->setTextCursor(m_editItem->name          ()); // Continues to next line
                                                                                       this->tabWidget_editor->setTabText(0, m_editItem->name());                  if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Yeast::type                        ) { this->comboBox_type->setCurrentIndex(static_cast<int>(m_editItem->type()));                                             if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Yeast::form                        ) { this->comboBox_form->setCurrentIndex(static_cast<int>(m_editItem->form()));                                             if (propName) { return; } }
   if (!propName || *propName == PropertyNames::NamedEntityWithInventory::inventory) { this->lineEdit_inventory     ->setAmount   (m_editItem->inventory       ());                                            if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Yeast::amountIsWeight              ) { this->checkBox_amountIsWeight->setCheckState((m_editItem->amountIsWeight()) ? Qt::Checked : Qt::Unchecked);             if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Yeast::laboratory                  ) { this->lineEdit_laboratory    ->setText     (m_editItem->laboratory      ()); lineEdit_laboratory->setCursorPosition(0); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Yeast::productID                   ) { this->lineEdit_productID     ->setText     (m_editItem->productID       ()); lineEdit_productID ->setCursorPosition(0); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Yeast::minTemperature_c            ) { this->lineEdit_minTemperature->setAmount   (m_editItem->minTemperature_c());                                            if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Yeast::maxTemperature_c            ) { this->lineEdit_maxTemperature->setAmount   (m_editItem->maxTemperature_c());                                            if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Yeast::flocculation                ) { this->comboBox_flocculation  ->setCurrentIndex(static_cast<int>(m_editItem->flocculation()));                           if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Yeast::attenuation_pct             ) { this->lineEdit_attenuation   ->setAmount   (m_editItem->attenuation_pct ());                                            if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Yeast::timesCultured               ) { this->lineEdit_timesCultured ->setAmount   (m_editItem->timesCultured   ());                                            if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Yeast::maxReuse                    ) { this->lineEdit_maxReuse      ->setAmount   (m_editItem->maxReuse        ());                                            if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Yeast::addToSecondary              ) { this->checkBox_addToSecondary->setCheckState((m_editItem->addToSecondary()) ? Qt::Checked : Qt::Unchecked);             if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Yeast::bestFor                     ) { this->textEdit_bestFor       ->setPlainText(m_editItem->bestFor         ());                                            if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Yeast::notes                       ) { this->textEdit_notes         ->setPlainText(m_editItem->notes           ());                                            if (propName) { return; } }
   return;
}

// Insert the boiler-plate stuff that we cannot do in EditorBase
EDITOR_COMMON_SLOT_DEFINITIONS(YeastEditor)
