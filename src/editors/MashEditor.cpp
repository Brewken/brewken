/*======================================================================================================================
 * editors/MashEditor.cpp is part of Brewken, and is copyright the following authors 2009-2026:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Kregg Kemper <gigatropolis@yahoo.com>
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
#include "editors/MashEditor.h"

#include <algorithm>

#include <QDebug>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QWidget>

#include "MashCalculator.h"
#include "database/ObjectStoreWrapper.h"
#include "measurement/Unit.h"
#include "model/Equipment.h"
#include "model/Mash.h"
#include "model/MashStep.h"
#include "model/Recipe.h"
#include "model/RecipeUtils.h"
#include "widgets/SmartAmounts.h"

#ifdef MANUALLY_INCLUDE_MOC
   // Explicitly doing this include reduces potential problems with AUTOMOC when compiling with CMake
   #include "moc_MashEditor.cpp"
#endif

MashEditor::MashEditor(QWidget* parent, QString const editorName) :
   QDialog(parent),
   EditorBase<MashEditor, Mash, MashEditorOptions>(editorName) {
   this->setupUi(this);
   this->postSetupUiInit({
      EDITOR_FIELD_NORM(Mash, label_name          , lineEdit_name       , NamedEntity::name             ),
      EDITOR_FIELD_NORM(Mash, label_grainTemp     , lineEdit_grainTemp  , Mash::grainTemp_c          , 1),
      EDITOR_FIELD_NORM(Mash, label_spargeTemp    , lineEdit_spargeTemp , Mash::spargeTemp_c         , 1),
      EDITOR_FIELD_NORM(Mash, label_spargePh      , lineEdit_spargePh   , Mash::ph                   , 0),
      EDITOR_FIELD_NORM(Mash, label_tunTemp       , lineEdit_tunTemp    , Mash::initialTunTemp_c     , 1),
      EDITOR_FIELD_NORM(Mash, label_tunMass       , lineEdit_tunMass    , Mash::tunWeight_kg            ),
      EDITOR_FIELD_NORM(Mash, label_tunSpHeat     , lineEdit_tunSpHeat  , Mash::tunSpecificHeat_calGC, 1),
      EDITOR_FIELD_NORM(Mash, label_totalMashWater, value_totalMashWater, Mash::totalMashWater_l     , 1),
      EDITOR_FIELD_NORM(Mash, label_totalTime     , value_totalTime     , Mash::totalTime_mins       , 0),
      EDITOR_FIELD_NORM(Mash, tab_notes           , textEdit_notes      , Mash::notes                   ),
   });

   //
   // The calculation controls are not fields of the Mash, so we set them up separately.  The thickness field shows
   // the same quantity as MashStep::liquorToGristRatio_lKg, which is where the calculation records it.
   //
   SMART_FIELD_INIT(MashEditor, label_thickness, lineEdit_thickness, MashStep, PropertyNames::MashStep::liquorToGristRatio_lKg, 2);
   // Order here must match MashCalculator::SpargeMethod
   this->comboBox_spargeMethod->addItem(tr("No sparge"   ), static_cast<int>(MashCalculator::SpargeMethod::NoSparge   ));
   this->comboBox_spargeMethod->addItem(tr("Batch sparge"), static_cast<int>(MashCalculator::SpargeMethod::BatchSparge));
   this->comboBox_spargeMethod->addItem(tr("Fly sparge"  ), static_cast<int>(MashCalculator::SpargeMethod::FlySparge  ));
   this->connect(this->comboBox_spargeMethod, &QComboBox::currentIndexChanged, this, &MashEditor::updateCalculatorControls);
   this->connect(this->pushButton_calculate , &QAbstractButton::clicked      , this, &MashEditor::calculate               );
   this->refreshCalculator();

   return;
}

MashEditor::~MashEditor() = default;

// Insert the boilerplate stuff that we cannot do in EditorWithRecipeBase
EDITOR_COMMON_CODE(Mash)

void MashEditor::postSetEditItem() {
   this->refreshCalculator();
   return;
}

void MashEditor::showEvent(QShowEvent * event) {
   // When we're editing a new mash for a recipe, m_newItemForRecipe is set after setEditItem, so we need to refresh here
   // too.
   this->refreshCalculator();
   this->QDialog::showEvent(event);
   return;
}

void MashEditor::focusCalculator() {
   this->tabWidget_editor->setCurrentWidget(this->tab_main);
   this->pushButton_calculate->setFocus();
   return;
}

Recipe * MashEditor::recipeForCalculation() const {
   if (!this->m_editItem || !this->m_recipeObs) {
      return nullptr;
   }
   if (this->m_newItemForRecipe == this->m_editItem) {
      // New mash that will be used by the recipe when the user clicks Save
      return this->m_recipeObs;
   }
   return this->currentRecipeIfItUsesEditItem();
}

void MashEditor::refreshCalculator() {
   Recipe const * const recipe = this->recipeForCalculation();
   QList<MashCalculator::Step> const steps =
      this->m_editItem ? MashCalculator::stepsFrom(*this->m_editItem) : QList<MashCalculator::Step>{};

   //
   // Show the sparge method and number of batches the mash currently has
   //
   MashCalculator::SpargeMethod const spargeMethod = MashCalculator::spargeMethod(steps);
   {
      QSignalBlocker const blocker{this->comboBox_spargeMethod};
      this->comboBox_spargeMethod->setCurrentIndex(
         this->comboBox_spargeMethod->findData(static_cast<int>(spargeMethod))
      );
   }
   int const numBatchSparges = std::count_if(
      steps.cbegin(), steps.cend(), [](auto const & step) { return step.type == MashStep::Type::BatchSparge; }
   );
   this->spinBox_numBatches->setValue(std::max(numBatchSparges, 1));

   //
   // Show the mash thickness the calculation would use.  (If there's no sparge, the strike water is normally all the
   // water, so its thickness isn't a useful starting point if the user switches to a sparge method.)
   //
   double thickness_LKg = MashCalculator::defaultThickness_LKg;
   if (!steps.isEmpty() && spargeMethod != MashCalculator::SpargeMethod::NoSparge) {
      MashCalculator::Step const & firstStep = steps.first();
      if (firstStep.liquorToGristRatio_lKg && *firstStep.liquorToGristRatio_lKg > 0.0) {
         thickness_LKg = *firstStep.liquorToGristRatio_lKg;
      } else if (recipe && firstStep.amount_l > 0.0) {
         thickness_LKg = recipe->mashThicknessForStrikeWater_LKg(firstStep.amount_l).value_or(thickness_LKg);
         if (thickness_LKg <= 0.0) {
            thickness_LKg = MashCalculator::defaultThickness_LKg;
         }
      }
   }
   this->lineEdit_thickness->setQuantity(thickness_LKg);

   this->comboBox_spargeMethod->setEnabled(recipe != nullptr);
   this->pushButton_calculate ->setEnabled(recipe != nullptr);
   this->updateCalculatorControls();

   if (!this->m_editItem) {
      this->label_calculateInfo->clear();
   } else if (!recipe) {
      this->showCalculatorMessage(
         tr("To calculate water, edit this mash from a recipe that uses it (eg with the Edit button on the recipe's "
            "Mash tab), as the calculation depends on the recipe's grain bill and equipment.")
      );
   } else {
      this->showCalculatorMessage(
         tr("Calculates for the grain bill and equipment of recipe \"%1\".  Values you have marked as fixed in a step "
            "are not changed.  Steps are changed straight away (ie without clicking Save).").arg(recipe->name())
      );
   }
   return;
}

void MashEditor::updateCalculatorControls() {
   bool const enabled = (this->recipeForCalculation() != nullptr);
   auto const spargeMethod =
      static_cast<MashCalculator::SpargeMethod>(this->comboBox_spargeMethod->currentData().toInt());
   this->spinBox_numBatches->setEnabled(enabled && spargeMethod == MashCalculator::SpargeMethod::BatchSparge);
   this->lineEdit_thickness->setEnabled(enabled && spargeMethod != MashCalculator::SpargeMethod::NoSparge);
   return;
}

void MashEditor::showCalculatorMessage(QString const & heading, QStringList const & details) {
   QString text = heading.toHtmlEscaped();
   if (!details.isEmpty()) {
      text += "<ul>";
      for (auto const & detail : details) {
         text += QString{"<li>%1</li>"}.arg(detail.toHtmlEscaped());
      }
      text += "</ul>";
   }
   this->label_calculateInfo->setText(text);
   return;
}

void MashEditor::calculate() {
   Recipe * const recipe = this->recipeForCalculation();
   if (!recipe || !this->m_editItem) {
      // Button should have been disabled
      qWarning() << Q_FUNC_INFO << "No recipe or no mash";
      return;
   }

   //
   // The calculation is for this recipe, so we mustn't change a mash that other recipes use (as the results would be
   // wrong for them), nor one that is locked.  In either case, we offer to make a copy for this recipe.  (A new mash
   // for the recipe is not yet used by anything, so is fine.)
   //
   if (this->m_newItemForRecipe != this->m_editItem) {
      bool const canChange = RecipeUtils::lockReason(*this->m_editItem) == RecipeUtils::LockReason::NotLocked;
      bool const isShared  = RecipeUtils::recipesUsing(*this->m_editItem).size() > 1;
      if (!canChange || isShared) {
         if (recipe->locked()) {
            QMessageBox::information(this,
                                     tr("Recipe is locked"),
                                     tr("This recipe is locked, so its mash cannot be changed."));
            return;
         }
         QMessageBox messageBox{this};
         messageBox.setIcon(QMessageBox::Question);
         messageBox.setWindowTitle(tr("Make a copy of mash?"));
         messageBox.setText(RecipeUtils::describeSharingAndLocking(*this->m_editItem, recipe));
         messageBox.setInformativeText(
            tr("Calculating water changes the steps of the mash, based on this recipe's grain bill and equipment.\n\n"
               "Do you want to make a copy of this mash for this recipe, so that you can do this without affecting "
               "anything else?")
         );
         QPushButton * copyButton = messageBox.addButton(tr("Make a copy"), QMessageBox::AcceptRole);
         messageBox.addButton(QMessageBox::Cancel);
         messageBox.setDefaultButton(copyButton);
         messageBox.exec();
         if (messageBox.clickedButton() != copyButton) {
            return;
         }
         this->makeCopyForRecipe();
      }
   }
   Mash & mash = *this->m_editItem;

   //
   // Use the values in the editor for the mash's own fields, as the user might have changed them without (yet) saving.
   //
   MashCalculator::Inputs inputs = MashCalculator::Inputs::fromRecipe(*recipe, mash);
   bool grainTempOk = false;
   double const grainTemp_c = this->lineEdit_grainTemp->getNonOptCanonicalQty(&grainTempOk);
   if (grainTempOk) {
      inputs.grainTemp_c = grainTemp_c;
   }
   inputs.tunTemp_c = this->lineEdit_tunTemp->getOptCanonicalQty().value_or(inputs.grainTemp_c);

   auto const spargeMethod =
      static_cast<MashCalculator::SpargeMethod>(this->comboBox_spargeMethod->currentData().toInt());
   QList<MashCalculator::Step> steps = MashCalculator::stepsFrom(mash);
   // Older mashes might have a sparge temperature of 0 °C, which really means "not set"
   std::optional<double> const spargeTemp_c = this->lineEdit_spargeTemp->getOptCanonicalQty();
   MashCalculator::setSparge(
      steps,
      spargeMethod,
      this->spinBox_numBatches->value(),
      (spargeTemp_c && *spargeTemp_c > 0.0) ? *spargeTemp_c : MashCalculator::defaultSpargeTemp_c
   );

   MashCalculator::Options options;
   if (spargeMethod != MashCalculator::SpargeMethod::NoSparge) {
      options.thickness_LKg = this->lineEdit_thickness->getOptCanonicalQty();
   }

   MashCalculator::Result const result = MashCalculator::calculate(inputs, steps, options);
   if (!result.ok()) {
      this->showCalculatorMessage(tr("Could not calculate the mash, so nothing has been changed:"),
                                  result.messages(MashCalculator::Problem::Severity::Error));
      return;
   }

   MashCalculator::apply(result.steps, mash);

   // Update the totals, without re-reading the other fields (which would lose any unsaved changes)
   using Base = EditorBase<MashEditor, Mash, MashEditorOptions>;
   this->Base::readFieldsFromEditItem(*PropertyNames::Mash::totalMashWater_l);
   this->Base::readFieldsFromEditItem(*PropertyNames::Mash::totalTime_mins  );

   QStringList const warnings = result.messages(MashCalculator::Problem::Severity::Warning);
   if (warnings.isEmpty()) {
      this->showCalculatorMessage(tr("Mash calculated for recipe \"%1\".").arg(recipe->name()));
   } else {
      this->showCalculatorMessage(tr("Mash calculated for recipe \"%1\", but please check the following:").arg(
                                     recipe->name()
                                  ),
                                  warnings);
   }
   return;
}
