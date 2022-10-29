/*======================================================================================================================
 * MainWindow.cpp is part of Brewken, and is copyright the following authors 2009-2022:
 *   • Aidan Roberts <aidanr67@gmail.com>
 *   • A.J. Drobnich <aj.drobnich@gmail.com>
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Dan Cavanagh <dan@dancavanagh.com>
 *   • Daniel Moreno <danielm5@users.noreply.github.com>
 *   • Daniel Pettersson <pettson81@gmail.com>
 *   • David Grundberg <individ@acc.umu.se>
 *   • Jonatan Pålsson <jonatan.p@gmail.com>
 *   • Kregg Kemper <gigatropolis@yahoo.com>
 *   • Mark de Wever <koraq@xs4all.nl>
 *   • Markus Mårtensson <mackan.90@gmail.com>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Maxime Lavigne <duguigne@gmail.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Ryan Hoobler <rhoob@yahoo.com>
 *   • Samuel Östling <MrOstling@gmail.com>
 *   • Ted Wright <tedwright@users.sourceforge.net>
 *   • Théophane Martin <theophane.m@gmail.com>
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
#include "MainWindow.h"

#if defined(Q_OS_WIN)
   #include <windows.h>
#endif

#include <memory>
#include <mutex> // For std::once_flag etc

#include <QAction>
#include <QBrush>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QDomNodeList>
#include <QFile>
#include <QFileDialog>
#include <QIcon>
#include <QInputDialog>
#include <QIODevice>
#include <QLinearGradient>
#include <QLineEdit>
#include <QList>
#include <QMainWindow>
#include <QMessageBox>
#include <QNetworkReply>
#include <QPen>
#include <QPixmap>
#include <QSize>
#include <QString>
#include <QTextStream>
#include <QtGui>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

#include "AboutDialog.h"
#include "AlcoholTool.h"
#include "Algorithms.h"
#include "AncestorDialog.h"
#include "Brewken.h"
#include "BrewNoteWidget.h"
#include "BtDatePopup.h"
#include "BtDigitWidget.h"
#include "BtFolder.h"
#include "BtHorizontalTabs.h"
#include "BtTabWidget.h"
#include "config.h"
#include "ConverterTool.h"
#include "database/Database.h"
#include "database/ObjectStoreWrapper.h"
#include "EquipmentEditor.h"
#include "EquipmentListModel.h"
#include "FermentableDialog.h"
#include "FermentableEditor.h"
#include "FermentableSortFilterProxyModel.h"
#include "HelpDialog.h"
#include "HopDialog.h"
#include "HopEditor.h"
#include "HopSortFilterProxyModel.h"
#include "Html.h"
#include "HydrometerTool.h"
#include "ImportExport.h"
#include "InventoryFormatter.h"
#include "MashDesigner.h"
#include "MashEditor.h"
#include "MashListModel.h"
#include "MashStepEditor.h"
#include "MashWizard.h"
#include "measurement/Unit.h"
#include "MiscDialog.h"
#include "MiscEditor.h"
#include "MiscSortFilterProxyModel.h"
#include "measurement/Measurement.h"
#include "model/BrewNote.h"
#include "model/Equipment.h"
#include "model/Fermentable.h"
#include "model/Mash.h"
#include "model/Recipe.h"
#include "model/Style.h"
#include "model/Yeast.h"
#include "NamedMashEditor.h"
#include "OgAdjuster.h"
#include "OptionDialog.h"
#include "PersistentSettings.h"
#include "PitchDialog.h"
#include "PrimingDialog.h"
#include "PrintAndPreviewDialog.h"
#include "RangedSlider.h"
#include "RecipeFormatter.h"
#include "RefractoDialog.h"
#include "RelationalUndoableUpdate.h"
#include "ScaleRecipeTool.h"
#include "StrikeWaterDialog.h"
#include "StyleEditor.h"
#include "StyleListModel.h"
#include "StyleSortFilterProxyModel.h"
#include "tableModels/FermentableTableModel.h"
#include "tableModels/HopTableModel.h"
#include "tableModels/MashStepTableModel.h"
#include "tableModels/MiscTableModel.h"
#include "tableModels/YeastTableModel.h"
#include "TimerMainDialog.h"
#include "UndoableAddOrRemove.h"
#include "UndoableAddOrRemoveList.h"
#include "utils/BtStringConst.h"
#include "utils/OptionalToStream.h"
#include "WaterDialog.h"
#include "WaterEditor.h"
#include "WaterListModel.h"
#include "YeastDialog.h"
#include "YeastEditor.h"
#include "YeastSortFilterProxyModel.h"

namespace {

   /**
    * \brief Generates the pop-up you see when you hover over the Brewken image above the trees, which is supposed to
    *        show the database type you are connected to, and some useful information with respect to that database.
    */
   QString getLabelToolTip() {
      Database const & database = Database::instance();
      QString toolTip{};
      QTextStream toolTipAsStream{&toolTip};
      toolTipAsStream <<
         "<html><head><style type=\"text/css\">" << Html::getCss(":/css/tooltip.css") << "</style></head>"
         "<body>"
         "<div id=\"headerdiv\">"
         "<table id=\"tooltip\">"
         "<caption>Using " << DatabaseHelper::getNameFromDbTypeName(database.dbType()) << "</caption>";
      auto connectionParms = database.displayableConnectionParms();
      for (auto parm : connectionParms) {
         toolTipAsStream <<
            "<tr><td class=\"left\">" << parm.first << ": </td><td class=\"value\">" << parm.second << "</td>";
      }
      toolTipAsStream << "</table></body></html>";
      return toolTip;
   }

   // We only want one instance of MainWindow, but we'd also like to be able to delete it when the program shuts down
   MainWindow * mainWindowInstance = nullptr;

   void createMainWindowInstance() {
      mainWindowInstance = new MainWindow();
      return;
   }
}

// This private implementation class holds all private non-virtual members of MainWindow
class MainWindow::impl {
public:

   impl(MainWindow & self) :
      self{self},
      fileOpener{} {
      return;
   }

   ~impl() = default;


   // TODO Try making this a smart pointer
   HelpDialog * helpDialog;

private:
   MainWindow & self;
   QFileDialog* fileOpener;
};


MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), pimpl{std::make_unique<impl>(*this)} {
   qDebug() << Q_FUNC_INFO;

   undoStack = new QUndoStack(this);

   // Need to call this parent class method to get all the widgets added (I think).
   this->setupUi(this);

   // Stop things looking ridiculously tiny on high DPI displays
   this->setSizesInPixelsBasedOnDpi();

   // Horizontal tabs, please
   tabWidget_Trees->tabBar()->setStyle(new BtHorizontalTabs);

   /* PLEASE DO NOT REMOVE.
    This code is left here, commented out, intentionally. The only way I can
    test internationalization is by forcing the locale manually. I am tired
    of having to figure this out every time I need to test.
    PLEASE DO NOT REMOVE.
   QLocale german(QLocale::German,QLocale::Germany);
   QLocale::setDefault(german);
   */

   // If the database doesn't load, we bail
   if (! Database::instance().loadSuccessful() )
      exit(1);

   // Set the window title.
   setWindowTitle( QString("Brewken - %1").arg(VERSIONSTRING) );

   // Null out the recipe
   recipeObs = nullptr;

   // Set up the printer
   printer = new QPrinter;
#if QT_VERSION < QT_VERSION_CHECK(5,15,0)
   printer->setPageSize(QPrinter::Letter);
#else
   printer->setPageSize(QPageSize(QPageSize::Letter));
#endif
   return;
}

void MainWindow::init() {
   qDebug() << Q_FUNC_INFO;
   this->setupCSS();
   // initialize all of the dialog windows
   this->setupDialogs();
   // initialize the ranged sliders
   this->setupRanges();
   // the dialogs have to be setup before this is called
   this->setupComboBoxes();
   // do all the work to configure the tables models and their proxies
   this->setupTables();
   // Create the keyboard shortcuts
   this->setupShortCuts();
   // Once more with the context menus too
   this->setupContextMenu();
   // do all the work for checkboxes (just one right now)
   this->setUpStateChanges();

   // This sets up things that might have been 'remembered' (ie stored in the config file) from a previous run of the
   // program - eg window size, which is stored in MainWindow::closeEvent().
   // Breaks the naming convention, doesn't it?
   this->restoreSavedState();

   // Connect menu item slots to triggered() signals
   this->setupTriggers();
   // Connect pushbutton slots to clicked() signals
   this->setupClicks();
   // connect combobox slots to activate() signals
   this->setupActivate();
   // connect signal slots for the line edits
   this->setupTextEdit();
   // connect the remaining labels
   this->setupLabels();
   // set up the drag/drop parts
   this->setupDrops();

   // Moved from Database class
   Recipe::connectSignals();
   qDebug() << Q_FUNC_INFO << "Recipe signals connected";
   Mash::connectSignals();
   qDebug() << Q_FUNC_INFO << "Mash signals connected";

   // I do not like this connection here.
   connect(ancestorDialog, &AncestorDialog::ancestoryChanged, treeView_recipe->model(), &BtTreeModel::versionedRecipe);
   connect(optionDialog, &OptionDialog::showAllAncestors, treeView_recipe->model(), &BtTreeModel::catchAncestors);
   connect(treeView_recipe, &BtTreeView::recipeSpawn, this, &MainWindow::versionedRecipe);

   // No connections from the database yet? Oh FSM, that probably means I'm
   // doing it wrong again.
   // .:TODO:. Change this so we use the newer deleted signal!
   connect(&ObjectStoreTyped<BrewNote>::getInstance(), &ObjectStoreTyped<BrewNote>::signalObjectDeleted, this, &MainWindow::closeBrewNote);

   // Set up the pretty tool tip. It doesn't really belong anywhere, so here it is
   // .:TODO:. When we allow users to change databases without restarting, we'll need to make sure to call this whenever
   // the databae is changed (as setToolTip() just takes static text as its parameter).
   label_Brewken->setToolTip(getLabelToolTip());

   qDebug() << Q_FUNC_INFO << "MainWindow initialisation complete";
   return;
}


// See https://herbsutter.com/gotw/_100/ for why we need to explicitly define the destructor here (and not in the header file)
MainWindow::~MainWindow() = default;

MainWindow & MainWindow::instance() {
   //
   // Since C++11, there is a standard thread-safe way to ensure a function is called exactly once
   //
   static std::once_flag initFlag_MainWindow;

   std::call_once(initFlag_MainWindow, createMainWindowInstance);

   Q_ASSERT(mainWindowInstance);
   return *mainWindowInstance;
}

void MainWindow::DeleteMainWindow() {
   delete mainWindowInstance;
   mainWindowInstance = nullptr;
}

void MainWindow::setSizesInPixelsBasedOnDpi()
{
   //
   // Default icon sizes are fine for low DPI monitors, but need changing on high-DPI systems.
   //
   // Fortunately, the icons are already SVGs, so we don't need to do anything more complicated than tell Qt what size
   // in pixels to render them.
   //
   // For the moment, we assume we don't need to change the icon size after set-up.  (In theory, it would be nice
   // to detect, on a multi-monitor system, whether we have moved from a high DPI to a low DPI screen or vice versa.
   // See https://doc.qt.io/qt-5/qdesktopwidget.html#screen-geometry for more on this.
   // But, for now, TBD how important a use case that is.  Perhaps a future enhancement...)
   //
   // Low DPI monitors are 72 or 96 DPI typically.  High DPI monitors can be 168 DPI (as reported by logicalDpiX(),
   // logicalDpiX()).  Default toolbar icon size of 22×22 looks fine on low DPI monitor.  So it seems 1/4-inch is a
   // good width and height for these icons.  Therefore divide DPI by 4 to get icon size.
   //
   auto dpiX = this->logicalDpiX();
   auto dpiY = this->logicalDpiY();
   qDebug() << QString("Logical DPI: %1,%2.  Physical DPI: %3,%4")
      .arg(dpiX)
      .arg(dpiY)
      .arg(this->physicalDpiX())
      .arg(this->physicalDpiY());
   auto defaultToolBarIconSize = this->toolBar->iconSize();
   qDebug() << QString("Default toolbar icon size: %1,%2")
      .arg(defaultToolBarIconSize.width())
      .arg(defaultToolBarIconSize.height());
   this->toolBar->setIconSize(QSize(dpiX/4,dpiY/4));

   //
   // Historically, tab icon sizes were, by default, smaller (16×16), but it seems more logical for them to be the same
   // size as the toolbar ones.
   //
   auto defaultTabIconSize = this->tabWidget_Trees->iconSize();
   qDebug() << QString("Default tab icon size: %1,%2")
      .arg(defaultTabIconSize.width())
      .arg(defaultTabIconSize.height());
   this->tabWidget_Trees->setIconSize(QSize(dpiX/4,dpiY/4));

   //
   // Default logo size is 100×30 pixels, which is actually the wrong aspect ratio for the underlying image (currently
   // 265 × 66 - ie aspect ratio of 4.015:1).
   //
   // Setting height to be 1/3 inch seems plausible for the default size, but looks a bit wrong in practice.  Using 1/2
   // height looks better.  Then width 265/66 × height.  (Note that we actually put the fraction in double literals to
   // avoid premature rounding.)
   //
   // This is a bit more work to implement because its a PNG image in a QLabel object
   //
   qDebug() << QString("Logo default size: %1,%2").arg(this->label_Brewken->width()).arg(this->label_Brewken->height());
   this->label_Brewken->setScaledContents(true);
   this->label_Brewken->setFixedSize((265.0/66.0) * dpiX/2,  // width = 265/66 × height = 265/66 × half an inch = (265/66) × (dpiX/2)
                                        dpiY/2);                // height = half an inch = dpiY/2
   qDebug() << QString("Logo new size: %1,%2").arg(this->label_Brewken->width()).arg(this->label_Brewken->height());

   return;
}


// Setup the keyboard shortcuts
void MainWindow::setupShortCuts()
{
   actionNewRecipe->setShortcut(QKeySequence::New);
   actionCopy_Recipe->setShortcut(QKeySequence::Copy);
   actionDeleteSelected->setShortcut(QKeySequence::Delete);
   actionUndo->setShortcut(QKeySequence::Undo);
   actionRedo->setShortcut(QKeySequence::Redo);
}

void MainWindow::setUpStateChanges()
{
   connect( checkBox_locked, &QCheckBox::stateChanged, this, &MainWindow::lockRecipe );
}

// Any manipulation of CSS for the MainWindow should be in here
void MainWindow::setupCSS()
{
   // Different palettes for some text. This is all done via style sheets now.
   QColor wPalette = tabWidget_recipeView->palette().color(QPalette::Active,QPalette::Base);

   //
   // NB: Using pixels for font sizes in Qt is bad because, given the significant variations in pixels-per-inch (aka
   // dots-per-inch / DPI) between "normal" and "high DPI" displays, a size specified in pixels will most likely be
   // dramatically wrong on some displays.  The simple solution is instead to use points (which are device independent)
   // to specify font size.
   //
   goodSS = QString( "QLineEdit:read-only { color: #008800; background: %1 }").arg(wPalette.name());
   lowSS  = QString( "QLineEdit:read-only { color: #0000D0; background: %1 }").arg(wPalette.name());
   highSS = QString( "QLineEdit:read-only { color: #D00000; background: %1 }").arg(wPalette.name());
   boldSS = QString( "QLineEdit:read-only { font: bold 10pt; color: #000000; background: %1 }").arg(wPalette.name());

   // The bold style sheet doesn't change, so set it here once.
   lineEdit_boilSg->setStyleSheet(boldSS);

   // Disabled fields should change color, but not become unreadable. Mucking
   // with the css seems the most reasonable way to do that.
   QString tabDisabled = QString("QWidget:disabled { color: #000000; background: #F0F0F0 }");
   tab_recipe->setStyleSheet(tabDisabled);
   tabWidget_ingredients->setStyleSheet(tabDisabled);

}

