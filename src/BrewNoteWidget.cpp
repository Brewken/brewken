/*======================================================================================================================
 * BrewNoteWidget.cpp is part of Brewken, and is copyright the following authors 2009-2021:
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

#include "Brewken.h"
#include "measurement/Measurement.h"
#include "model/BrewNote.h"
#include "PersistentSettings.h"

BrewNoteWidget::BrewNoteWidget(QWidget *parent) : QWidget(parent) {
   setupUi(this);
   bNoteObs = 0;
   setObjectName("BrewNoteWidget");

   connect(lineEdit_SG, &BtLineEdit::textModified, this, &BrewNoteWidget::updateSG);
   connect(lineEdit_volIntoBK, &BtLineEdit::textModified, this, &BrewNoteWidget::updateVolumeIntoBK_l);
   connect(lineEdit_strikeTemp, &BtLineEdit::textModified, this, &BrewNoteWidget::updateStrikeTemp_c);
   connect(lineEdit_mashFinTemp, &BtLineEdit::textModified, this, &BrewNoteWidget::updateMashFinTemp_c);

   connect(lineEdit_OG, &BtLineEdit::textModified, this, &BrewNoteWidget::updateOG);
   connect(lineEdit_postBoilVol, &BtLineEdit::textModified, this, &BrewNoteWidget::updatePostBoilVolume_l);
   connect(lineEdit_volIntoFerm, &BtLineEdit::textModified, this, &BrewNoteWidget::updateVolumeIntoFerm_l);
   connect(lineEdit_pitchTemp, &BtLineEdit::textModified, this, &BrewNoteWidget::updatePitchTemp_c);

   connect(lineEdit_FG, &BtLineEdit::textModified, this, &BrewNoteWidget::updateFG);
   connect(lineEdit_finalVol, &BtLineEdit::textModified, this, &BrewNoteWidget::updateFinalVolume_l);
   connect(lineEdit_fermentDate, &QDateTimeEdit::dateChanged, this, &BrewNoteWidget::updateFermentDate);

   connect(btTextEdit_brewNotes, &BtTextEdit::textModified, this, &BrewNoteWidget::updateNotes);

   // A few labels on this page need special handling, so I connect them here
   // instead of how we would normally do this.
   connect(btLabel_projectedOg, &BtLabel::changedUnitSystemOrScale, this, &BrewNoteWidget::updateProjOg);
///   connect(btLabel_fermentDate, &BtLabel::changedUnitSystemOrScale, this, &BrewNoteWidget::updateDateFormat);

   // I think this might work
///   updateDateFormat(Measurement::Unit::noUnit, Measurement::UnitSystem::noScale);
}

BrewNoteWidget::~BrewNoteWidget() = default;

////.:TODO:. REINSTATE
// I should really do this better, but I really cannot bring myself to do
// another UnitSystem for one input field.
/*void BrewNoteWidget::updateDateFormat(Measurement::Unit::unitDisplay display,Measurement::UnitSystem::RelativeScale scale) {
   QString format;
   // I need the new unit, not the old
   Measurement::Unit::unitDisplay unitDsp = static_cast<Measurement::Unit::unitDisplay>(
      PersistentSettings::value(PropertyNames::BrewNote::fermentDate,
                                Brewken::getDateFormat(),
                                PersistentSettings::Sections::page_postferment,
                                PersistentSettings::UNIT).toInt()
   );

   switch(unitDsp) {
      case Measurement::Unit::displayUS:
         format = "MM-dd-yyyy";
         break;
      case Measurement::Unit::displayImp:
         format = "dd-MM-yyyy";
         break;
      case Measurement::Unit::displaySI:
      default:
         format = "yyyy-MM-dd";
   }
   lineEdit_fermentDate->setDisplayFormat(format);
   return;
}*/


void BrewNoteWidget::updateProjOg(Measurement::UnitSystem const * oldUnitSystem, Measurement::UnitSystem::RelativeScale oldScale) {
   double low  = 0.95;
   double high = 1.05;
   int precision = 3;

   // I don't think we care about the old unit or scale, just the new ones
   auto displayUnitSystem = Measurement::getUnitSystemForField(*PropertyNames::BrewNote::projOg,
                                                               *PersistentSettings::Sections::page_preboil);


   if (nullptr == displayUnitSystem) {
      displayUnitSystem = &Measurement::getDisplayUnitSystem(Measurement::Volume);
   }

   if (*displayUnitSystem == Measurement::UnitSystems::density_Plato) {
      precision = 0;
   }

   double quant = Measurement::amountDisplay(bNoteObs,
                                             page_preboil,
                                             PropertyNames::BrewNote::projOg,
                                             &Measurement::Units::sp_grav);
   this->lcdnumber_projectedOG->setLowLim( low  * quant);
   this->lcdnumber_projectedOG->setHighLim(high * quant);
   this->lcdnumber_projectedOG->display(quant, precision);
   return;
}

