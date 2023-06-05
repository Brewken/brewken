/*======================================================================================================================
 * editors/StyleEditor.cpp is part of Brewken, and is copyright the following authors 2009-2023:
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
#include "editors/StyleEditor.h"

#include <QInputDialog>

#include "BtHorizontalTabs.h"
#include "database/ObjectStoreWrapper.h"
#include "measurement/Unit.h"
#include "model/Style.h"
#include "StyleListModel.h"
#include "StyleSortFilterProxyModel.h"

StyleEditor::StyleEditor(QWidget* parent /*, bool singleStyleEditor*/) :
   QDialog{parent},
   EditorBase<Style, StyleEditor>() {
   setupUi(this);

///   if (singleStyleEditor) {
///      for (int i = 0; i < horizontalLayout_styles->count(); ++i) {
///         QWidget* w = horizontalLayout_styles->itemAt(i)->widget();
///         if (w) {
///            w->setVisible(false);
///         }
///      }
///
///      pushButton_new->setVisible(false);
///   }

///   this->tabWidget_profile->tabBar()->setStyle(new BtHorizontalTabs);

///   this->styleListModel = new StyleListModel(styleComboBox);
///   this->styleProxyModel = new StyleSortFilterProxyModel(styleComboBox);
///   this->styleProxyModel->setDynamicSortFilter(true);
///   this->styleProxyModel->setSourceModel(styleListModel);
///   this->styleComboBox->setModel(styleProxyModel);

   // Note that the Min / Max pairs of entry fields each share a label (which is shown to the left of both fields)
   SMART_FIELD_INIT(StyleEditor, label_name          , lineEdit_name          , Style, PropertyNames::NamedEntity::name       );
   SMART_FIELD_INIT(StyleEditor, label_category      , lineEdit_category      , Style, PropertyNames::Style::category         );
   SMART_FIELD_INIT(StyleEditor, label_categoryNumber, lineEdit_categoryNumber, Style, PropertyNames::Style::categoryNumber   );
   SMART_FIELD_INIT(StyleEditor, label_styleLetter   , lineEdit_styleLetter   , Style, PropertyNames::Style::styleLetter      );
   SMART_FIELD_INIT(StyleEditor, label_styleGuide    , lineEdit_styleGuide    , Style, PropertyNames::Style::styleGuide       );
   SMART_FIELD_INIT(StyleEditor, label_og            , lineEdit_ogMin         , Style, PropertyNames::Style::ogMin            );
   SMART_FIELD_INIT(StyleEditor, label_og            , lineEdit_ogMax         , Style, PropertyNames::Style::ogMax            );
   SMART_FIELD_INIT(StyleEditor, label_fg            , lineEdit_fgMin         , Style, PropertyNames::Style::fgMin            );
   SMART_FIELD_INIT(StyleEditor, label_fg            , lineEdit_fgMax         , Style, PropertyNames::Style::fgMax            );
   SMART_FIELD_INIT(StyleEditor, label_ibu           , lineEdit_ibuMin        , Style, PropertyNames::Style::ibuMin        , 0);
   SMART_FIELD_INIT(StyleEditor, label_ibu           , lineEdit_ibuMax        , Style, PropertyNames::Style::ibuMax        , 0);
   SMART_FIELD_INIT(StyleEditor, label_color         , lineEdit_colorMin      , Style, PropertyNames::Style::colorMin_srm     );
   SMART_FIELD_INIT(StyleEditor, label_color         , lineEdit_colorMax      , Style, PropertyNames::Style::colorMax_srm     );
   SMART_FIELD_INIT(StyleEditor, label_carb          , lineEdit_carbMin       , Style, PropertyNames::Style::carbMin_vol   , 0);
   SMART_FIELD_INIT(StyleEditor, label_carb          , lineEdit_carbMax       , Style, PropertyNames::Style::carbMax_vol   , 0);
   SMART_FIELD_INIT(StyleEditor, label_abv           , lineEdit_abvMin        , Style, PropertyNames::Style::abvMin_pct    , 1);
   SMART_FIELD_INIT(StyleEditor, label_abv           , lineEdit_abvMax        , Style, PropertyNames::Style::abvMax_pct    , 1);

   BT_COMBO_BOX_INIT(StyleEditor, comboBox_type, Style, type);

   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞


///   // Note, per https://wiki.qt.io/New_Signal_Slot_Syntax#Default_arguments_in_slot, the use of a trivial lambda
///   // function to allow use of default argument on newStyle() slot
///   connect(this->pushButton_save  , &QAbstractButton::clicked     , this, &StyleEditor::save                     );
///   connect(this->pushButton_new   , &QAbstractButton::clicked     , this, [this]() { this->newStyle(); return; } );
///   connect(this->pushButton_cancel, &QAbstractButton::clicked     , this, &StyleEditor::clearAndClose            );
///   connect(this->pushButton_remove, &QAbstractButton::clicked     , this, &StyleEditor::removeStyle              );
///   connect(this->styleComboBox    , &QComboBox::currentTextChanged, this, &StyleEditor::styleSelected            );
///
///   this->setStyle(styleListModel->at(styleComboBox->currentIndex()));
   this->connectSignalsAndSlots();
   return;
}

