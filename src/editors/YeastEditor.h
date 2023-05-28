/*======================================================================================================================
 * editors/YeastEditor.h is part of Brewken, and is copyright the following authors 2009-2023:
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
#ifndef EDITORS_YEASTEDITOR_H
#define EDITORS_YEASTEDITOR_H
#pragma once

#include "ui_yeastEditor.h"

#include <QDialog>
#include <QMetaProperty>
#include <QVariant>

#include "editors/EditorBase.h"

// Forward declarations.
class Yeast;

/*!
 * \class YeastEditor
 *
 * \brief View/controller dialog for modifying yeasts.
 *
 *        See comment on EditorBase::connectSignalsAndSlots for why we need to have \c public, not \c private
 *        inheritance from the Ui base.
 */
class YeastEditor : public QDialog, public Ui::yeastEditor, public EditorBase<Yeast, YeastEditor> {
   Q_OBJECT

public:
   YeastEditor(QWidget * parent = nullptr);
   virtual ~YeastEditor();

   void writeFieldsToEditItem();
   void writeLateFieldsToEditItem();
   void readFieldsFromEditItem(std::optional<QString> propName);

public slots:
   // Standard editor slots
   void save();
   void clearAndClose();
   void changed(QMetaProperty, QVariant);
   void clickedNew();
};

#endif
