/*======================================================================================================================
 * BrewNoteWidget.cpp is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Jeff Bailey <skydvr38@verizon.net>
 *   • Jonatan Pålsson <jonatan.p@gmail.com>
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
#include "BrewNoteWidget.h"

#include <QDate>
#include <QDebug>

#include "Localization.h"
#include "measurement/Measurement.h"
#include "model/BrewNote.h"
#include "PersistentSettings.h"

namespace {
   double const lowLimitPct  = 0.95;
   double const highLimitPct = 1.05;
}

BrewNoteWidget::BrewNoteWidget(QWidget *parent) : QWidget(parent) {
   setupUi(this);
   bNoteObs = 0;
   setObjectName("BrewNoteWidget");

   SMART_LINE_EDIT_INIT(BrewNoteWidget, BrewNote, lineEdit_Fg         , PropertyNames::BrewNote::fg              , *this->label_Fg         );
   SMART_LINE_EDIT_INIT(BrewNoteWidget, BrewNote, lineEdit_Og         , PropertyNames::BrewNote::og              , *this->label_Og         );
   SMART_LINE_EDIT_INIT(BrewNoteWidget, BrewNote, lineEdit_Sg         , PropertyNames::BrewNote::sg              , *this->label_Sg         );
   SMART_LINE_EDIT_INIT(BrewNoteWidget, BrewNote, lineEdit_mashFinTemp, PropertyNames::BrewNote::mashFinTemp_c   , *this->label_mashFinTemp);
   SMART_LINE_EDIT_INIT(BrewNoteWidget, BrewNote, lineEdit_pitchTemp  , PropertyNames::BrewNote::pitchTemp_c     , *this->label_pitchTemp  );
   SMART_LINE_EDIT_INIT(BrewNoteWidget, BrewNote, lineEdit_strikeTemp , PropertyNames::BrewNote::strikeTemp_c    , *this->label_strikeTemp );
   SMART_LINE_EDIT_INIT(BrewNoteWidget, BrewNote, lineEdit_finalVolume, PropertyNames::BrewNote::finalVolume_l   , *this->label_finalVolume);
   SMART_LINE_EDIT_INIT(BrewNoteWidget, BrewNote, lineEdit_postBoilVol, PropertyNames::BrewNote::postBoilVolume_l, *this->label_postBoilVol);
   SMART_LINE_EDIT_INIT(BrewNoteWidget, BrewNote, lineEdit_volIntoBk  , PropertyNames::BrewNote::volumeIntoBK_l  , *this->label_volIntoBk  );
   SMART_LINE_EDIT_INIT(BrewNoteWidget, BrewNote, lineEdit_volIntoFerm, PropertyNames::BrewNote::volumeIntoFerm_l, *this->label_volIntoFerm);

   connect(this->lineEdit_Sg,          &SmartLineEdit::textModified,   this, &BrewNoteWidget::updateSG              );
   connect(this->lineEdit_volIntoBk,   &SmartLineEdit::textModified,   this, &BrewNoteWidget::updateVolumeIntoBK_l  );
   connect(this->lineEdit_strikeTemp,  &SmartLineEdit::textModified,   this, &BrewNoteWidget::updateStrikeTemp_c    );
   connect(this->lineEdit_mashFinTemp, &SmartLineEdit::textModified,   this, &BrewNoteWidget::updateMashFinTemp_c   );
   connect(this->lineEdit_Og,          &SmartLineEdit::textModified,   this, &BrewNoteWidget::updateOG              );
   connect(this->lineEdit_postBoilVol, &SmartLineEdit::textModified,   this, &BrewNoteWidget::updatePostBoilVolume_l);
   connect(this->lineEdit_volIntoFerm, &SmartLineEdit::textModified,   this, &BrewNoteWidget::updateVolumeIntoFerm_l);
   connect(this->lineEdit_pitchTemp,   &SmartLineEdit::textModified,   this, &BrewNoteWidget::updatePitchTemp_c     );
   connect(this->lineEdit_Fg,          &SmartLineEdit::textModified,   this, &BrewNoteWidget::updateFG              );
   connect(this->lineEdit_finalVolume, &SmartLineEdit::textModified,   this, &BrewNoteWidget::updateFinalVolume_l   );
   connect(this->lineEdit_fermentDate, &QDateTimeEdit::dateChanged, this, &BrewNoteWidget::updateFermentDate     );
   connect(this->btTextEdit_brewNotes, &BtTextEdit::textModified,   this, &BrewNoteWidget::updateNotes           );

   // A few labels on this page need special handling, so I connect them here
   // instead of how we would normally do this.
   connect(this->label_projectedOg, &SmartLabel::changedSystemOfMeasurementOrScale, this, &BrewNoteWidget::updateProjOg);
///   connect(btLabel_fermentDate, &BtLabel::changedSystemOfMeasurementOrScale, this, &BrewNoteWidget::updateDateFormat);

   // I think this might work
   updateDateFormat();
   return;
}

BrewNoteWidget::~BrewNoteWidget() = default;

//.:TBD:. See comment in PitchDialog::updateProductionDate() for how we might re-implement per-field date format
// selection
// I should really do this better, but I really cannot bring myself to do
// another UnitSystem for one input field.
void BrewNoteWidget::updateDateFormat() {
//   auto dateFormat = Localization::getDateFormatForField(PersistentSettings::BrewNote::fermentDate,
//                                                         PersistentSettings::Sections::page_postferment);
   auto dateFormat = Localization::getDateFormat();
   QString format = Localization::numericToStringDateFormat(dateFormat);
   this->lineEdit_fermentDate->setDisplayFormat(format);
   return;
}


void BrewNoteWidget::updateProjOg() {
   // Density UnitSystems only have one scale, so we don't bother looking up UnitSystem::RelativeScale
   auto forcedSystemOfMeasurement =
      Measurement::getForcedSystemOfMeasurementForField(*PropertyNames::BrewNote::projOg,
                                                        *PersistentSettings::Sections::page_preboil);
   double quant = Measurement::amountDisplay(Measurement::Amount{this->bNoteObs->projOg(), Measurement::Units::sp_grav},
                                             forcedSystemOfMeasurement);
   this->lcdnumber_projectedOG->setLowLim( lowLimitPct  * quant);
   this->lcdnumber_projectedOG->setHighLim(highLimitPct * quant);

   Measurement::UnitSystem const & displayUnitSystem =
      Measurement::getUnitSystemForField(*PropertyNames::BrewNote::projOg,
                                         *PersistentSettings::Sections::page_preboil,
                                         Measurement::PhysicalQuantity::Density);
   int precision = (displayUnitSystem == Measurement::UnitSystems::density_Plato) ? 0 : 3;

   this->lcdnumber_projectedOG->display(quant, precision);
   return;
}

void BrewNoteWidget::setBrewNote(BrewNote* bNote) {

   if (this->bNoteObs) {
      disconnect(this->bNoteObs, nullptr, this, nullptr);
   }

   if (bNote) {
      this->bNoteObs = bNote;
      connect(this->bNoteObs, &NamedEntity::changed, this, &BrewNoteWidget::changed);

      // Set the highs and the lows for the lcds
      lcdnumber_effBK->setLowLim (bNoteObs->projEff_pct() * lowLimitPct);
      lcdnumber_effBK->setHighLim(bNoteObs->projEff_pct() * highLimitPct);

      lcdnumber_projectedOG->setLowLim (bNoteObs->projOg() * lowLimitPct);
      lcdnumber_projectedOG->setHighLim(bNoteObs->projOg() * highLimitPct);

      lcdnumber_brewhouseEff->setLowLim (bNoteObs->projEff_pct() * lowLimitPct);
      lcdnumber_brewhouseEff->setHighLim(bNoteObs->projEff_pct() * highLimitPct);

      lcdnumber_projABV->setLowLim (bNoteObs->projABV_pct() * lowLimitPct);
      lcdnumber_projABV->setHighLim(bNoteObs->projABV_pct() * highLimitPct);

      lcdnumber_abv->setLowLim (bNoteObs->projABV_pct() * lowLimitPct);
      lcdnumber_abv->setHighLim(bNoteObs->projABV_pct() * highLimitPct);

      lcdnumber_atten->setLowLim (bNoteObs->projAtten() * lowLimitPct);
      lcdnumber_atten->setHighLim(bNoteObs->projAtten() * highLimitPct);

      lcdnumber_projAtten->setLowLim (bNoteObs->projAtten() * lowLimitPct);
      lcdnumber_projAtten->setHighLim(bNoteObs->projAtten() * highLimitPct);

      showChanges();
   }
   return;
}

bool BrewNoteWidget::isBrewNote(BrewNote* note) {
   return this->bNoteObs == note;
}

void BrewNoteWidget::updateSG()                                { if (this->bNoteObs) { this->bNoteObs->setSg              (this->lineEdit_Sg         ->toCanonical().quantity());                      } return; }
void BrewNoteWidget::updateVolumeIntoBK_l()                    { if (this->bNoteObs) { this->bNoteObs->setVolumeIntoBK_l  (this->lineEdit_volIntoBk  ->toCanonical().quantity());                      } return; }
void BrewNoteWidget::updateStrikeTemp_c()                      { if (this->bNoteObs) { this->bNoteObs->setStrikeTemp_c    (this->lineEdit_strikeTemp ->toCanonical().quantity());                      } return; }
void BrewNoteWidget::updateMashFinTemp_c()                     { if (this->bNoteObs) { this->bNoteObs->setMashFinTemp_c   (this->lineEdit_mashFinTemp->toCanonical().quantity());                      } return; }
void BrewNoteWidget::updateOG()                                { if (this->bNoteObs) { this->bNoteObs->setOg              (this->lineEdit_Og         ->toCanonical().quantity());                      } return; }
void BrewNoteWidget::updatePostBoilVolume_l()                  { if (this->bNoteObs) { this->bNoteObs->setPostBoilVolume_l(this->lineEdit_postBoilVol->toCanonical().quantity()); this->showChanges(); } return; }
void BrewNoteWidget::updateVolumeIntoFerm_l()                  { if (this->bNoteObs) { this->bNoteObs->setVolumeIntoFerm_l(this->lineEdit_volIntoFerm->toCanonical().quantity()); this->showChanges(); } return; }
void BrewNoteWidget::updatePitchTemp_c()                       { if (this->bNoteObs) { this->bNoteObs->setPitchTemp_c     (this->lineEdit_pitchTemp  ->toCanonical().quantity()); this->showChanges(); } return; }
void BrewNoteWidget::updateFG()                                { if (this->bNoteObs) { this->bNoteObs->setFg              (this->lineEdit_Fg         ->toCanonical().quantity()); this->showChanges(); } return; }
void BrewNoteWidget::updateFinalVolume_l()                     { if (this->bNoteObs) { this->bNoteObs->setFinalVolume_l   (this->lineEdit_finalVolume   ->toCanonical().quantity());                      } return; }
void BrewNoteWidget::updateFermentDate(QDate const & datetime) { if (this->bNoteObs) { this->bNoteObs->setFermentDate     (datetime);                                                                  } return; }
void BrewNoteWidget::updateNotes()                             { if (this->bNoteObs) { this->bNoteObs->setNotes           (this->btTextEdit_brewNotes->toPlainText() );                                } return; }

void BrewNoteWidget::changed([[maybe_unused]] QMetaProperty prop,
                             [[maybe_unused]] QVariant val) {
   if (this->sender() != this->bNoteObs) {
      return;
   }

   this->showChanges();
   return;
}

void BrewNoteWidget::showChanges([[maybe_unused]] QString field) {
   if (!this->bNoteObs) {
      return;
   }

   this->lineEdit_Sg         ->setAmount   (bNoteObs->sg              ());
   this->lineEdit_volIntoBk  ->setAmount   (bNoteObs->volumeIntoBK_l  ());
   this->lineEdit_strikeTemp ->setAmount   (bNoteObs->strikeTemp_c    ());
   this->lineEdit_mashFinTemp->setAmount   (bNoteObs->mashFinTemp_c   ());
   this->lineEdit_Og         ->setAmount   (bNoteObs->og              ());
   this->lineEdit_postBoilVol->setAmount   (bNoteObs->postBoilVolume_l());
   this->lineEdit_volIntoFerm->setAmount   (bNoteObs->volumeIntoFerm_l());
   this->lineEdit_pitchTemp  ->setAmount   (bNoteObs->pitchTemp_c     ());
   this->lineEdit_Fg         ->setAmount   (bNoteObs->fg              ());
   this->lineEdit_finalVolume   ->setAmount   (bNoteObs->finalVolume_l   ());
   this->lineEdit_fermentDate->setDate     (bNoteObs->fermentDate     ());
   this->btTextEdit_brewNotes->setPlainText(bNoteObs->notes           ());

   // Now with the calculated stuff
   this->lcdnumber_effBK->display(bNoteObs->effIntoBK_pct(),2);

   // Need to think about these? Maybe use the bubbles?
   this->updateProjOg(); // this requires more work, but updateProj does it

   this->lcdnumber_brewhouseEff->display(bNoteObs->brewhouseEff_pct(), 2);
   this->lcdnumber_projABV     ->display(bNoteObs->projABV_pct     (), 2);
   this->lcdnumber_abv         ->display(bNoteObs->abv             (), 2);
   this->lcdnumber_atten       ->display(bNoteObs->attenuation     (), 2);
   this->lcdnumber_projAtten   ->display(bNoteObs->projAtten       (), 2);
   return;
}

void BrewNoteWidget::focusOutEvent([[maybe_unused]] QFocusEvent * e) {
   return;
}
