/*======================================================================================================================
 * PrintAndPreviewDialog.h is part of Brewken, and is copyright the following authors 2021:
 *   • Mattias Måhl <mattias@kejsarsten.com>
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
#ifndef PRINTANDPREVIEWDIALOG_H
#define PRINTANDPREVIEWDIALOG_H
#pragma once
#include "ui_BtPrintAndPreview.h"

#include <QDialog>
#include <QMap>
#include <QPageSize>
#include <QPrinter>
#include <QPrintPreviewWidget>
#include <QString>
#include <QTextBrowser>
#include <QWidget>

#include "BrewDayFormatter.h"
#include "MainWindow.h"
#include "model/Recipe.h"
#include "RecipeFormatter.h"


/*!
 * \class PrintAndPreviewDialog
 *
 * \brief Handle all printing and saving as PDF/HTML.
 */
class PrintAndPreviewDialog : public QDialog, private Ui::BtPrintAndPreview {
   Q_OBJECT

public:
   /**
    * @brief Construct a new Print And Preview Dialog object
    *
    * @param parent
    */
   PrintAndPreviewDialog(MainWindow *parent);

   /**
    * @brief Destroy the Print And Preview Dialog object
    */
   virtual ~PrintAndPreviewDialog();

   /**
    * @brief shows the dialog and updates the Printing preview.
    *
    * @param e
    */
   virtual void showEvent(QShowEvent *e);

public slots:
   void printDocument(QPrinter * printer);
   void outputRadioButtonsClicked();
   void orientationRadioButtonsClicked();
   void selectedPrinterChanged(int index);
   void selectedPaperChanged(int index);
   void resetAndClose(bool checked);
   void checkBoxRecipe_toggle(bool checked);
   void checkBoxBrewday_toggle(bool checked);
   void checkBoxInventoryAll_toggle(bool checked);
   void checkBoxInventoryIngredient_toggle(bool checked);
   void verticalTabWidget_currentChanged(int index);

private:

   /**
    * @brief Sets allt signal connections for the controls in this Dialog
    */
   void setupConnections();

   /**
    * @brief Adds the Preview controls to the Dialog including QPrintPreviewWidgen as well as QTextBrowser for HTML
    *        preview.  By default QPrintPreviewWidget is visible.
    */
   void setupPreviewWidgets();

   /**
    * @brief Set the Print selection objects according to selected output
    */
   void setPrintingControls();

   /**
    * @brief collects all available printers on the system and stores them into the printer selector Combobox.
    */
   void collectPrinterInfo();

   /**
    * @brief collects all available paper sizes for the selected printer and stores them into the Paper size selector
    *        Combobox.
    */
   void collectSupportedPageSizes();

   /**
    * @brief generates a list of Page sizes for use in Dialog when there is no printer installed on the system.
    *
    * @return QList<QPageSize>
    */
   QList<QPageSize> generatePageSizeList();

   /**
    * @brief collects the selected recipe from the Mainwindow.
    */
   void collectRecipe();

   /**
    * @brief Handles the printing. sends to the selected output format.
    */
   void handlePrinting();

   /**
    * @brief Updates the preview to the currently set options.
    */
   void updatePreview();

   QPrintPreviewWidget* previewWidget;
   RecipeFormatter* recipeFormatter;
   BrewDayFormatter* brewDayFormatter;
   MainWindow *mainWindow;
   Recipe *selectedRecipe;
   QPrinter * printer = nullptr;
   QMap<QString, QPageSize> PageSizeMap;
   QTextBrowser *htmlDocument;
   QPageSize currentlySelectedPageSize;

};
#endif
