/*======================================================================================================================
 * ingredientDialogs/FermentableDialog.h is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Daniel Pettersson <pettson81@gmail.com>
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
#ifndef INGREDIENTDIALOGS_FERMENTABLEDIALOG_H
#define INGREDIENTDIALOGS_FERMENTABLEDIALOG_H
#pragma once

#include <QDialog>
#include <QEvent>

#include "editors/FermentableEditor.h"
#include "ingredientDialogs/IngredientDialog.h"
#include "model/Fermentable.h"

// Forward declarations.
class MainWindow;
class FermentableTableModel;
class FermentableSortFilterProxyModel;

/*!
 * \class FermentableDialog
 *
 * \brief View/controller class that shows the list of fermentables in the database.
 */
class FermentableDialog : public QDialog, public IngredientDialog<Fermentable,
                                                                  FermentableDialog,
                                                                  FermentableTableModel,
                                                                  FermentableSortFilterProxyModel,
                                                                  FermentableEditor> {
   Q_OBJECT

public:
   FermentableDialog(MainWindow* parent);
   virtual ~FermentableDialog();

public slots:
   void addIngredient(QModelIndex const & index);
   void removeIngredient();
   void editSelected();
   void newIngredient();
   void filterIngredients(QString searchExpression);

protected:
   virtual void changeEvent(QEvent* event);
};

#endif
