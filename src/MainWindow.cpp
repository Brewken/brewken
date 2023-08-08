/*======================================================================================================================
 * MainWindow.cpp is part of Brewken, and is copyright the following authors 2009-2023:
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

#include <algorithm>
#include <memory>
#include <mutex> // For std::once_flag etc

#include <QAction>
#include <QBrush>
#include <QDesktopWidget>
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
#include <QPen>
#include <QPixmap>
#include <QSize>
#include <QString>
#include <QTextStream>
#include <QtGui>
#include <QToolButton>
#include <QUndoStack>
#include <QUrl>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

#include "AboutDialog.h"
#include "AlcoholTool.h"
#include "Algorithms.h"
#include "AncestorDialog.h"
#include "Application.h"
#include "BrewNoteWidget.h"
#include "BtDatePopup.h"
#include "BtFolder.h"
#include "BtHorizontalTabs.h"
#include "BtTabWidget.h"
#include "catalogs/EquipmentCatalog.h"
#include "catalogs/FermentableCatalog.h"
#include "catalogs/HopCatalog.h"
#include "catalogs/MiscCatalog.h"
#include "catalogs/StyleCatalog.h"
#include "catalogs/YeastCatalog.h"
#include "config.h"
#include "ConverterTool.h"
#include "database/Database.h"
#include "database/ObjectStoreWrapper.h"
#include "editors/EquipmentEditor.h"
#include "editors/FermentableEditor.h"
#include "editors/HopEditor.h"
#include "editors/MashEditor.h"
#include "editors/MashStepEditor.h"
#include "editors/MiscEditor.h"
#include "editors/NamedMashEditor.h"
#include "editors/StyleEditor.h"
#include "editors/WaterEditor.h"
#include "editors/YeastEditor.h"
#include "HelpDialog.h"
#include "Html.h"
#include "HydrometerTool.h"
#include "ImportExport.h"
#include "InventoryFormatter.h"
#include "listModels/EquipmentListModel.h"
#include "listModels/MashListModel.h"
#include "listModels/StyleListModel.h"
#include "listModels/WaterListModel.h"
#include "MashDesigner.h"
#include "MashWizard.h"
#include "measurement/Measurement.h"
#include "measurement/Unit.h"
#include "model/Boil.h"
#include "model/BrewNote.h"
#include "model/Equipment.h"
#include "model/Fermentable.h"
#include "model/Fermentation.h"
#include "model/Mash.h"
#include "model/Recipe.h"
#include "model/Style.h"
#include "model/Yeast.h"
#include "OgAdjuster.h"
#include "OptionDialog.h"
#include "PersistentSettings.h"
#include "PitchDialog.h"
#include "PrimingDialog.h"
#include "PrintAndPreviewDialog.h"
#include "RangedSlider.h"
#include "RecipeFormatter.h"
#include "RefractoDialog.h"
#include "ScaleRecipeTool.h"
#include "sortFilterProxyModels/FermentableSortFilterProxyModel.h"
#include "sortFilterProxyModels/RecipeAdditionHopSortFilterProxyModel.h"
#include "sortFilterProxyModels/MiscSortFilterProxyModel.h"
#include "sortFilterProxyModels/StyleSortFilterProxyModel.h"
#include "sortFilterProxyModels/YeastSortFilterProxyModel.h"
#include "StrikeWaterDialog.h"
#include "tableModels/FermentableTableModel.h"
#include "tableModels/RecipeAdditionHopTableModel.h"
#include "tableModels/MashStepTableModel.h"
#include "tableModels/MiscTableModel.h"
#include "tableModels/YeastTableModel.h"
#include "TimerMainDialog.h"
#include "undoRedo/RelationalUndoableUpdate.h"
#include "undoRedo/UndoableAddOrRemove.h"
#include "undoRedo/UndoableAddOrRemoveList.h"
#include "utils/BtStringConst.h"
#include "utils/OptionalHelpers.h"
#include "WaterDialog.h"

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

   /**
    *
    */
   void updateDensitySlider(RangedSlider & slider,
                            SmartLabel const & label,
                            double const minCanonicalValue,
                            double const maxCanonicalValue,
                            double const maxPossibleCanonicalValue) {
      slider.setPreferredRange(label.getRangeToDisplay(minCanonicalValue, maxCanonicalValue        ));
      slider.setRange         (label.getRangeToDisplay(1.000            , maxPossibleCanonicalValue));

      Measurement::UnitSystem const & displayUnitSystem = label.getDisplayUnitSystem();
      if (displayUnitSystem == Measurement::UnitSystems::density_Plato) {
         slider.setPrecision(1);
         slider.setTickMarks(2, 5);
      } else {
         slider.setPrecision(3);
         slider.setTickMarks(0.010, 2);
      }
      return;
   }

   /**
    *
    */
   void updateColorSlider(RangedSlider & slider,
                          SmartLabel const & label,
                          double const minCanonicalValue,
                          double const maxCanonicalValue) {
      slider.setPreferredRange(label.getRangeToDisplay(minCanonicalValue, maxCanonicalValue));
      slider.setRange         (label.getRangeToDisplay(1                , 44               ));

      Measurement::UnitSystem const & displayUnitSystem = label.getDisplayUnitSystem();
      slider.setTickMarks(displayUnitSystem == Measurement::UnitSystems::color_StandardReferenceMethod ? 10 : 40, 2);

      return;
   }

}

// This private implementation class holds all private non-virtual members of MainWindow
class MainWindow::impl {
public:

   impl(MainWindow & self) :
      m_self{self},
      fileOpener{},
      m_undoStack{std::make_unique<QUndoStack>(&m_self)},
      m_fermTableModel    {nullptr},
      m_hopAdditionsTableModel     {nullptr},
      m_mashStepTableModel{nullptr},
      m_miscTableModel    {nullptr},
      m_yeastTableModel   {nullptr},
      m_fermTableProxy    {nullptr},
      m_hopAdditionsTableProxy     {nullptr},
      m_miscTableProxy    {nullptr},
      m_styleProxyModel   {nullptr},
      m_yeastTableProxy   {nullptr} {
      return;
   }

   ~impl() = default;

   /**
    * \brief Configure the tables and their proxies
    *
    *        Anything creating new tables models, filter proxies and configuring the two should go in here
    */
   void setupTables() {
      // Set table models.
      // Fermentables
      m_fermTableModel = std::make_unique<FermentableTableModel>(m_self.fermentableTable);
      m_fermTableProxy = std::make_unique<FermentableSortFilterProxyModel>(m_self.fermentableTable, false);
      m_fermTableProxy->setSourceModel(m_fermTableModel.get());
      m_self.fermentableTable->setItemDelegate(new FermentableItemDelegate(m_self.fermentableTable, *m_fermTableModel));
      m_self.fermentableTable->setModel(m_fermTableProxy.get());
      // Make the fermentable table show grain percentages in row headers.
      m_fermTableModel->setDisplayPercentages(true);
      // Double clicking the name column pops up an edit dialog for the selected item
      connect(m_self.fermentableTable, &QTableView::doubleClicked, &m_self, [&](const QModelIndex &idx) {
         if (idx.column() == 0) {
            m_self.editSelectedFermentable();
         }
      });

      // Hop additions
      m_hopAdditionsTableModel = std::make_unique<RecipeAdditionHopTableModel>(m_self.hopAdditionTable);
      m_hopAdditionsTableProxy = std::make_unique<RecipeAdditionHopSortFilterProxyModel>(m_self.hopAdditionTable, false);
      m_hopAdditionsTableProxy->setSourceModel(m_hopAdditionsTableModel.get());
      m_self.hopAdditionTable->setItemDelegate(new RecipeAdditionHopItemDelegate(m_self.hopAdditionTable, *m_hopAdditionsTableModel));
      m_self.hopAdditionTable->setModel(m_hopAdditionsTableProxy.get());
      // RecipeAdditionHop table show IBUs in row headers.
      m_hopAdditionsTableModel->setShowIBUs(true);
      connect(m_self.hopAdditionTable, &QTableView::doubleClicked, &m_self, [&](const QModelIndex &idx) {
         if (idx.column() == 0) {
            m_self.editHopOfSelectedHopAddition();
         }
      });

      // Misc
      m_miscTableModel = std::make_unique<MiscTableModel>(m_self.miscTable);
      m_miscTableProxy = std::make_unique<MiscSortFilterProxyModel>(m_self.miscTable, false);
      m_miscTableProxy->setSourceModel(m_miscTableModel.get());
      m_self.miscTable->setItemDelegate(new MiscItemDelegate(m_self.miscTable, *m_miscTableModel));
      m_self.miscTable->setModel(m_miscTableProxy.get());
      connect(m_self.miscTable, &QTableView::doubleClicked, &m_self, [&](const QModelIndex &idx) {
         if (idx.column() == 0) {
            m_self.editSelectedMisc();
         }
      });

      // Yeast
      m_yeastTableModel = std::make_unique<YeastTableModel>(m_self.yeastTable);
      m_yeastTableProxy = std::make_unique<YeastSortFilterProxyModel>(m_self.yeastTable, false);
      m_yeastTableProxy->setSourceModel(m_yeastTableModel.get());
      m_self.yeastTable->setItemDelegate(new YeastItemDelegate(m_self.yeastTable, *m_yeastTableModel));
      m_self.yeastTable->setModel(m_yeastTableProxy.get());
      connect(m_self.yeastTable, &QTableView::doubleClicked, &m_self, [&](const QModelIndex &idx) {
         if (idx.column() == 0) {
            m_self.editSelectedYeast();
         }
      });

      // Mashes
      m_mashStepTableModel = std::make_unique<MashStepTableModel>(m_self.mashStepTableWidget);
      m_self.mashStepTableWidget->setItemDelegate(new MashStepItemDelegate(m_self.mashStepTableWidget, *m_mashStepTableModel));
      m_self.mashStepTableWidget->setModel(m_mashStepTableModel.get());
      connect(m_self.mashStepTableWidget, &QTableView::doubleClicked, &m_self, [&](const QModelIndex &idx) {
         if (idx.column() == 0) {
            m_self.editSelectedMashStep();
         }
      });

      // Enable sorting in the main tables.
      m_self.fermentableTable->horizontalHeader()->setSortIndicator(static_cast<int>(FermentableTableModel::ColumnIndex::Amount), Qt::DescendingOrder );
      m_self.fermentableTable->setSortingEnabled(true);
      m_fermTableProxy->setDynamicSortFilter(true);
      m_self.hopAdditionTable->horizontalHeader()->setSortIndicator(static_cast<int>(RecipeAdditionHopTableModel::ColumnIndex::Time), Qt::DescendingOrder );
      m_self.hopAdditionTable->setSortingEnabled(true);
      m_hopAdditionsTableProxy->setDynamicSortFilter(true);
      m_self.miscTable->horizontalHeader()->setSortIndicator(static_cast<int>(MiscTableModel::ColumnIndex::Use), Qt::DescendingOrder );
      m_self.miscTable->setSortingEnabled(true);
      m_miscTableProxy->setDynamicSortFilter(true);
      m_self.yeastTable->horizontalHeader()->setSortIndicator(static_cast<int>(YeastTableModel::ColumnIndex::Name), Qt::DescendingOrder );
      m_self.yeastTable->setSortingEnabled(true);
      m_yeastTableProxy->setDynamicSortFilter(true);
   }

