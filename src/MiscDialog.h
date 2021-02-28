/**
 * MiscDialog.h is part of Brewken, and is copyright the following authors 2009-2015:
 *   • Daniel Pettersson <pettson81@gmail.com>
 *   • Jeff Bailey <skydvr38@verizon.net>
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
 */
#ifndef MISCDIALOG_H
#define MISCDIALOG_H

#include <QWidget>
#include <QDialog>
#include <QEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableView>
#include <QSpacerItem>
#include <QPushButton>

// Forward declarations.
class MainWindow;
class MiscEditor;
class MiscTableModel;
class MiscSortFilterProxyModel;

/*!
 * \class MiscDialog
 *
 *
 * \brief View/controller dialog for the miscs in the database.
 */
class MiscDialog : public QDialog
{
   Q_OBJECT

public:
   MiscDialog(MainWindow* parent);
   virtual ~MiscDialog() {}

   //! \name Public UI Variables
   //! @{
   QVBoxLayout *verticalLayout;
   QTableView *tableWidget;
   QHBoxLayout *horizontalLayout;
   QLineEdit *qLineEdit_searchBox;
   QSpacerItem *horizontalSpacer;
   QPushButton *pushButton_addToRecipe;
   QPushButton *pushButton_new;
   QPushButton *pushButton_edit;
   QPushButton *pushButton_remove;
   //! @}

   void newMisc(QString folder);
public slots:
   //! Add the selected misc to the current recipe.
   void addMisc(const QModelIndex& = QModelIndex());
   //! Delete selected misc from the database.
   void removeMisc();
   //! Bring up the editor for the selected misc.
   void editSelected();
   //! Add a new misc to the database.
   void newMisc();
   //! Filter out the matching miscs.
   void filterMisc(QString searchExpression);

protected:

   virtual void changeEvent(QEvent* event)
   {
      if(event->type() == QEvent::LanguageChange)
         retranslateUi();
      QDialog::changeEvent(event);
   }

private:
   MainWindow* mainWindow;
   MiscTableModel* miscTableModel;
   MiscSortFilterProxyModel* miscTableProxy;
   int numMiscs;
   MiscEditor* miscEdit;

   void doLayout();
   void retranslateUi();
};

#endif   /* MISCDIALOG_H */
