/*======================================================================================================================
 * ingredientDialogs/MiscDialog.h is part of Brewken, and is copyright the following authors 2009-2023:
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
#ifndef INGREDIENTDIALOGS_MISCDIALOG_H
#define INGREDIENTDIALOGS_MISCDIALOG_H
#pragma once

#include <QDialog>
#include <QEvent>

#include "editors/MiscEditor.h"
#include "ingredientDialogs/IngredientDialog.h"
#include "model/Misc.h"

// Forward declarations.
class MainWindow;
class MiscTableModel;
class MiscSortFilterProxyModel;

/*!
 * \class MiscDialog
 *
 * \brief View/controller class for showing/editing the list of miscs in the database.
 */
class MiscDialog : public QDialog, public IngredientDialog<Misc,
                                                           MiscDialog,
                                                           MiscTableModel,
                                                           MiscSortFilterProxyModel,
                                                           MiscEditor> {
   Q_OBJECT

public:
   MiscDialog(MainWindow* parent);
   virtual ~MiscDialog();

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
