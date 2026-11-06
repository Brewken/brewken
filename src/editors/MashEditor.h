/*======================================================================================================================
 * editors/MashEditor.h is part of Brewken, and is copyright the following authors 2009-2025:
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
#ifndef EDITORS_MASHEDITOR_H
#define EDITORS_MASHEDITOR_H
#pragma once

#include <memory>

#include <QDialog>
#include <QMetaProperty>
#include <QShowEvent>
#include <QStringList>
#include <QVariant>
#include "ui_mashEditor.h"

#include "editors/EditorBase.h"
#include "model/Mash.h"

#define MashEditorOptions EditorBaseOptions{ .recipe = true, .idDisplay = true, .numRecipesUsing = true }
/*!
 * \class MashEditor
 *
 * \brief View/controller dialog for editing a mash.
 */
class MashEditor : public QDialog,
                   public Ui::mashEditor,
                   public EditorBase<MashEditor, Mash, MashEditorOptions> {
   Q_OBJECT

   EDITOR_COMMON_DECL(Mash, MashEditorOptions)

public:
   //! Called by \c EditorBase after the edit item is set
   void postSetEditItem();

   /**
    * \brief Show the "Calculate water" part of the editor, with the focus on the Calculate button.  Used when the user
    *        asks to calculate the mash from the main window.
    */
   void focusCalculator();

public slots:
   /**
    * \brief Use \c MashCalculator to work out the water amounts and temperatures (etc) for the mash being edited, for
    *        the current recipe.  If the mash is locked or shared with other recipes, we first offer to make a copy for
    *        the current recipe.
    */
   void calculate();

   //! Enable/disable the calculation controls that depend on the selected sparge method
   void updateCalculatorControls();

protected:
   virtual void showEvent(QShowEvent * event) override;

private:
   /**
    * \return The recipe whose grain bill and equipment we use to calculate the mash -- ie the current recipe, if it
    *         uses the mash being edited (or the mash is a new one for the recipe) -- or \c nullptr if there isn't one.
    */
   Recipe * recipeForCalculation() const;

   //! Set up the calculation controls for the current edit item and recipe
   void refreshCalculator();

   //! Show the result of a calculation (or why it couldn't be done)
   void showCalculatorMessage(QString const & heading, QStringList const & details = {});

   //
   // TBD: We used to have the following function invoked by a button:
   //
   //    //! Get the tun mass and specific heat from the equipment.
   //    void fromEquipment();
   //
   // But, now that we are more explicitly allowing a Mash to be shared by more than one Recipe, this makes less sense.
   // We would either have to make the button only appear when the Mash is associated with exactly one Recipe, or we
   // would need to have some more complicated control to allow the user to select (either directly or indirectly) the
   // Equipment from which to copy the info.
   //
};

#endif
