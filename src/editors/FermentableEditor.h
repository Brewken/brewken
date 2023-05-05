/*======================================================================================================================
 * editors/FermentableEditor.h is part of Brewken, and is copyright the following authors 2009-2023:
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
#ifndef EDITORS_FERMENTABLEEDITOR_H
#define EDITORS_FERMENTABLEEDITOR_H
#pragma once

#include "ui_fermentableEditor.h"

#include <QDialog>
#include <QMetaProperty>
#include <QString>

// Forward declarations.
class Fermentable;

/*!
 * \class FermentableEditor
 *
 * \brief Fermentable view/controller dialog that allows you to edit Fermentables.
 */
class FermentableEditor : public QDialog, private Ui::fermentableEditor {
   Q_OBJECT

public:
   FermentableEditor(QWidget *parent=nullptr);
   virtual ~FermentableEditor();
   void setFermentable(Fermentable* f);
   void newFermentable(QString folder);

public slots:
   void save();
   void clearAndClose();
   void clickedNewFermentable();
   void setIsWeight(bool state);

private:
   Fermentable* obsFerm;
   /*! Updates the UI elements effected by the \b metaProp of
    *  the fermentable we are watching. If \b metaProp is null,
    *  then update all the UI elements at once.
    */
   void showChanges(QMetaProperty* metaProp = nullptr);
};

#endif
