/*======================================================================================================================
 * MainWindow.h is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Aidan Roberts <aidanr67@gmail.com>
 *   • Dan Cavanagh <dan@dancavanagh.com>
 *   • Daniel Pettersson <pettson81@gmail.com>
 *   • Jeff Bailey <skydvr38@verizon.net>
 *   • Mark de Wever <koraq@xs4all.nl>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Maxime Lavigne <duguigne@gmail.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Ryan Hoobler <rhoob@yahoo.com>
 *   • Samuel Östling <MrOstling@gmail.com>
 *   • Ted Wright <tedwright@users.sourceforge.net>
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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#pragma once

#include <functional>
#include <memory> // For PImpl

#include <QCloseEvent>
#include <QMainWindow>
#include <QPalette>
#include <QPrintDialog>
#include <QPrinter>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <QWidget>

#include "ui_mainWindow.h"
#include "undoRedo/SimpleUndoableUpdate.h"
#include "utils/NoCopy.h"

// Forward Declarations

class BrewNoteWidget;
class OptionDialog;
class PropertyPath;
class Recipe;

class RecipeAdditionHop;

/*!
 * \class MainWindow
 *
 * \brief Brewken's main window. This is a view/controller class.
 */
class MainWindow : public QMainWindow, public Ui::mainWindow {
   Q_OBJECT

   friend class OptionDialog;
public:
   MainWindow(QWidget* parent=nullptr);
   virtual ~MainWindow();

   //
   // This is a short-term trick to save me adding .get() to lots of calls
   //
   using QObject::connect;
   template<typename A, typename B, typename C, typename D>
   void connect(std::unique_ptr<A> & a, B b, C c, D d) {
      this->connect(a.get(), b, c, d);
   }
   template<typename A, typename B, typename C, typename D>
   void connect(A a, B b, std::unique_ptr<C> & c, D d) {
      this->connect(a, b, c.get(), d);
   }

   static MainWindow & instance();
   /**
    * \brief Call at program termination to clean-up.  Caller's responsibility not to subsequently call (or use the
    *        return value from) \c MainWindow::instance().
    */
   static void DeleteMainWindow();

   /**
    * \brief This needs to be called immediately after the constructor.  It does the remaining initialisation of the
    *        object.  This function cannot be called from the constructor as, in certain circumstances, it will invoke
    *        code that calls Application::mainWindow() which returns a pointer to the MainWindow and therefore needs the
    *        MainWindow constructor to have returned!
    */
   void init();

   //! \brief Get the currently observed recipe.
   Recipe* currentRecipe();

   bool verifyImport(QString tag, QString name);
   bool verifyDelete(QString tab, QString name);

   void setBrewNoteByIndex(const QModelIndex &index);
   void setBrewNote(BrewNote* bNote);

   //! \brief Doing updates via this method makes them undoable (and redoable).  This is the simplified version
   //         which suffices for modifications to most individual non-relational attributes.
   void doOrRedoUpdate(NamedEntity & updatee,
                       TypeInfo const & typeInfo,
                       QVariant newValue,
                       QString const & description,
                       QUndoCommand * parent = nullptr);

   /**
    * \brief This version of \c doOrRedoUpdate is needed when updating a property that has (or might have) a non-trivial
    *        \c PropertyPath
    */
   void doOrRedoUpdate(NamedEntity & updatee,
                       PropertyPath const & propertyPath,
                       TypeInfo const & typeInfo,
                       QVariant newValue,
                       QString const & description,
                       QUndoCommand * parent = nullptr);

   /**
    * \brief Add given \c Fermentable / \c Hop / \c Misc / \c Yeast to the Recipe
    *
    *        Fortunately this does not need to be a slot function (as slots cannot be templated)
    */
   template<class NE> void addToRecipe(std::shared_ptr<NE> ne);

   void addFermentableToRecipe(Fermentable * fermentable);
   void addHopToRecipe(Hop * hop);

public slots:

   //! \brief Accepts Recipe changes, and takes appropriate action to show the changes.
   void changed(QMetaProperty,QVariant);

   void treeActivated(const QModelIndex &index);
   //! \brief View the given recipe.
   void setRecipe(Recipe* recipe);