// Most dialogs are initialized in here. That should include any initial
// configurations as well
void MainWindow::setupDialogs()
{
   dialog_about = new AboutDialog(this);
   this->pimpl->helpDialog = new HelpDialog(this);
   equipEditor = new EquipmentEditor(this);
   singleEquipEditor = new EquipmentEditor(this, true);
   fermDialog = new FermentableDialog(this);
   fermEditor = new FermentableEditor(this);
   hopDialog = new HopDialog(this);
   hopEditor = new HopEditor(this);
   mashEditor = new MashEditor(this);
   mashStepEditor = new MashStepEditor(this);
   mashWizard = new MashWizard(this);
   miscDialog = new MiscDialog(this);
   miscEditor = new MiscEditor(this);
   styleEditor = new StyleEditor(this);
   singleStyleEditor = new StyleEditor(this,true);
   yeastDialog = new YeastDialog(this);
   yeastEditor = new YeastEditor(this);
   optionDialog = new OptionDialog(this);
   recipeScaler = new ScaleRecipeTool(this);
   recipeFormatter = new RecipeFormatter(this);
   printAndPreviewDialog = new PrintAndPreviewDialog(this);
   ogAdjuster = new OgAdjuster(this);
   converterTool = new ConverterTool(this);
   hydrometerTool = new HydrometerTool(this);
   alcoholTool = new AlcoholTool(this);
   timerMainDialog = new TimerMainDialog(this);
   primingDialog = new PrimingDialog(this);
   strikeWaterDialog = new StrikeWaterDialog(this);
   refractoDialog = new RefractoDialog(this);
   mashDesigner = new MashDesigner(this);
   pitchDialog = new PitchDialog(this);
   btDatePopup = new BtDatePopup(this);

   waterDialog = new WaterDialog(this);
   waterEditor = new WaterEditor(this);

   ancestorDialog = new AncestorDialog(this);


}

