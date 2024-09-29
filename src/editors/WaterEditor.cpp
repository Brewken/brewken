/*======================================================================================================================
 * editors/WaterEditor.cpp is part of Brewken, and is copyright the following authors 2009-2024:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Jeff Bailey <skydvr38@verizon.net>
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
#include "editors/WaterEditor.h"

#include <QDebug>
#include <QInputDialog>

#include "database/ObjectStoreWrapper.h"
#include "model/Water.h"

///// This private implementation class holds all private non-virtual members of WaterEditor
///class WaterEditor::impl {
///public:
///   /**
///    * Constructor
///    */
///   impl() :
//////                                    observedWater{},
///                                    editedWater{} {
///      return;
///   }
///
///   /**
///    * Destructor
///    */
///   ~impl() = default;
///
///   // This is the Water object we are "observing" and to which our edits will be committed if and when the user clicks
///   // OK
//////   std::shared_ptr<Water> observedWater;
///   // This is a temporary copy of the "observed" Water that holds the live edits (which will be saved if the user clicks
///   // OK and lost if the user clicks Cancel)
///   std::unique_ptr<Water> editedWater;
///};

WaterEditor::WaterEditor(QWidget *parent, QString const editorName) :
   QDialog(parent),
   EditorBase<WaterEditor, Water, WaterEditorOptions>(editorName) {
///   pimpl{std::make_unique<impl>()} {
   this->setupUi(this);
   this->postSetupUiInit(
      {
       EDITOR_FIELD_NORM(Water, label_name            , lineEdit_name             , NamedEntity::name      ),
       EDITOR_FIELD_NORM(Water, label_notes           , textEdit_notes            , Water::notes           ),
       EDITOR_FIELD_NORM(Water, label_ca              , lineEdit_ca               , Water::calcium_ppm     , 2),
       EDITOR_FIELD_NORM(Water, label_cl              , lineEdit_cl               , Water::chloride_ppm    , 2),
       EDITOR_FIELD_NORM(Water, label_mg              , lineEdit_mg               , Water::magnesium_ppm   , 2),
       EDITOR_FIELD_NORM(Water, label_so4             , lineEdit_so4              , Water::sulfate_ppm     , 2),
       EDITOR_FIELD_NORM(Water, label_na              , lineEdit_na               , Water::sodium_ppm      , 2),
       EDITOR_FIELD_NORM(Water, label_alk             , lineEdit_alk              , Water::alkalinity_ppm  , 2),
       EDITOR_FIELD_NORM(Water, label_pH              , lineEdit_ph               , Water::ph              , 2),
       EDITOR_FIELD_NORM(Water, label_alkalinityAsHCO3, boolCombo_alkalinityAsHCO3, Water::alkalinityAsHCO3, tr("CaCO3"), tr("HCO3")),
       // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
       EDITOR_FIELD_NORM(Water, label_carbonate       , lineEdit_carbonate        , Water::carbonate_ppm   , 2),
       EDITOR_FIELD_NORM(Water, label_potassium       , lineEdit_potassium        , Water::potassium_ppm   , 2),
       EDITOR_FIELD_NORM(Water, label_iron            , lineEdit_iron             , Water::iron_ppm        , 2),
       EDITOR_FIELD_NORM(Water, label_nitrate         , lineEdit_nitrate          , Water::nitrate_ppm     , 2),
       EDITOR_FIELD_NORM(Water, label_nitrite         , lineEdit_nitrite          , Water::nitrite_ppm     , 2),
       EDITOR_FIELD_NORM(Water, label_flouride        , lineEdit_flouride         , Water::flouride_ppm    , 2),
      }
   );

   // .:TBD:. The QLineEdit::textEdited and QPlainTextEdit::textChanged signals below are sent somewhat more frequently
   // than we really need - ie every time you type a character in the name or notes field.  We should perhaps look at
   // changing the corresponding field types...
///   connect(this->buttonBox,           &QDialogButtonBox::accepted,    this, &WaterEditor::saveAndClose      );
///   connect(this->buttonBox,           &QDialogButtonBox::rejected,    this, &WaterEditor::clearAndClose     );
///   connect(this->boolCombo_alkalinityAsHCO3, &QComboBox::currentTextChanged, this, &WaterEditor::inputFieldModified);
///   connect(this->lineEdit_alk,        &SmartLineEdit::textModified,   this, &WaterEditor::inputFieldModified);
///   connect(this->lineEdit_ca,         &SmartLineEdit::textModified,   this, &WaterEditor::inputFieldModified);
///   connect(this->lineEdit_cl,         &SmartLineEdit::textModified,   this, &WaterEditor::inputFieldModified);
///   connect(this->lineEdit_mg,         &SmartLineEdit::textModified,   this, &WaterEditor::inputFieldModified);
///   connect(this->lineEdit_na,         &SmartLineEdit::textModified,   this, &WaterEditor::inputFieldModified);
///   connect(this->lineEdit_name,       &QLineEdit::textEdited,         this, &WaterEditor::inputFieldModified);
///   connect(this->lineEdit_ph,         &SmartLineEdit::textModified,   this, &WaterEditor::inputFieldModified);
///   connect(this->lineEdit_so4,        &SmartLineEdit::textModified,   this, &WaterEditor::inputFieldModified);
///   connect(this->textEdit_notes,      &QPlainTextEdit::textChanged,   this, &WaterEditor::inputFieldModified);

   this->waterEditRadarChart->init(
      tr("PPM"),
      50,
      {
         {PropertyNames::Water::calcium_ppm,     tr("Calcium"    )},
         {PropertyNames::Water::bicarbonate_ppm, tr("Bicarbonate")},
         {PropertyNames::Water::sulfate_ppm,     tr("Sulfate"    )},
         {PropertyNames::Water::chloride_ppm,    tr("Chloride"   )},
         {PropertyNames::Water::sodium_ppm,      tr("Sodium"     )},
         {PropertyNames::Water::magnesium_ppm,   tr("Magnesium"  )}
      }
   );

   return;
}

