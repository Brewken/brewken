/*======================================================================================================================
 * editors/FermentationStepEditor.h is part of Brewken, and is copyright the following authors 2024:
 *   • Matt Young <mfsy@yahoo.com>
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
#ifndef EDITORS_FERMENTATIONSTEPEDITOR_H
#define EDITORS_FERMENTATIONSTEPEDITOR_H
#pragma once

#include <QDialog>

#include "ui_fermentationStepEditor.h"

#include "editors/EditorBase.h"

// Forward declarations.
class FermentationStep;

/*!
 * \class FermentationStepEditor
 *
 * \brief View/controller dialog for editing fermentation steps.
 */
class FermentationStepEditor : public QDialog,
                               public Ui::fermentationStepEditor,
                               public EditorBase<FermentationStepEditor, FermentationStep> {
   Q_OBJECT

   EDITOR_COMMON_DECL(FermentationStep)

};

#endif
