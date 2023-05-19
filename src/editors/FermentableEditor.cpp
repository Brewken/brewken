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

   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   BT_COMBO_BOX_INIT(FermentableEditor, comboBox_grainGroup, Fermentable, grainGroup);

   SMART_FIELD_INIT(FermentableEditor, label_producer              , lineEdit_producer              , Fermentable, PropertyNames::Fermentable::producer                 );
   SMART_FIELD_INIT(FermentableEditor, label_productId             , lineEdit_productId             , Fermentable, PropertyNames::Fermentable::productId                );
   SMART_FIELD_INIT(FermentableEditor, label_fineGrindYield_pct    , lineEdit_fineGrindYield_pct    , Fermentable, PropertyNames::Fermentable::fineGrindYield_pct    , 1);
   SMART_FIELD_INIT(FermentableEditor, label_coarseGrindYield_pct  , lineEdit_coarseGrindYield_pct  , Fermentable, PropertyNames::Fermentable::coarseGrindYield_pct  , 1);
   SMART_FIELD_INIT(FermentableEditor, label_potentialYield_sg     , lineEdit_potentialYield_sg     , Fermentable, PropertyNames::Fermentable::potentialYield_sg        );
   SMART_FIELD_INIT(FermentableEditor, label_alphaAmylase_dextUnits, lineEdit_alphaAmylase_dextUnits, Fermentable, PropertyNames::Fermentable::alphaAmylase_dextUnits   );
   SMART_FIELD_INIT(FermentableEditor, label_kolbachIndex_pct      , lineEdit_kolbachIndex_pct      , Fermentable, PropertyNames::Fermentable::kolbachIndex_pct      , 1);
   SMART_FIELD_INIT(FermentableEditor, label_hardnessPrpGlassy_pct , lineEdit_hardnessPrpGlassy_pct , Fermentable, PropertyNames::Fermentable::hardnessPrpGlassy_pct , 1);
   SMART_FIELD_INIT(FermentableEditor, label_hardnessPrpHalf_pct   , lineEdit_hardnessPrpHalf_pct   , Fermentable, PropertyNames::Fermentable::hardnessPrpHalf_pct   , 1);
   SMART_FIELD_INIT(FermentableEditor, label_hardnessPrpMealy_pct  , lineEdit_hardnessPrpMealy_pct  , Fermentable, PropertyNames::Fermentable::hardnessPrpMealy_pct  , 1);
   SMART_FIELD_INIT(FermentableEditor, label_kernelSizePrpPlump_pct, lineEdit_kernelSizePrpPlump_pct, Fermentable, PropertyNames::Fermentable::kernelSizePrpPlump_pct, 1);
   SMART_FIELD_INIT(FermentableEditor, label_kernelSizePrpThin_pct , lineEdit_kernelSizePrpThin_pct , Fermentable, PropertyNames::Fermentable::kernelSizePrpThin_pct , 1);
   SMART_FIELD_INIT(FermentableEditor, label_friability_pct        , lineEdit_friability_pct        , Fermentable, PropertyNames::Fermentable::friability_pct        , 1);
   SMART_FIELD_INIT(FermentableEditor, label_di_ph                 , lineEdit_di_ph                 , Fermentable, PropertyNames::Fermentable::di_ph                 , 1);
   SMART_FIELD_INIT(FermentableEditor, label_viscosity_cP          , lineEdit_viscosity_cP          , Fermentable, PropertyNames::Fermentable::viscosity_cP             );
   SMART_FIELD_INIT(FermentableEditor, label_dmsP                  , lineEdit_dmsP                  , Fermentable, PropertyNames::Fermentable::dmsP                  , 1);
   SMART_FIELD_INIT(FermentableEditor, label_fan                   , lineEdit_fan                   , Fermentable, PropertyNames::Fermentable::fan                   , 1);
   SMART_FIELD_INIT(FermentableEditor, label_fermentability_pct    , lineEdit_fermentability_pct    , Fermentable, PropertyNames::Fermentable::fermentability_pct    , 1);
   SMART_FIELD_INIT(FermentableEditor, label_betaGlucan            , lineEdit_betaGlucan            , Fermentable, PropertyNames::Fermentable::betaGlucan            , 1);

   SMART_CHECK_BOX_INIT(FermentableEditor, checkBox_amountIsWeight           , label_amountIsWeight           , lineEdit_inventory , Fermentable, amountIsWeight           );

   SMART_CHECK_BOX_INIT(FermentableEditor, checkBox_dmsPIsMassPerVolume      , label_dmsPIsMassPerVolume      , lineEdit_dmsP      , Fermentable, dmsPIsMassPerVolume      );
   SMART_CHECK_BOX_INIT(FermentableEditor, checkBox_fanIsMassPerVolume       , label_fanIsMassPerVolume       , lineEdit_fan       , Fermentable, fanIsMassPerVolume       );
   SMART_CHECK_BOX_INIT(FermentableEditor, checkBox_betaGlucanIsMassPerVolume, label_betaGlucanIsMassPerVolume, lineEdit_betaGlucan, Fermentable, betaGlucanIsMassPerVolume);

   this->connectSignalsAndSlots();
   return;
}