//WaterEditor::~WaterEditor() = default;
WaterEditor::~WaterEditor() {
   qDebug() << Q_FUNC_INFO << "Cleaning up";
   if (this->m_editItem) {
      qDebug() <<
         Q_FUNC_INFO << this->m_editorName << ": Was observing" << this->m_editItem->name() <<
         "#" << this->m_editItem->key() << " @" << static_cast<void *>(this->m_editItem.get()) <<
         " (use count" << this->m_editItem.use_count() << ")";
   }
   if (this->m_liveEditItem) {
      qDebug() <<
         Q_FUNC_INFO << this->m_editorName << ": Was editing" << this->m_liveEditItem->name() <<
         "#" << this->m_liveEditItem->key() << " @" << static_cast<void *>(this->m_liveEditItem.get());
   }
   return;
}

///void WaterEditor::setWater(std::shared_ptr<Water> water) {
///
///   if (this->m_editItem) {
///      qDebug() <<
///         Q_FUNC_INFO << this->m_editorName << ": Stop observing" << this->m_editItem->name() <<
///         "#" << this->m_editItem->key() << " @" << static_cast<void *>(this->m_editItem.get()) <<
///         " (use count" << this->m_editItem.use_count() << ")";
///      disconnect(this->m_editItem.get(), nullptr, this, nullptr);
///      this->m_editItem.reset();
///   }
///
///   if (water) {
///      this->m_editItem = water;
///      qDebug() <<
///         Q_FUNC_INFO << this->m_editorName << ": Now observing" << this->m_editItem->name() <<
///         "#" << this->m_editItem->key() << " @" << static_cast<void *>(this->m_editItem.get()) <<
///         " (use count" << this->m_editItem.use_count() << ")";
///      this->waterEditRadarChart->addSeries(tr("Current"), Qt::darkGreen, *this->m_editItem);
///      connect(this->m_editItem.get(), &NamedEntity::changed, this, &WaterEditor::changed);
///
///      // Make a copy of the Water object we are observing
///      this->m_liveEditItem = std::make_unique<Water>(*this->m_editItem);
//////      this->m_liveEditItem->setAmount(0.0);
///      this->waterEditRadarChart->addSeries(tr("Modified"), Qt::green, *this->m_liveEditItem);
///
///      this->showChanges();
///   } else {
///      qDebug() << Q_FUNC_INFO << this->m_editorName << ": Observing Nothing";
///   }
///
///   return;
///}