StyleEditor::~StyleEditor() = default;

///void StyleEditor::setStyle( Style* s ) {
///   if (this->obsStyle) {
///      disconnect(this->obsStyle, 0, this, 0);
///   }
///
///   this->obsStyle = s;
///   if (this->obsStyle) {
///      connect(this->obsStyle, &NamedEntity::changed, this, &StyleEditor::changed);
///      qDebug() << Q_FUNC_INFO << "Editing style #" << this->obsStyle->key() << ":" << this->obsStyle->name();
///      showChanges();
///   }
///
///   styleComboBox->setCurrentIndex(styleListModel->indexOf(this->obsStyle));
///   return;
///}
///
///void StyleEditor::removeStyle() {
///   if (this->obsStyle) {
///      ObjectStoreWrapper::softDelete(*this->obsStyle);
///   }
///
///   setStyle(0);
///   return;
///}
///
///void StyleEditor::styleSelected( const QString& /*text*/ ) {
///   QModelIndex proxyIndex( styleProxyModel->index(styleComboBox->currentIndex(),0) );
///   QModelIndex sourceIndex( styleProxyModel->mapToSource(proxyIndex) );
///   setStyle( styleListModel->at(sourceIndex.row()) );
///   return;
///}

///void StyleEditor::save() {
///   qDebug() << Q_FUNC_INFO;
///   if (!this->obsStyle) {
///      setVisible(false);
///      return;
///   }
void StyleEditor::writeFieldsToEditItem() {

   m_editItem->setName          (this->lineEdit_name          ->text                       ());
   m_editItem->setCategory      (this->lineEdit_category      ->text                       ());
   m_editItem->setCategoryNumber(this->lineEdit_categoryNumber->text                       ());
   m_editItem->setStyleLetter   (this->lineEdit_styleLetter   ->text                       ());
   m_editItem->setStyleGuide    (this->lineEdit_styleGuide    ->text                       ());
   m_editItem->setType          (this->comboBox_type          ->getNonOptValue<Style::Type>());
   m_editItem->setOgMin         (this->lineEdit_ogMin         ->getNonOptCanonicalQty      ());
   m_editItem->setOgMax         (this->lineEdit_ogMax         ->getNonOptCanonicalQty      ());
   m_editItem->setFgMin         (this->lineEdit_fgMin         ->getNonOptCanonicalQty      ());
   m_editItem->setFgMax         (this->lineEdit_fgMax         ->getNonOptCanonicalQty      ());
   m_editItem->setIbuMin        (this->lineEdit_ibuMin        ->getNonOptValue<double>     ());
   m_editItem->setIbuMax        (this->lineEdit_ibuMax        ->getNonOptValue<double>     ());
   m_editItem->setColorMin_srm  (this->lineEdit_colorMin      ->getNonOptCanonicalQty      ());
   m_editItem->setColorMax_srm  (this->lineEdit_colorMax      ->getNonOptCanonicalQty      ());
   m_editItem->setCarbMin_vol   (this->lineEdit_carbMin       ->getNonOptCanonicalQty      ());
   m_editItem->setCarbMax_vol   (this->lineEdit_carbMax       ->getNonOptCanonicalQty      ());
   m_editItem->setAbvMin_pct    (this->lineEdit_abvMin        ->getNonOptValue<double>     ());
   m_editItem->setAbvMax_pct    (this->lineEdit_abvMax        ->getNonOptValue<double>     ());
///   this->m_editItem->setProfile       (textEdit_profile       ->toPlainText                ());
   m_editItem->setIngredients   (this->textEdit_ingredients   ->toPlainText                ());
   m_editItem->setExamples      (this->textEdit_examples      ->toPlainText                ());
   m_editItem->setNotes         (this->textEdit_notes         ->toPlainText                ());
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞


///   if (this->obsStyle->key() < 0) {
///      ObjectStoreWrapper::insert(*this->obsStyle);
///   }
///
///   setVisible(false);
   return;
}

void StyleEditor::writeLateFieldsToEditItem() {
   // Nothing to do here for Style
   return;
}

///void StyleEditor::newStyle(QString folder) {
///   QString name = QInputDialog::getText(this, tr("Style name"), tr("Style name:"));
///   if (name.isEmpty()) {
///      return;
///   }
///
///   Style *s = new Style(name);
///   if (!folder.isEmpty()) {
///      s->setFolder(folder);
///   }
///
///   this->setStyle(s);
///   this->show();
///   return;
///}
///
///void StyleEditor::clearAndClose() {
///   this->setVisible(false);
///   return;
///}
///
///void StyleEditor::changed(QMetaProperty const property, QVariant const value) {
///   qDebug() << Q_FUNC_INFO << property.name() << "=" << value;
///   this->showChanges(&property);
///   return;
///}

