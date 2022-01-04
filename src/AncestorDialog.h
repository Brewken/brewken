/*======================================================================================================================
 * AncestorDialog.h is part of Brewken, and is copyright the following authors 2021:
 *   • Mik Firestone <mikfire@fastmail.com>
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
#ifndef ANCESTORDIALOG_H
#define ANCESTORDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QWidget>

#include "model/Recipe.h"
#include "ui_ancestorDialog.h"

class MainWindow;

/*!
 * \class AncestorDialog
 *
 * \brief View/controller dialog for setting up ancestoral trees
 */
class AncestorDialog : public QDialog, public Ui::ancestorDialog {
   Q_OBJECT

public:
   AncestorDialog(QWidget * parent = nullptr);
   virtual ~AncestorDialog() {}

   void setAncestor(Recipe * anc);

public slots:
   void connectDescendant();
   void activateButton();
   void ancestorSelected(int ndx);

signals:
   void ancestoryChanged(Recipe * ancestor, Recipe * descendant);

private:
   MainWindow * mainWindow;

   void buildAncestorBox();
   void buildDescendantBox(Recipe * ignore);
   static bool recipeLessThan(Recipe * right, Recipe * left);
};

#endif
