/*======================================================================================================================
 * ingredientDialogs/YeastDialog.h is part of Brewken, and is copyright the following authors 2009-2023:
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
#ifndef INGREDIENTDIALOGS_YEASTDIALOG_H
#define INGREDIENTDIALOGS_YEASTDIALOG_H
#pragma once

#include <QDialog>
#include <QEvent>

#include "editors/YeastEditor.h"
#include "ingredientDialogs/IngredientDialog.h"
#include "model/Yeast.h"

// Forward declarations.
class MainWindow;
class YeastTableModel;
class YeastSortFilterProxyModel;

/*!
 * \class YeastDialog
 *
 * \brief View/controller class for showing/editing the list of yeasts in the database.
 */
class YeastDialog : public QDialog, public IngredientDialog<Yeast,
                                                            YeastDialog,
                                                            YeastTableModel,
                                                            YeastSortFilterProxyModel,
                                                            YeastEditor> {
   Q_OBJECT

public:
   YeastDialog(MainWindow* parent);
   virtual ~YeastDialog();

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