void WaterEditor::postSetEditItem() {
   if (this->m_editItem) {
      // Note that we don't need to remove the old series from any previous Water objects as the call to addSeries will
      // replace them.
      this->waterEditRadarChart->addSeries(tr("Current"), Qt::darkGreen, *this->m_editItem);

///      // Make a copy of the Water object we are observing
///      this->m_liveEditItem = std::make_unique<Water>(*this->m_editItem);
      this->waterEditRadarChart->addSeries(tr("Modified"), Qt::green, *this->m_liveEditItem);
   }
   return;
}

///void WaterEditor::newWater(QString folder) {
///   QString name = QInputDialog::getText(this, tr("Water name"),
///                                              tr("Water name:"));
///   if (name.isEmpty()) {
///      return;
///   }
///
///   qDebug() << Q_FUNC_INFO << this->m_editorName << ": Creating new Water, " << name;
///
///   this->setWater(std::make_shared<Water>(name));
///   if (!folder.isEmpty()) {
///      this->m_editItem->setFolder(folder);
///   }
///
///   setVisible(true);
///
///   return;
///}

///void WaterEditor::showChanges([[maybe_unused]] QMetaProperty const * prop) {
///
///   if (!this->m_editItem) {
///      return;
///   }
///
///   QString propName;
///
///   bool updateAll = false;
///
///   if (prop == nullptr) {
///      qDebug() << Q_FUNC_INFO << this->m_editorName << ": Update all";
///      updateAll = true;
///   } else {
///      propName = prop->name();
///      qDebug() << Q_FUNC_INFO << this->m_editorName << ": Changed" << propName;
///   }
///
///   if (updateAll || propName == PropertyNames::NamedEntity::name      ) { this->lineEdit_name->setText    (this->m_editItem->name           ()); if (!updateAll) { return; } }
///   if (updateAll || propName == PropertyNames::Water::calcium_ppm     ) { this->lineEdit_ca  ->setQuantity(this->m_editItem->calcium_ppm    ()); if (!updateAll) { return; } }
///   if (updateAll || propName == PropertyNames::Water::magnesium_ppm   ) { this->lineEdit_mg  ->setQuantity(this->m_editItem->magnesium_ppm  ()); if (!updateAll) { return; } }
///   if (updateAll || propName == PropertyNames::Water::sulfate_ppm     ) { this->lineEdit_so4 ->setQuantity(this->m_editItem->sulfate_ppm    ()); if (!updateAll) { return; } }
///   if (updateAll || propName == PropertyNames::Water::sodium_ppm      ) { this->lineEdit_na  ->setQuantity(this->m_editItem->sodium_ppm     ()); if (!updateAll) { return; } }
///   if (updateAll || propName == PropertyNames::Water::chloride_ppm    ) { this->lineEdit_cl  ->setQuantity(this->m_editItem->chloride_ppm   ()); if (!updateAll) { return; } }
///   if (updateAll || propName == PropertyNames::Water::bicarbonate_ppm ) { this->lineEdit_alk ->setQuantity(this->m_editItem->bicarbonate_ppm()); if (!updateAll) { return; } }
///   if (updateAll || propName == PropertyNames::Water::ph              ) { this->lineEdit_ph  ->setQuantity(this->m_editItem->ph             ()); if (!updateAll) { return; } }
///   if (updateAll || propName == PropertyNames::Water::alkalinityAsHCO3) {
///      bool typeless = this->m_editItem->alkalinityAsHCO3();
///      this->comboBox_alk->setCurrentIndex(comboBox_alk->findText(typeless ? "HCO3" : "CaCO3"));
///      if (!updateAll) { return; }
///   }
///   if (updateAll || propName == PropertyNames::Water::notes           ) { this->plainTextEdit_notes->setPlainText(this->m_editItem->notes() );    if (!updateAll) { return; } }
///   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
///   if (updateAll || propName == PropertyNames::Water::carbonate_ppm ) { this->lineEdit_alk ->setQuantity(this->m_editItem->carbonate_ppm()); if (!updateAll) { return; } }
///   if (updateAll || propName == PropertyNames::Water::potassium_ppm ) { this->lineEdit_alk ->setQuantity(this->m_editItem->potassium_ppm()); if (!updateAll) { return; } }
///   if (updateAll || propName == PropertyNames::Water::iron_ppm      ) { this->lineEdit_alk ->setQuantity(this->m_editItem->iron_ppm     ()); if (!updateAll) { return; } }
///   if (updateAll || propName == PropertyNames::Water::nitrate_ppm   ) { this->lineEdit_alk ->setQuantity(this->m_editItem->nitrate_ppm  ()); if (!updateAll) { return; } }
///   if (updateAll || propName == PropertyNames::Water::nitrite_ppm   ) { this->lineEdit_alk ->setQuantity(this->m_editItem->nitrite_ppm  ()); if (!updateAll) { return; } }
///   if (updateAll || propName == PropertyNames::Water::flouride_ppm  ) { this->lineEdit_alk ->setQuantity(this->m_editItem->flouride_ppm ()); if (!updateAll) { return; } }
///
///   return;
///}