   //! \brief Update Recipe name to that given by the relevant widget.
   void updateRecipeName();
   //! \brief Redisplay the OG, FG, etc ranges for the style of the current recipe.  Needs to be called whenever the recipe style is changed.
   void displayRangesEtcForCurrentRecipeStyle();
   //! \brief Update Recipe Style to that given by the relevant widget.
   void updateRecipeStyle();
   //! \brief Update Recipe Equipment to that given by the relevant widget.
   void updateRecipeEquipment();
   //! \brief Update Recipe batch size to that given by the relevant widget.
   void updateRecipeBatchSize();
   //! \brief Update Recipe boil size to that given by the relevant widget.
   void updateRecipeBoilSize();
   //! \brief Update Recipe boil time to that given by the relevant widget.
   void updateRecipeBoilTime();
   //! \brief Update Recipe efficiency to that given by the relevant widget.
   void updateRecipeEfficiency();
   //! \brief Update Recipe's mash
   void updateRecipeMash();

   //! \brief Update the main windows statusbar.
   void updateStatus(const QString status);

   //! \brief Close a brewnote tab if we must (because of the BrewNote being deleted)
   void closeBrewNote(int brewNoteId, std::shared_ptr<QObject> object);

   //! \brief Remove selected Fermentable(s) from the Recipe.
   void removeSelectedFermentableAddition();
   //! \brief Edit selected Fermentable.
   void editFermentableOfSelectedFermentableAddition();

   //! \brief Show the pitch dialog.
   void showPitchDialog();

   /**
    * \brief Remove selected Hop addition(s) from the Recipe.
    *
    *        The name is a bit cumbersome, but is more accurate than, say, `removeSelectedHop`.  You might have the same
    *        hop added at two different points, and this is only removing one of those additions.
    */
   void removeSelectedHopAddition();
   /**
    * \brief Edit the Hop in the selected Hop addition.
    */
   void editHopOfSelectedHopAddition();

   //! \brief Remove selected Misc(s) from the Recipe.
   void removeSelectedMisc();
   //! \brief Edit selected Misc.
   void editSelectedMisc();

   //! \brief Remove selected Yeast(s) from the Recipe.
   void removeSelectedYeast();
   //! \brief Edit selected Yeast
   void editSelectedYeast();

   //! \brief Invoke the pop-up Window to add a new mash step to (the mash of) the recipe.
   void addMashStep();
   //! \brief Actually add the new mash step to (the mash of) the recipe (in an undoable way).
   void addMashStepToMash(std::shared_ptr<MashStep> mashStep);
   //! \brief Move currently selected mash step down.
   void moveSelectedMashStepUp();
   //! \brief Move currently selected mash step up.
   void moveSelectedMashStepDown();
   //! \brief Remove currently selected mash step.
   void removeSelectedMashStep();
   //! \brief Edit currently selected mash step.
   void editSelectedMashStep();
   //! \brief Set the current recipe's mash to the one selected in the mash combo box.
   //void setMashToCurrentlySelected();
   //! \brief Save the current recipe's mash to be used in other recipes.
   void saveMash();
   //! \brief Remove the current mash from the recipe, and replace with a blank one.
   void removeMash();

   //! \brief Create a new recipe in the database.
   void newRecipe();
   //! \brief Export current recipe to BeerXML.
   void exportRecipe();
   //! \brief Display file selection dialog and import BeerXML files.
   void importFiles();
   //! \brief Create a duplicate of the current recipe.
   void copyRecipe();

   //! \brief Implements "> Edit > Undo"
   void editUndo();
   //! \brief Implements "> Edit > Redo"
   void editRedo();

   //! \brief Create a new folder
   void newFolder();
   void renameFolder();

   void deleteSelected();
   void copySelected();
   void exportSelected();

   //! \brief Backup the database.
   void backup();
   //! \brief Restore the database.
   void restoreFromBackup();

   //! \brief makes sure we can do water chemistry before we show the window
   void popChemistry();

   //! \brief draws a context menu, the exact nature of which depends on which
   //tree is focused
   void contextMenu(const QPoint &point);
   //! \brief creates a new brewnote
   void newBrewNote();
   //! \brief copies an existing brewnote to a new brewday
   void reBrewNote();
   void brewItHelper();
   void brewAgainHelper();
   void reduceInventory();
   void changeBrewDate();
   void fixBrewNote();

