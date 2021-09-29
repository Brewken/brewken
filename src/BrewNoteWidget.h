/*======================================================================================================================
 * BrewNoteWidget.h is part of Brewken, and is copyright the following authors 2009-2021:
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
#ifndef BREWNOTEWIDGET_H
#define BREWNOTEWIDGET_H
#pragma once

#include <QWidget>
#include <QDialog>
#include <QMetaProperty>
#include <QVariant>
#include "ui_brewNoteWidget.h"

// Forward declarations.
class BrewNote;

/*!
 * \class BrewNoteWidget
 *
 * \brief View/controller widget that edits BrewNotes.
 */
class BrewNoteWidget : public QWidget, public Ui::brewNoteWidget
{
    Q_OBJECT

public:
   BrewNoteWidget(QWidget *parent = 0);
   virtual ~BrewNoteWidget() {}

   void setBrewNote(BrewNote* bNote);
   bool isBrewNote(BrewNote* note);

   void focusOutEvent(QFocusEvent *e);

public slots:
   void updateSG();
   void updateVolumeIntoBK_l();
   void updateStrikeTemp_c();
   void updateMashFinTemp_c();

   void updateOG();
   void updatePostBoilVolume_l();
   void updateVolumeIntoFerm_l();
   void updatePitchTemp_c();

   void updateFG();
   void updateFinalVolume_l();
   void updateFermentDate(QDate const & datetime);
   void updateDateFormat(Unit::unitDisplay display,Unit::RelativeScale scale);

   void updateNotes();
//   void saveAll();

   void changed(QMetaProperty,QVariant);
   void showChanges(QString field = "");

   void updateProjOg(Unit::unitDisplay oldUnit, Unit::RelativeScale oldScale);


private:
   BrewNote* bNoteObs;


};

#endif