FermentableEditor::~FermentableEditor() = default;

void FermentableEditor::writeFieldsToEditItem() {

   this->m_editItem->setType(this->comboBox_type      ->getNonOptValue<Fermentable::Type      >());

   this->m_editItem->setName                  (this->lineEdit_name          ->text                  ());
   this->m_editItem->setYield_pct             (this->lineEdit_yield         ->getNonOptValue<double>());
   this->m_editItem->setColor_srm             (this->lineEdit_color         ->getNonOptValue<double>());
   this->m_editItem->setAddAfterBoil          (this->checkBox_addAfterBoil  ->checkState() == Qt::Checked);
   this->m_editItem->setOrigin                (this->lineEdit_origin        ->text                  ());
   this->m_editItem->setSupplier              (this->lineEdit_supplier      ->text                  ());
   this->m_editItem->setCoarseFineDiff_pct    (this->lineEdit_coarseFineDiff->getNonOptValue<double>());
   this->m_editItem->setMoisture_pct          (this->lineEdit_moisture      ->getNonOptValue<double>());
   this->m_editItem->setDiastaticPower_lintner(this->lineEdit_diastaticPower->getNonOptValue<double>());
   this->m_editItem->setProtein_pct           (this->lineEdit_protein       ->getNonOptValue<double>());
   this->m_editItem->setMaxInBatch_pct        (this->lineEdit_maxInBatch    ->getNonOptValue<double>());
   this->m_editItem->setRecommendMash         (this->checkBox_recommendMash ->checkState() == Qt::Checked);
   this->m_editItem->setIsMashed              (this->checkBox_isMashed      ->checkState() == Qt::Checked);
   this->m_editItem->setIbuGalPerLb           (this->lineEdit_ibuGalPerLb   ->getNonOptValue<double>()); // .:TBD:. No metric measure?
   this->m_editItem->setNotes                 (this->textEdit_notes         ->toPlainText           ());
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   this->m_editItem->setAmountIsWeight           (this->checkBox_amountIsWeight           ->isChecked          ());
   this->m_editItem->setGrainGroup               (this->comboBox_grainGroup               ->getOptValue<Fermentable::GrainGroup>());
   this->m_editItem->setProducer                 (this->lineEdit_producer                 ->text               ());
   this->m_editItem->setProductId                (this->lineEdit_productId                ->text               ());
   this->m_editItem->setFineGrindYield_pct       (this->lineEdit_fineGrindYield_pct       ->getOptValue<double>());
   this->m_editItem->setCoarseGrindYield_pct     (this->lineEdit_coarseGrindYield_pct     ->getOptValue<double>());
   this->m_editItem->setPotentialYield_sg        (this->lineEdit_potentialYield_sg        ->getOptValue<double>());
   this->m_editItem->setAlphaAmylase_dextUnits   (this->lineEdit_alphaAmylase_dextUnits   ->getOptValue<double>());
   this->m_editItem->setKolbachIndex_pct         (this->lineEdit_kolbachIndex_pct         ->getOptValue<double>());
   this->m_editItem->setHardnessPrpGlassy_pct    (this->lineEdit_hardnessPrpGlassy_pct    ->getOptValue<double>());
   this->m_editItem->setHardnessPrpHalf_pct      (this->lineEdit_hardnessPrpHalf_pct      ->getOptValue<double>());
   this->m_editItem->setHardnessPrpMealy_pct     (this->lineEdit_hardnessPrpMealy_pct     ->getOptValue<double>());
   this->m_editItem->setKernelSizePrpPlump_pct   (this->lineEdit_kernelSizePrpPlump_pct   ->getOptValue<double>());
   this->m_editItem->setKernelSizePrpThin_pct    (this->lineEdit_kernelSizePrpThin_pct    ->getOptValue<double>());
   this->m_editItem->setFriability_pct           (this->lineEdit_friability_pct           ->getOptValue<double>());
   this->m_editItem->setDi_ph                    (this->lineEdit_di_ph                    ->getOptValue<double>());
   this->m_editItem->setViscosity_cP             (this->lineEdit_viscosity_cP             ->getOptValue<double>());
   this->m_editItem->setDmsP                     (this->lineEdit_dmsP                     ->getOptValue<double>());
   this->m_editItem->setDmsPIsMassPerVolume      (this->checkBox_dmsPIsMassPerVolume      ->isChecked          ());
   this->m_editItem->setFan                      (this->lineEdit_fan                      ->getOptValue<double>());
   this->m_editItem->setFanIsMassPerVolume       (this->checkBox_fanIsMassPerVolume       ->isChecked          ());
   this->m_editItem->setFermentability_pct       (this->lineEdit_fermentability_pct       ->getOptValue<double>());
   this->m_editItem->setBetaGlucan               (this->lineEdit_betaGlucan               ->getOptValue<double>());
   this->m_editItem->setBetaGlucanIsMassPerVolume(this->checkBox_betaGlucanIsMassPerVolume->isChecked          ());

   return;
}

