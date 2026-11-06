/*======================================================================================================================
 * MashWizard.cpp is part of Brewken, and is copyright the following authors 2009-2026:
 *   • Adam Hawes <ach@hawes.net.au>
 *   • Brian Rower <brian.rower@gmail.com>
 *   • David Grundberg <individ@acc.umu.se>
 *   • Greg Meess <Daedalus12@gmail.com>
 *   • Jeff Bailey <skydvr38@verizon.net>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Rob Taylor <robtaylor@floopily.org>
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
#include "MashWizard.h"

#include <QButtonGroup>
#include <QMessageBox>

#include "MashCalculator.h"
#include "measurement/Measurement.h"
#include "model/Mash.h"
#include "model/MashStep.h"
#include "widgets/LockAndShareBar.h"

#ifdef MANUALLY_INCLUDE_MOC
   // Explicitly doing this include reduces potential problems with AUTOMOC when compiling with CMake
   #include "moc_MashWizard.cpp"
#endif

MashWizard::MashWizard(QWidget* parent) :
   QDialog(parent) {
   this->setupUi(this);
   this->m_bGroup = new QButtonGroup();
   this->m_bGroup->addButton(radioButton_noSparge);
   this->m_bGroup->addButton(radioButton_batchSparge);
   this->m_bGroup->addButton(radioButton_flySparge);

//   radioButton_batchSparge->setChecked(true);

   connect(this->m_bGroup , QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this, &MashWizard::toggleSpinBox);
   connect(this->buttonBox, &QDialogButtonBox::accepted,                                    this, &MashWizard::wizardry);
   connect(this->buttonBox, &QDialogButtonBox::rejected,                                    this, &QWidget::close);
   return;
}

void MashWizard::toggleSpinBox(QAbstractButton* button) {
   if (button == radioButton_noSparge) {
      this->widget_batches->setEnabled(false);
      this->widget_mashThickness->setEnabled(false);
   } else if (button == radioButton_flySparge) {
      this->widget_batches->setEnabled(false);
      this->widget_mashThickness->setEnabled(true);
   } else {
      this->widget_batches->setEnabled(true);
      this->widget_mashThickness->setEnabled(true);
   }
   return;
}

void MashWizard::setRecipe(Recipe* rec) {
   m_recObs = rec;
   return;
}

void MashWizard::show() {
   // Don't show if there is no Recipe or no Mash!
   if (!this->m_recObs || !this->m_recObs->mash()) {
      return;
   }

   // Ensure at least one mash step.
   if (this->m_recObs->mash()->mashSteps().size() == 0) {
      QMessageBox::information(this, tr("No steps"), tr("There must be at least one mash step to run the wizard."));
      return;
   }

   Measurement::getThicknessUnits(&this->m_volumeUnit, &this->m_weightUnit);
   this->label_mashThickness->setText(
      tr("Mash thickness (%1/%2)").arg(m_volumeUnit->getDisplayAbbreviation(), m_weightUnit->getDisplayAbbreviation())
   );

   auto firstStep = m_recObs->mash()->mashSteps().first();
   auto lastStep  = m_recObs->mash()->mashSteps().last();

   //
   // Recalculate the mash thickness.  As in MashCalculator, this is the thickness of the grain bed, so does not include
   // any water that is in the mash tun outside the grain bed (see Equipment::strikeWaterAllowance_l).
   //
   double thickNum = MashCalculator::defaultThickness_LKg;
   if (firstStep->amount_l() > 0.0) {
      thickNum = m_recObs->mashThicknessForStrikeWater_LKg(firstStep->amount_l()).value_or(thickNum);
   }
   if (thickNum <= 0.0) {
      thickNum = MashCalculator::defaultThickness_LKg;
   }
   double const thickness = thickNum * m_weightUnit->toCanonical(1).quantity / m_volumeUnit->toCanonical(1).quantity ;
   this->doubleSpinBox_thickness->setValue(thickness);

   // Is this a batch, fly or no sparge?
   if (firstStep == lastStep) {
      this->radioButton_noSparge->setChecked(true);
      this->widget_batches->setEnabled(false);
      this->widget_mashThickness->setEnabled(false);
   } else if (lastStep->type() == MashStep::Type::FlySparge) {
      this->radioButton_flySparge->setChecked(true);
      this->widget_batches->setEnabled(false);
      this->widget_mashThickness->setEnabled(true);
   } else {
      int const numSpargeSteps = m_recObs->mash()->numMashStepsMatching(
         [](MashStep const & ms) { return ms.isSparge(); }
      );

      this->radioButton_batchSparge->setChecked(true);
      this->widget_batches->setEnabled(true);
      this->widget_mashThickness->setEnabled(true);
      this->spinBox_batches->setValue(numSpargeSteps);
   }

   this->setVisible(true);
   return;
}

void MashWizard::wizardry() {
   if (!this->m_recObs || !this->m_recObs->mash()) {
      return;
   }
   std::shared_ptr<Mash> const recipeMash = this->m_recObs->mash();

   //
   // All the calculations are done by MashCalculator.  We just need to tell it what the user chose here.
   //
   QAbstractButton const * const checkedButton = this->m_bGroup->checkedButton();
   MashCalculator::SpargeMethod const spargeMethod =
      checkedButton == radioButton_noSparge    ? MashCalculator::SpargeMethod::NoSparge    :
      checkedButton == radioButton_batchSparge ? MashCalculator::SpargeMethod::BatchSparge :
                                                 MashCalculator::SpargeMethod::FlySparge;

   QList<MashCalculator::Step> steps = MashCalculator::stepsFrom(*recipeMash);
   MashCalculator::setSparge(steps,
                             spargeMethod,
                             this->spinBox_batches->value(),
                             recipeMash->spargeTemp_c().value_or(MashCalculator::defaultSpargeTemp_c));

   MashCalculator::Options options;
   if (spargeMethod != MashCalculator::SpargeMethod::NoSparge) {
      // The thickness spin box is in the user's units
      options.thickness_LKg = this->doubleSpinBox_thickness->value() *
                              this->m_volumeUnit->toCanonical(1.0).quantity /
                              this->m_weightUnit->toCanonical(1.0).quantity;
   }

   MashCalculator::Result const result =
      MashCalculator::calculate(MashCalculator::Inputs::fromRecipe(*this->m_recObs), steps, options);
   if (!result.ok()) {
      QMessageBox::information(this,
                               tr("Cannot calculate mash"),
                               result.messages(MashCalculator::Problem::Severity::Error).join("\n\n"));
      return;
   }

   //
   // The wizard changes the steps of the mash, using the recipe's grain bill etc.  We mustn't do this if the mash is
   // shared with other recipes (as the results would be wrong for them) or is locked (including because it's used by a
   // locked recipe), so, in that case, we offer to make a copy.  (MashCalculator::apply works on the copy, as it has
   // the same steps as the original.)
   //
   std::shared_ptr<Mash> mash = LockAndShare::getChangeable<Mash>(
      this,
      *this->m_recObs,
      tr("The Mash Wizard changes the amounts and temperatures of the steps of the recipe's mash, based on this "
         "recipe's grain bill and equipment.")
   );
   if (!mash) {
      return;
   }

   MashCalculator::apply(result.steps, *mash);

   QStringList const warnings = result.messages(MashCalculator::Problem::Severity::Warning);
   if (!warnings.isEmpty()) {
      QMessageBox::information(this, tr("Please check the mash"), warnings.join("\n\n"));
   }
   return;
}