///void StyleEditor::clear() {
///   lineEdit_name          ->setText(QString(""));
///   lineEdit_category      ->setText(QString(""));
///   lineEdit_categoryNumber->setText(QString(""));
///   lineEdit_styleLetter   ->setText(QString(""));
///   lineEdit_styleGuide    ->setText(QString(""));
///   lineEdit_ogMin         ->setText(QString(""));
///   lineEdit_ogMax         ->setText(QString(""));
///   lineEdit_fgMin         ->setText(QString(""));
///   lineEdit_fgMax         ->setText(QString(""));
///   lineEdit_ibuMin        ->setText(QString(""));
///   lineEdit_ibuMax        ->setText(QString(""));
///   lineEdit_colorMin      ->setText(QString(""));
///   lineEdit_colorMax      ->setText(QString(""));
///   lineEdit_carbMin       ->setText(QString(""));
///   lineEdit_carbMax       ->setText(QString(""));
///   lineEdit_abvMin        ->setText(QString(""));
///   lineEdit_abvMax        ->setText(QString(""));
///   textEdit_profile       ->setText(QString(""));
///   textEdit_ingredients   ->setText(QString(""));
///   textEdit_examples      ->setText(QString(""));
///   textEdit_notes         ->setText(QString(""));
///   return;
///}

///void StyleEditor::showChanges(QMetaProperty const * metaProp) {
///   if (!this->obsStyle) {
///      this->clear();
///      return;
///   }
///
///   bool updateAll = true;
///   QString propName;
///   if (metaProp) {
///      updateAll = false;
///      propName = metaProp->name();
/////   QVariant val = metaProp->read(this->obsStyle);
///   }
void StyleEditor::readFieldsFromEditItem(std::optional<QString> propName) {
   if (!propName || *propName == PropertyNames::NamedEntity::name    ) { this->lineEdit_name          ->setTextCursor(m_editItem->name          ()); // Continues to next line
                                                                         /* this->tabWidget_editor->setTabText(0, m_editItem->name()); */                 if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Style::category      ) { lineEdit_category      ->setText   (m_editItem->category      ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Style::categoryNumber) { lineEdit_categoryNumber->setText   (m_editItem->categoryNumber()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Style::styleLetter   ) { lineEdit_styleLetter   ->setText   (m_editItem->styleLetter   ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Style::styleGuide    ) { lineEdit_styleGuide    ->setText   (m_editItem->styleGuide    ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Style::type          ) { comboBox_type          ->setValue  (m_editItem->type          ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Style::ogMin         ) { lineEdit_ogMin         ->setAmount (m_editItem->ogMin         ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Style::ogMax         ) { lineEdit_ogMax         ->setAmount (m_editItem->ogMax         ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Style::fgMin         ) { lineEdit_fgMin         ->setAmount (m_editItem->fgMin         ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Style::fgMax         ) { lineEdit_fgMax         ->setAmount (m_editItem->fgMax         ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Style::ibuMin        ) { lineEdit_ibuMin        ->setAmount (m_editItem->ibuMin        ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Style::ibuMax        ) { lineEdit_ibuMax        ->setAmount (m_editItem->ibuMax        ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Style::colorMin_srm  ) { lineEdit_colorMin      ->setAmount (m_editItem->colorMin_srm  ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Style::colorMax_srm  ) { lineEdit_colorMax      ->setAmount (m_editItem->colorMax_srm  ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Style::carbMin_vol   ) { lineEdit_carbMin       ->setAmount (m_editItem->carbMin_vol   ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Style::carbMax_vol   ) { lineEdit_carbMax       ->setAmount (m_editItem->carbMax_vol   ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Style::abvMin_pct    ) { lineEdit_abvMin        ->setAmount (m_editItem->abvMin_pct    ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Style::abvMax_pct    ) { lineEdit_abvMax        ->setAmount (m_editItem->abvMax_pct    ()); if (propName) { return; } }
///   if (!propName || *propName == PropertyNames::Style::profile       ) { textEdit_profile       ->setText   (m_editItem->profile       ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Style::ingredients   ) { textEdit_ingredients   ->setText   (m_editItem->ingredients   ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Style::examples      ) { textEdit_examples      ->setText   (m_editItem->examples      ()); if (propName) { return; } }
   if (!propName || *propName == PropertyNames::Style::notes         ) { textEdit_notes         ->setText   (m_editItem->notes         ()); if (propName) { return; } }
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞

   return;
}

// Insert the boiler-plate stuff that we cannot do in EditorBase
EDITOR_COMMON_SLOT_DEFINITIONS(StyleEditor)