void BrewNoteWidget::setBrewNote(BrewNote* bNote) {
   double low = 0.95;
   double high = 1.05;

   if( bNoteObs != 0 )
      disconnect( bNoteObs, 0, this, 0 );

   if ( bNote )
   {
      bNoteObs = bNote;
      connect( bNoteObs, &NamedEntity::changed, this, &BrewNoteWidget::changed );

      // Set the highs and the lows for the lcds
      lcdnumber_effBK->setLowLim(bNoteObs->projEff_pct() * low);
      lcdnumber_effBK->setHighLim(bNoteObs->projEff_pct() * high);

      lcdnumber_projectedOG->setLowLim( bNoteObs->projOg() * low);
      lcdnumber_projectedOG->setHighLim( bNoteObs->projOg() * high);

      lcdnumber_brewhouseEff->setLowLim(bNoteObs->projEff_pct() * low);
      lcdnumber_brewhouseEff->setHighLim(bNoteObs->projEff_pct() * high);

      lcdnumber_projABV->setLowLim( bNoteObs->projABV_pct() * low);
      lcdnumber_projABV->setHighLim( bNoteObs->projABV_pct() * high);

      lcdnumber_abv->setLowLim( bNoteObs->projABV_pct() * low);
      lcdnumber_abv->setHighLim( bNoteObs->projABV_pct() * high);

      lcdnumber_atten->setLowLim( bNoteObs->projAtten() * low );
      lcdnumber_atten->setHighLim( bNoteObs->projAtten() * high );

      lcdnumber_projAtten->setLowLim( bNoteObs->projAtten() * low );
      lcdnumber_projAtten->setHighLim( bNoteObs->projAtten() * high );

      showChanges();
   }
   return;
}

bool BrewNoteWidget::isBrewNote(BrewNote* note) {
   return bNoteObs == note;
}

void BrewNoteWidget::updateSG() {
   if (bNoteObs == 0) {
      return;
   }

   bNoteObs->setSg(lineEdit_SG->toSI());
   return;
}

void BrewNoteWidget::updateVolumeIntoBK_l() {
   if (bNoteObs == 0) {
      return;
   }

   bNoteObs->setVolumeIntoBK_l(lineEdit_volIntoBK->toSI());
   return;
}

void BrewNoteWidget::updateStrikeTemp_c() {
   if (bNoteObs == 0) {
      return;
   }

   bNoteObs->setStrikeTemp_c(lineEdit_strikeTemp->toSI());
   return;
}

void BrewNoteWidget::updateMashFinTemp_c() {
   if (bNoteObs == 0) {
      return;
   }

   bNoteObs->setMashFinTemp_c(lineEdit_mashFinTemp->toSI());
   return;
}

void BrewNoteWidget::updateOG() {
   if (bNoteObs == 0) {
      return;
   }

   bNoteObs->setOg(lineEdit_OG->toSI());
   return;
}

void BrewNoteWidget::updatePostBoilVolume_l() {
   if (bNoteObs == 0) {
      return;
   }

   bNoteObs->setPostBoilVolume_l(lineEdit_postBoilVol->toSI());
   showChanges();
   return;
}

void BrewNoteWidget::updateVolumeIntoFerm_l() {
   if (bNoteObs == 0) {
      return;
   }

   bNoteObs->setVolumeIntoFerm_l(lineEdit_volIntoFerm->toSI());
   showChanges();
   return;
}

void BrewNoteWidget::updatePitchTemp_c() {
   if (bNoteObs == 0) {
      return;
   }

   bNoteObs->setPitchTemp_c(lineEdit_pitchTemp->toSI());
   showChanges();
   return;
}

void BrewNoteWidget::updateFG() {
   if (bNoteObs == 0) {
      return;
   }

   bNoteObs->setFg(lineEdit_FG->toSI());
   showChanges();
   return;
}

void BrewNoteWidget::updateFinalVolume_l() {
   if (bNoteObs == 0) {
      return;
   }

   bNoteObs->setFinalVolume_l(lineEdit_finalVol->toSI());
//   showChanges();
   return;
}

void BrewNoteWidget::updateFermentDate(QDate const & datetime) {
   if (bNoteObs == 0) {
      return;
   }

   bNoteObs->setFermentDate(datetime);
   return;
}

void BrewNoteWidget::updateNotes() {
   if (bNoteObs == 0) {
      return;
   }

   bNoteObs->setNotes(btTextEdit_brewNotes->toPlainText() );
   return;
}

void BrewNoteWidget::changed(QMetaProperty /*prop*/, QVariant /*val*/) {
   if ( sender() != bNoteObs ) {
      return;
   }

   showChanges();
   return;
}

void BrewNoteWidget::showChanges(QString field) {
   if (bNoteObs == 0) {
      return;
   }

   lineEdit_SG->setText(bNoteObs);
   lineEdit_volIntoBK->setText(bNoteObs);
   lineEdit_strikeTemp->setText(bNoteObs);
   lineEdit_mashFinTemp->setText(bNoteObs);
   lineEdit_OG->setText(bNoteObs);
   lineEdit_postBoilVol->setText(bNoteObs);
   lineEdit_volIntoFerm->setText(bNoteObs);
   lineEdit_pitchTemp->setText(bNoteObs);
   lineEdit_FG->setText(bNoteObs);
   lineEdit_finalVol->setText(bNoteObs);

   lineEdit_fermentDate->setDate(bNoteObs->fermentDate());
   btTextEdit_brewNotes->setPlainText(bNoteObs->notes());

   // Now with the calculated stuff
   lcdnumber_effBK->display(bNoteObs->effIntoBK_pct(),2);

   // Need to think about these? Maybe use the bubbles?
   this->updateProjOg(nullptr, Measurement::UnitSystem::noScale); // this requires more work, but updateProj does it

   lcdnumber_brewhouseEff->display(bNoteObs->brewhouseEff_pct(),2);
   lcdnumber_projABV->display(bNoteObs->projABV_pct(),2);
   lcdnumber_abv->display(bNoteObs->abv(),2);
   lcdnumber_atten->display(bNoteObs->attenuation(),2);
   lcdnumber_projAtten->display(bNoteObs->projAtten(),2);
   return;
}

void BrewNoteWidget::focusOutEvent(QFocusEvent *e) {
   return;
}
