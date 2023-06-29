/*======================================================================================================================
 * editors/MashStepEditor.h is part of Brewken, and is copyright the following authors 2009-2023:
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
#ifndef EDITORS_MASHSTEPEDITOR_H
#define EDITORS_MASHSTEPEDITOR_H
#pragma once

#include <memory>

#include <QDialog>
#include <QWidget>
#include <QMetaProperty>
#include <QVariant>

#include "ui_mashStepEditor.h"

#include "editors/EditorBase.h"

// Forward declarations.
class MashStep;
class Mash;

/*!
 * \class MashStepEditor
 *
 * \brief View/controller dialog for editing mash steps.
 */
class MashStepEditor : public QDialog, public Ui::mashStepEditor, public EditorBase<MashStep, MashStepEditor> {
   Q_OBJECT

   EDITOR_COMMON_DECL(MashStep)

///public:
///   MashStepEditor(QWidget* parent=nullptr);
///   virtual ~MashStepEditor() {}
///
///   void setMashStep(std::shared_ptr<MashStep> step);

public slots:
///   void saveAndClose();
///   //! View/edit the given mash step.
///   void close();
   /*!
    * Grays out irrelevant portions of the dialog.
    * \param text - one of {"Infusion","Decoction","Temperature"} describing the mash step.
    */
   void grayOutStuff(const QString& text);
///   void changed(QMetaProperty, QVariant);

///private:
///   /*! Updates the UI elements effected by the \b metaProp of
///    *  the step we are watching. If \b metaProp is null,
///    *  then update all the UI elements at once.
///    */
///   void showChanges(QMetaProperty* metaProp = 0);
///   void clear();
///   std::shared_ptr<MashStep> obs;
};

#endif