void FermentableEditor::writeLateFieldsToEditItem() {
   // Since inventory amount isn't really an attribute of the Fermentable, it's best to store it after we know the
   // Fermentable has a DB record.
   this->m_editItem->setInventoryAmount(lineEdit_inventory->toCanonical().quantity());
   return;
}

void FermentableEditor::readFieldsFromEditItem(std::optional<QString> propName) {
   if (!propName || *propName == PropertyNames::NamedEntity::name                  ) { this->lineEdit_name          ->setTextCursor(m_editItem->name                  ()); // Continues to next line
                                                                                       this->tabWidget_editor->setTabText(0, m_editItem->name());                                               if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::type                  ) { this->comboBox_type          ->setValue     (m_editItem->type                  ());                      if (propName) { return; } }
   if (!propName || *propName == PropertyNames::NamedEntityWithInventory::inventory) { this->lineEdit_inventory     ->setAmount    (m_editItem->inventory             ());                      if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::yield_pct             ) { this->lineEdit_yield         ->setAmount    (m_editItem->yield_pct             ());                      if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::color_srm             ) { this->lineEdit_color         ->setAmount    (m_editItem->color_srm             ());                      if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::addAfterBoil          ) { this->checkBox_addAfterBoil  ->setCheckState(m_editItem->addAfterBoil() ? Qt::Checked : Qt::Unchecked);  if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::origin                ) { this->lineEdit_origin        ->setTextCursor(m_editItem->origin                ());                      if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::supplier              ) { this->lineEdit_supplier      ->setTextCursor(m_editItem->supplier              ());                      if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::coarseFineDiff_pct    ) { this->lineEdit_coarseFineDiff->setAmount    (m_editItem->coarseFineDiff_pct    ());                      if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::moisture_pct          ) { this->lineEdit_moisture      ->setAmount    (m_editItem->moisture_pct          ());                      if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::diastaticPower_lintner) { this->lineEdit_diastaticPower->setAmount    (m_editItem->diastaticPower_lintner());                      if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::protein_pct           ) { this->lineEdit_protein       ->setAmount    (m_editItem->protein_pct           ());                      if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::maxInBatch_pct        ) { this->lineEdit_maxInBatch    ->setAmount    (m_editItem->maxInBatch_pct        ());                      if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::recommendMash         ) { this->checkBox_recommendMash ->setCheckState(m_editItem->recommendMash() ? Qt::Checked : Qt::Unchecked); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::isMashed              ) { this->checkBox_isMashed      ->setCheckState(m_editItem->isMashed()      ? Qt::Checked : Qt::Unchecked); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::ibuGalPerLb           ) { this->lineEdit_ibuGalPerLb   ->setAmount    (m_editItem->ibuGalPerLb           ());                      if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::notes                 ) { this->textEdit_notes         ->setPlainText (m_editItem->notes                 ());                      if (propName) { return; } }
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   if (!propName || *propName == PropertyNames::Fermentable::amountIsWeight           ) { this->checkBox_amountIsWeight           ->setChecked   (m_editItem->amountIsWeight           ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::grainGroup               ) { this->comboBox_grainGroup               ->setValue     (m_editItem->grainGroup               ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::producer                 ) { this->lineEdit_producer                 ->setTextCursor(m_editItem->producer                 ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::productId                ) { this->lineEdit_productId                ->setTextCursor(m_editItem->productId                ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::fineGrindYield_pct       ) { this->lineEdit_fineGrindYield_pct       ->setAmount    (m_editItem->fineGrindYield_pct       ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::coarseGrindYield_pct     ) { this->lineEdit_coarseGrindYield_pct     ->setAmount    (m_editItem->coarseGrindYield_pct     ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::potentialYield_sg        ) { this->lineEdit_potentialYield_sg        ->setAmount    (m_editItem->potentialYield_sg        ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::alphaAmylase_dextUnits   ) { this->lineEdit_alphaAmylase_dextUnits   ->setAmount    (m_editItem->alphaAmylase_dextUnits   ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::kolbachIndex_pct         ) { this->lineEdit_kolbachIndex_pct         ->setAmount    (m_editItem->kolbachIndex_pct         ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::hardnessPrpGlassy_pct    ) { this->lineEdit_hardnessPrpGlassy_pct    ->setAmount    (m_editItem->hardnessPrpGlassy_pct    ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::hardnessPrpHalf_pct      ) { this->lineEdit_hardnessPrpHalf_pct      ->setAmount    (m_editItem->hardnessPrpHalf_pct      ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::hardnessPrpMealy_pct     ) { this->lineEdit_hardnessPrpMealy_pct     ->setAmount    (m_editItem->hardnessPrpMealy_pct     ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::kernelSizePrpPlump_pct   ) { this->lineEdit_kernelSizePrpPlump_pct   ->setAmount    (m_editItem->kernelSizePrpPlump_pct   ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::kernelSizePrpThin_pct    ) { this->lineEdit_kernelSizePrpThin_pct    ->setAmount    (m_editItem->kernelSizePrpThin_pct    ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::friability_pct           ) { this->lineEdit_friability_pct           ->setAmount    (m_editItem->friability_pct           ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::di_ph                    ) { this->lineEdit_di_ph                    ->setAmount    (m_editItem->di_ph                    ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::viscosity_cP             ) { this->lineEdit_viscosity_cP             ->setAmount    (m_editItem->viscosity_cP             ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::dmsP                     ) { this->lineEdit_dmsP                     ->setAmount    (m_editItem->dmsP                     ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::dmsPIsMassPerVolume      ) { this->checkBox_dmsPIsMassPerVolume      ->setChecked   (m_editItem->dmsPIsMassPerVolume      ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::fan                      ) { this->lineEdit_fan                      ->setAmount    (m_editItem->fan                      ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::fanIsMassPerVolume       ) { this->checkBox_fanIsMassPerVolume       ->setChecked   (m_editItem->fanIsMassPerVolume       ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::fermentability_pct       ) { this->lineEdit_fermentability_pct       ->setAmount    (m_editItem->fermentability_pct       ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::betaGlucan               ) { this->lineEdit_betaGlucan               ->setAmount    (m_editItem->betaGlucan               ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Fermentable::betaGlucanIsMassPerVolume) { this->checkBox_betaGlucanIsMassPerVolume->setChecked   (m_editItem->betaGlucanIsMassPerVolume()); if (propName) { return; } }

   return;
}

// Insert the boiler-plate stuff that we cannot do in EditorBase
EDITOR_COMMON_SLOT_DEFINITIONS(FermentableEditor)
