/*======================================================================================================================
 * ingredientDialogs/HopDialog.h is part of Brewken, and is copyright the following authors 2009-2023:
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
#ifndef INGREDIENTDIALOGS_HOPDIALOG_H
#define INGREDIENTDIALOGS_HOPDIALOG_H
#pragma once

#include <QDialog>
#include <QEvent>

#include "editors/HopEditor.h"
#include "ingredientDialogs/IngredientDialog.h"
#include "model/Hop.h"

// Forward declarations.
class MainWindow;
class HopTableModel;
class HopSortFilterProxyModel;

/*!
 * \class HopDialog
 *
 * \brief View/controller class for showing/editing the list of hops in the database.
 */
class HopDialog : public QDialog, public IngredientDialog<Hop,
                                                          HopDialog,
                                                          HopTableModel,
                                                          HopSortFilterProxyModel,
                                                          HopEditor> {
   Q_OBJECT

public:
   HopDialog(MainWindow* parent);
   virtual ~HopDialog();

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