   /**
    * \brief Create the dialogs, including the file dialogs
    *
    *        Most dialogs are initialized in here. That should include any initial configurations as well.
    */
   void setupDialogs() {
      m_aboutDialog           = std::make_unique<AboutDialog          >(&m_self);
      m_helpDialog            = std::make_unique<HelpDialog           >(&m_self);
      m_equipCatalog          = std::make_unique<EquipmentCatalog     >(&m_self);
      m_equipEditor           = std::make_unique<EquipmentEditor      >(&m_self);
      m_fermCatalog           = std::make_unique<FermentableCatalog   >(&m_self);
      m_fermEditor            = std::make_unique<FermentableEditor    >(&m_self);
      m_hopCatalog            = std::make_unique<HopCatalog           >(&m_self);
      m_hopEditor             = std::make_unique<HopEditor            >(&m_self);
      m_mashEditor            = std::make_unique<MashEditor           >(&m_self);
      m_mashStepEditor        = std::make_unique<MashStepEditor       >(&m_self);
      m_mashWizard            = std::make_unique<MashWizard           >(&m_self);
      m_miscCatalog           = std::make_unique<MiscCatalog          >(&m_self);
      m_miscEditor            = std::make_unique<MiscEditor           >(&m_self);
      m_styleCatalog          = std::make_unique<StyleCatalog         >(&m_self);
      m_styleEditor           = std::make_unique<StyleEditor          >(&m_self);
      m_yeastCatalog          = std::make_unique<YeastCatalog         >(&m_self);
      m_yeastEditor           = std::make_unique<YeastEditor          >(&m_self);
      m_optionDialog          = std::make_unique<OptionDialog         >(&m_self);
      m_recipeScaler          = std::make_unique<ScaleRecipeTool      >(&m_self);
      m_recipeFormatter       = std::make_unique<RecipeFormatter      >(&m_self);
      m_printAndPreviewDialog = std::make_unique<PrintAndPreviewDialog>(&m_self);
      m_ogAdjuster            = std::make_unique<OgAdjuster           >(&m_self);
      m_converterTool         = std::make_unique<ConverterTool        >(&m_self);
      m_hydrometerTool        = std::make_unique<HydrometerTool       >(&m_self);
      m_alcoholTool           = std::make_unique<AlcoholTool          >(&m_self);
      m_timerMainDialog       = std::make_unique<TimerMainDialog      >(&m_self);
      m_primingDialog         = std::make_unique<PrimingDialog        >(&m_self);
      m_strikeWaterDialog     = std::make_unique<StrikeWaterDialog    >(&m_self);
      m_refractoDialog        = std::make_unique<RefractoDialog       >(&m_self);
      m_mashDesigner          = std::make_unique<MashDesigner         >(&m_self);
      m_pitchDialog           = std::make_unique<PitchDialog          >(&m_self);
      m_btDatePopup           = std::make_unique<BtDatePopup          >(&m_self);
      m_waterDialog           = std::make_unique<WaterDialog          >(&m_self);
      m_waterEditor           = std::make_unique<WaterEditor          >(&m_self);
      m_ancestorDialog        = std::make_unique<AncestorDialog       >(&m_self);

      return;
   }

   /**
    * \brief Configure combo boxes and their list models
    *
    *        Any new combo boxes, along with their list models, should be initialized here
    */
   void setupComboBoxes() {
      // Set equipment combo box model.
      m_equipmentListModel = std::make_unique<EquipmentListModel>(m_self.equipmentComboBox);
      m_self.equipmentComboBox->setModel(m_equipmentListModel.get());

      // Set the style combo box
      m_styleListModel = std::make_unique<StyleListModel>(m_self.styleComboBox);
      m_styleProxyModel = std::make_unique<StyleSortFilterProxyModel>(m_self.styleComboBox);
      m_styleProxyModel->setDynamicSortFilter(true);
      m_styleProxyModel->setSortLocaleAware(true);
      m_styleProxyModel->setSourceModel(m_styleListModel.get());
      m_styleProxyModel->sort(0);
      m_self.styleComboBox->setModel(m_styleProxyModel.get());

      // Set the mash combo box
      m_mashListModel = std::make_unique<MashListModel>(m_self.mashComboBox);
      m_self.mashComboBox->setModel(m_mashListModel.get());

      // Nothing to say.
      m_namedMashEditor = std::make_unique<NamedMashEditor>(&m_self, m_mashStepEditor.get());
      // I don't think this is used yet
      m_singleNamedMashEditor = std::make_unique<NamedMashEditor>(&m_self, m_mashStepEditor.get(), true);
   }

   /**
    * \brief Common code for getting the currently highlighted entry in one of the recipe additions tables
    *        (hopAdditions, etc).
    */
   template<class NE, class Table, class Proxy, class TableModel>
   NE * selected(Table * table, Proxy * proxy, TableModel * tableModel) {
      QModelIndexList selected = table->selectionModel()->selectedIndexes();

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

      QModelIndex modelIndex = proxy->mapToSource(viewIndex);
      return tableModel->getRow(modelIndex.row()).get();
   }

   Fermentable       * selectedFermentable() { return this->selected<Fermentable      >(m_self.fermentableTable, this->m_fermTableProxy.get()        , this->m_fermTableModel.get()        ); }
   RecipeAdditionHop * selectedHopAddition() { return this->selected<RecipeAdditionHop>(m_self.hopAdditionTable, this->m_hopAdditionsTableProxy.get(), this->m_hopAdditionsTableModel.get()); }
   Misc              * selectedMisc       () { return this->selected<Misc             >(m_self.miscTable       , this->m_miscTableProxy.get()        , this->m_miscTableModel.get()        ); }
   Yeast             * selectedYeast      () { return this->selected<Yeast            >(m_self.yeastTable      , this->m_yeastTableProxy.get()       , this->m_yeastTableModel.get()       ); }

   /**
    * \brief Use this for adding \c RecipeAdditionHop etc
    */
   template<class NE>
   void doRecipeAddition(std::shared_ptr<NE> ne) {
      Q_ASSERT(ne);

      this->m_self.doOrRedoUpdate(
         newUndoableAddOrRemove(*this->m_self.recipeObs,
                                &Recipe::addAddition<NE>,
                                ne,
                                &Recipe::removeAddition<NE>,
                                QString(tr("Add %1 to recipe")).arg(NE::LocalisedName))
      );

      // Since we just added an ingredient, switch the focus to the tab that lists that type of ingredient.  We rely here
      // on the individual tabs following a naming convention (recipeHopTab, recipeFermentableTab, etc)
      // Note that we want the untranslated class name because this is not for display but to refer to a QWidget inside
      // tabWidget_ingredients
      auto const widgetName = QString("recipe%1Tab").arg(NE::staticMetaObject.className());
      qDebug() << Q_FUNC_INFO << widgetName;
      QWidget * widget = this->m_self.tabWidget_ingredients->findChild<QWidget *>(widgetName);
      Q_ASSERT(widget);
      this->m_self.tabWidget_ingredients->setCurrentWidget(widget);

      // We don't need to call this->pimpl->m_hopAdditionsTableModel->addHop(hop) here (or the equivalent for fermentable, misc or
      // yeast) because the change to the recipe will already have triggered the necessary updates to
      // this->pimpl->m_hopAdditionsTableModel/this->pimpl->m_fermentableTableModel/etc.
      return;
   }