// Configures the range widgets for the bubbles
void MainWindow::setupRanges()
{
   styleRangeWidget_og->setRange(1.000, 1.120);
   styleRangeWidget_og->setPrecision(3);
   styleRangeWidget_og->setTickMarks(0.010, 2);

   styleRangeWidget_fg->setRange(1.000, 1.030);
   styleRangeWidget_fg->setPrecision(3);
   styleRangeWidget_fg->setTickMarks(0.010, 2);

   styleRangeWidget_abv->setRange(0.0, 15.0);
   styleRangeWidget_abv->setPrecision(1);
   styleRangeWidget_abv->setTickMarks(1, 2);

   styleRangeWidget_ibu->setRange(0.0, 120.0);
   styleRangeWidget_ibu->setPrecision(1);
   styleRangeWidget_ibu->setTickMarks(10, 2);

   // definitely cheating, but I don't feel like making a whole subclass just to support this
   // or the next.
   rangeWidget_batchsize->setRange(0, recipeObs == nullptr ? 19.0 : recipeObs->batchSize_l());
   rangeWidget_batchsize->setPrecision(1);
   rangeWidget_batchsize->setTickMarks(2,5);

   rangeWidget_batchsize->setBackgroundBrush(QColor(255,255,255));
   rangeWidget_batchsize->setPreferredRangeBrush(QColor(55,138,251));
   rangeWidget_batchsize->setMarkerBrush(QBrush(Qt::NoBrush));

   rangeWidget_boilsize->setRange(0, recipeObs == nullptr? 24.0 : recipeObs->boilVolume_l());
   rangeWidget_boilsize->setPrecision(1);
   rangeWidget_boilsize->setTickMarks(2,5);

   rangeWidget_boilsize->setBackgroundBrush(QColor(255,255,255));
   rangeWidget_boilsize->setPreferredRangeBrush(QColor(55,138,251));
   rangeWidget_boilsize->setMarkerBrush(QBrush(Qt::NoBrush));

   const int srmMax = 50;
   styleRangeWidget_srm->setRange(0.0, static_cast<double>(srmMax));
   styleRangeWidget_srm->setPrecision(1);
   styleRangeWidget_srm->setTickMarks(10, 2);
   // Need to change appearance of color slider
   {
      // The styleRangeWidget_srm should display beer color in the background
      QLinearGradient grad( 0,0, 1,0 );
      grad.setCoordinateMode(QGradient::ObjectBoundingMode);
      for( int i=0; i <= srmMax; ++i )
      {
         double srm = i;
         grad.setColorAt( srm/static_cast<double>(srmMax), Algorithms::srmToColor(srm));
      }
      styleRangeWidget_srm->setBackgroundBrush(grad);

      // The styleRangeWidget_srm should display a "window" to show acceptable colors for the style
      styleRangeWidget_srm->setPreferredRangeBrush(QColor(0,0,0,0));
      styleRangeWidget_srm->setPreferredRangePen(QPen(Qt::black, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

      // Half-height "tick" for color marker
      grad = QLinearGradient( 0,0, 0,1 );
      grad.setCoordinateMode(QGradient::ObjectBoundingMode);
      grad.setColorAt( 0, QColor(255,255,255,255) );
      grad.setColorAt( 0.49, QColor(255,255,255,255) );
      grad.setColorAt( 0.50, QColor(255,255,255,0) );
      grad.setColorAt( 1, QColor(255,255,255,0) );
      styleRangeWidget_srm->setMarkerBrush(grad);
   }
}

// Any new combo boxes, along with their list models, should be initialized
// here
void MainWindow::setupComboBoxes()
{
   // Set equipment combo box model.
   equipmentListModel = new EquipmentListModel(equipmentComboBox);
   equipmentComboBox->setModel(equipmentListModel);

   // Set the style combo box
   styleListModel = new StyleListModel(styleComboBox);
   styleProxyModel = new StyleSortFilterProxyModel(styleComboBox);
   styleProxyModel->setDynamicSortFilter(true);
   styleProxyModel->setSortLocaleAware(true);
   styleProxyModel->setSourceModel(styleListModel);
   styleProxyModel->sort(0);
   styleComboBox->setModel(styleProxyModel);

   // Set the mash combo box
   mashListModel =  new MashListModel(mashComboBox);
   mashComboBox->setModel(mashListModel);

   // Nothing to say.
   namedMashEditor = new NamedMashEditor(this, mashStepEditor);
   // I don't think this is used yet
   singleNamedMashEditor = new NamedMashEditor(this,mashStepEditor,true);
}

// Anything creating new tables models, filter proxies and configuring the two
// should go in here
void MainWindow::setupTables()
{
   // Set table models.
   // Fermentables
   fermTableModel = new FermentableTableModel(fermentableTable);
   fermTableProxy = new FermentableSortFilterProxyModel(fermentableTable,false);
   fermTableProxy->setSourceModel(fermTableModel);
   fermentableTable->setItemDelegate(new FermentableItemDelegate(fermentableTable));
   fermentableTable->setModel(fermTableProxy);
   // Make the fermentable table show grain percentages in row headers.
   fermTableModel->setDisplayPercentages(true);
   // Double clicking the name column pops up an edit dialog for the selected item
   connect( fermentableTable, &QTableView::doubleClicked, this, [&](const QModelIndex &idx) {
       if (idx.column() == 0)
           MainWindow::editSelectedFermentable();
   });

   // Hops
   hopTableModel = new HopTableModel(hopTable);
   hopTableProxy = new HopSortFilterProxyModel(hopTable, false);
   hopTableProxy->setSourceModel(hopTableModel);
   hopTable->setItemDelegate(new HopItemDelegate(hopTable));
   hopTable->setModel(hopTableProxy);
   // Hop table show IBUs in row headers.
   hopTableModel->setShowIBUs(true);
   connect( hopTable, &QTableView::doubleClicked, this, [&](const QModelIndex &idx) {
       if (idx.column() == 0)
           MainWindow::editSelectedHop();
   });

   // Misc
   miscTableModel = new MiscTableModel(miscTable);
   miscTableProxy = new MiscSortFilterProxyModel(miscTable,false);
   miscTableProxy->setSourceModel(miscTableModel);
   miscTable->setItemDelegate(new MiscItemDelegate(miscTable));
   miscTable->setModel(miscTableProxy);
   connect( miscTable, &QTableView::doubleClicked, this, [&](const QModelIndex &idx) {
       if (idx.column() == 0)
           MainWindow::editSelectedMisc();
   });

   // Yeast
   yeastTableModel = new YeastTableModel(yeastTable);
   yeastTableProxy = new YeastSortFilterProxyModel(yeastTable,false);
   yeastTableProxy->setSourceModel(yeastTableModel);
   yeastTable->setItemDelegate(new YeastItemDelegate(yeastTable));
   yeastTable->setModel(yeastTableProxy);
   connect( yeastTable, &QTableView::doubleClicked, this, [&](const QModelIndex &idx) {
       if (idx.column() == 0)
           MainWindow::editSelectedYeast();
   });

   // Mashes
   mashStepTableModel = new MashStepTableModel(mashStepTableWidget);
   mashStepTableWidget->setItemDelegate(new MashStepItemDelegate());
   mashStepTableWidget->setModel(mashStepTableModel);
   connect( mashStepTableWidget, &QTableView::doubleClicked, this, [&](const QModelIndex &idx) {
       if (idx.column() == 0)
           MainWindow::editSelectedMashStep();
   });

   // Enable sorting in the main tables.
   fermentableTable->horizontalHeader()->setSortIndicator( FERMAMOUNTCOL, Qt::DescendingOrder );
   fermentableTable->setSortingEnabled(true);
   fermTableProxy->setDynamicSortFilter(true);
   hopTable->horizontalHeader()->setSortIndicator( HOPTIMECOL, Qt::DescendingOrder );
   hopTable->setSortingEnabled(true);
   hopTableProxy->setDynamicSortFilter(true);
   miscTable->horizontalHeader()->setSortIndicator( MISCUSECOL, Qt::DescendingOrder );
   miscTable->setSortingEnabled(true);
   miscTableProxy->setDynamicSortFilter(true);
   yeastTable->horizontalHeader()->setSortIndicator( YEASTNAMECOL, Qt::DescendingOrder );
   yeastTable->setSortingEnabled(true);
   yeastTableProxy->setDynamicSortFilter(true);
}

// Anything resulting in a restoreState() should go in here
void MainWindow::restoreSavedState() {

   // If we saved a size the last time we ran, use it
   if (PersistentSettings::contains(PersistentSettings::Names::geometry)) {
      restoreGeometry(PersistentSettings::value(PersistentSettings::Names::geometry).toByteArray());
      restoreState(PersistentSettings::value(PersistentSettings::Names::windowState).toByteArray());
   } else {
      // otherwise, guess a reasonable size at 1/4 of the screen.
      QDesktopWidget *desktop = QApplication::desktop();
      int width = desktop->width();
      int height = desktop->height();
      this->resize(width/2,height/2);

      // Or we could do the same in one line:
      // this->resize(QDesktopWidget().availableGeometry(this).size() * 0.5);
   }

   // If we saved the selected recipe name the last time we ran, select it and show it.
   int key = -1;
   if (PersistentSettings::contains(PersistentSettings::Names::recipeKey)) {
      key = PersistentSettings::value(PersistentSettings::Names::recipeKey).toInt();
   } else {
      auto firstRecipeWeFind = ObjectStoreTyped<Recipe>::getInstance().findFirstMatching(
         // This trivial lambda gives us the first recipe in the list, if there is one
         [](std::shared_ptr<Recipe> obj) {return true;}
      );
      if (firstRecipeWeFind) {
         key = firstRecipeWeFind.value()->key();
      }
   }
   if ( key > -1 ) {
      this->recipeObs = ObjectStoreWrapper::getByIdRaw<Recipe>(key);
      QModelIndex rIdx = treeView_recipe->findElement(recipeObs);

      setRecipe(recipeObs);
      setTreeSelection(rIdx);
   }

   // UI restore state
   if (PersistentSettings::contains(PersistentSettings::Names::splitter_vertical_State,
                                    PersistentSettings::Sections::MainWindow)) {
      splitter_vertical->restoreState(PersistentSettings::value(PersistentSettings::Names::splitter_vertical_State,
                                                                QVariant(),
                                                                PersistentSettings::Sections::MainWindow).toByteArray());
   }
   if (PersistentSettings::contains(PersistentSettings::Names::splitter_horizontal_State,
                                    PersistentSettings::Sections::MainWindow)) {
      splitter_horizontal->restoreState(PersistentSettings::value(PersistentSettings::Names::splitter_horizontal_State,
                                                                  QVariant(),
                                                                  PersistentSettings::Sections::MainWindow).toByteArray());
   }
   if (PersistentSettings::contains(PersistentSettings::Names::treeView_recipe_headerState,
                                    PersistentSettings::Sections::MainWindow)) {
      treeView_recipe->header()->restoreState(PersistentSettings::value(PersistentSettings::Names::treeView_recipe_headerState,
                                                                        QVariant(),
                                                                        PersistentSettings::Sections::MainWindow).toByteArray());
   }
   if (PersistentSettings::contains(PersistentSettings::Names::treeView_style_headerState,
                                    PersistentSettings::Sections::MainWindow)) {
      treeView_style->header()->restoreState(PersistentSettings::value(PersistentSettings::Names::treeView_style_headerState,
                                                                       QVariant(),
                                                                       PersistentSettings::Sections::MainWindow).toByteArray());
   }
   if (PersistentSettings::contains(PersistentSettings::Names::treeView_equip_headerState,
                                    PersistentSettings::Sections::MainWindow)) {
      treeView_equip->header()->restoreState(PersistentSettings::value(PersistentSettings::Names::treeView_equip_headerState,
                                                                       QVariant(),
                                                                       PersistentSettings::Sections::MainWindow).toByteArray());
   }
   if (PersistentSettings::contains(PersistentSettings::Names::treeView_ferm_headerState,
                                    PersistentSettings::Sections::MainWindow)) {
      treeView_ferm->header()->restoreState(PersistentSettings::value(PersistentSettings::Names::treeView_ferm_headerState,
                                                                      QVariant(),
                                                                      PersistentSettings::Sections::MainWindow).toByteArray());
   }
   if (PersistentSettings::contains(PersistentSettings::Names::treeView_hops_headerState,
                                    PersistentSettings::Sections::MainWindow)) {
      treeView_hops->header()->restoreState(PersistentSettings::value(PersistentSettings::Names::treeView_hops_headerState,
                                                                      QVariant(),
                                                                      PersistentSettings::Sections::MainWindow).toByteArray());
   }
   if (PersistentSettings::contains(PersistentSettings::Names::treeView_misc_headerState,
                                    PersistentSettings::Sections::MainWindow)) {
      treeView_misc->header()->restoreState(PersistentSettings::value(PersistentSettings::Names::treeView_misc_headerState,
                                                                      QVariant(),
                                                                      PersistentSettings::Sections::MainWindow).toByteArray());
   }
   if (PersistentSettings::contains(PersistentSettings::Names::treeView_yeast_headerState,
                                    PersistentSettings::Sections::MainWindow)) {
      treeView_yeast->header()->restoreState(PersistentSettings::value(PersistentSettings::Names::treeView_yeast_headerState,
                                                                       QVariant(),
                                                                       PersistentSettings::Sections::MainWindow).toByteArray());
   }
   if (PersistentSettings::contains(PersistentSettings::Names::mashStepTableWidget_headerState,
                                    PersistentSettings::Sections::MainWindow)) {
      mashStepTableWidget->horizontalHeader()->restoreState(PersistentSettings::value(PersistentSettings::Names::mashStepTableWidget_headerState,
                                                                                      QVariant(),
                                                                                      PersistentSettings::Sections::MainWindow).toByteArray());
   }
}

// menu items with a SIGNAL of triggered() should go in here.
void MainWindow::setupTriggers() {
   // Connect actions defined in *.ui files to methods in code
   connect(actionExit,                       &QAction::triggered, this,                    &QWidget::close);                   // > File > Exit
   connect(actionAbout_Brewken,              &QAction::triggered, dialog_about,            &QWidget::show);                    // > About > About Brewken
   connect(actionHelp,                       &QAction::triggered, this->pimpl->helpDialog, &QWidget::show);                    // > About > Help

   connect(actionNewRecipe,                  &QAction::triggered, this,                    &MainWindow::newRecipe);            // > File > New Recipe
   connect(actionImportFromXml,              &QAction::triggered, this,                    &MainWindow::importFiles);          // > File > Import Recipes
   connect(actionExportToXml,                &QAction::triggered, this,                    &MainWindow::exportRecipe);         // > File > Export Recipes
   connect(actionUndo,                       &QAction::triggered, this,                    &MainWindow::editUndo);             // > Edit > Undo
   connect(actionRedo,                       &QAction::triggered, this,                    &MainWindow::editRedo);             // > Edit > Redo
   setUndoRedoEnable();
   connect(actionEquipments,                 &QAction::triggered, equipEditor,             &QWidget::show);                    // > View > Equipments
   connect(actionMashs,                      &QAction::triggered, namedMashEditor,         &QWidget::show);                    // > View > Mashs
   connect(actionStyles,                     &QAction::triggered, styleEditor,             &QWidget::show);                    // > View > Styles
   connect(actionFermentables,               &QAction::triggered, fermDialog,              &QWidget::show);                    // > View > Fermentables
   connect(actionHops,                       &QAction::triggered, hopDialog,               &QWidget::show);                    // > View > Hops
   connect(actionMiscs,                      &QAction::triggered, miscDialog,              &QWidget::show);                    // > View > Miscs
   connect(actionYeasts,                     &QAction::triggered, yeastDialog,             &QWidget::show);                    // > View > Yeasts
   connect(actionOptions,                    &QAction::triggered, optionDialog,            &OptionDialog::show);               // > Tools > Options
//   connect( actionManual, &QAction::triggered, this, &MainWindow::openManual);                                               // > About > Manual
   connect(actionScale_Recipe,               &QAction::triggered, recipeScaler,            &QWidget::show);                    // > Tools > Scale Recipe
   connect(action_recipeToTextClipboard,     &QAction::triggered, recipeFormatter,         &RecipeFormatter::toTextClipboard); // > Tools > Recipe to Clipboard as Text
   connect(actionConvert_Units,              &QAction::triggered, converterTool,           &QWidget::show);                    // > Tools > Convert Units
   connect(actionHydrometer_Temp_Adjustment, &QAction::triggered, hydrometerTool,          &QWidget::show);                    // > Tools > Hydrometer Temp Adjustment
   connect(actionAlcohol_Percentage_Tool,    &QAction::triggered, alcoholTool,             &QWidget::show);                    // > Tools > Alcohol
   connect(actionOG_Correction_Help,         &QAction::triggered, ogAdjuster,              &QWidget::show);                    // > Tools > OG Correction Help
   connect(actionCopy_Recipe,                &QAction::triggered, this,                    &MainWindow::copyRecipe);           // > File > Copy Recipe
   connect(actionPriming_Calculator,         &QAction::triggered, primingDialog,           &QWidget::show);                    // > Tools > Priming Calculator
   connect(actionStrikeWater_Calculator,     &QAction::triggered, strikeWaterDialog,       &QWidget::show);                    // > Tools > Strike Water Calculator
   connect(actionRefractometer_Tools,        &QAction::triggered, refractoDialog,          &QWidget::show);                    // > Tools > Refractometer Tools
   connect(actionPitch_Rate_Calculator,      &QAction::triggered, this,                    &MainWindow::showPitchDialog);      // > Tools > Pitch Rate Calculator
   connect(actionTimers,                     &QAction::triggered, timerMainDialog,         &QWidget::show);                    // > Tools > Timers
   connect(actionDeleteSelected,             &QAction::triggered, this,                    &MainWindow::deleteSelected);
   connect(actionWater_Chemistry,            &QAction::triggered, this,                    &MainWindow::popChemistry);         // > Tools > Water Chemistry
   connect(actionAncestors,                  &QAction::triggered, this,                    &MainWindow::setAncestor);          // > Tools > Ancestors
   connect(action_brewit,                    &QAction::triggered, this,                    &MainWindow::brewItHelper);
   //One Dialog to rule them all, at least all printing and export.
   connect(actionPrint, &QAction::triggered, printAndPreviewDialog, &QWidget::show);                                           // > File > Print and Preview

   // postgresql cannot backup or restore yet. I would like to find some way
   // around this, but for now just disable
   if ( Database::instance().dbType() == Database::DbType::PGSQL ) {
      actionBackup_Database->setEnabled(false);                                                                         // > File > Database > Backup
      actionRestore_Database->setEnabled(false);                                                                        // > File > Database > Restore
   }
   else {
      connect(actionBackup_Database,  &QAction::triggered, this, &MainWindow::backup);                                  // > File > Database > Backup
      connect(actionRestore_Database, &QAction::triggered, this, &MainWindow::restoreFromBackup);                       // > File > Database > Restore
   }
   return;
}

// pushbuttons with a SIGNAL of clicked() should go in here.
void MainWindow::setupClicks() {
   connect(this->equipmentButton,           &QAbstractButton::clicked, this,         &MainWindow::showEquipmentEditor);
   connect(this->styleButton,               &QAbstractButton::clicked, this,         &MainWindow::showStyleEditor);
   connect(this->mashButton,                &QAbstractButton::clicked, mashEditor,   &MashEditor::showEditor);
   connect(this->pushButton_addFerm,        &QAbstractButton::clicked, fermDialog,   &QWidget::show);
   connect(this->pushButton_addHop,         &QAbstractButton::clicked, hopDialog,    &QWidget::show);
   connect(this->pushButton_addMisc,        &QAbstractButton::clicked, miscDialog,   &QWidget::show);
   connect(this->pushButton_addYeast,       &QAbstractButton::clicked, yeastDialog,  &QWidget::show);
   connect(this->pushButton_removeFerm,     &QAbstractButton::clicked, this,         &MainWindow::removeSelectedFermentable);
   connect(this->pushButton_removeHop,      &QAbstractButton::clicked, this,         &MainWindow::removeSelectedHop);
   connect(this->pushButton_removeMisc,     &QAbstractButton::clicked, this,         &MainWindow::removeSelectedMisc);
   connect(this->pushButton_removeYeast,    &QAbstractButton::clicked, this,         &MainWindow::removeSelectedYeast);
   connect(this->pushButton_editFerm,       &QAbstractButton::clicked, this,         &MainWindow::editSelectedFermentable);
   connect(this->pushButton_editMisc,       &QAbstractButton::clicked, this,         &MainWindow::editSelectedMisc);
   connect(this->pushButton_editHop,        &QAbstractButton::clicked, this,         &MainWindow::editSelectedHop);
   connect(this->pushButton_editYeast,      &QAbstractButton::clicked, this,         &MainWindow::editSelectedYeast);
   connect(this->pushButton_editMash,       &QAbstractButton::clicked, mashEditor,   &MashEditor::showEditor);
   connect(this->pushButton_addMashStep,    &QAbstractButton::clicked, this,         &MainWindow::addMashStep);
   connect(this->pushButton_removeMashStep, &QAbstractButton::clicked, this,         &MainWindow::removeSelectedMashStep);
   connect(this->pushButton_editMashStep,   &QAbstractButton::clicked, this,         &MainWindow::editSelectedMashStep);
   connect(this->pushButton_mashWizard,     &QAbstractButton::clicked, mashWizard,   &MashWizard::show);
   connect(this->pushButton_saveMash,       &QAbstractButton::clicked, this,         &MainWindow::saveMash);
   connect(this->pushButton_mashDes,        &QAbstractButton::clicked, mashDesigner, &MashDesigner::show);
   connect(this->pushButton_mashUp,         &QAbstractButton::clicked, this,         &MainWindow::moveSelectedMashStepUp);
   connect(this->pushButton_mashDown,       &QAbstractButton::clicked, this,         &MainWindow::moveSelectedMashStepDown);
   connect(this->pushButton_mashRemove,     &QAbstractButton::clicked, this,         &MainWindow::removeMash);
   return;
}

// comboBoxes with a SIGNAL of activated() should go in here.
void MainWindow::setupActivate() {
   connect(this->equipmentComboBox, QOverload<int>::of(&QComboBox::activated), this, &MainWindow::updateRecipeEquipment);
   connect(this->styleComboBox,     QOverload<int>::of(&QComboBox::activated), this, &MainWindow::updateRecipeStyle);
   connect(this->mashComboBox,      QOverload<int>::of(&QComboBox::activated), this, &MainWindow::updateRecipeMash);
   return;
}

// lineEdits with either an editingFinished() or a textModified() should go in
// here
void MainWindow::setupTextEdit() {
   connect(this->lineEdit_name,       &QLineEdit::editingFinished, this, &MainWindow::updateRecipeName);
   connect(this->lineEdit_batchSize,  &BtLineEdit::textModified,   this, &MainWindow::updateRecipeBatchSize);
   connect(this->lineEdit_boilSize,   &BtLineEdit::textModified,   this, &MainWindow::updateRecipeBoilSize);
   connect(this->lineEdit_boilTime,   &BtLineEdit::textModified,   this, &MainWindow::updateRecipeBoilTime);
   connect(this->lineEdit_efficiency, &BtLineEdit::textModified,   this, &MainWindow::updateRecipeEfficiency);
   return;
}

// anything using a BtLabel::changedSystemOfMeasurementOrScale signal should go in here
void MainWindow::setupLabels() {
   // These are the sliders. I need to consider these harder, but small steps
   connect(this->oGLabel,       &BtLabel::changedSystemOfMeasurementOrScale, this, &MainWindow::redisplayLabel);
   connect(this->fGLabel,       &BtLabel::changedSystemOfMeasurementOrScale, this, &MainWindow::redisplayLabel);
   connect(this->colorSRMLabel, &BtLabel::changedSystemOfMeasurementOrScale, this, &MainWindow::redisplayLabel);
   return;
}

// anything with a BtTabWidget::set* signal should go in here
void MainWindow::setupDrops() {
   // drag and drop. maybe
   connect(this->tabWidget_recipeView,  &BtTabWidget::setRecipe,       this, &MainWindow::setRecipe);
   connect(this->tabWidget_recipeView,  &BtTabWidget::setEquipment,    this, &MainWindow::droppedRecipeEquipment);
   connect(this->tabWidget_recipeView,  &BtTabWidget::setStyle,        this, &MainWindow::droppedRecipeStyle);
   connect(this->tabWidget_ingredients, &BtTabWidget::setFermentables, this, &MainWindow::droppedRecipeFermentable);
   connect(this->tabWidget_ingredients, &BtTabWidget::setHops,         this, &MainWindow::droppedRecipeHop);
   connect(this->tabWidget_ingredients, &BtTabWidget::setMiscs,        this, &MainWindow::droppedRecipeMisc);
   connect(this->tabWidget_ingredients, &BtTabWidget::setYeasts,       this, &MainWindow::droppedRecipeYeast);
   return;
}

void MainWindow::deleteSelected() {
   QModelIndexList selected;
   BtTreeView* active = qobject_cast<BtTreeView*>(tabWidget_Trees->currentWidget()->focusWidget());

   // This happens after startup when nothing is selected
   if (!active) {
      qDebug() << Q_FUNC_INFO << "Nothing selected, so nothing to delete";
      return;
   }

   QModelIndex start = active->selectionModel()->selectedRows().first();
   qDebug() << Q_FUNC_INFO << "Delete starting from row" << start.row();
   active->deleteSelected(active->selectionModel()->selectedRows());

   //
   // Now that we deleted the selected recipe, we don't want it to appear in the main window any more, so let's select
   // another one.
   //
   // Most of the time, after deleting the nth recipe, the new nth item is also a recipe.  If there isn't an nth item
   // (eg because the recipe(s) we deleted were at the end of the list) then let's go back to the 1st item.  But then
   // we have to make sure to skip over folders.
   //
   // .:TBD:. This works if you have plenty of recipes outside folders.  If all your recipes are inside folders, then
   // we should so a proper search through the tree to find the first recipe and then expand the folder that it's in.
   // Doesn't feel like that logic belongs here.  Would be better to create BtTreeView::firstNonFolder() or similar.
   //
   if (!start.isValid() || !active->type(start)) {
      int oldRow = start.row();
      start = active->first();
      qDebug() << Q_FUNC_INFO << "Row" << oldRow << "no longer valid, so returning to first (" << start.row() << ")";
   }

   while (start.isValid() && active->type(start) == BtTreeItem::Type::FOLDER) {
      qDebug() << Q_FUNC_INFO << "Skipping over folder at row" << start.row();
      // Once all platforms are on Qt 5.11 or later, we can write:
      // start = start.siblingAtRow(start.row() + 1);
      start = start.sibling(start.row() + 1, start.column());
   }

   if (start.isValid()) {
      qDebug() << Q_FUNC_INFO << "Row" << start.row() << "is" << active->type(start);
      if (active->type(start) == BtTreeItem::Type::RECIPE) {
         this->setRecipe(treeView_recipe->getItem<Recipe>(start));
      }
      this->setTreeSelection(start);
   }

   return;
}

void MainWindow::treeActivated(const QModelIndex &index) {
   QObject* calledBy = sender();
   // Not sure how this could happen, but better safe the sigsegv'd
   if (!calledBy) {
      return;
   }

   BtTreeView* active = qobject_cast<BtTreeView*>(calledBy);
   // If the sender cannot be morphed into a BtTreeView object
   if (!active) {
      qWarning() << Q_FUNC_INFO << "Unrecognised sender" << calledBy->metaObject()->className();
      return;
   }

   auto itemType = active->type(index);
   if (!itemType) {
      qWarning() << Q_FUNC_INFO << "Unknown type for index" << index;
   } else {
      switch (*itemType) {
         case BtTreeItem::Type::RECIPE:
            setRecipe(treeView_recipe->getItem<Recipe>(index));
            break;
         case BtTreeItem::Type::EQUIPMENT:
            {
               Equipment * kit = active->getItem<Equipment>(index);
               if ( kit ) {
                  singleEquipEditor->setEquipment(kit);
                  singleEquipEditor->show();
               }
            }
            break;
         case BtTreeItem::Type::FERMENTABLE:
            {
               Fermentable * ferm = active->getItem<Fermentable>(index);
               if (ferm) {
                  fermEditor->setFermentable(ferm);
                  fermEditor->show();
               }
            }
            break;
         case BtTreeItem::Type::HOP:
            {
               Hop* h = active->getItem<Hop>(index);
               if (h) {
                  hopEditor->setHop(h);
                  hopEditor->show();
               }
            }
            break;
         case BtTreeItem::Type::MISC:
            {
               Misc * m = active->getItem<Misc>(index);
               if (m) {
                  miscEditor->setMisc(m);
                  miscEditor->show();
               }
            }
            break;
         case BtTreeItem::Type::STYLE:
            {
               Style * s = active->getItem<Style>(index);
               if ( s ) {
                  singleStyleEditor->setStyle(s);
                  singleStyleEditor->show();
               }
            }
            break;
         case BtTreeItem::Type::YEAST:
            {
               Yeast * y = active->getItem<Yeast>(index);
               if (y) {
                  yeastEditor->setYeast(y);
                  yeastEditor->show();
               }
            }
            break;
         case BtTreeItem::Type::BREWNOTE:
            setBrewNoteByIndex(index);
            break;
         case BtTreeItem::Type::FOLDER:  // default behavior is fine, but no warning
            break;
         case BtTreeItem::Type::WATER:
            {
               Water * w = active->getItem<Water>(index);
               if (w) {
                  waterEditor->setWater(ObjectStoreWrapper::getSharedFromRaw(w));
                  waterEditor->show();
               }
            }
            break;
      }
   }
   treeView_recipe->setCurrentIndex(index);
   return;
}

void MainWindow::setBrewNoteByIndex(const QModelIndex &index)
{
   BrewNoteWidget* ni;

   auto bNote = treeView_recipe->getItem<BrewNote>(index);

   if ( ! bNote )
      return;
   // HERE
   // This is some clean up work. REMOVE FROM HERE TO THERE
   if ( bNote->projPoints() < 15 )
   {
      double pnts = bNote->projPoints();
      bNote->setProjPoints(pnts);
   }
   if ( bNote->effIntoBK_pct() < 10 )
   {
      bNote->calculateEffIntoBK_pct();
      bNote->calculateBrewHouseEff_pct();
   }
   // THERE

   Recipe* parent  = ObjectStoreWrapper::getByIdRaw<Recipe>(bNote->getRecipeId());
   QModelIndex pNdx = treeView_recipe->parent(index);

   // this gets complex. Versioning means we can't just clear the open
   // brewnote tabs out.
   if ( parent != this->recipeObs ) {
      if ( ! this->recipeObs->isMyAncestor(*parent) ) {
         setRecipe(parent);
      }
      else if ( treeView_recipe->ancestorsAreShowing(pNdx) ) {
         tabWidget_recipeView->setCurrentIndex(0);
         // Start closing from the right (highest index) down. Anything else dumps
         // core in the most unpleasant of fashions
         int tabs = tabWidget_recipeView->count() - 1;
         for (int i = tabs; i >= 0; --i) {
            if (tabWidget_recipeView->widget(i)->objectName() == "BrewNoteWidget")
               tabWidget_recipeView->removeTab(i);
         }
         setRecipe(parent);
      }
   }

   ni = findBrewNoteWidget(bNote);
   if ( ! ni )
   {
      ni = new BrewNoteWidget(tabWidget_recipeView);
      ni->setBrewNote(bNote);
   }

   tabWidget_recipeView->addTab(ni,bNote->brewDate_short());
   tabWidget_recipeView->setCurrentWidget(ni);

}

BrewNoteWidget* MainWindow::findBrewNoteWidget(BrewNote* b)
{
   for (int i = 0; i < tabWidget_recipeView->count(); ++i)
   {
      if (tabWidget_recipeView->widget(i)->objectName() == "BrewNoteWidget")
      {
         BrewNoteWidget* ni = qobject_cast<BrewNoteWidget*>(tabWidget_recipeView->widget(i));
         if ( ni->isBrewNote(b) )
            return ni;
      }
   }
   return nullptr;
}

void MainWindow::setBrewNote(BrewNote* bNote)
{
   QString tabname;
   BrewNoteWidget* ni = findBrewNoteWidget(bNote);

   if ( ni )
   {
      tabWidget_recipeView->setCurrentWidget(ni);
      return;
   }

   ni = new BrewNoteWidget(tabWidget_recipeView);
   ni->setBrewNote(bNote);

   tabWidget_recipeView->addTab(ni,bNote->brewDate_short());
   tabWidget_recipeView->setCurrentWidget(ni);
}

void MainWindow::setAncestor()
{
   Recipe* rec;
   if ( this->recipeObs ) {
      rec = this->recipeObs;
   } else {
      QModelIndexList indexes = treeView_recipe->selectionModel()->selectedRows();
      rec = treeView_recipe->getItem<Recipe>(indexes[0]);
   }

   ancestorDialog->setAncestor(rec);
   ancestorDialog->show();
}


// Can handle null recipes.
void MainWindow::setRecipe(Recipe* recipe) {
   // Don't like void pointers.
   if (!recipe) {
      return;
   }

   qDebug() << Q_FUNC_INFO << "Recipe #" << recipe->key() << ":" << recipe->name();

   int tabs = 0;

   // Make sure this MainWindow is paying attention...
   if (this->recipeObs) {
      disconnect(this->recipeObs, nullptr, this, nullptr);
   }
   this->recipeObs = recipe;

   this->recStyle = recipe->style();
   recEquip = recipe->equipment();
   this->displayRangesEtcForCurrentRecipeStyle();

   // Reset all previous recipe shit.
   fermTableModel->observeRecipe(recipe);
   hopTableModel->observeRecipe(recipe);
   miscTableModel->observeRecipe(recipe);
   yeastTableModel->observeRecipe(recipe);
   mashStepTableModel->setMash(recipeObs->mash());

   // Clean out any brew notes
   tabWidget_recipeView->setCurrentIndex(0);
   // Start closing from the right (highest index) down. Anything else dumps
   // core in the most unpleasant of fashions
   tabs = tabWidget_recipeView->count() - 1;
   for (int i = tabs; i >= 0; --i) {
      if (tabWidget_recipeView->widget(i)->objectName() == "BrewNoteWidget")
         tabWidget_recipeView->removeTab(i);
   }

   // Tell some of our other widgets to observe the new recipe.
   mashWizard->setRecipe(recipe);
   brewDayScrollWidget->setRecipe(recipe);
   equipmentListModel->observeRecipe(recipe);
   recipeFormatter->setRecipe(recipe);
   ogAdjuster->setRecipe(recipe);
   recipeExtrasWidget->setRecipe(recipe);
   mashDesigner->setRecipe(recipe);
   equipmentButton->setRecipe(recipe);
   singleEquipEditor->setEquipment(recEquip);
   styleButton->setRecipe(recipe);
   singleStyleEditor->setStyle(recipe->style());

   mashEditor->setMash(recipeObs->mash());
   mashEditor->setRecipe(recipeObs);

   mashButton->setMash(recipeObs->mash());
   recipeScaler->setRecipe(recipeObs);

   // Set the locked flag as required
   checkBox_locked->setCheckState( recipe->locked() ? Qt::Checked : Qt::Unchecked );
   lockRecipe( recipe->locked() ? Qt::Checked : Qt::Unchecked );

   // Here's the fun part. If the recipe is locked and display is false, then
   // you have said "show versions" and we will not all the recipe to be
   // unlocked. Hmmm. Skeptical Mik is skeptical
   if ( recipe->locked() && ! recipe->display() ) {
      checkBox_locked->setEnabled( false );
   }
   else {
      checkBox_locked->setEnabled( true );
   }

   checkBox_locked->setCheckState( recipe->locked() ? Qt::Checked : Qt::Unchecked );
   lockRecipe(recipe->locked() ? Qt::Checked : Qt::Unchecked );

   // changes in how the data is loaded means we may not have fired all the signals we should have
   // this makes sure the signals are fired. This is likely a 5kg hammer driving a finishing nail.
   recipe->recalcAll();

   // If you don't connect this late, every previous set of an attribute
   // causes this signal to be slotted, which then causes showChanges() to be
   // called.
   connect( recipeObs, SIGNAL(changed(QMetaProperty,QVariant)), this, SLOT(changed(QMetaProperty,QVariant)) );
   showChanges();
}

// When a recipe is locked, many fields need to be disabled.
void MainWindow::lockRecipe(int state)
{
   if ( this->recipeObs == nullptr )
      return;

   // If I am locking a recipe (lock == true ), I want to disable fields
   // (enable == false). If I am unlocking (lock == false), I want to enable
   // fields (enable == true). This just makes that easy
   bool lockIt = state == Qt::Checked;
   bool enabled = ! lockIt;

   // Lock/unlock the recipe, then disable/enable the fields. I am leaving the
   // name field as editable. I may regret that, but if you are defining an
   // inheritance tree, you may want to remove strings from the ancestoral
   // names
   this->recipeObs->setLocked(lockIt);

   // I could disable tab_recipe, but would not prevent you from unlocking the
   // recipe because that field would also be disabled
   qWidget_styleBox->setEnabled(enabled);
   qWidget_equipmentBox->setEnabled(enabled);
   lineEdit_batchSize->setEnabled(enabled);
   lineEdit_boilSize->setEnabled(enabled);
   lineEdit_efficiency->setEnabled(enabled);
   lineEdit_boilTime->setEnabled(enabled);

   // locked recipes cannot be deleted
   actionDeleteSelected->setEnabled(enabled);
   treeView_recipe->enableDelete(enabled);

   treeView_recipe->setDragDropMode( lockIt ? QAbstractItemView::NoDragDrop : QAbstractItemView::DragDrop);
   tabWidget_ingredients->setAcceptDrops( enabled );

   // Onto the tables. Four lines each to disable edits, drag/drop and deletes
   fermentableTable->setEnabled(enabled);
   pushButton_addFerm->setEnabled(enabled);
   pushButton_removeFerm->setEnabled(enabled);
   pushButton_editFerm->setEnabled(enabled);

   hopTable->setEnabled(enabled);
   pushButton_addHop->setEnabled(enabled);
   pushButton_removeHop->setEnabled(enabled);
   pushButton_editHop->setEnabled(enabled);

   miscTable->setEnabled(enabled);
   pushButton_addMisc->setEnabled(enabled);
   pushButton_removeMisc->setEnabled(enabled);
   pushButton_editMisc->setEnabled(enabled);

   yeastTable->setEnabled(enabled);
   pushButton_addYeast->setEnabled(enabled);
   pushButton_removeYeast->setEnabled(enabled);
   pushButton_editYeast->setEnabled(enabled);

   fermDialog->pushButton_addToRecipe->setEnabled(enabled);
   hopDialog->pushButton_addToRecipe->setEnabled(enabled);
   miscDialog->pushButton_addToRecipe->setEnabled(enabled);
   yeastDialog->pushButton_addToRecipe->setEnabled(enabled);
   // mashes still need dealing with
   //

}

void MainWindow::changed(QMetaProperty prop, QVariant val) {
   QString propName(prop.name());

   if (propName == PropertyNames::Recipe::equipment) {
      this->recEquip = val.value<Equipment *>();
      this->singleEquipEditor->setEquipment(this->recEquip);
   } else if (propName == PropertyNames::Recipe::style) {
      //recStyle = recipeObs->style();
      this->recStyle = val.value<Style*>();
      this->singleStyleEditor->setStyle(this->recStyle);
   }

   this->showChanges(&prop);
   return;
}

void MainWindow::updateDensitySlider(BtStringConst const & propertyNameMin,
                                     BtStringConst const & propertyNameMax,
                                     BtStringConst const & propertyNameCurrent,
                                     RangedSlider* slider,
                                     double max) {
   Measurement::UnitSystem const & displayUnitSystem =
      Measurement::getUnitSystemForField(*propertyNameCurrent,
                                         *PersistentSettings::Sections::tab_recipe,
                                         Measurement::PhysicalQuantity::Density);
   slider->setPreferredRange(Measurement::displayRange(recStyle,
                                                       this->tab_recipe,
                                                       propertyNameMin,
                                                       propertyNameMax,
                                                       &Measurement::Units::sp_grav));
   slider->setRange(Measurement::displayRange(this->tab_recipe,
                                              propertyNameCurrent,
                                              1.000,
                                              max,
                                              Measurement::Units::sp_grav));

   if (displayUnitSystem == Measurement::UnitSystems::density_Plato) {
      slider->setPrecision(1);
      slider->setTickMarks(2,5);
   } else {
      slider->setPrecision(3);
      slider->setTickMarks(0.010, 2);
   }
   return;
}

void MainWindow::updateColorSlider(BtStringConst const & propertyNameMin,
                                   BtStringConst const & propertyNameMax,
                                   BtStringConst const & propertyNameCurrent,
                                   RangedSlider * slider) {
   Measurement::UnitSystem const & displayUnitSystem =
      Measurement::getUnitSystemForField(*propertyNameCurrent,
                                         *PersistentSettings::Sections::tab_recipe,
                                         Measurement::PhysicalQuantity::Color);

   slider->setPreferredRange(Measurement::displayRange(recStyle,
                                                       this->tab_recipe,
                                                       propertyNameMin,
                                                       propertyNameMax,
                                                       &Measurement::Units::srm));
   slider->setRange(Measurement::displayRange(this->tab_recipe,
                                              propertyNameCurrent,
                                              1,
                                              44,
                                              Measurement::Units::srm));
   slider->setTickMarks(displayUnitSystem == Measurement::UnitSystems::color_StandardReferenceMethod ? 10 : 40, 2);

   return;
}

void MainWindow::showChanges(QMetaProperty* prop) {
   if (recipeObs == nullptr) {
      return;
   }

   bool updateAll = (prop == nullptr);
   QString propName;

   if (prop) {
      propName = prop->name();
   }

   // May St. Stevens preserve me
   lineEdit_name->setText(recipeObs->name());
   lineEdit_batchSize->setText(recipeObs);
   lineEdit_boilSize->setText(recipeObs);
   lineEdit_efficiency->setText(recipeObs);
   lineEdit_boilTime->setText(recipeObs);
   lineEdit_name->setCursorPosition(0);
   lineEdit_batchSize->setCursorPosition(0);
   lineEdit_boilSize->setCursorPosition(0);
   lineEdit_efficiency->setCursorPosition(0);
   lineEdit_boilTime->setCursorPosition(0);
/*
   lineEdit_calcBatchSize->setText(recipeObs);
   lineEdit_calcBoilSize->setText(recipeObs);
*/

   // Color manipulation
/*
   if( 0.95*recipeObs->batchSize_l() <= recipeObs->finalVolume_l() && recipeObs->finalVolume_l() <= 1.05*recipeObs->batchSize_l() )
      lineEdit_calcBatchSize->setStyleSheet(goodSS);
   else if( recipeObs->finalVolume_l() < 0.95*recipeObs->batchSize_l() )
      lineEdit_calcBatchSize->setStyleSheet(lowSS);
   else
      lineEdit_calcBatchSize->setStyleSheet(highSS);

   if( 0.95*recipeObs->boilSize_l() <= recipeObs->boilVolume_l() && recipeObs->boilVolume_l() <= 1.05*recipeObs->boilSize_l() )
      lineEdit_calcBoilSize->setStyleSheet(goodSS);
   else if( recipeObs->boilVolume_l() < 0.95* recipeObs->boilSize_l() )
      lineEdit_calcBoilSize->setStyleSheet(lowSS);
   else
      lineEdit_calcBoilSize->setStyleSheet(highSS);
*/
   lineEdit_boilSg->setText(recipeObs);

   updateDensitySlider(PropertyNames::Style::ogMin, PropertyNames::Style::ogMax, PropertyNames::Recipe::og, styleRangeWidget_og, 1.120);
   styleRangeWidget_og->setValue(Measurement::amountDisplay(recipeObs, tab_recipe, PropertyNames::Recipe::og, &Measurement::Units::sp_grav));

   updateDensitySlider(PropertyNames::Style::fgMin, PropertyNames::Style::fgMax, PropertyNames::Recipe::fg, styleRangeWidget_fg, 1.03);
   styleRangeWidget_fg->setValue(Measurement::amountDisplay(recipeObs, tab_recipe, PropertyNames::Recipe::fg, &Measurement::Units::sp_grav));

   styleRangeWidget_abv->setValue(recipeObs->ABV_pct());
   styleRangeWidget_ibu->setValue(recipeObs->IBU());

   rangeWidget_batchsize->setRange(0, Measurement::amountDisplay(recipeObs, tab_recipe, PropertyNames::Recipe::batchSize_l, &Measurement::Units::liters));
   rangeWidget_batchsize->setPreferredRange(0, Measurement::amountDisplay(recipeObs, tab_recipe, PropertyNames::Recipe::finalVolume_l, &Measurement::Units::liters));
   rangeWidget_batchsize->setValue(Measurement::amountDisplay(recipeObs, tab_recipe, PropertyNames::Recipe::finalVolume_l, &Measurement::Units::liters));

   rangeWidget_boilsize->setRange(0, Measurement::amountDisplay(recipeObs, tab_recipe, PropertyNames::Recipe::boilSize_l, &Measurement::Units::liters));
   rangeWidget_boilsize->setPreferredRange(0, Measurement::amountDisplay(recipeObs, tab_recipe, PropertyNames::Recipe::boilVolume_l, &Measurement::Units::liters));
   rangeWidget_boilsize->setValue(Measurement::amountDisplay(recipeObs, tab_recipe, PropertyNames::Recipe::boilVolume_l, &Measurement::Units::liters));

   /* Colors need the same basic treatment as gravity */
   updateColorSlider(PropertyNames::Style::colorMin_srm,
                     PropertyNames::Style::colorMax_srm,
                     PropertyNames::Recipe::color_srm,
                     styleRangeWidget_srm);
   styleRangeWidget_srm->setValue(Measurement::amountDisplay(recipeObs, tab_recipe, PropertyNames::Recipe::color_srm, &Measurement::Units::srm));

   // In some, incomplete, recipes, OG is approximately 1.000, which then makes GU close to 0 and thus IBU/GU insanely
   // large.  Besides being meaningless, such a large number takes up a lot of space.  So, where gravity units are
   // below 1, we just show IBU on the IBU/GU slider.
   auto gravityUnits = (recipeObs->og()-1)*1000;
   if (gravityUnits < 1) {
      gravityUnits = 1;
   }
   ibuGuSlider->setValue(recipeObs->IBU()/gravityUnits);

   label_calories->setText(
      QString("%1").arg(Measurement::getDisplayUnitSystem(Measurement::PhysicalQuantity::Volume) == Measurement::UnitSystems::volume_Metric ? recipeObs->calories33cl() : recipeObs->calories12oz(),0,'f',0)
   );

   // See if we need to change the mash in the table.
   if ((updateAll && recipeObs->mash()) ||
       (propName == "mash" && recipeObs->mash())) {
      mashStepTableModel->setMash(recipeObs->mash());
   }

   // Not sure about this, but I am annoyed that modifying the hop usage
   // modifiers isn't automatically updating my display
   if (updateAll) {
     recipeObs->recalcIBU();
     hopTableProxy->invalidate();
   }
   return;
}

void MainWindow::updateRecipeName() {
   if (recipeObs == nullptr || ! lineEdit_name->isModified()) {
      return;
   }

   this->doOrRedoUpdate(*this->recipeObs, PropertyNames::NamedEntity::name, lineEdit_name->text(), tr("Change Recipe Name"));
   return;
}

void MainWindow::displayRangesEtcForCurrentRecipeStyle() {
   if ( this->recipeObs == nullptr ) {
      return;
   }

   Style * style = this->recipeObs->style();
   if ( style == nullptr ) {
      return;
   }

   styleRangeWidget_og->setPreferredRange(Measurement::displayRange(style,
                                                                    this->tab_recipe,
                                                                    PropertyNames::Style::ogMin,
                                                                    PropertyNames::Style::ogMax,
                                                                    &Measurement::Units::sp_grav));
   styleRangeWidget_fg->setPreferredRange(Measurement::displayRange(style,
                                                                    this->tab_recipe,
                                                                    PropertyNames::Style::fgMin,
                                                                    PropertyNames::Style::fgMax,
                                                                    &Measurement::Units::sp_grav));

   styleRangeWidget_abv->setPreferredRange(style->abvMin_pct(), style->abvMax_pct());
   styleRangeWidget_ibu->setPreferredRange(style->ibuMin(), style->ibuMax());
   styleRangeWidget_srm->setPreferredRange(Measurement::displayRange(style,
                                                                     this->tab_recipe,
                                                                     PropertyNames::Style::colorMin_srm,
                                                                     PropertyNames::Style::colorMax_srm,
                                                                     &Measurement::Units::srm));

   this->styleButton->setStyle(style);

   return;
}

void MainWindow::updateRecipeStyle() {
   if (recipeObs == nullptr) {
      return;
   }

   QModelIndex proxyIndex( styleProxyModel->index(styleComboBox->currentIndex(),0) );
   QModelIndex sourceIndex( styleProxyModel->mapToSource(proxyIndex) );
   Style * selected = styleListModel->at(sourceIndex.row());
   if (selected) {
      this->doOrRedoUpdate(
         newRelationalUndoableUpdate(*this->recipeObs,
                                     &Recipe::setStyle,
                                     this->recipeObs->style(),
                                     selected,
                                     &MainWindow::displayRangesEtcForCurrentRecipeStyle,
                                     tr("Change Recipe Style"))
      );
   }
   return;
}

void MainWindow::updateRecipeMash() {
   if (this->recipeObs == nullptr) {
      return;
   }

   Mash* selected = mashListModel->at(mashComboBox->currentIndex());
   if (selected) {
      // The Recipe will decide whether it needs to make a copy of the Mash, hence why we don't reuse "selected" below
      this->recipeObs->setMash(selected);
      mashEditor->setMash(this->recipeObs->mash());
      mashButton->setMash(this->recipeObs->mash());
   }
   return;
}

void MainWindow::updateRecipeEquipment() {
  droppedRecipeEquipment(equipmentListModel->at(equipmentComboBox->currentIndex()));
  return;
}

void MainWindow::updateEquipmentButton() {
   if (this->recipeObs != nullptr) {
      this->equipmentButton->setEquipment(this->recipeObs->equipment());
   }
   return;
}

void MainWindow::droppedRecipeEquipment(Equipment *kit) {
   if (recipeObs == nullptr) {
      return;
   }

   // equip may be null.
   if (kit == nullptr) {
      return;
   }

   // We need to hang on to this QUndoCommand pointer because there might be other updates linked to it - see below
   auto equipmentUpdate = newRelationalUndoableUpdate(*this->recipeObs,
                                                      &Recipe::setEquipment,
                                                      this->recipeObs->equipment(),
                                                      kit,
                                                      &MainWindow::updateEquipmentButton,
                                                      tr("Change Recipe Kit"));

   // Keep the mash tun weight and specific heat up to date.
   Mash * m = recipeObs->mash();
   if (m) {
      new SimpleUndoableUpdate(*m, PropertyNames::Mash::tunWeight_kg, kit->tunWeight_kg(), tr("Change Tun Weight"), equipmentUpdate);
      new SimpleUndoableUpdate(*m, PropertyNames::Mash::tunSpecificHeat_calGC, kit->tunSpecificHeat_calGC(), tr("Change Tun Specific Heat"), equipmentUpdate);
   }

   if (QMessageBox::question(this,
                             tr("Equipment request"),
                             tr("Would you like to set the batch size, boil size and time to that requested by the equipment?"),
                             QMessageBox::Yes,
                             QMessageBox::No) == QMessageBox::Yes) {
      // If we do update batch size etc as a result of the equipment change, then we want those updates to undo/redo
      // if and when the equipment change is undone/redone.  Setting the parent field on a QUndoCommand makes that
      // parent the owner, responsible for invoking, deleting, etc.  Technically the descriptions of these subcommands
      // won't ever be seen by the user, but there's no harm in setting them.
      // (The previous call here to mashEditor->setRecipe() was a roundabout way of calling setTunWeight_kg() and
      // setTunSpecificHeat_calGC() on the mash.)
      new SimpleUndoableUpdate(*this->recipeObs, PropertyNames::Recipe::batchSize_l, kit->batchSize_l(), tr("Change Batch Size"), equipmentUpdate);
      new SimpleUndoableUpdate(*this->recipeObs, PropertyNames::Recipe::boilSize_l, kit->boilSize_l(), tr("Change Boil Size"), equipmentUpdate);
      new SimpleUndoableUpdate(*this->recipeObs, PropertyNames::Recipe::boilTime_min, kit->boilTime_min(), tr("Change Boil Time"), equipmentUpdate);
   }

   // This will do the equipment update and any related updates - see above
   this->doOrRedoUpdate(equipmentUpdate);
   return;
}

// This isn't called when we think it is...!
void MainWindow::droppedRecipeStyle(Style* style) {
   qDebug() << "MainWindow::droppedRecipeStyle";

   if (!this->recipeObs) {
      return;
   }
   // When the style is changed, we also need to update what is shown on the Style button
   qDebug() << "MainWindow::droppedRecipeStyle - do or redo";
   this->doOrRedoUpdate(
      newRelationalUndoableUpdate(*this->recipeObs,
                                  &Recipe::setStyle,
                                  this->recipeObs->style(),
                                  style,
                                  &MainWindow::displayRangesEtcForCurrentRecipeStyle,
                                  tr("Change Recipe Style"))
   );

   return;
}

// Well, aint this a kick in the pants. Apparently I can't template a slot
void MainWindow::droppedRecipeFermentable(QList<Fermentable*>ferms) {
   if (!this->recipeObs) {
      return;
   }

   if (tabWidget_ingredients->currentWidget() != fermentableTab) {
      tabWidget_ingredients->setCurrentWidget(fermentableTab);
   }
   this->doOrRedoUpdate(
      newUndoableAddOrRemoveList(*this->recipeObs,
                                 &Recipe::add<Fermentable>,
                                 ferms,
                                 &Recipe::remove<Fermentable>,
                                 tr("Drop fermentables on a recipe"))
   );
   return;
}

void MainWindow::droppedRecipeHop(QList<Hop*>hops) {
   if (!this->recipeObs) {
      return;
   }

   if (tabWidget_ingredients->currentWidget() != hopsTab) {
      tabWidget_ingredients->setCurrentWidget(hopsTab);
   }
   this->doOrRedoUpdate(
      newUndoableAddOrRemoveList(*this->recipeObs,
                                 &Recipe::add<Hop>,
                                 hops,
                                 &Recipe::remove<Hop>,
                                 tr("Drop hops on a recipe"))
   );
   return;
}

void MainWindow::droppedRecipeMisc(QList<Misc*>miscs) {
   if (!this->recipeObs) {
      return;
   }

   if (tabWidget_ingredients->currentWidget() != miscTab) {
      tabWidget_ingredients->setCurrentWidget(miscTab);
   }
   this->doOrRedoUpdate(
      newUndoableAddOrRemoveList(*this->recipeObs,
                                 &Recipe::add<Misc>,
                                 miscs,
                                 &Recipe::remove<Misc>,
                                 tr("Drop misc on a recipe"))
   );
   return;
}

void MainWindow::droppedRecipeYeast(QList<Yeast*>yeasts) {
   if (!this->recipeObs) {
      return;
   }

   if ( tabWidget_ingredients->currentWidget() != yeastTab )
      tabWidget_ingredients->setCurrentWidget(yeastTab);
   this->doOrRedoUpdate(
      newUndoableAddOrRemoveList(*this->recipeObs,
                                 &Recipe::add<Yeast>,
                                 yeasts,
                                 &Recipe::remove<Yeast>,
                                 tr("Drop yeast on a recipe"))
   );
}

void MainWindow::updateRecipeBatchSize() {
   if (!this->recipeObs) {
      return;
   }

   this->doOrRedoUpdate(*this->recipeObs,
                        PropertyNames::Recipe::batchSize_l,
                        lineEdit_batchSize->toSI().quantity,
                        tr("Change Batch Size"));
}

void MainWindow::updateRecipeBoilSize() {
   if (!this->recipeObs) {
      return;
   }

   this->doOrRedoUpdate(*this->recipeObs,
                        PropertyNames::Recipe::boilSize_l,
                        lineEdit_boilSize->toSI().quantity,
                        tr("Change Boil Size"));
}

void MainWindow::updateRecipeBoilTime() {
   if (!this->recipeObs) {
      return;
   }

   Equipment* kit = recipeObs->equipment();
   double boilTime = Measurement::qStringToSI(lineEdit_boilTime->text(), Measurement::PhysicalQuantity::Time).quantity;

   // Here, we rely on a signal/slot connection to propagate the equipment
   // changes to recipeObs->boilTime_min and maybe recipeObs->boilSize_l
   // NOTE: This works because kit is the recipe's equipment, not the generic
   // equipment in the recipe drop down.
   if (kit) {
      this->doOrRedoUpdate(*kit, PropertyNames::Equipment::boilTime_min, boilTime, tr("Change Boil Time"));
   } else {
      this->doOrRedoUpdate(*this->recipeObs, PropertyNames::Recipe::boilTime_min, boilTime, tr("Change Boil Time"));
   }

   return;
}

void MainWindow::updateRecipeEfficiency() {
   qDebug() << Q_FUNC_INFO << lineEdit_efficiency->getWidgetText();
   if (!this->recipeObs) {
      return;
   }

   this->doOrRedoUpdate(*this->recipeObs,
                        PropertyNames::Recipe::efficiency_pct,
                        lineEdit_efficiency->getValueAs<unsigned int>(),
                        tr("Change Recipe Efficiency"));
   return;
}

void MainWindow::addFermentableToRecipe(std::shared_ptr<Fermentable> ferm) {
   Q_ASSERT(ferm);
   this->doOrRedoUpdate(
      newUndoableAddOrRemove(*this->recipeObs,
                             &Recipe::add<Fermentable>,
                             ferm,
                             &Recipe::remove<Fermentable>,
                             tr("Add fermentable to recipe"))
   );
   // We don't need to call fermTableModel->addFermentable(ferm) here because the change to the recipe will already have
   // triggered the necessary updates to fermTableModel.
   return;
}

void MainWindow::addHopToRecipe(std::shared_ptr<Hop> hop) {
   Q_ASSERT(hop);
   this->doOrRedoUpdate(
      newUndoableAddOrRemove(*this->recipeObs,
                             &Recipe::add<Hop>,
                             hop,
                             &Recipe::remove<Hop>,
                             tr("Add hop to recipe"))
   );
   // We don't need to call hopTableModel->addHop(hop) here because the change to the recipe will already have
   // triggered the necessary updates to hopTableModel.
}

void MainWindow::addMiscToRecipe(std::shared_ptr<Misc> misc) {
   Q_ASSERT(misc);
   this->doOrRedoUpdate(
      newUndoableAddOrRemove(*this->recipeObs,
                             &Recipe::add<Misc>,
                             misc,
                             &Recipe::remove<Misc>,
                             tr("Add misc to recipe"))
   );
   // We don't need to call miscTableModel->addMisc(misc) here because the change to the recipe will already have
   // triggered the necessary updates to miscTableModel.
   return;
}

void MainWindow::addYeastToRecipe(std::shared_ptr<Yeast> yeast) {
   Q_ASSERT(yeast);
   this->doOrRedoUpdate(
      newUndoableAddOrRemove(*this->recipeObs,
                             &Recipe::add<Yeast>,
                             yeast,
                             &Recipe::remove<Yeast>,
                             tr("Add yeast to recipe"))
   );
   // We don't need to call yeastTableModel->addYeast(yeast) here because the change to the recipe will already have
   // triggered the necessary updates to yeastTableModel.
   return;
}

void MainWindow::addMashStepToMash(std::shared_ptr<MashStep> mashStep) {
   qDebug() << Q_FUNC_INFO;
   //
   // Mash Steps are a bit different from most other NamedEntity objects in that they don't really have an independent
   // existence.  If you ask a Mash to remove a MashStep then it will also tell the ObjectStore to delete it, but, when
   // we're adding a MashStep to a Mash it's easier (for eg the implementation of undo/redo) if we add it to the
   // ObjectStore before we call Mash::addMashStep().
   //
   ObjectStoreWrapper::insert(mashStep);
   this->doOrRedoUpdate(
      newUndoableAddOrRemove(*this->recipeObs->mash(),
                             &Mash::addMashStep,
                             mashStep,
                             &Mash::removeMashStep,
                             tr("Add mash step to recipe"))
   );
   // We don't need to call mashStepTableModel->addMashStep(mashStep) here because the change to the mash will already
   // have triggered the necessary updates to mashStepTableModel.
   return;
}

/**
 * This is akin to a special case of MainWindow::exportSelected()
 */
void MainWindow::exportRecipe() {
   if (!this->recipeObs) {
      return;
   }

   QList<Recipe *> recipes;
   recipes.append(this->recipeObs);

   ImportExport::exportToFile(&recipes);
   return;
}

Recipe* MainWindow::currentRecipe() {
   return recipeObs;
}

void MainWindow::setUndoRedoEnable() {
   Q_ASSERT(this->undoStack != 0);
   actionUndo->setEnabled(this->undoStack->canUndo());
   actionRedo->setEnabled(this->undoStack->canRedo());

   actionUndo->setText(QString(tr("Undo %1").arg(this->undoStack->undoText())));
   actionRedo->setText(QString(tr("Redo %1").arg(this->undoStack->redoText())));

   return;
}

void MainWindow::doOrRedoUpdate(QUndoCommand * update) {
   Q_ASSERT(this->undoStack != nullptr);
   Q_ASSERT(update != nullptr);
   this->undoStack->push(update);
   this->setUndoRedoEnable();
   return;
}

void MainWindow::doOrRedoUpdate(QObject & updatee,
                                BtStringConst const & propertyName,
                                QVariant newValue,
                                QString const & description,
                                QUndoCommand * parent) {
///   qDebug() << Q_FUNC_INFO << "Updating" << propertyName << "on" << updatee.metaObject()->className();
///   qDebug() << Q_FUNC_INFO << "this=" << static_cast<void *>(this);
   this->doOrRedoUpdate(new SimpleUndoableUpdate(updatee, propertyName, newValue, description));
   return;
}

// For undo/redo, we use Qt's Undo framework
void MainWindow::editUndo()
{
   Q_ASSERT(this->undoStack != 0);
   if ( !this->undoStack->canUndo() ) {
      qDebug() << "Undo called but nothing to undo";
   } else {
      this->undoStack->undo();
   }

   setUndoRedoEnable();
   return;
}

void MainWindow::editRedo()
{
   Q_ASSERT(this->undoStack != 0);
   if ( !this->undoStack->canRedo() ) {
      qDebug() << "Redo called but nothing to redo";
   } else {
      this->undoStack->redo();
   }

   setUndoRedoEnable();
   return;
}

Fermentable* MainWindow::selectedFermentable() {
   QModelIndexList selected = fermentableTable->selectionModel()->selectedIndexes();

   int size = selected.size();
   if (size == 0) {
      return nullptr;
   }

   // Make sure only one row is selected.
   QModelIndex viewIndex = selected[0];
   int row = viewIndex.row();
   for (int i = 1; i < size; ++i ) {
      if (selected[i].row() != row) {
         return nullptr;
      }
   }

   QModelIndex modelIndex = fermTableProxy->mapToSource(viewIndex);
   return fermTableModel->getRow(modelIndex.row()).get();
}

Hop* MainWindow::selectedHop() {
   QModelIndexList selected = hopTable->selectionModel()->selectedIndexes();

   int size = selected.size();
   if (size == 0) {
      return nullptr;
   }

   // Make sure only one row is selected.
   QModelIndex viewIndex = selected[0];
   int row = viewIndex.row();
   for (int i = 1; i < size; ++i ) {
      if (selected[i].row() != row) {
         return nullptr;
      }
   }

   QModelIndex modelIndex = hopTableProxy->mapToSource(viewIndex);
   return hopTableModel->getRow(modelIndex.row()).get();
}

Misc* MainWindow::selectedMisc() {
   QModelIndexList selected = miscTable->selectionModel()->selectedIndexes();

   int size = selected.size();
   if (size == 0) {
      return nullptr;
   }

   // Make sure only one row is selected.
   QModelIndex viewIndex = selected[0];
   int row = viewIndex.row();
   for (int i = 1; i < size; ++i ) {
      if (selected[i].row() != row) {
         return nullptr;
      }
   }

   QModelIndex modelIndex = miscTableProxy->mapToSource(viewIndex);
   return miscTableModel->getRow(modelIndex.row()).get();
}

Yeast* MainWindow::selectedYeast() {
   QModelIndexList selected = yeastTable->selectionModel()->selectedIndexes();

   int size = selected.size();
   if (size == 0) {
      return nullptr;
   }

   // Make sure only one row is selected.
   QModelIndex viewIndex = selected[0];
   int row = viewIndex.row();
   for (int i = 1; i < size; ++i ) {
      if (selected[i].row() != row) {
         return nullptr;
      }
   }

   QModelIndex modelIndex = yeastTableProxy->mapToSource(viewIndex);
   return yeastTableModel->getRow(modelIndex.row()).get();
}

void MainWindow::removeHop(std::shared_ptr<Hop> itemToRemove) {
   this->hopTableModel->remove(itemToRemove);
   return;
}
void MainWindow::removeFermentable(std::shared_ptr<Fermentable> itemToRemove) {
   this->fermTableModel->remove(itemToRemove);
   return;
}
void MainWindow::removeMisc(std::shared_ptr<Misc> itemToRemove) {
   this->miscTableModel->remove(itemToRemove);
   return;
}
void MainWindow::removeYeast(std::shared_ptr<Yeast> itemToRemove) {
   this->yeastTableModel->remove(itemToRemove);
   return;
}

void MainWindow::removeMashStep(std::shared_ptr<MashStep> itemToRemove) {
   this->mashStepTableModel->remove(itemToRemove);
   return;
}

void MainWindow::removeSelectedFermentable() {

   QModelIndexList selected = fermentableTable->selectionModel()->selectedIndexes();
   int size = selected.size();

   qDebug() << QString("MainWindow::removeSelectedFermentable() %1 items selected to remove").arg(size);

   if (size == 0) {
      return;
   }

   QList< std::shared_ptr<Fermentable> > itemsToRemove;
   for(int i = 0; i < size; i++) {
      QModelIndex viewIndex = selected.at(i);
      QModelIndex modelIndex = fermTableProxy->mapToSource(viewIndex);

      itemsToRemove.append(fermTableModel->getRow(modelIndex.row()));
   }

   for (auto item : itemsToRemove) {
      this->doOrRedoUpdate(
         newUndoableAddOrRemove(*this->recipeObs,
                                &Recipe::remove<Fermentable>,
                                item,
                                &Recipe::add<Fermentable>,
                                &MainWindow::removeFermentable,
                                static_cast<void (MainWindow::*)(std::shared_ptr<Fermentable>)>(nullptr),
                                tr("Remove fermentable from recipe"))
      );
    }

    return;
}

void MainWindow::editSelectedFermentable() {
   Fermentable* f = selectedFermentable();
   if( f == nullptr )
      return;

   fermEditor->setFermentable(f);
   fermEditor->show();
}

void MainWindow::editSelectedMisc() {
   Misc* m = selectedMisc();
   if( m == nullptr )
      return;

   miscEditor->setMisc(m);
   miscEditor->show();
}

void MainWindow::editSelectedHop()
{
   Hop* h = selectedHop();
   if( h == nullptr )
      return;

   hopEditor->setHop(h);
   hopEditor->show();
}

void MainWindow::editSelectedYeast()
{
   Yeast* y = selectedYeast();
   if( y == nullptr )
      return;

   yeastEditor->setYeast(y);
   yeastEditor->show();
}

void MainWindow::removeSelectedHop() {
   QModelIndexList selected = hopTable->selectionModel()->selectedIndexes();
   QList< std::shared_ptr<Hop> > itemsToRemove;

   int size = selected.size();
   if (size == 0) {
      return;
   }

   for (int i = 0; i < size; i++) {
      QModelIndex viewIndex = selected.at(i);
      QModelIndex modelIndex = hopTableProxy->mapToSource(viewIndex);
      itemsToRemove.append(hopTableModel->getRow(modelIndex.row()));
   }

   for (auto item : itemsToRemove) {
      this->doOrRedoUpdate(
         newUndoableAddOrRemove(*this->recipeObs,
                                 &Recipe::remove<Hop>,
                                 item,
                                 &Recipe::add<Hop>,
                                 &MainWindow::removeHop,
                                 static_cast<void (MainWindow::*)(std::shared_ptr<Hop>)>(nullptr),
                                 tr("Remove hop from recipe"))
      );
   }

}


void MainWindow::removeSelectedMisc() {
   QModelIndexList selected = miscTable->selectionModel()->selectedIndexes();
   QList< std::shared_ptr<Misc> > itemsToRemove;

   int size = selected.size();
   if (size == 0) {
      return;
   }

   for (int i = 0; i < size; i++) {
      QModelIndex viewIndex = selected.at(i);
      QModelIndex modelIndex = miscTableProxy->mapToSource(viewIndex);

      itemsToRemove.append(miscTableModel->getRow(modelIndex.row()));
   }

   for (auto item : itemsToRemove) {
      this->doOrRedoUpdate(
         newUndoableAddOrRemove(*this->recipeObs,
                                 &Recipe::remove<Misc>,
                                 item,
                                 &Recipe::add<Misc>,
                                 &MainWindow::removeMisc,
                                 static_cast<void (MainWindow::*)(std::shared_ptr<Misc>)>(nullptr),
                                 tr("Remove misc from recipe"))
      );
   }
}

void MainWindow::removeSelectedYeast() {
   QModelIndexList selected = yeastTable->selectionModel()->selectedIndexes();
   QList< std::shared_ptr<Yeast> > itemsToRemove;

   int size = selected.size();
   if (size == 0) {
      return;
   }

   for(int i = 0; i < size; i++) {
      QModelIndex viewIndex = selected.at(i);
      QModelIndex modelIndex = yeastTableProxy->mapToSource(viewIndex);

      itemsToRemove.append(yeastTableModel->getRow(modelIndex.row()));
   }

   for (auto item : itemsToRemove) {
      this->doOrRedoUpdate(
         newUndoableAddOrRemove(*this->recipeObs,
                                 &Recipe::remove<Yeast>,
                                 item,
                                 &Recipe::add<Yeast>,
                                 &MainWindow::removeYeast,
                                 static_cast<void (MainWindow::*)(std::shared_ptr<Yeast>)>(nullptr),
                                 tr("Remove yeast from recipe"))
      );
   }
}

void MainWindow::newRecipe()
{
   QString name = QInputDialog::getText(this, tr("Recipe name"),
                                          tr("Recipe name:"));
   QVariant defEquipKey = PersistentSettings::value(PersistentSettings::Names::defaultEquipmentKey, -1);
   QObject* selection = sender();

   if( name.isEmpty() )
      return;

   Recipe* newRec = new Recipe(name);

   // bad things happened -- let somebody know
   if ( ! newRec ) {
      QMessageBox::warning(this,tr("Error creating recipe"),
                           tr("An error was returned while creating %1").arg(name));
      return;
   }
   // Set the following stuff so everything appears nice
   // and the calculations don't divide by zero... things like that.
   newRec->setBatchSize_l(18.93); // 5 gallons
   newRec->setBoilSize_l(23.47);  // 6.2 gallons
   newRec->setEfficiency_pct(70.0);

   // we need a valid key, so insert the recipe before we add equipment
   if ( defEquipKey != -1 )
   {
      Equipment *e = ObjectStoreWrapper::getByIdRaw<Equipment>(defEquipKey.toInt());
      // I really want to do this before we've written the object to the
      // database
      if ( e ) {
         newRec->setBatchSize_l( e->batchSize_l() );
         newRec->setBoilSize_l( e->boilSize_l() );
         newRec->setBoilTime_min( e->boilTime_min() );
         newRec->setEquipment(e);
      }
   }

   ObjectStoreWrapper::insert(*newRec);

   // a new recipe will be put in a folder if you right click on a recipe or
   // folder. Otherwise, it goes into the main window?
   if ( selection ) {
      BtTreeView* sent = qobject_cast<BtTreeView*>(tabWidget_Trees->currentWidget()->focusWidget());
      if ( sent )
      {
         QModelIndexList indexes = sent->selectionModel()->selectedRows();
         // This is a little weird. There is an edge case where nothing is
         // selected and you click the big blue + button.
         if ( indexes.size() > 0 )
         {
            if ( sent->type(indexes.at(0)) == BtTreeItem::Type::RECIPE )
            {
               auto foo = sent->getItem<Recipe>(indexes.at(0));

               if ( foo && ! foo->folder().isEmpty()) {
                  newRec->setFolder( foo->folder() );
               }
            }
            else if ( sent->type(indexes.at(0)) == BtTreeItem::Type::FOLDER )
            {
               BtFolder* foo = sent->getItem<BtFolder>(indexes.at(0));
               if ( foo ) {
                  newRec->setFolder( foo->fullPath() );
               }
            }
         }
      }
   }
   setTreeSelection(treeView_recipe->findElement(newRec));
   setRecipe(newRec);
   return;
}

void MainWindow::newFolder() {
   // get the currently active tree
   BtTreeView* active = qobject_cast<BtTreeView*>(tabWidget_Trees->currentWidget()->focusWidget());

   if (!active) {
      return;
   }

   QModelIndexList indexes = active->selectionModel()->selectedRows();
   QModelIndex starter = indexes.at(0);

   // Where to start from
   QString dPath = active->folderName(starter);

   QString name = QInputDialog::getText(this, tr("Folder name"), tr("Folder name:"), QLineEdit::Normal, dPath);
   // User clicks cancel
   if (name.isEmpty())
      return;
   // Do some input validation here.

   // Nice little builtin to collapse leading and following white space
   name = name.simplified();
   if ( name.isEmpty() ) {
      QMessageBox::critical( this, tr("Bad Name"),
                             tr("A folder name must have at least one non-whitespace character in it"));
      return;
   }

#if QT_VERSION < QT_VERSION_CHECK(5,15,0)
   QString::SplitBehavior skip = QString::SkipEmptyParts;
#else
   Qt::SplitBehaviorFlags skip = Qt::SkipEmptyParts;
#endif
   if ( name.split("/", skip).isEmpty() ) {
      QMessageBox::critical( this, tr("Bad Name"), tr("A folder name must have at least one non-/ character in it"));
      return;
   }
   active->addFolder(name);
}

void MainWindow::renameFolder() {
   BtTreeView* active = qobject_cast<BtTreeView*>(tabWidget_Trees->currentWidget()->focusWidget());

   // If the sender cannot be morphed into a BtTreeView object
   if ( active == nullptr ) {
      return;
   }

   // I don't think I can figure out what the behavior will be if you select
   // many items
   QModelIndexList indexes = active->selectionModel()->selectedRows();
   QModelIndex starter = indexes.at(0);

   // The item to be renamed
   // Don't rename anything other than a folder
   if ( active->type(starter) != BtTreeItem::Type::FOLDER) {
      return;
   }

   BtFolder* victim = active->getItem<BtFolder>(starter);
   QString newName = QInputDialog::getText(this,
                                           tr("Folder name"),
                                           tr("Folder name:"),
                                           QLineEdit::Normal,
                                           victim->name());

   // User clicks cancel
   if (newName.isEmpty()) {
      return;
   }
   // Do some input validation here.

   // Nice little builtin to collapse leading and following white space
   newName = newName.simplified();
   if (newName.isEmpty()) {
      QMessageBox::critical( this, tr("Bad Name"),
                             tr("A folder name must have at least one non-whitespace character in it"));
      return;
   }

#if QT_VERSION < QT_VERSION_CHECK(5,15,0)
   QString::SplitBehavior skip = QString::SkipEmptyParts;
#else
   Qt::SplitBehaviorFlags skip = Qt::SkipEmptyParts;
#endif
   if ( newName.split("/", skip).isEmpty() ) {
      QMessageBox::critical( this, tr("Bad Name"), tr("A folder name must have at least one non-/ character in it"));
      return;
   }
   newName = victim->path() % "/" % newName;

   // Delgate this work to the tree.
   active->renameFolder(victim,newName);
}

void MainWindow::setTreeSelection(QModelIndex item) {
   qDebug() << Q_FUNC_INFO;

   if (! item.isValid()) {
      qDebug() << Q_FUNC_INFO << "Item not valid";
      return;
   }

   BtTreeView *active = qobject_cast<BtTreeView*>(tabWidget_Trees->currentWidget()->focusWidget());
   if ( active == nullptr ) {
      active = qobject_cast<BtTreeView*>(treeView_recipe);
   }

   // Couldn't cast the active item to a BtTreeView
   if ( active == nullptr ) {
      qDebug() << Q_FUNC_INFO << "Couldn't cast the active item to a BtTreeView";
      return;
   }

   QModelIndex parent = active->parent(item);

   active->setCurrentIndex(item);
   if ( active->type(parent) == BtTreeItem::Type::FOLDER && ! active->isExpanded(parent) ) {
      active->setExpanded(parent, true);
   }
   active->scrollTo(item,QAbstractItemView::PositionAtCenter);
   return;
}

// reduces the inventory by the selected recipes
void MainWindow::reduceInventory(){

   QModelIndexList indexes = treeView_recipe->selectionModel()->selectedRows();

   foreach(QModelIndex selected, indexes) {
      Recipe* rec = treeView_recipe->getItem<Recipe>(selected);
      if ( rec == nullptr ) {
         //try the parent recipe
         rec = treeView_recipe->getItem<Recipe>(treeView_recipe->parent(selected));
         if ( rec == nullptr ) {
            continue;
         }
      }

      // Make sure everything is properly set and selected
      if( rec != recipeObs ) {
         setRecipe(rec);
      }

      int i = 0;
      //reduce fermentables
      QList<Fermentable*> flist = rec->fermentables();
      if ( flist.size() > 0 ){
         for( i = 0; static_cast<int>(i) < flist.size(); ++i ) {
            double newVal=flist[i]->inventory() - flist[i]->amount_kg();
            newVal = (newVal < 0) ? 0 : newVal;
            flist[i]->setInventoryAmount(newVal);
         }
      }

      //reduce misc
      QList<Misc*> mlist = rec->miscs();
      if ( mlist.size() > 0 ) {
         for( i = 0; static_cast<int>(i) < mlist.size(); ++i ) {
            double newVal=mlist[i]->inventory() - mlist[i]->amount();
            newVal = (newVal < 0) ? 0 : newVal;
            mlist[i]->setInventoryAmount(newVal);
         }
      }
      //reduce hops
      QList<Hop*> hlist = rec->hops();
      if( hlist.size() > 0 ) {
         for( i = 0; static_cast<int>(i) < hlist.size(); ++i ) {
            double newVal = hlist[i]->inventory() - hlist[i]->amount_kg();
            newVal = (newVal < 0) ? 0 : newVal;
            hlist[i]->setInventoryAmount(newVal);
         }
      }
      //reduce yeast
      QList<Yeast*> ylist = rec->yeasts();
      if(ylist.size() > 0){
         for( i = 0; static_cast<int>(i) < ylist.size(); ++i )
         {
            //Yeast inventory is done by quanta not amount
            // .:TBD:. I think "quanta" is being used to mean "number of packets" or something
            int newVal = ylist[i]->inventory() - 1;
            newVal = (newVal < 0) ? 0 : newVal;
            ylist[i]->setInventoryQuanta(newVal);
         }
      }
   }
}

// Need to make sure the recipe tree is active, I think
void MainWindow::newBrewNote() {
   QModelIndexList indexes = treeView_recipe->selectionModel()->selectedRows();
   QModelIndex bIndex;

   for (QModelIndex selected : indexes) {
      Recipe*   rec   = treeView_recipe->getItem<Recipe>(selected);
      if (!rec) {
         continue;
      }

      // Make sure everything is properly set and selected
      if (rec != recipeObs) {
         setRecipe(rec);
      }

      auto bNote = std::make_shared<BrewNote>(*rec);
      bNote->populateNote(rec);
      bNote->setBrewDate();
      ObjectStoreWrapper::insert(bNote);

      this->setBrewNote(bNote.get());

      bIndex = treeView_recipe->findElement(bNote.get());
      if ( bIndex.isValid() )
         setTreeSelection(bIndex);
   }
}

void MainWindow::reBrewNote() {
   QModelIndexList indexes = treeView_recipe->selectionModel()->selectedRows();
   for (QModelIndex selected : indexes) {
      BrewNote* old   = treeView_recipe->getItem<BrewNote>(selected);
      Recipe* rec     = treeView_recipe->getItem<Recipe>(treeView_recipe->parent(selected));

      if (! old || ! rec) {
         return;
      }

      auto bNote = std::make_shared<BrewNote>(*old);
      bNote->setBrewDate();
      ObjectStoreWrapper::insert(bNote);

      if (rec != recipeObs) {
         setRecipe(rec);
      }

      setBrewNote(bNote.get());

      setTreeSelection(treeView_recipe->findElement(bNote.get()));
   }
}

void MainWindow::brewItHelper() {
   newBrewNote();
   reduceInventory();
}

void MainWindow::brewAgainHelper() {
   reBrewNote();
   reduceInventory();
}

void MainWindow::backup() {
   // NB: QDir does all the necessary magic of translating '/' to whatever current platform's directory separator is
   QString defaultBackupFileName = QDir::currentPath() + "/" + Database::getDefaultBackupFileName();
   QString backupFileName = QFileDialog::getSaveFileName(this, tr("Backup Database"), defaultBackupFileName);
   qDebug() << QString("Database backup filename \"%1\"").arg(backupFileName);

   // If the filename returned from the dialog is empty, it means the user clicked cancel, so we should stop trying to do the backup
   if (!backupFileName.isEmpty())
   {
      bool success = Database::instance().backupToFile(backupFileName);

      if( ! success )
         QMessageBox::warning( this, tr("Oops!"), tr("Could not copy the files for some reason."));
   }
}

void MainWindow::restoreFromBackup()
{
   if( QMessageBox::question(
          this,
          tr("A Warning"),
          tr("This will obliterate your current set of recipes and ingredients. Do you want to continue?"),
          QMessageBox::Yes, QMessageBox::No
       )
       == QMessageBox::No
   )
   {
      return;
   }

   QString restoreDbFile = QFileDialog::getOpenFileName(this, tr("Choose File"), "", tr("SQLite (*.sqlite)"));
   bool success = Database::instance().restoreFromFile(restoreDbFile);

   if( ! success )
      QMessageBox::warning( this, tr("Oops!"), tr("For some reason, the operation failed.") );
   else
      QMessageBox::information(this, tr("Restart"), tr("Please restart Brewken."));
   //TODO: do this without requiring restarting :)
}

// Imports all the recipes, hops, equipment or whatever from a BeerXML file into the database.
void MainWindow::importFiles() {
   ImportExport::importFromFiles();
   return;
}

bool MainWindow::verifyImport(QString tag, QString name)
{
   return QMessageBox::question(this, tr("Import %1?").arg(tag), tr("Import %1?").arg(name),
                                QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes;
}

void MainWindow::addMashStep() {
   if( recipeObs == nullptr || recipeObs->mash() == nullptr ) {
      QMessageBox::information(this, tr("No mash"), tr("Trying to add a mash step without a mash. Please create a mash first.") );
      return;
   }

   // This ultimately gets stored in MainWindow::addMashStepToMash()
   auto step = std::make_shared<MashStep>("");
   this->mashStepEditor->setMashStep(step);
   this->mashStepEditor->setVisible(true);
   return;
}

void MainWindow::removeSelectedMashStep() {
   if (!this->recipeObs) {
      return;
   }
   Mash* mash = this->recipeObs->mash();
   if (!mash) {
      return;
   }

   QModelIndexList selected = mashStepTableWidget->selectionModel()->selectedIndexes();

   int size = selected.size();
   if (size == 0) {
      return;
   }

   // Make sure only one row is selected.
   int row = selected[0].row();
   for (int i = 1; i < size; ++i) {
      if (selected[i].row() != row) {
         return;
      }
   }

   auto step = mashStepTableModel->getRow(row);
   this->doOrRedoUpdate(
      newUndoableAddOrRemove(*this->recipeObs->mash(),
                              &Mash::removeMashStep,
                              step,
                              &Mash::addMashStep,
                              &MainWindow::removeMashStep,
                              static_cast<void (MainWindow::*)(std::shared_ptr<MashStep>)>(nullptr),
                              tr("Remove mash step"))
   );

   return;
}

void MainWindow::moveSelectedMashStepUp()
{
   QModelIndexList selected = mashStepTableWidget->selectionModel()->selectedIndexes();
   int row, size, i;

   size = selected.size();
   if( size == 0 )
      return;

   // Make sure only one row is selected.
   row = selected[0].row();
   for( i = 1; i < size; ++i )
   {
      if( selected[i].row() != row )
         return;
   }

   // Make sure we can actually move it up.
   if( row < 1 ) {
      return;
   }

   this->mashStepTableModel->moveStepUp(row);
   return;
}

void MainWindow::moveSelectedMashStepDown()
{
   QModelIndexList selected = mashStepTableWidget->selectionModel()->selectedIndexes();
   int row, size, i;

   size = selected.size();
   if( size == 0 )
      return;

   // Make sure only one row is selected.
   row = selected[0].row();
   for( i = 1; i < size; ++i )
   {
      if( selected[i].row() != row )
         return;
   }

   // Make sure it's not the last row so we can move it down.
   if( row >= mashStepTableModel->rowCount() - 1 ) {
      return;
   }

   this->mashStepTableModel->moveStepDown(row);
   return;
}

void MainWindow::editSelectedMashStep() {
   if (!recipeObs || !recipeObs->mash()) {
      return;
   }

   QModelIndexList selected = mashStepTableWidget->selectionModel()->selectedIndexes();

   int size = selected.size();
   if (size == 0) {
      return;
   }

   // Make sure only one row is selected.
   int row = selected[0].row();
   for (int i = 1; i < size; ++i) {
      if (selected[i].row() != row) {
         return;
      }
   }

   auto step = mashStepTableModel->getRow(static_cast<unsigned int>(row));
   mashStepEditor->setMashStep(step);
   mashStepEditor->setVisible(true);
   return;
}

void MainWindow::removeMash() {
   Mash *m = mashButton->mash();

   if( m == nullptr) {
      return;
   }

   //due to way this is designed, we can't have a NULL mash, so
   //we need to remove all the mash steps and then remove the mash
   //from the database.
   //remove from db

   m->removeAllMashSteps();
   ObjectStoreWrapper::softDelete(*m);

   auto defaultMash = std::make_shared<Mash>();
   ObjectStoreWrapper::insert(defaultMash);
   this->recipeObs->setMash(defaultMash.get());

   mashStepTableModel->setMash(defaultMash.get());

   //remove from combobox handled automatically by qt
   mashButton->setMash(defaultMash.get());

}

void MainWindow::closeEvent(QCloseEvent* /*event*/)
{
   Brewken::saveSystemOptions();
   PersistentSettings::insert(PersistentSettings::Names::geometry, saveGeometry());
   PersistentSettings::insert(PersistentSettings::Names::windowState, saveState());
   if ( recipeObs )
      PersistentSettings::insert(PersistentSettings::Names::recipeKey, recipeObs->key());

   //UI save state
   PersistentSettings::insert(PersistentSettings::Names::splitter_vertical_State, splitter_vertical->saveState(), PersistentSettings::Sections::MainWindow);
   PersistentSettings::insert(PersistentSettings::Names::splitter_horizontal_State, splitter_horizontal->saveState(), PersistentSettings::Sections::MainWindow);
   PersistentSettings::insert(PersistentSettings::Names::treeView_recipe_headerState, treeView_recipe->header()->saveState(), PersistentSettings::Sections::MainWindow);
   PersistentSettings::insert(PersistentSettings::Names::treeView_style_headerState, treeView_style->header()->saveState(), PersistentSettings::Sections::MainWindow);
   PersistentSettings::insert(PersistentSettings::Names::treeView_equip_headerState, treeView_equip->header()->saveState(), PersistentSettings::Sections::MainWindow);
   PersistentSettings::insert(PersistentSettings::Names::treeView_ferm_headerState, treeView_ferm->header()->saveState(), PersistentSettings::Sections::MainWindow);
   PersistentSettings::insert(PersistentSettings::Names::treeView_hops_headerState, treeView_hops->header()->saveState(), PersistentSettings::Sections::MainWindow);
   PersistentSettings::insert(PersistentSettings::Names::treeView_misc_headerState, treeView_misc->header()->saveState(), PersistentSettings::Sections::MainWindow);
   PersistentSettings::insert(PersistentSettings::Names::treeView_yeast_headerState, treeView_yeast->header()->saveState(), PersistentSettings::Sections::MainWindow);
   PersistentSettings::insert(PersistentSettings::Names::mashStepTableWidget_headerState, mashStepTableWidget->horizontalHeader()->saveState(), PersistentSettings::Sections::MainWindow);

   // After unloading the database, can't make any more queries to it, so first
   // make the main window disappear so that redraw events won't inadvertently
   // cause any more queries.
   setVisible(false);

}

void MainWindow::copyRecipe()
{
   QString name = QInputDialog::getText( this, tr("Copy Recipe"), tr("Enter a unique name for the copy.") );

   if (name.isEmpty()) {
      return;
   }

   auto newRec = std::make_shared<Recipe>(*this->recipeObs); // Create a deep copy
   newRec->setName(name);
   ObjectStoreTyped<Recipe>::getInstance().insert(newRec);
   return;
}

void MainWindow::saveMash() {
   if ( recipeObs == nullptr || recipeObs->mash() == nullptr ) {
      return;
   }

   Mash* mash = recipeObs->mash();
   // Ensure the mash has a name.
   if( mash->name() == "" ) {
      QMessageBox::information( this, tr("Oops!"), tr("Please give your mash a name before saving.") );
      return;
   }


   // The current UI doesn't make this 100% clear, but what we're actually doing here is saving a _copy_ of the current
   // Recipe's mash.

   // NOTE: should NOT displace recipeObs' current mash.
   auto newMash = ObjectStoreWrapper::insertCopyOf(*mash);
   // NOTE: need to set the display to true for the saved, named mash to work
   newMash->setDisplay(true);
   mashButton->setMash(newMash.get());
   return;
}

// We build the menus at start up time.  This just needs to exec the proper
// menu.
void MainWindow::contextMenu(const QPoint &point)
{
   QObject* calledBy = sender();
   BtTreeView* active;
   QModelIndex selected;
   QMenu* tempMenu;

   // Not sure how this could happen, but better safe the sigsegv'd
   if ( calledBy == nullptr )
      return;

   active = qobject_cast<BtTreeView*>(calledBy);

   // If the sender cannot be morphed into a BtTreeView object
   if ( active == nullptr )
      return;

   selected = active->indexAt(point);
   if (! selected.isValid())
      return;

   tempMenu = active->contextMenu(selected);

   if (tempMenu)
      tempMenu->exec(active->mapToGlobal(point));
}

void MainWindow::setupContextMenu()
{

   treeView_recipe->setupContextMenu(this,this);
   treeView_equip->setupContextMenu(this,singleEquipEditor);

   treeView_ferm->setupContextMenu(this,fermDialog);
   treeView_hops->setupContextMenu(this,hopDialog);
   treeView_misc->setupContextMenu(this,miscDialog);
   treeView_style->setupContextMenu(this,singleStyleEditor);
   treeView_yeast->setupContextMenu(this,yeastDialog);
   treeView_water->setupContextMenu(this,waterEditor);

   // TreeView for clicks, both double and right
   connect( treeView_recipe, &QAbstractItemView::doubleClicked, this, &MainWindow::treeActivated);
   connect( treeView_recipe, &QWidget::customContextMenuRequested, this, &MainWindow::contextMenu);

   connect( treeView_equip, &QAbstractItemView::doubleClicked, this, &MainWindow::treeActivated);
   connect( treeView_equip, &QWidget::customContextMenuRequested, this, &MainWindow::contextMenu);

   connect( treeView_ferm, &QAbstractItemView::doubleClicked, this, &MainWindow::treeActivated);
   connect( treeView_ferm, &QWidget::customContextMenuRequested, this, &MainWindow::contextMenu);

   connect( treeView_hops, &QAbstractItemView::doubleClicked, this, &MainWindow::treeActivated);
   connect( treeView_hops, &QWidget::customContextMenuRequested, this, &MainWindow::contextMenu);

   connect( treeView_misc, &QAbstractItemView::doubleClicked, this, &MainWindow::treeActivated);
   connect( treeView_misc, &QWidget::customContextMenuRequested, this, &MainWindow::contextMenu);

   connect( treeView_yeast, &QAbstractItemView::doubleClicked, this, &MainWindow::treeActivated);
   connect( treeView_yeast, &QWidget::customContextMenuRequested, this, &MainWindow::contextMenu);

   connect( treeView_style, &QAbstractItemView::doubleClicked, this, &MainWindow::treeActivated);
   connect( treeView_style, &QWidget::customContextMenuRequested, this, &MainWindow::contextMenu);

   connect( treeView_water, &QAbstractItemView::doubleClicked, this, &MainWindow::treeActivated);
   connect( treeView_water, &QWidget::customContextMenuRequested, this, &MainWindow::contextMenu);
}

void MainWindow::copySelected()
{
   QModelIndexList selected;
   BtTreeView* active = qobject_cast<BtTreeView*>(tabWidget_Trees->currentWidget()->focusWidget());

   active->copySelected(active->selectionModel()->selectedRows());
}

void MainWindow::exportSelected() {
   BtTreeView const * active = qobject_cast<BtTreeView*>(this->tabWidget_Trees->currentWidget()->focusWidget());
   if (active == nullptr) {
      qDebug() << Q_FUNC_INFO << "No active tree so can't get a selection";
      return;
   }

   QModelIndexList selected = active->selectionModel()->selectedRows();
   if (selected.count() == 0) {
      qDebug() << Q_FUNC_INFO << "Nothing selected, so nothing to export";
      return;
   }

   //
   // I think the way that UI works at the moment, we're only going to get one type of thing selected at a time.
   // Nevertheless, if this were to change in future, there is no inherent reason not to be able to export different
   // types of things at the same time.
   //
   // We therefore gather all the selected things together so that we write out all the Hops together, all the Styles
   // together and so on, because BeerXML wants them all in group tags (<HOPS>...</HOPS>, etc).
   //
   QList<Equipment *>   equipments;
   QList<Fermentable *> fermentables;
   QList<Hop *>         hops;
   QList<Misc *>        miscs;
   QList<Recipe *>      recipes;
   QList<Style *>       styles;
   QList<Water *>       waters;
   QList<Yeast *>       yeasts;

   int count = 0;
   for (auto & selection : selected) {
      auto itemType = active->type(selection);
      if (!itemType) {
         qWarning() << Q_FUNC_INFO << "Unknown type for selection" << selection;
      } else {
         switch(*itemType) {
            case BtTreeItem::Type::RECIPE:
               recipes.append(treeView_recipe->getItem<Recipe>(selection));
               ++count;
               break;
            case BtTreeItem::Type::EQUIPMENT:
               equipments.append(treeView_equip->getItem<Equipment>(selection));
               ++count;
               break;
            case BtTreeItem::Type::FERMENTABLE:
               fermentables.append(treeView_ferm->getItem<Fermentable>(selection));
               ++count;
               break;
            case BtTreeItem::Type::HOP:
               hops.append(treeView_hops->getItem<Hop>(selection));
               ++count;
               break;
            case BtTreeItem::Type::MISC:
               miscs.append(treeView_misc->getItem<Misc>(selection));
               ++count;
               break;
            case BtTreeItem::Type::STYLE:
               styles.append(treeView_style->getItem<Style>(selection));
               ++count;
               break;
            case BtTreeItem::Type::WATER:
               waters.append(treeView_water->getItem<Water>(selection));
               ++count;
               break;
            case BtTreeItem::Type::YEAST:
               yeasts.append(treeView_yeast->getItem<Yeast>(selection));
               ++count;
               break;
            case BtTreeItem::Type::FOLDER:
               qDebug() << Q_FUNC_INFO << "Can't export selected Folder to XML as BeerXML does not support it";
               break;
            case BtTreeItem::Type::BREWNOTE:
               qDebug() << Q_FUNC_INFO << "Can't export selected BrewNote to XML as BeerXML does not support it";
               break;
            default:
               // This shouldn't happen, because we should explicitly cover all the types above
               qWarning() << Q_FUNC_INFO << "Don't know how to export BtTreeItem type" << static_cast<int>(*itemType);
               break;
         }
      }
   }

   if (0 == count) {
      qDebug() << Q_FUNC_INFO << "Nothing selected was exportable to XML";
      QMessageBox msgBox{QMessageBox::Critical,
                         tr("Nothing to export"),
                         tr("None of the selected items is exportable")};
      msgBox.exec();
      return;
   }

   ImportExport::exportToFile(&recipes,
                              &equipments,
                              &fermentables,
                              &hops,
                              &miscs,
                              &styles,
                              &waters,
                              &yeasts);
   return;
}

void MainWindow::finishCheckingVersion()
{
   QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
   if( reply == nullptr )
      return;

   QString remoteVersion(reply->readAll());

   // If there is an error, just return.
   if( reply->error() != QNetworkReply::NoError )
      return;

   // If the remote version is newer...
   if( !remoteVersion.startsWith(VERSIONSTRING) )
   {
      // ...and the user wants to download the new version...
      if( QMessageBox::information(this,
                                   QObject::tr("New Version"),
                                   QObject::tr("Version %1 is now available. Download it?").arg(remoteVersion),
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::Yes) == QMessageBox::Yes )
      {
         // ...take them to the website.
         QDesktopServices::openUrl(QUrl("http://www.brewken.org/download.html"));
      }
      else // ... and the user does NOT want to download the new version...
      {
         // ... and they want us to stop bothering them...
         if( QMessageBox::question(this,
                                   QObject::tr("New Version"),
                                   QObject::tr("Stop bothering you about new versions?"),
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::Yes) == QMessageBox::Yes)
         {
            // ... make a note to stop bothering the user about the new version.
            Brewken::setCheckVersion(false);
         }
      }
   }
   else // The current version is newest so...
   {
      // ...make a note to bother users about future new versions.
      // This means that when a user downloads the new version, this
      // variable will always get reset to true.
      Brewken::setCheckVersion(true);
   }
}

void MainWindow::redisplayLabel()
{
   // There is a lot of magic going on in the showChanges(). I can either
   // duplicate that magic or I can just call showChanges().
   showChanges();
}

void MainWindow::showPitchDialog()
{
   // First, copy the current recipe og and volume.
   if( recipeObs )
   {
      pitchDialog->setWortVolume_l( recipeObs->finalVolume_l() );
      pitchDialog->setWortDensity( recipeObs->og() );
      pitchDialog->calculate();
   }

   pitchDialog->show();
}

void MainWindow::showEquipmentEditor()
{
   if ( recipeObs && ! recipeObs->equipment() )
   {
      QMessageBox::warning( this, tr("No equipment"), tr("You must select or define an equipment profile first."));
   }
   else
   {
      singleEquipEditor->setEquipment(recipeObs->equipment());
      singleEquipEditor->show();
   }
}

void MainWindow::showStyleEditor()
{
   if ( recipeObs && ! recipeObs->style() )
   {
      QMessageBox::warning( this, tr("No style"), tr("You must select a style first."));
   }
   else
   {
      singleStyleEditor->setStyle(recipeObs->style());
      singleStyleEditor->show();
   }
}

void MainWindow::changeBrewDate()
{
   QModelIndexList indexes = treeView_recipe->selectionModel()->selectedRows();
   QDate newDate;

   for (QModelIndex selected : indexes) {
      auto target = treeView_recipe->getItem<BrewNote>(selected);

      // No idea how this could happen, but I've seen stranger things
      if ( ! target )
         continue;

      // Pop the calendar, get the date.
      if ( btDatePopup->exec() == QDialog::Accepted )
      {
         newDate = btDatePopup->selectedDate();
         target->setBrewDate(newDate);

         // If this note is open in a tab
         BrewNoteWidget* ni = findBrewNoteWidget(target);
         if ( ni )
         {
            tabWidget_recipeView->setTabText(tabWidget_recipeView->indexOf(ni), target->brewDate_short());
            return;
         }
      }
   }
}

void MainWindow::fixBrewNote()
{
   QModelIndexList indexes = treeView_recipe->selectionModel()->selectedRows();
   QDate newDate;

   for (QModelIndex selected : indexes) {
      auto target = treeView_recipe->getItem<BrewNote>(selected);

      // No idea how this could happen, but I've seen stranger things
      if ( ! target ) {
         continue;
      }

      auto noteParent = treeView_recipe->getItem<Recipe>( treeView_recipe->parent(selected));

      if ( ! noteParent ) {
         continue;
      }

      target->recalculateEff(noteParent);
   }
}

void MainWindow::updateStatus(const QString status) {
   if( statusBar() )
      statusBar()->showMessage(status, 3000);
}

void MainWindow::versionedRecipe(Recipe* descendant)
{
   QModelIndex ndx = treeView_recipe->findElement(descendant);
   setRecipe(descendant);
   treeView_recipe->setCurrentIndex(ndx);
}

void MainWindow::closeBrewNote(int brewNoteId, std::shared_ptr<QObject> object) {
   BrewNote* b = std::static_pointer_cast<BrewNote>(object).get();
   Recipe* parent = ObjectStoreWrapper::getByIdRaw<Recipe>(b->getRecipeId());

   // If this isn't the focused recipe, do nothing because there are no tabs
   // to close.
   if ( parent != recipeObs )
      return;

   BrewNoteWidget* ni = findBrewNoteWidget(b);

   if ( ni )
      tabWidget_recipeView->removeTab( tabWidget_recipeView->indexOf(ni));

   return;

}

void MainWindow::popChemistry()
{
   bool allow = false;

   if ( recipeObs ) {

      Mash* eMash = recipeObs->mash();
      if ( eMash && eMash->mashSteps().size() > 0 ) {
         allow = true;
      }
   }

   // late binding for the win?
   if (allow ) {
      waterDialog->setRecipe(recipeObs);
      waterDialog->show();
   }
   else {
      QMessageBox::warning( this, tr("No Mash"), tr("You must define a mash first."));
   }
}