///void WaterEditor::inputFieldModified() {
///   //
///   // What we're doing here is, if one of the input fields on the dialog is modified, we update the corresponding
///   // field(s) on this->m_liveEditItem and replot the radar chart.  That way the user can see the "shape" of their
///   // changes in real time.
///   //
///   // When we come to close the window, depending on whether the user clicked "OK" or "Cancel" we then either copy the
///   // changes to the "observed" water (this->m_editItem) or discard them (resetting this->m_liveEditItem
///   // to be the same as this->m_editItem).
///   //
///   QObject const * const signalSender = this->sender();
///   // Usually leave the next line commented as otherwise get too much logging when user is typing in notes or name
///   // fields.
/////   qDebug() << Q_FUNC_INFO << this->m_editorName << ": signal from" << signalSender;
///   if (signalSender && signalSender->parent() == this) {
///      // .:TBD:. Need to get to the bottom of the relationship between Water::alkalinity and Water::bicarbonate_ppm.  It
///      //         feels wrong that we just set both from the same input, but probably needs some more profound thought
///      //         about what exactly correct behaviour should be.
///      if      (signalSender == this->boolCombo_alkalinityAsHCO3)         {this->m_liveEditItem->setAlkalinityAsHCO3(this->boolCombo_alkalinityAsHCO3 ->currentText() == QString("HCO3"));}
///      else if (signalSender == this->lineEdit_alk)         {this->m_liveEditItem->setBicarbonate_ppm (this->lineEdit_alk ->getNonOptCanonicalQty());  // NB continues on next line!
///                                                            this->m_liveEditItem->setAlkalinity_ppm  (this->lineEdit_alk ->getNonOptCanonicalQty());        }
///      else if (signalSender == this->lineEdit_ca)          {this->m_liveEditItem->setCalcium_ppm     (this->lineEdit_ca  ->getNonOptCanonicalQty());        }
///      else if (signalSender == this->lineEdit_cl)          {this->m_liveEditItem->setChloride_ppm    (this->lineEdit_cl  ->getNonOptCanonicalQty());        }
///      else if (signalSender == this->lineEdit_mg)          {this->m_liveEditItem->setMagnesium_ppm   (this->lineEdit_mg  ->getNonOptCanonicalQty());        }
///      else if (signalSender == this->lineEdit_na)          {this->m_liveEditItem->setSodium_ppm      (this->lineEdit_na  ->getNonOptCanonicalQty());        }
///      else if (signalSender == this->lineEdit_name)        {this->m_liveEditItem->setName            (this->lineEdit_name->text());                         }
///      else if (signalSender == this->lineEdit_ph)          {this->m_liveEditItem->setPh              (this->lineEdit_ph  ->getNonOptCanonicalQty());        }
///      else if (signalSender == this->lineEdit_so4)         {this->m_liveEditItem->setSulfate_ppm     (this->lineEdit_so4 ->getNonOptCanonicalQty());        }
///      else if (signalSender == this->textEdit_notes)       {this->m_liveEditItem->setNotes           (this->textEdit_notes->toPlainText());                 }
///      else {
///         // If we get here, it's probably a coding error
///         qWarning() << Q_FUNC_INFO << "Unrecognised child";
///      }
///
///      //
///      // Strictly speaking we don't always need to replot the radar chart - eg if a text field changed it doesn't affect
///      // the chart - but, for the moment, we just keep things simple and always replot.
///      //
///      this->waterEditRadarChart->replot();
///   }
///   return;
///}