   /**
    * \brief Use this for removing \c RecipeAdditionHop etc
    */
   template<class NE, class Table, class Proxy, class TableModel>
   void doRemoveRecipeAddition(Table * table, Proxy * proxy, TableModel * tableModel) {
      QModelIndexList selected = table->selectionModel()->selectedIndexes();
      QList< std::shared_ptr<NE> > itemsToRemove;

      int size = selected.size();
      if (size == 0) {
         return;
      }

      for (int i = 0; i < size; i++) {
         QModelIndex viewIndex = selected.at(i);
         QModelIndex modelIndex = proxy->mapToSource(viewIndex);
         itemsToRemove.append(tableModel->getRow(modelIndex.row()));
      }

      for (auto item : itemsToRemove) {
         this->m_self.doOrRedoUpdate(
            newUndoableAddOrRemove(*this->m_self.recipeObs,
                                    &Recipe::removeAddition<NE>,
                                    item,
                                    &Recipe::addAddition<NE>,
                                    tr("Remove %1 from recipe").arg(NE::LocalisedName))
         );
         tableModel->remove(item);
      }
      return;
   }


   //================================================ MEMBER VARIABLES =================================================

   MainWindow & m_self;
   QFileDialog* fileOpener;

   // Undo / Redo, using the Qt Undo framework
   std::unique_ptr<QUndoStack> m_undoStack;

   // all things tables should go here.
   std::unique_ptr<FermentableTableModel> m_fermTableModel    ;
   std::unique_ptr<RecipeAdditionHopTableModel> m_hopAdditionsTableModel;
   std::unique_ptr<MashStepTableModel   > m_mashStepTableModel;
   std::unique_ptr<MiscTableModel       > m_miscTableModel    ;
   std::unique_ptr<YeastTableModel      > m_yeastTableModel   ;

   // all things sort/filter proxy go here
   std::unique_ptr<FermentableSortFilterProxyModel> m_fermTableProxy ;
   std::unique_ptr<RecipeAdditionHopSortFilterProxyModel> m_hopAdditionsTableProxy  ;
   std::unique_ptr<MiscSortFilterProxyModel       > m_miscTableProxy ;
   std::unique_ptr<StyleSortFilterProxyModel      > m_styleProxyModel;
   std::unique_ptr<YeastSortFilterProxyModel      > m_yeastTableProxy;

   // All initialised in setupDialogs
   std::unique_ptr<AboutDialog          > m_aboutDialog          ;
   std::unique_ptr<HelpDialog           > m_helpDialog           ;
   std::unique_ptr<EquipmentCatalog     > m_equipCatalog         ;
   std::unique_ptr<EquipmentEditor      > m_equipEditor          ;
   std::unique_ptr<FermentableCatalog   > m_fermCatalog          ;
   std::unique_ptr<FermentableEditor    > m_fermEditor           ;
   std::unique_ptr<HopCatalog           > m_hopCatalog           ;
   std::unique_ptr<HopEditor            > m_hopEditor            ;
   std::unique_ptr<MashEditor           > m_mashEditor           ;
   std::unique_ptr<MashStepEditor       > m_mashStepEditor       ;
   std::unique_ptr<MashWizard           > m_mashWizard           ;
   std::unique_ptr<MiscCatalog          > m_miscCatalog          ;
   std::unique_ptr<MiscEditor           > m_miscEditor           ;
   std::unique_ptr<StyleCatalog         > m_styleCatalog         ;
   std::unique_ptr<StyleEditor          > m_styleEditor          ;
   std::unique_ptr<YeastCatalog         > m_yeastCatalog         ;
   std::unique_ptr<YeastEditor          > m_yeastEditor          ;
   std::unique_ptr<OptionDialog         > m_optionDialog         ;
   std::unique_ptr<ScaleRecipeTool      > m_recipeScaler         ;
   std::unique_ptr<RecipeFormatter      > m_recipeFormatter      ;
   std::unique_ptr<PrintAndPreviewDialog> m_printAndPreviewDialog;
   std::unique_ptr<OgAdjuster           > m_ogAdjuster           ;
   std::unique_ptr<ConverterTool        > m_converterTool        ;
   std::unique_ptr<HydrometerTool       > m_hydrometerTool       ;
   std::unique_ptr<AlcoholTool          > m_alcoholTool          ;
   std::unique_ptr<TimerMainDialog      > m_timerMainDialog      ;
   std::unique_ptr<PrimingDialog        > m_primingDialog        ;
   std::unique_ptr<StrikeWaterDialog    > m_strikeWaterDialog    ;
   std::unique_ptr<RefractoDialog       > m_refractoDialog       ;
   std::unique_ptr<MashDesigner         > m_mashDesigner         ;
   std::unique_ptr<PitchDialog          > m_pitchDialog          ;
   std::unique_ptr<BtDatePopup          > m_btDatePopup          ;
   std::unique_ptr<WaterDialog          > m_waterDialog          ;
   std::unique_ptr<WaterEditor          > m_waterEditor          ;
   std::unique_ptr<AncestorDialog       > m_ancestorDialog       ;

   // all things lists should go here
   std::unique_ptr<EquipmentListModel> m_equipmentListModel;
   std::unique_ptr<MashListModel     > m_mashListModel     ;
   std::unique_ptr<StyleListModel    > m_styleListModel    ;
//   std::unique_ptr<WaterListModel> waterListModel;  Appears to be unused...
   std::unique_ptr<NamedMashEditor> m_namedMashEditor;
   std::unique_ptr<NamedMashEditor> m_singleNamedMashEditor;


};


MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), pimpl{std::make_unique<impl>(*this)} {
   qDebug() << Q_FUNC_INFO;

   // Need to call this parent class method to get all the widgets added (I think).
   this->setupUi(this);

   // Initialise smart labels etc early, but after call to this->setupUi() because otherwise member variables such as
   // label_name will not yet be set.
   // .:TBD:. We should fix some of these inconsistently-named labels
   SMART_FIELD_INIT(MainWindow, label_name           , lineEdit_name      , Recipe, PropertyNames::NamedEntity::name        );
   SMART_FIELD_INIT(MainWindow, label_targetBatchSize, lineEdit_batchSize , Recipe, PropertyNames::Recipe::batchSize_l   , 2);
   SMART_FIELD_INIT(MainWindow, label_targetBoilSize , lineEdit_boilSize  , Recipe, PropertyNames::Recipe::boilSize_l    , 2);
   SMART_FIELD_INIT(MainWindow, label_efficiency     , lineEdit_efficiency, Recipe, PropertyNames::Recipe::efficiency_pct, 1);
   SMART_FIELD_INIT(MainWindow, label_boilTime       , lineEdit_boilTime  , Recipe, PropertyNames::Recipe::boilTime_min  , 1);
   SMART_FIELD_INIT(MainWindow, label_boilSg         , lineEdit_boilSg    , Recipe, PropertyNames::Recipe::boilGrav      , 3);

   SMART_FIELD_INIT_NO_SF(MainWindow, oGLabel        , Recipe, PropertyNames::Recipe::og        );
   SMART_FIELD_INIT_NO_SF(MainWindow, fGLabel        , Recipe, PropertyNames::Recipe::fg        );
   SMART_FIELD_INIT_NO_SF(MainWindow, colorSRMLabel  , Recipe, PropertyNames::Recipe::color_srm );
   SMART_FIELD_INIT_NO_SF(MainWindow, label_batchSize, Recipe, PropertyNames::Recipe::boilSize_l);
   SMART_FIELD_INIT_NO_SF(MainWindow, label_boilSize , Recipe, PropertyNames::Recipe::boilSize_l);

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
   if (!Database::instance().loadSuccessful() ) {
      qCritical() << Q_FUNC_INFO << "Could not load database";

      // Ask the application nicely to quit
      QCoreApplication::quit();
      // If it didn't, we ask more firmly
      QCoreApplication::exit(1);
      // If the framework won't play ball, we invoke a higher power.
      exit(1);
   }

   // Now let's ensure all the data is read in from the DB
   QString errorMessage{};
   if (!InitialiseAllObjectStores(errorMessage)) {
      bool bail = true;
      if (Application::isInteractive()) {
         // Can't use QErrorMessage here as it's not flexible enough for what we need
         QMessageBox dataLoadErrorMessageBox;
         dataLoadErrorMessageBox.setWindowTitle(tr("Error Loading Data"));
         dataLoadErrorMessageBox.setText(errorMessage);
         dataLoadErrorMessageBox.setInformativeText(
            tr("The program may not work if you ignore this error.\n\n"
               "See logs for more details.\n\n"
               "If you need help, please open an issue "
               "at %1").arg(CONFIG_HOMEPAGE_URL)
         );
         dataLoadErrorMessageBox.setStandardButtons(QMessageBox::Ignore | QMessageBox::Close);
         dataLoadErrorMessageBox.setDefaultButton(QMessageBox::Close);
         int ret = dataLoadErrorMessageBox.exec();
         if (ret == QMessageBox::Close) {
            qDebug() << Q_FUNC_INFO << "User clicked \"Close\".  Exiting.";
         } else {
            qWarning() <<
               Q_FUNC_INFO << "User clicked \"Ignore\" after errors loading data.  PROGRAM MAY NOT FUNCTION CORRECTLY!";
            bail = false;
         }
      }
      if (bail) {
         // Either the user clicked Close, or we're not interactive.  Either way, we quit in the same way as above.
         qDebug() << Q_FUNC_INFO << "Exiting...";
         QCoreApplication::quit();
         qDebug() << Q_FUNC_INFO << "Still Exiting...";
         QCoreApplication::exit(1);
         qDebug() << Q_FUNC_INFO << "Really Exiting now...";
         exit(1);
      }
   }

   // Set the window title.
   setWindowTitle( QString("Brewken - %1").arg(CONFIG_VERSION_STRING) );

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
   this->pimpl->setupDialogs();
   // initialize the ranged sliders
   this->setupRanges();
   // the dialogs have to be setup before this is called
   this->pimpl->setupComboBoxes();
   // do all the work to configure the tables models and their proxies
   this->pimpl->setupTables();
   // Create the keyboard shortcuts
   this->setupShortCuts();
   // Once more with the context menus too
   this->setupContextMenu();
   // do all the work for checkboxes (just one right now)
   this->setUpStateChanges();

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

   // This sets up things that might have been 'remembered' (ie stored in the config file) from a previous run of the
   // program - eg window size, which is stored in MainWindow::closeEvent().
   // Breaks the naming convention, doesn't it?
   this->restoreSavedState();

   // Moved from Database class
   Recipe::connectSignalsForAllRecipes();
   qDebug() << Q_FUNC_INFO << "Recipe signals connected";
   Mash::connectSignals();
   qDebug() << Q_FUNC_INFO << "Mash signals connected";
   Boil::connectSignals();
   qDebug() << Q_FUNC_INFO << "Boil signals connected";
   Fermentation::connectSignals();
   qDebug() << Q_FUNC_INFO << "Fermentation signals connected";

   // I do not like this connection here.
   connect(this->pimpl->m_ancestorDialog,  &AncestorDialog::ancestoryChanged, treeView_recipe->model(), &BtTreeModel::versionedRecipe);
   connect(this->pimpl->m_optionDialog,    &OptionDialog::showAllAncestors,   treeView_recipe->model(), &BtTreeModel::catchAncestors );
   connect(this->treeView_recipe, &BtTreeView::recipeSpawn,          this,                     &MainWindow::versionedRecipe );

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

// Configures the range widgets for the bubbles
void MainWindow::setupRanges() {
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
   rangeWidget_batchSize->setRange(0, recipeObs == nullptr ? 19.0 : recipeObs->batchSize_l());
   rangeWidget_batchSize->setPrecision(1);
   rangeWidget_batchSize->setTickMarks(2,5);

   rangeWidget_batchSize->setBackgroundBrush(QColor(255,255,255));
   rangeWidget_batchSize->setPreferredRangeBrush(QColor(55,138,251));
   rangeWidget_batchSize->setMarkerBrush(QBrush(Qt::NoBrush));

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
         []([[maybe_unused]] std::shared_ptr<Recipe> obj) {return true;}
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

   //UI restore state
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
   connect(actionExit                      , &QAction::triggered, this                                      , &QWidget::close                  ); // > File > Exit
   connect(actionAbout_Brewken             , &QAction::triggered, this->pimpl->m_aboutDialog.get()          , &QWidget::show                   ); // > About > About Brewken
   connect(actionHelp                      , &QAction::triggered, this->pimpl->m_helpDialog.get()           , &QWidget::show                   ); // > About > Help

   connect(actionNewRecipe                 , &QAction::triggered, this                                      , &MainWindow::newRecipe           ); // > File > New Recipe
   connect(actionImportFromXml             , &QAction::triggered, this                                      , &MainWindow::importFiles         ); // > File > Import Recipes
   connect(actionExportToXml               , &QAction::triggered, this                                      , &MainWindow::exportRecipe        ); // > File > Export Recipes
   connect(actionUndo                      , &QAction::triggered, this                                      , &MainWindow::editUndo            ); // > Edit > Undo
   connect(actionRedo                      , &QAction::triggered, this                                      , &MainWindow::editRedo            ); // > Edit > Redo
   setUndoRedoEnable();
   connect(actionEquipments                , &QAction::triggered, this->pimpl->m_equipCatalog.get()         , &QWidget::show                   ); // > View > Equipments
   connect(actionMashs                     , &QAction::triggered, this->pimpl->m_namedMashEditor.get()      , &QWidget::show                   ); // > View > Mashs
   connect(actionStyles                    , &QAction::triggered, this->pimpl->m_styleCatalog.get()         , &QWidget::show                   ); // > View > Styles
   connect(actionFermentables              , &QAction::triggered, this->pimpl->m_fermCatalog.get()          , &QWidget::show                   ); // > View > Fermentables
   connect(actionHops                      , &QAction::triggered, this->pimpl->m_hopCatalog.get()           , &QWidget::show                   ); // > View > Hops
   connect(actionMiscs                     , &QAction::triggered, this->pimpl->m_miscCatalog.get()          , &QWidget::show                   ); // > View > Miscs
   connect(actionYeasts                    , &QAction::triggered, this->pimpl->m_yeastCatalog.get()         , &QWidget::show                   ); // > View > Yeasts
   connect(actionOptions                   , &QAction::triggered, this->pimpl->m_optionDialog.get()         , &OptionDialog::show              ); // > Tools > Options
//   connect( actionManual, &QAction::triggered, this, &MainWindow::openManual);                                               // > About > Manual
   connect(actionScale_Recipe              , &QAction::triggered, this->pimpl->m_recipeScaler.get()         , &QWidget::show                   ); // > Tools > Scale Recipe
   connect(action_recipeToTextClipboard    , &QAction::triggered, this->pimpl->m_recipeFormatter.get()      , &RecipeFormatter::toTextClipboard); // > Tools > Recipe to Clipboard as Text
   connect(actionConvert_Units             , &QAction::triggered, this->pimpl->m_converterTool.get()        , &QWidget::show                   ); // > Tools > Convert Units
   connect(actionHydrometer_Temp_Adjustment, &QAction::triggered, this->pimpl->m_hydrometerTool.get()       , &QWidget::show                   ); // > Tools > Hydrometer Temp Adjustment
   connect(actionAlcohol_Percentage_Tool   , &QAction::triggered, this->pimpl->m_alcoholTool.get()          , &QWidget::show                   ); // > Tools > Alcohol
   connect(actionOG_Correction_Help        , &QAction::triggered, this->pimpl->m_ogAdjuster.get()           , &QWidget::show                   ); // > Tools > OG Correction Help
   connect(actionCopy_Recipe               , &QAction::triggered, this                                      , &MainWindow::copyRecipe          ); // > File > Copy Recipe
   connect(actionPriming_Calculator        , &QAction::triggered, this->pimpl->m_primingDialog.get()        , &QWidget::show                   ); // > Tools > Priming Calculator
   connect(actionStrikeWater_Calculator    , &QAction::triggered, this->pimpl->m_strikeWaterDialog.get()    , &QWidget::show                   ); // > Tools > Strike Water Calculator
   connect(actionRefractometer_Tools       , &QAction::triggered, this->pimpl->m_refractoDialog.get()       , &QWidget::show                   ); // > Tools > Refractometer Tools
   connect(actionPitch_Rate_Calculator     , &QAction::triggered, this                                      , &MainWindow::showPitchDialog     ); // > Tools > Pitch Rate Calculator
   connect(actionTimers                    , &QAction::triggered, this->pimpl->m_timerMainDialog.get()      , &QWidget::show                   ); // > Tools > Timers
   connect(actionDeleteSelected            , &QAction::triggered, this                                      , &MainWindow::deleteSelected      );
   connect(actionWater_Chemistry           , &QAction::triggered, this                                      , &MainWindow::popChemistry        ); // > Tools > Water Chemistry
   connect(actionAncestors                 , &QAction::triggered, this                                      , &MainWindow::setAncestor         ); // > Tools > Ancestors
   connect(action_brewit                   , &QAction::triggered, this                                      , &MainWindow::brewItHelper        );
   //One Dialog to rule them all, at least all printing and export.
   connect(actionPrint                     , &QAction::triggered, this->pimpl->m_printAndPreviewDialog.get(), &QWidget::show                   ); // > File > Print and Preview

   // postgresql cannot backup or restore yet. I would like to find some way
   // around this, but for now just disable
   if ( Database::instance().dbType() == Database::DbType::PGSQL ) {
      actionBackup_Database->setEnabled(false);                                                                         // > File > Database > Backup
      actionRestore_Database->setEnabled(false);                                                                        // > File > Database > Restore
   }
   else {
      connect( actionBackup_Database, &QAction::triggered, this, &MainWindow::backup );                                 // > File > Database > Backup
      connect( actionRestore_Database, &QAction::triggered, this, &MainWindow::restoreFromBackup );                     // > File > Database > Restore
   }
   return;
}

// pushbuttons with a SIGNAL of clicked() should go in here.
void MainWindow::setupClicks() {
   connect(this->equipmentButton          , &QAbstractButton::clicked, this        , &MainWindow::showEquipmentEditor      );
   connect(this->styleButton              , &QAbstractButton::clicked, this        , &MainWindow::showStyleEditor          );
   connect(this->mashButton               , &QAbstractButton::clicked, this->pimpl->m_mashEditor  , &MashEditor::showEditor               );
   connect(this->pushButton_addFerm       , &QAbstractButton::clicked, this->pimpl->m_fermCatalog  , &QWidget::show                        );
   connect(this->pushButton_addHop        , &QAbstractButton::clicked, this->pimpl->m_hopCatalog   , &QWidget::show                        );
   connect(this->pushButton_addMisc       , &QAbstractButton::clicked, this->pimpl->m_miscCatalog  , &QWidget::show                        );
   connect(this->pushButton_addYeast      , &QAbstractButton::clicked, this->pimpl->m_yeastCatalog , &QWidget::show                        );
   connect(this->pushButton_removeFerm    , &QAbstractButton::clicked, this        , &MainWindow::removeSelectedFermentable);
   connect(this->pushButton_removeHop     , &QAbstractButton::clicked, this        , &MainWindow::removeSelectedHopAddition        );
   connect(this->pushButton_removeMisc    , &QAbstractButton::clicked, this        , &MainWindow::removeSelectedMisc       );
   connect(this->pushButton_removeYeast   , &QAbstractButton::clicked, this        , &MainWindow::removeSelectedYeast      );
   connect(this->pushButton_editFerm      , &QAbstractButton::clicked, this        , &MainWindow::editSelectedFermentable  );
   connect(this->pushButton_editMisc      , &QAbstractButton::clicked, this        , &MainWindow::editSelectedMisc         );
   connect(this->pushButton_editHop       , &QAbstractButton::clicked, this        , &MainWindow::editHopOfSelectedHopAddition          );
   connect(this->pushButton_editYeast     , &QAbstractButton::clicked, this        , &MainWindow::editSelectedYeast        );
   connect(this->pushButton_editMash      , &QAbstractButton::clicked, this->pimpl->m_mashEditor  , &MashEditor::showEditor               );
   connect(this->pushButton_addMashStep   , &QAbstractButton::clicked, this        , &MainWindow::addMashStep              );
   connect(this->pushButton_removeMashStep, &QAbstractButton::clicked, this        , &MainWindow::removeSelectedMashStep   );
   connect(this->pushButton_editMashStep  , &QAbstractButton::clicked, this        , &MainWindow::editSelectedMashStep     );
   connect(this->pushButton_mashWizard    , &QAbstractButton::clicked, this->pimpl->m_mashWizard  , &MashWizard::show                     );
   connect(this->pushButton_saveMash      , &QAbstractButton::clicked, this        , &MainWindow::saveMash                 );
   connect(this->pushButton_mashDes       , &QAbstractButton::clicked, this->pimpl->m_mashDesigner, &MashDesigner::show                   );
   connect(this->pushButton_mashUp        , &QAbstractButton::clicked, this        , &MainWindow::moveSelectedMashStepUp   );
   connect(this->pushButton_mashDown      , &QAbstractButton::clicked, this        , &MainWindow::moveSelectedMashStepDown );
   connect(this->pushButton_mashRemove    , &QAbstractButton::clicked, this        , &MainWindow::removeMash               );
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
   connect(this->lineEdit_name      , &QLineEdit::editingFinished,  this, &MainWindow::updateRecipeName);
   connect(this->lineEdit_batchSize , &SmartLineEdit::textModified, this, &MainWindow::updateRecipeBatchSize);
   connect(this->lineEdit_boilSize  , &SmartLineEdit::textModified, this, &MainWindow::updateRecipeBoilSize);
   connect(this->lineEdit_boilTime  , &SmartLineEdit::textModified, this, &MainWindow::updateRecipeBoilTime);
   connect(this->lineEdit_efficiency, &SmartLineEdit::textModified, this, &MainWindow::updateRecipeEfficiency);
   return;
}

// anything using a SmartLabel::changedSystemOfMeasurementOrScale signal should go in here
void MainWindow::setupLabels() {
   // These are the sliders. I need to consider these harder, but small steps
   connect(this->oGLabel,       &SmartLabel::changedSystemOfMeasurementOrScale, this, &MainWindow::redisplayLabel);
   connect(this->fGLabel,       &SmartLabel::changedSystemOfMeasurementOrScale, this, &MainWindow::redisplayLabel);
   connect(this->colorSRMLabel, &SmartLabel::changedSystemOfMeasurementOrScale, this, &MainWindow::redisplayLabel);
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
                  this->pimpl->m_equipEditor->setEditItem(ObjectStoreWrapper::getSharedFromRaw(kit));
                  this->pimpl->m_equipEditor->show();
               }
            }
            break;
         case BtTreeItem::Type::FERMENTABLE:
            {
               Fermentable * ferm = active->getItem<Fermentable>(index);
               if (ferm) {
                  this->pimpl->m_fermEditor->setEditItem(ObjectStoreWrapper::getSharedFromRaw(ferm));
                  this->pimpl->m_fermEditor->show();
               }
            }
            break;
         case BtTreeItem::Type::HOP:
            {
               Hop* hop = active->getItem<Hop>(index);
               if (hop) {
                  this->pimpl->m_hopEditor->setEditItem(ObjectStoreWrapper::getSharedFromRaw(hop));
                  this->pimpl->m_hopEditor->show();
               }
            }
            break;
         case BtTreeItem::Type::MISC:
            {
               Misc * misc = active->getItem<Misc>(index);
               if (misc) {
                  this->pimpl->m_miscEditor->setEditItem(ObjectStoreWrapper::getSharedFromRaw(misc));
                  this->pimpl->m_miscEditor->show();
               }
            }
            break;
         case BtTreeItem::Type::STYLE:
            {
               Style * style = active->getItem<Style>(index);
               if (style) {
                  this->pimpl->m_styleEditor->setEditItem(ObjectStoreWrapper::getSharedFromRaw(style));
                  this->pimpl->m_styleEditor->show();
               }
            }
            break;
         case BtTreeItem::Type::YEAST:
            {
               Yeast * yeast = active->getItem<Yeast>(index);
               if (yeast) {
                  this->pimpl->m_yeastEditor->setEditItem(ObjectStoreWrapper::getSharedFromRaw(yeast));
                  this->pimpl->m_yeastEditor->show();
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
                  this->pimpl->m_waterEditor->setWater(ObjectStoreWrapper::getSharedFromRaw(w));
                  this->pimpl->m_waterEditor->show();
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

   if ( ni ) {
      tabWidget_recipeView->setCurrentWidget(ni);
      return;
   }

   ni = new BrewNoteWidget(tabWidget_recipeView);
   ni->setBrewNote(bNote);

   this->tabWidget_recipeView->addTab(ni,bNote->brewDate_short());
   this->tabWidget_recipeView->setCurrentWidget(ni);
   return;
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

   this->pimpl->m_ancestorDialog->setAncestor(rec);
   this->pimpl->m_ancestorDialog->show();
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
   this->pimpl->m_fermTableModel->observeRecipe(recipe);
   this->pimpl->m_hopAdditionsTableModel->observeRecipe(recipe);
   this->pimpl->m_miscTableModel->observeRecipe(recipe);
   this->pimpl->m_yeastTableModel->observeRecipe(recipe);
   this->pimpl->m_mashStepTableModel->setMash(recipeObs->mash());

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
   this->pimpl->m_mashWizard->setRecipe(recipe);
   brewDayScrollWidget->setRecipe(recipe);
   this->pimpl->m_equipmentListModel->observeRecipe(recipe);
   this->pimpl->m_recipeFormatter->setRecipe(recipe);
   this->pimpl->m_ogAdjuster->setRecipe(recipe);
   recipeExtrasWidget->setRecipe(recipe);
   this->pimpl->m_mashDesigner->setRecipe(recipe);
   equipmentButton->setRecipe(recipe);
   if (recEquip) {
      this->pimpl->m_equipEditor->setEditItem(ObjectStoreWrapper::getSharedFromRaw(recEquip));
   }
   styleButton->setRecipe(recipe);
   if (recipe->style()) {
      this->pimpl->m_styleEditor->setEditItem(ObjectStoreWrapper::getSharedFromRaw(recipe->style()));
   }

   this->pimpl->m_mashEditor->setMash(recipeObs->mash());
   this->pimpl->m_mashEditor->setRecipe(recipeObs);

   mashButton->setMash(recipeObs->mash());
   this->pimpl->m_recipeScaler->setRecipe(recipeObs);

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
void MainWindow::lockRecipe(int state) {
   if (!this->recipeObs) {
      return;
   }

   // If I am locking a recipe (lock == true ), I want to disable fields
   // (enable == false). If I am unlocking (lock == false), I want to enable
   // fields (enable == true). This just makes that easy
   bool const lockIt = state == Qt::Checked;
   bool const enabled = ! lockIt;

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

   hopAdditionTable->setEnabled(enabled);
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

   this->pimpl->m_fermCatalog ->setEnableAddToRecipe(enabled);
   this->pimpl->m_hopCatalog  ->setEnableAddToRecipe(enabled);
   this->pimpl->m_miscCatalog ->setEnableAddToRecipe(enabled);
   this->pimpl->m_yeastCatalog->setEnableAddToRecipe(enabled);
   // mashes still need dealing with
   //
   return;
}

void MainWindow::changed(QMetaProperty prop, QVariant val) {
   QString propName(prop.name());

   if (propName == PropertyNames::Recipe::equipment) {
      this->recEquip = val.value<Equipment *>();
      this->pimpl->m_equipEditor->setEditItem(ObjectStoreWrapper::getSharedFromRaw(this->recEquip));
   } else if (propName == PropertyNames::Recipe::style) {
      //recStyle = recipeObs->style();
      this->recStyle = val.value<Style*>();
      this->pimpl->m_styleEditor->setEditItem(ObjectStoreWrapper::getSharedFromRaw(this->recStyle));
   }

   this->showChanges(&prop);
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
   this->lineEdit_name      ->setText  (this->recipeObs->name          ());
   this->lineEdit_batchSize ->setAmount(this->recipeObs->batchSize_l   ());
   // TODO: One day we'll want to do some work to properly handle no-boil recipes....
   double const boilSize = recipeObs->boil() ? (*recipeObs->boil())->preBoilSize_l().value_or(0.0) : 0.0;
   this->lineEdit_boilSize  ->setAmount(boilSize);
   this->lineEdit_efficiency->setAmount(this->recipeObs->efficiency_pct());
   this->lineEdit_boilTime  ->setAmount(this->recipeObs->boilTime_min  ());
   this->lineEdit_name      ->setCursorPosition(0);
   this->lineEdit_batchSize ->setCursorPosition(0);
   this->lineEdit_boilSize  ->setCursorPosition(0);
   this->lineEdit_efficiency->setCursorPosition(0);
   this->lineEdit_boilTime  ->setCursorPosition(0);
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
   this->lineEdit_boilSg->setAmount(this->recipeObs->boilGrav());

   Style const * style = this->recipeObs->style();
   if (style) {
      updateDensitySlider(*this->styleRangeWidget_og, *this->oGLabel, style->ogMin(), style->ogMax(), 1.120);
   }
   this->styleRangeWidget_og->setValue(this->oGLabel->getAmountToDisplay(recipeObs->og()));

   if (style) {
      updateDensitySlider(*this->styleRangeWidget_fg, *this->fGLabel, style->fgMin(), style->fgMax(), 1.030);
   }
   this->styleRangeWidget_fg->setValue(this->fGLabel->getAmountToDisplay(recipeObs->fg()));

   this->styleRangeWidget_abv->setValue(recipeObs->ABV_pct());
   this->styleRangeWidget_ibu->setValue(recipeObs->IBU());

   this->rangeWidget_batchSize->setRange         (0,
                                                  this->label_batchSize->getAmountToDisplay(this->recipeObs->batchSize_l()));
   this->rangeWidget_batchSize->setPreferredRange(0,
                                                  this->label_batchSize->getAmountToDisplay(this->recipeObs->finalVolume_l()));
   this->rangeWidget_batchSize->setValue         (this->label_batchSize->getAmountToDisplay(this->recipeObs->finalVolume_l()));

   this->rangeWidget_boilsize->setRange         (0,
                                                 this->label_boilSize->getAmountToDisplay(boilSize));
   this->rangeWidget_boilsize->setPreferredRange(0,
                                                 this->label_boilSize->getAmountToDisplay(this->recipeObs->boilVolume_l()));
   this->rangeWidget_boilsize->setValue         (this->label_boilSize->getAmountToDisplay(this->recipeObs->boilVolume_l()));

   /* Colors need the same basic treatment as gravity */
   if (style) {
      updateColorSlider(*this->styleRangeWidget_srm,
                        *this->colorSRMLabel,
                        style->colorMin_srm(),
                        style->colorMax_srm());
   }
   this->styleRangeWidget_srm->setValue(this->colorSRMLabel->getAmountToDisplay(this->recipeObs->color_srm()));

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
      this->pimpl->m_mashStepTableModel->setMash(recipeObs->mash());
   }

   // Not sure about this, but I am annoyed that modifying the hop usage
   // modifiers isn't automatically updating my display
   if (updateAll) {
     recipeObs->recalcIBU();
     this->pimpl->m_hopAdditionsTableProxy->invalidate();
   }
   return;
}

void MainWindow::updateRecipeName() {
   if (recipeObs == nullptr || ! lineEdit_name->isModified()) {
      return;
   }

   this->doOrRedoUpdate(*this->recipeObs,
                        TYPE_INFO(Recipe, NamedEntity, name),
                        lineEdit_name->text(),
                        tr("Change Recipe Name"));
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

   this->styleRangeWidget_og->setPreferredRange(this->oGLabel->getRangeToDisplay(style->ogMin(), style->ogMax()));

   this->styleRangeWidget_fg->setPreferredRange(this->fGLabel->getRangeToDisplay(style->ogMin(), style->ogMax()));

   // If min and/or max ABV is not set on the Style, then use some sensible outer limit(s)
   this->styleRangeWidget_abv->setPreferredRange(style->abvMin_pct().value_or(0.0), style->abvMax_pct().value_or(50.0));
   this->styleRangeWidget_ibu->setPreferredRange(style->ibuMin(), style->ibuMax());
   this->styleRangeWidget_srm->setPreferredRange(this->colorSRMLabel->getRangeToDisplay(style->colorMin_srm(),
                                                                                        style->colorMax_srm()));
   this->styleButton->setStyle(style);

   return;
}

void MainWindow::updateRecipeStyle() {
   if (recipeObs == nullptr) {
      return;
   }

   QModelIndex proxyIndex( this->pimpl->m_styleProxyModel->index(styleComboBox->currentIndex(),0) );
   QModelIndex sourceIndex( this->pimpl->m_styleProxyModel->mapToSource(proxyIndex) );
   Style * selected = this->pimpl->m_styleListModel->at(sourceIndex.row());
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

   Mash* selected = this->pimpl->m_mashListModel->at(mashComboBox->currentIndex());
   if (selected) {
      // The Recipe will decide whether it needs to make a copy of the Mash, hence why we don't reuse "selected" below
      this->recipeObs->setMash(selected);
      this->pimpl->m_mashEditor->setMash(this->recipeObs->mash());
      mashButton->setMash(this->recipeObs->mash());
   }
   return;
}

void MainWindow::updateRecipeEquipment() {
  droppedRecipeEquipment(this->pimpl->m_equipmentListModel->at(equipmentComboBox->currentIndex()));
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
      new SimpleUndoableUpdate(*m, TYPE_INFO(Mash, mashTunWeight_kg         ), kit->mashTunWeight_kg()         , tr("Change Tun Weight")       , equipmentUpdate);
      new SimpleUndoableUpdate(*m, TYPE_INFO(Mash, mashTunSpecificHeat_calGC), kit->mashTunSpecificHeat_calGC(), tr("Change Tun Specific Heat"), equipmentUpdate);
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
      // (The previous call here to this->pimpl->m_mashEditor->setRecipe() was a roundabout way of calling setTunWeight_kg() and
      // setMashTunSpecificHeat_calGC() on the mash.)
      new SimpleUndoableUpdate(*this->recipeObs, TYPE_INFO(Recipe, batchSize_l ), kit->fermenterBatchSize_l() , tr("Change Batch Size"), equipmentUpdate);
      new SimpleUndoableUpdate(*this->recipeObs, TYPE_INFO(Recipe, boilSize_l  ), kit->kettleBoilSize_l()  , tr("Change Boil Size") , equipmentUpdate);
      new SimpleUndoableUpdate(*this->recipeObs, TYPE_INFO(Recipe, boilTime_min), kit->boilTime_min(), tr("Change Boil Time") , equipmentUpdate);
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

   if (tabWidget_ingredients->currentWidget() != recipeFermentableTab) {
      tabWidget_ingredients->setCurrentWidget(recipeFermentableTab);
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

void MainWindow::droppedRecipeHop(QList<Hop *> hops) {
   if (!this->recipeObs) {
      return;
   }

   if (tabWidget_ingredients->currentWidget() != recipeHopTab) {
      tabWidget_ingredients->setCurrentWidget(recipeHopTab);
   }

   auto hopAdditions = RecipeAdditionHop::create(*this->recipeObs, hops);

   this->doOrRedoUpdate(
      newUndoableAddOrRemoveList(*this->recipeObs,
                                 &Recipe::addAddition<RecipeAdditionHop>,
                                 hopAdditions,
                                 &Recipe::removeAddition<RecipeAdditionHop>,
                                 tr("Drop hops on a recipe"))
   );
   return;
}

void MainWindow::droppedRecipeMisc(QList<Misc*>miscs) {
   if (!this->recipeObs) {
      return;
   }

   if (tabWidget_ingredients->currentWidget() != recipeMiscTab) {
      tabWidget_ingredients->setCurrentWidget(recipeMiscTab);
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

   if ( tabWidget_ingredients->currentWidget() != recipeYeastTab )
      tabWidget_ingredients->setCurrentWidget(recipeYeastTab);
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
                        TYPE_INFO(Recipe, batchSize_l),
                        lineEdit_batchSize->toCanonical().quantity(),
                        tr("Change Batch Size"));
}

void MainWindow::updateRecipeBoilSize() {
   if (!this->recipeObs) {
      return;
   }

   this->doOrRedoUpdate(*this->recipeObs,
                        TYPE_INFO(Recipe, boilSize_l),
                        lineEdit_boilSize->toCanonical().quantity(),
                        tr("Change Boil Size"));
}

void MainWindow::updateRecipeBoilTime() {
   if (!this->recipeObs) {
      return;
   }

   Equipment* kit = recipeObs->equipment();
   double boilTime = Measurement::qStringToSI(lineEdit_boilTime->text(), Measurement::PhysicalQuantity::Time).quantity();

   // Here, we rely on a signal/slot connection to propagate the equipment changes to recipeObs->boilTime_min and maybe
   // recipeObs->boilSize_l
   // NOTE: This works because kit is the recipe's equipment, not the generic equipment in the recipe drop down.
   if (kit) {
      this->doOrRedoUpdate(*kit, TYPE_INFO(Equipment, boilTime_min), boilTime, tr("Change Boil Time"));
   } else {
      this->doOrRedoUpdate(*this->recipeObs, TYPE_INFO(Recipe, boilTime_min), boilTime, tr("Change Boil Time"));
   }

   return;
}

void MainWindow::updateRecipeEfficiency() {
   qDebug() << Q_FUNC_INFO << lineEdit_efficiency->getNonOptValue<double>();
   if (!this->recipeObs) {
      return;
   }

   this->doOrRedoUpdate(*this->recipeObs,
                        TYPE_INFO(Recipe, efficiency_pct),
                        lineEdit_efficiency->getNonOptValue<unsigned int>(),
                        tr("Change Recipe Efficiency"));
   return;
}

template<class NE>
void MainWindow::addToRecipe(std::shared_ptr<NE> ne) {
   Q_ASSERT(ne);

   this->doOrRedoUpdate(
      newUndoableAddOrRemove(*this->recipeObs,
                             &Recipe::add<NE>,
                             ne,
                             &Recipe::remove<NE>,
                             QString(tr("Add %1 to recipe")).arg(NE::LocalisedName))
   );

   // Since we just added an ingredient, switch the focus to the tab that lists that type of ingredient.  We rely here
   // on the individual tabs following a naming convention (recipeHopTab, recipeFermentableTab, etc)
   // Note that we want the untranslated class name because this is not for display but to refer to a QWidget inside
   // tabWidget_ingredients
   auto const widgetName = QString("recipe%1Tab").arg(NE::staticMetaObject.className());
   qDebug() << Q_FUNC_INFO << widgetName;
   QWidget * widget = this->tabWidget_ingredients->findChild<QWidget *>(widgetName);
   Q_ASSERT(widget);
   this->tabWidget_ingredients->setCurrentWidget(widget);

   // We don't need to call this->pimpl->m_hopAdditionsTableModel->addHop(hop) here (or the equivalent for fermentable, misc or
   // yeast) because the change to the recipe will already have triggered the necessary updates to
   // this->pimpl->m_hopAdditionsTableModel/this->pimpl->m_fermentableTableModel/etc.
   return;
}
//
// Instantiate the above template function for the types that are going to use it
// (This is all just a trick to allow the template definition to be here in the .cpp file and not in the header.)
//
template void MainWindow::addToRecipe(std::shared_ptr<Fermentable> ne);
template void MainWindow::addToRecipe(std::shared_ptr<Misc       > ne);
template void MainWindow::addToRecipe(std::shared_ptr<Yeast      > ne);

void MainWindow::addHopToRecipe(Hop * hop) {
   if (!this->recipeObs) {
      return;
   }
   auto hopAddition = std::make_shared<RecipeAdditionHop>(*this->recipeObs, *hop);
   this->pimpl->doRecipeAddition(hopAddition);
   return;
}

void MainWindow::removeSelectedHopAddition() {
   this->pimpl->doRemoveRecipeAddition<RecipeAdditionHop>(hopAdditionTable,
                                                          this->pimpl->m_hopAdditionsTableProxy.get(),
                                                          this->pimpl->m_hopAdditionsTableModel.get());
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
                             &Mash::addStep,
                             mashStep,
                             &Mash::removeStep,
                             tr("Add mash step to recipe"))
   );
   // We don't need to call this->pimpl->m_mashStepTableModel->addMashStep(mashStep) here because the change to the mash will already
   // have triggered the necessary updates to this->pimpl->m_mashStepTableModel.
   return;
}

/**
 * This is akin to a special case of MainWindow::exportSelected()
 */
void MainWindow::exportRecipe() {
   if (!this->recipeObs) {
      return;
   }

   QList<Recipe const *> recipes;
   recipes.append(this->recipeObs);

   ImportExport::exportToFile(&recipes);
   return;
}

Recipe* MainWindow::currentRecipe() {
   return recipeObs;
}

void MainWindow::setUndoRedoEnable() {
   Q_ASSERT(this->pimpl->m_undoStack != 0);
   actionUndo->setEnabled(this->pimpl->m_undoStack->canUndo());
   actionRedo->setEnabled(this->pimpl->m_undoStack->canRedo());

   actionUndo->setText(QString(tr("Undo %1").arg(this->pimpl->m_undoStack->undoText())));
   actionRedo->setText(QString(tr("Redo %1").arg(this->pimpl->m_undoStack->redoText())));

   return;
}

void MainWindow::doOrRedoUpdate(QUndoCommand * update) {
   Q_ASSERT(this->pimpl->m_undoStack != nullptr);
   Q_ASSERT(update != nullptr);
   this->pimpl->m_undoStack->push(update);
   this->setUndoRedoEnable();
   return;
}

void MainWindow::doOrRedoUpdate(NamedEntity & updatee,
                                TypeInfo const & typeInfo,
                                QVariant newValue,
                                QString const & description,
                                [[maybe_unused]] QUndoCommand * parent) {
   this->doOrRedoUpdate(new SimpleUndoableUpdate(updatee, typeInfo, newValue, description));
   return;
}

// For undo/redo, we use Qt's Undo framework
void MainWindow::editUndo() {
   Q_ASSERT(this->pimpl->m_undoStack != 0);
   if ( !this->pimpl->m_undoStack->canUndo() ) {
      qDebug() << "Undo called but nothing to undo";
   } else {
      this->pimpl->m_undoStack->undo();
   }

   setUndoRedoEnable();
   return;
}

void MainWindow::editRedo() {
   Q_ASSERT(this->pimpl->m_undoStack != 0);
   if ( !this->pimpl->m_undoStack->canRedo() ) {
      qDebug() << "Redo called but nothing to redo";
   } else {
      this->pimpl->m_undoStack->redo();
   }

   setUndoRedoEnable();
   return;
}

// .:TBD:. Can we get rid of this now?
void MainWindow::removeHopAddition(std::shared_ptr<RecipeAdditionHop> itemToRemove) {
   this->pimpl->m_hopAdditionsTableModel->remove(itemToRemove);
   return;
}
void MainWindow::removeFermentable(std::shared_ptr<Fermentable> itemToRemove) {
   this->pimpl->m_fermTableModel->remove(itemToRemove);
   return;
}
void MainWindow::removeMisc(std::shared_ptr<Misc> itemToRemove) {
   this->pimpl->m_miscTableModel->remove(itemToRemove);
   return;
}
void MainWindow::removeYeast(std::shared_ptr<Yeast> itemToRemove) {
   this->pimpl->m_yeastTableModel->remove(itemToRemove);
   return;
}

void MainWindow::removeMashStep(std::shared_ptr<MashStep> itemToRemove) {
   this->pimpl->m_mashStepTableModel->remove(itemToRemove);
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
      QModelIndex modelIndex = this->pimpl->m_fermTableProxy->mapToSource(viewIndex);

      itemsToRemove.append(this->pimpl->m_fermTableModel->getRow(modelIndex.row()));
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
   Fermentable* f = this->pimpl->selectedFermentable();
   if (!f) {
      return;
   }

   this->pimpl->m_fermEditor->setEditItem(ObjectStoreWrapper::getSharedFromRaw(f));
   this->pimpl->m_fermEditor->show();
   return;
}

void MainWindow::editSelectedMisc() {
   Misc* m = this->pimpl->selectedMisc();
   if (!m) {
      return;
   }

   this->pimpl->m_miscEditor->setEditItem(ObjectStoreWrapper::getSharedFromRaw(m));
   this->pimpl->m_miscEditor->show();
}

void MainWindow::editHopOfSelectedHopAddition() {
   RecipeAdditionHop * hopAddition = this->pimpl->selectedHopAddition();
   if (!hopAddition) {
      return;
   }

   Hop * hop = hopAddition->hop();
   if (!hop) {
      return;
   }

   this->pimpl->m_hopEditor->setEditItem(ObjectStoreWrapper::getSharedFromRaw(hop));
   this->pimpl->m_hopEditor->show();
   return;
}

void MainWindow::editSelectedYeast() {
   Yeast* y = this->pimpl->selectedYeast();
   if (!y) {
      return;
   }

   this->pimpl->m_yeastEditor->setEditItem(ObjectStoreWrapper::getSharedFromRaw(y));
   this->pimpl->m_yeastEditor->show();
   return;
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
      QModelIndex modelIndex = this->pimpl->m_miscTableProxy->mapToSource(viewIndex);

      itemsToRemove.append(this->pimpl->m_miscTableModel->getRow(modelIndex.row()));
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
      QModelIndex modelIndex = this->pimpl->m_yeastTableProxy->mapToSource(viewIndex);

      itemsToRemove.append(this->pimpl->m_yeastTableModel->getRow(modelIndex.row()));
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

   std::shared_ptr<Recipe> newRec = std::make_shared<Recipe>(name);

   // bad things happened -- let somebody know
   if (!newRec) {
      QMessageBox::warning(this,
                           tr("Error creating recipe"),
                           tr("An error was returned while creating %1").arg(name));
      return;
   }
   ObjectStoreWrapper::insert(newRec);
   std::shared_ptr<Boil> newBoil = std::make_shared<Boil>(QString("Boil for").arg(name));
   newRec->setBoil(newBoil);
   ObjectStoreWrapper::insert(newBoil);

   // Set the following stuff so everything appears nice
   // and the calculations don't divide by zero... things like that.
   newRec->setBatchSize_l(18.93); // 5 gallons
   newBoil->setPreBoilSize_l(23.47);  // 6.2 gallons
   newRec->setEfficiency_pct(70.0);

   // we need a valid key, so insert the recipe before we add equipment
   if ( defEquipKey != -1 )
   {
      Equipment *e = ObjectStoreWrapper::getByIdRaw<Equipment>(defEquipKey.toInt());
      // I really want to do this before we've written the object to the
      // database
      if ( e ) {
         newRec->setBatchSize_l( e->fermenterBatchSize_l() );
         newBoil->setPreBoilSize_l( e->kettleBoilSize_l() );
         newBoil->setBoilTime_mins( e->boilTime_min().value_or(Equipment::default_boilTime_min) );
         newRec->setEquipment(e);
      }
   }


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
   this->setTreeSelection(treeView_recipe->findElement(newRec.get()));
   this->setRecipe(newRec.get());
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
void MainWindow::reduceInventory() {

   for (QModelIndex selected : treeView_recipe->selectionModel()->selectedRows()) {
      Recipe* rec = treeView_recipe->getItem<Recipe>(selected);
      if (rec == nullptr) {
         // Try the parent recipe
         rec = treeView_recipe->getItem<Recipe>(treeView_recipe->parent(selected));
         if (rec == nullptr) {
            continue;
         }
      }

      // Make sure everything is properly set and selected
      if (rec != recipeObs) {
         setRecipe(rec);
      }

      //
      // Reduce fermentables, miscs, hops, yeasts
      //
      // Note that yeast inventory is done by "quanta" not amount, probably trying to indicate number of packets rather
      // than weight.  Of course, even though quanta implies an integer measurement, inventory() still returns a double
      // so there's some heroic casting going on here.
      //
      // For fermentables and miscs, the amount can be either mass or volume, but we don't worry about which here as we
      // assume that a given type of fermentable/misc is always measured in the same way.
      //
      for (auto ii : rec->fermentables()) { ii->setInventoryAmount(std::max(                 ii->inventory()  - ii->amount(),    0.0)); }
      for (auto ii : rec->miscs       ()) { ii->setInventoryAmount(std::max(                 ii->inventory()  - ii->amount(),    0.0)); }
///      for (auto ii : rec->hops        ()) { ii->setInventoryAmount(std::max(                 ii->inventory()  - ii->amount_kg(), 0.0)); }
      for (auto ii : rec->yeasts      ()) { ii->setInventoryQuanta(std::max(static_cast<int>(ii->inventory()) - 1,               0  )); }

/// TODO      for (auto ii : rec->hopAdditions()) {
   }

   return;
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
   this->pimpl->m_mashStepEditor->setEditItem(step);
   this->pimpl->m_mashStepEditor->setVisible(true);
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

   auto step = this->pimpl->m_mashStepTableModel->getRow(row);
   this->doOrRedoUpdate(
      newUndoableAddOrRemove(*this->recipeObs->mash(),
                              &Mash::removeStep,
                              step,
                              &Mash::addStep,
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

   this->pimpl->m_mashStepTableModel->moveStepUp(row);
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
   if( row >= this->pimpl->m_mashStepTableModel->rowCount() - 1 ) {
      return;
   }

   this->pimpl->m_mashStepTableModel->moveStepDown(row);
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

   auto step = this->pimpl->m_mashStepTableModel->getRow(static_cast<unsigned int>(row));
   this->pimpl->m_mashStepEditor->setEditItem(step);
   this->pimpl->m_mashStepEditor->setVisible(true);
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

   m->removeAllSteps();
   ObjectStoreWrapper::softDelete(*m);

   auto defaultMash = std::make_shared<Mash>();
   ObjectStoreWrapper::insert(defaultMash);
   this->recipeObs->setMash(defaultMash.get());

   this->pimpl->m_mashStepTableModel->setMash(defaultMash.get());

   //remove from combobox handled automatically by qt
   mashButton->setMash(defaultMash.get());

}

void MainWindow::closeEvent(QCloseEvent* /*event*/)
{
   Application::saveSystemOptions();
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

void MainWindow::copyRecipe() {
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

void MainWindow::setupContextMenu() {

   treeView_recipe->setupContextMenu(this, this);
   treeView_equip->setupContextMenu(this, this->pimpl->m_equipEditor.get());
   treeView_ferm ->setupContextMenu(this, this->pimpl->m_fermEditor       .get());
   treeView_hops ->setupContextMenu(this, this->pimpl->m_hopEditor        .get());
   treeView_misc ->setupContextMenu(this, this->pimpl->m_miscEditor       .get());
   treeView_style->setupContextMenu(this, this->pimpl->m_styleEditor.get());
   treeView_yeast->setupContextMenu(this, this->pimpl->m_yeastEditor      .get());
   treeView_water->setupContextMenu(this, this->pimpl->m_waterEditor      .get());

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
   QList<Equipment   const *> equipments;
   QList<Fermentable const *> fermentables;
   QList<Hop         const *> hops;
   QList<Misc        const *> miscs;
   QList<Recipe      const *> recipes;
   QList<Style       const *> styles;
   QList<Water       const *> waters;
   QList<Yeast       const *> yeasts;

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
      this->pimpl->m_pitchDialog->setWortVolume_l( recipeObs->finalVolume_l() );
      this->pimpl->m_pitchDialog->setWortDensity( recipeObs->og() );
      this->pimpl->m_pitchDialog->calculate();
   }

   this->pimpl->m_pitchDialog->show();
}

void MainWindow::showEquipmentEditor()
{
   if ( recipeObs && ! recipeObs->equipment() )
   {
      QMessageBox::warning( this, tr("No equipment"), tr("You must select or define an equipment profile first."));
   }
   else
   {
      this->pimpl->m_equipEditor->setEditItem(ObjectStoreWrapper::getSharedFromRaw(recipeObs->equipment()));
      this->pimpl->m_equipEditor->show();
   }
}

void MainWindow::showStyleEditor() {
   if ( recipeObs && ! recipeObs->style() ) {
      QMessageBox::warning( this, tr("No style"), tr("You must select a style first."));
   } else {
      this->pimpl->m_styleEditor->setEditItem(ObjectStoreWrapper::getSharedFromRaw(recipeObs->style()));
      this->pimpl->m_styleEditor->show();
   }
   return;
}

void MainWindow::changeBrewDate() {
   QModelIndexList indexes = treeView_recipe->selectionModel()->selectedRows();

   for (QModelIndex selected : indexes) {
      auto target = treeView_recipe->getItem<BrewNote>(selected);

      // No idea how this could happen, but I've seen stranger things
      if ( ! target )
         continue;

      // Pop the calendar, get the date.
      if ( this->pimpl->m_btDatePopup->exec() == QDialog::Accepted )
      {
         QDate newDate = this->pimpl->m_btDatePopup->selectedDate();
         target->setBrewDate(newDate);

         // If this note is open in a tab
         BrewNoteWidget* ni = findBrewNoteWidget(target);
         if (ni) {
            tabWidget_recipeView->setTabText(tabWidget_recipeView->indexOf(ni), target->brewDate_short());
            return;
         }
      }
   }
   return;
}

void MainWindow::fixBrewNote() {
   QModelIndexList indexes = treeView_recipe->selectionModel()->selectedRows();

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
   return;
}

void MainWindow::updateStatus(const QString status) {
   if (statusBar()) {
      statusBar()->showMessage(status, 3000);
   }
   return;
}

void MainWindow::versionedRecipe(Recipe* descendant) {
   QModelIndex ndx = treeView_recipe->findElement(descendant);
   setRecipe(descendant);
   treeView_recipe->setCurrentIndex(ndx);
   return;
}

// .:TBD:. Seems redundant to pass both the brewnote ID and a pointer to it; we only need one of these
void MainWindow::closeBrewNote([[maybe_unused]] int brewNoteId, std::shared_ptr<QObject> object) {
   BrewNote* b = std::static_pointer_cast<BrewNote>(object).get();
   Recipe* parent = ObjectStoreWrapper::getByIdRaw<Recipe>(b->getRecipeId());

   // If this isn't the focused recipe, do nothing because there are no tabs
   // to close.
   if (parent != recipeObs) {
      return;
   }

   BrewNoteWidget* ni = findBrewNoteWidget(b);
   if (ni) {
      tabWidget_recipeView->removeTab( tabWidget_recipeView->indexOf(ni));
   }

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
      this->pimpl->m_waterDialog->setRecipe(recipeObs);
      this->pimpl->m_waterDialog->show();
   }
   else {
      QMessageBox::warning( this, tr("No Mash"), tr("You must define a mash first."));
   }
}