   void redisplayLabel();

   void showEquipmentEditor();
   void showStyleEditor();

   void updateEquipmentButton();

   //! \brief Set all the things based on a drop event
   void droppedRecipeEquipment(Equipment *kit);
   void droppedRecipeStyle(Style *style);
   void droppedRecipeFermentable(QList<Fermentable*>ferms);
   void droppedRecipeHop(QList<Hop*>hops);
   void droppedRecipeMisc(QList<Misc*>miscs);
   void droppedRecipeYeast(QList<Yeast*>yeasts);

   void versionedRecipe(Recipe* descendant);

   //! \brief Doing updates via this method makes them undoable (and redoable).  This is the most generic version
   //         which requires the caller to construct a QUndoCommand.
   void doOrRedoUpdate(QUndoCommand * update);

    //! \brief to lock or not was never the question before now.
   void lockRecipe(int state);
   //! \brief prepopulate the ancestorDialog when the menu is selected
   void setAncestor();

   /*!
    * \brief Make the widgets in the window update changes.
    *
    * Updates all the widgets with info about the currently
    * selected Recipe, except for the tables.
    *
    * \param prop Not yet used. Will indicate which Recipe property has changed.
    */
   void showChanges(QMetaProperty* prop = nullptr);

protected:
   virtual void closeEvent(QCloseEvent* event);

private slots:
   //! \brief Set whether undo / redo commands are enabled
   void setUndoRedoEnable();

private:
   // Private implementation details - see https://herbsutter.com/gotw/_100/
   class impl;
   std::unique_ptr<impl> pimpl;

   // Insert all the usual boilerplate to prevent copy/assignment/move
   NO_COPY_DECLARATIONS(MainWindow)

   // TODO: At the moment, these need to be in MainWindow itself rather than in the pimpl because of the way function
   // pointers get passed to UndoableAddOrRemove.  We should fix that at some point.
   void removeHopAddition        (std::shared_ptr<RecipeAdditionHop        > itemToRemove);
   void removeFermentableAddition(std::shared_ptr<RecipeAdditionFermentable> itemToRemove);
   void removeMisc(std::shared_ptr<Misc> itemToRemove);
   void removeYeast(std::shared_ptr<Yeast> itemToRemove);
   void removeMashStep(std::shared_ptr<MashStep> itemToRemove);

   Recipe* recipeObs;
   // TBD: (MY 2020-11-24) Not sure whether we need to store recipe style (since it ought to be available from the
   //      recipe) or whether this is just for convenience.
   Style* recStyle;
   Equipment* recEquip;

   QString highSS, lowSS, goodSS, boldSS; // Palette replacements

   QList<QMenu*> contextMenus;
   QDialog* brewDayDialog;
   QPrinter *printer;

   int confirmDelete;

   //! \brief Fix pixel dimensions according to dots-per-inch (DPI) of screen we're on.
   void setSizesInPixelsBasedOnDpi();

   //! \brief Find an open brewnote tab, if it is open
   BrewNoteWidget* findBrewNoteWidget(BrewNote* b);

   //! \brief Scroll to the given \c item in the currently visible item tree.
   void setTreeSelection(QModelIndex item);

   //! \brief Set the keyboard shortcuts.
   void setupShortCuts();
   //! \brief Set the BtTreeView context menus.
   void setupContextMenu();
   //! \brief Create the CSS strings
   void setupCSS();
   //! \brief Configure the range sliders
   void setupRanges();
   //! \brief Restore any saved states
   void restoreSavedState();
   //! \brief Connect the signal/slots for actions
   void setupTriggers();
   //! \brief Connect signal/slots for buttons
   void setupClicks();
   //! \brief Connect signal/slots for combo boxes
   void setupActivate();
   //! \brief Connect signal/slots for combo boxes
   void setupLabels();
   //! \brief Connect signal/slots for text edits
   void setupTextEdit();
   //! \brief Connect signal/slots drag/drop
   void setupDrops();
   //! \brief Connect signal/slots for check boxes
   void setUpStateChanges();

};

#endif