void WaterEditor::postInputFieldModified() {
   //
   // Strictly speaking we don't always need to replot the radar chart - eg if a text field changed it doesn't affect
   // the chart - but, for the moment, we just keep things simple and always replot.
   //
   this->waterEditRadarChart->replot();
   return;
}

///void WaterEditor::changed(QMetaProperty prop, QVariant /*val*/) {
///   if (sender() == this->m_editItem.get()) {
///      this->showChanges(&prop);
///   }
///
///   this->waterEditRadarChart->replot();
///   return;
///}

///void WaterEditor::saveAndClose() {
///   qDebug() << Q_FUNC_INFO << this->m_editorName;
///   if (!this->m_editItem) {
///      // For the moment, if we weren't given a Water object (via setWater) then we don't try to save any changes when
///      // the editor is closed.  Arguably, if the user has actually filled in a bunch of data, then we should use that
///      // to create and save a new Water object.
///      qDebug() << Q_FUNC_INFO << "Save and close with no Water specified, so discarding any inputs";
///      return;
///   }
///
///   // Apply all the edits
///   if (this->m_liveEditItem) {
///      *this->m_editItem = *this->m_liveEditItem;
///      qDebug() <<
///         Q_FUNC_INFO << this->m_editorName << ": Applied edits to Water #" << this->m_editItem->key() <<
///         ":" << this->m_editItem->name();
///   }
///
///   //
///   // TBD: When we're called from WaterDialog, it is that window that is responsible for adding new Water objects to the
///   //      Recipe (which results in the Water object being saved in the DB).  Saving the Water object means the current
///   //      logic in WaterDialog won't pick up that it needs to be added to the Recipe.
///   //
///   if (this->m_editItem->key() < 0) {
///      qDebug() << Q_FUNC_INFO << "Writing new Water:" << this->m_editItem->name();
///      ObjectStoreWrapper::insert(this->m_editItem);
///   }
///
///   setVisible(false);
///   return;
///}

///void WaterEditor::clearAndClose() {
///   qDebug() << Q_FUNC_INFO << this->m_editorName;
///
///   // At this point, we want to clear edits, but we _don't_ want to stop observing the Water that's been given to us as
///   // our creator (eg WaterDialog) may redisplay us without a repeat call to setWater.
///
///   // This reverts all the input fields
///   this->showChanges();
///
///   // Revert all the edits in our temporary copy of the "observed" Water
///   if (this->m_editItem && this->m_liveEditItem) {
///      *this->m_liveEditItem = *this->m_editItem;
///      qDebug() <<
///         Q_FUNC_INFO << this->m_editorName << ": Discarded edits to Water #" <<
///         this->m_editItem->key() << ":" << this->m_editItem->name();
///   }
///
///   setVisible(false); // Hide the window.
///   return;
///}

///void WaterEditor::writeFieldsToEditItem() {
///   return;
///}
///
///void WaterEditor::writeLateFieldsToEditItem() {
///   return;
///}
///
///void WaterEditor::readFieldsFromEditItem([[maybe_unused]] std::optional<QString> propName) {
///   return;
///}

EDITOR_COMMON_CODE(Water)
