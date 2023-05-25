/*======================================================================================================================
 * tableModels/FermentableTableModel.cpp is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Daniel Pettersson <pettson81@gmail.com>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Samuel Östling <MrOstling@gmail.com>
 *   • Tim Payne <swstim@gmail.com>
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
#include "tableModels/FermentableTableModel.h"

#include <array>

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAbstractTableModel>
#include <QComboBox>
#include <QDebug>
#include <QHeaderView>
#include <QItemEditorFactory>
#include <QLineEdit>
#include <QListWidget>
#include <QModelIndex>
#include <QRect>
#include <QSize>
#include <QString>
#include <QStyle>
#include <QVariant>
#include <QVector>
#include <QWidget>

#include "database/ObjectStoreWrapper.h"
#include "MainWindow.h"
#include "measurement/Measurement.h"
#include "measurement/Unit.h"
#include "model/Inventory.h"
#include "model/Recipe.h"
#include "PersistentSettings.h"
#include "tableModels/ItemDelegate.h"
#include "utils/BtStringConst.h"
#include "widgets/BtComboBox.h"

namespace {
   //
   // We have a bunch of logic for interpreting Fermentable::isMashed() and Fermentable::addAfterBoil() which used to
   // live in the Fermentable class itself but is only used in this table model, so I moved it here to simplify
   // Fermentable.
   //
   // Additionally, we used to assume that a thing that is a grain and not mashed must be steeped.  This is not
   // necessarily true.  I have simplified things so we now just show two options - Mashed and Not Mashed.
   //
   // Note that these two arrays rely on the fact that static_cast<int>(false) == 0 and static_cast<int>(true) == 1
   //
   std::array<QString const, 2> descAddAfterBoil {
      QObject::tr("Normal"), // addAfterBoil() == false
      QObject::tr("Late")    // addAfterBoil() == true
   };
   std::array<QString const, 2> descIsMashed {
      QObject::tr("Not mashed"), // isMashed() == false
      QObject::tr("Mashed")      // isMashed() == true
   };

}

// .:TODO:. We need to unify some of the logic from Misc into common code with Fermentable so we can write the handling
// for weight/volume once.  What's here for the moment is showing weight/volume but not allowing it to be edited.

//=====================CLASS FermentableTableModel==============================
FermentableTableModel::FermentableTableModel(QTableView* parent, bool editable) :
   BtTableModelInventory{
      parent,
      editable,
      {
         // NB: Need PropertyNames::Fermentable::amountWithUnits not PropertyNames::Fermentable::amount below so we can
         //     handle mass-or-volume generically in TableModelBase.
         SMART_COLUMN_HEADER_DEFN(FermentableTableModel, Name     , tr("Name"       ), Fermentable, PropertyNames::NamedEntity::name          ),
         SMART_COLUMN_HEADER_DEFN(FermentableTableModel, Type     , tr("Type"       ), Fermentable, PropertyNames::Fermentable::type          , EnumInfo{Fermentable::typeStringMapping, Fermentable::typeDisplayNames}),
         SMART_COLUMN_HEADER_DEFN(FermentableTableModel, Amount   , tr("Amount"     ), Fermentable, PropertyNames::Fermentable::amountWithUnits),
         SMART_COLUMN_HEADER_DEFN(FermentableTableModel, Inventory, tr("Inventory"  ), Fermentable, PropertyNames::Fermentable::amount        ), // No inventory property name TODO Fix this!
         SMART_COLUMN_HEADER_DEFN(FermentableTableModel, IsWeight , tr("Amount Type"), Fermentable, PropertyNames::Fermentable::amountIsWeight, BoolInfo{tr("Volume"    ), tr("Weight")}),
         SMART_COLUMN_HEADER_DEFN(FermentableTableModel, IsMashed , tr("Method"     ), Fermentable, PropertyNames::Fermentable::isMashed      , BoolInfo{tr("Not mashed"), tr("Mashed")}),
         SMART_COLUMN_HEADER_DEFN(FermentableTableModel, AfterBoil, tr("Addition"   ), Fermentable, PropertyNames::Fermentable::addAfterBoil  , BoolInfo{tr("Normal"    ), tr("Late"  )}),
         SMART_COLUMN_HEADER_DEFN(FermentableTableModel, Yield    , tr("Yield %"    ), Fermentable, PropertyNames::Fermentable::yield_pct     , PrecisionInfo{1}),
         SMART_COLUMN_HEADER_DEFN(FermentableTableModel, Color    , tr("Color"      ), Fermentable, PropertyNames::Fermentable::color_srm     , PrecisionInfo{1}),
      }
   },
   TableModelBase<FermentableTableModel, Fermentable>{},
   displayPercentages(false),
   totalFermMass_kg(0) {

   // for units and scales
   setObjectName("fermentableTable");

   QHeaderView* headerView = parentTableWidget->horizontalHeader();
   connect(headerView, &QWidget::customContextMenuRequested, this, &FermentableTableModel::contextMenu);
   connect(&ObjectStoreTyped<InventoryFermentable>::getInstance(), &ObjectStoreTyped<InventoryFermentable>::signalPropertyChanged, this, &FermentableTableModel::changedInventory);
   return;
}

FermentableTableModel::~FermentableTableModel() = default;

// .:TODO:.:JSON:.  Now that fermentables can also be measured by volume, we might need to rethink this
void FermentableTableModel::added  (std::shared_ptr<Fermentable> item) { if (item->amountIsWeight()) { this->totalFermMass_kg += item->amount(); } return; }
void FermentableTableModel::removed(std::shared_ptr<Fermentable> item) { if (item->amountIsWeight()) { this->totalFermMass_kg -= item->amount(); } return; }
void FermentableTableModel::removedAll() { this->totalFermMass_kg = 0; return; };

void FermentableTableModel::observeRecipe(Recipe* rec) {
   if (this->recObs) {
      qDebug() << Q_FUNC_INFO << "Unobserve Recipe #" << this->recObs->key() << "(" << this->recObs->name() << ")";
      disconnect(this->recObs, nullptr, this, nullptr);
      this->removeAll();
   }

   this->recObs = rec;
   if (this->recObs) {
      qDebug() << Q_FUNC_INFO << "Observe Recipe #" << this->recObs->key() << "(" << this->recObs->name() << ")";

      connect(this->recObs, &NamedEntity::changed, this, &FermentableTableModel::changed);
      this->addFermentables(this->recObs->getAll<Fermentable>());
   }
   return;
}

void FermentableTableModel::observeDatabase(bool val) {
   if ( val ) {
      // Observing a database and a recipe are mutually exclusive.
      this->observeRecipe(nullptr);

      this->removeAll();
      connect(&ObjectStoreTyped<Fermentable>::getInstance(), &ObjectStoreTyped<Fermentable>::signalObjectInserted, this, &FermentableTableModel::addFermentable);
      connect(&ObjectStoreTyped<Fermentable>::getInstance(), &ObjectStoreTyped<Fermentable>::signalObjectDeleted,  this, &FermentableTableModel::removeFermentable);
      this->addFermentables(ObjectStoreWrapper::getAll<Fermentable>());
   } else {
      disconnect(&ObjectStoreTyped<Fermentable>::getInstance(), nullptr, this, nullptr);
      this->removeAll();
   }
   return;
}

void FermentableTableModel::addFermentable(int fermId) {
   auto ferm = ObjectStoreWrapper::getById<Fermentable>(fermId);
   qDebug() << Q_FUNC_INFO << ferm->name();

   // Check to see if it's already in the list
   if (this->rows.contains(ferm)) {
      return;
   }

   // If we are observing the database, ensure that the ferm is undeleted and fit to display.
   if (this->recObs == nullptr && (ferm->deleted() || !ferm->display())) {
      return;
   }

   // If we are watching a Recipe and the new Fermentable does not belong to it then there is nothing for us to do
   if (this->recObs) {
      Recipe * recipeOfNewFermentable = ferm->getOwningRecipe();
      if (recipeOfNewFermentable && this->recObs->key() != recipeOfNewFermentable->key()) {
         qDebug() <<
            Q_FUNC_INFO << "Ignoring signal about new Ferementable #" << ferm->key() << "as it belongs to Recipe #" <<
            recipeOfNewFermentable->key() << "and we are watching Recipe #" << this->recObs->key();
         return;
      }
   }

   this->add(ferm);
   return;
}

void FermentableTableModel::addFermentables(QList<std::shared_ptr<Fermentable> > ferms) {
   qDebug() << Q_FUNC_INFO << "Add up to " << ferms.size() << " fermentables to existing list of " << this->rows.size();

   auto tmp = this->removeDuplicates(ferms, this->recObs);

   qDebug() << Q_FUNC_INFO << QString("After de-duping, adding %1 fermentables").arg(tmp.size());

   int size = this->rows.size();
   if (size+tmp.size()) {
      beginInsertRows( QModelIndex(), size, size+tmp.size()-1 );
      this->rows.append(tmp);

      for (auto ferm : tmp) {
         connect(ferm.get(), &NamedEntity::changed, this, &FermentableTableModel::changed);
         if (ferm->amountIsWeight()) {
            totalFermMass_kg += ferm->amount();
         }
      }

      endInsertRows();
   }
}

void FermentableTableModel::removeFermentable([[maybe_unused]] int fermId,
                                              std::shared_ptr<QObject> object) {
   this->remove(std::static_pointer_cast<Fermentable>(object));
   return;
}

void FermentableTableModel::updateTotalGrains() {
   this->totalFermMass_kg = 0;
   for (auto const & ferm : this->rows) {
      if (ferm->amountIsWeight()) {
         totalFermMass_kg += ferm->amount();
      }
   }
   return;
}

void FermentableTableModel::setDisplayPercentages(bool var) {
   this->displayPercentages = var;
   return;
}

void FermentableTableModel::changedInventory(int invKey, BtStringConst const & propertyName) {

   if (propertyName == PropertyNames::Inventory::amount) {
      for (int ii = 0; ii < this->rows.size(); ++ii) {
         if (invKey == this->rows.at(ii)->inventoryId()) {
            emit dataChanged(QAbstractItemModel::createIndex(ii, static_cast<int>(FermentableTableModel::ColumnIndex::Inventory)),
                             QAbstractItemModel::createIndex(ii, static_cast<int>(FermentableTableModel::ColumnIndex::Inventory)));
         }
      }
   }
   return;
}

void FermentableTableModel::changed(QMetaProperty prop, [[maybe_unused]] QVariant val) {
//   qDebug() << Q_FUNC_INFO << prop.name();

   // Is sender one of our fermentables?
   Fermentable* fermSender = qobject_cast<Fermentable*>(sender());
   if (fermSender) {
      int ii = this->findIndexOf(fermSender);
      if (ii < 0) {
         return;
      }

      this->updateTotalGrains();
      emit dataChanged(QAbstractItemModel::createIndex(ii, 0),
                       QAbstractItemModel::createIndex(ii, this->columnCount() - 1));
      if (displayPercentages && rowCount() > 0) {
         emit headerDataChanged(Qt::Vertical, 0, rowCount() - 1);
      }
      return;
   }

   // See if our recipe gained or lost fermentables.
   Recipe* recSender = qobject_cast<Recipe*>(sender());
   if (recSender && recSender == recObs && prop.name() == PropertyNames::Recipe::fermentableIds) {
      this->removeAll();
      this->addFermentables(this->recObs->getAll<Fermentable>());
   }

   return;
}

int FermentableTableModel::rowCount(QModelIndex const & /*parent*/) const {
   return this->rows.size();
}

QVariant FermentableTableModel::data(QModelIndex const & index, int role) const {
   if (!this->isIndexOk(index)) {
      return QVariant();
   }

   auto row = this->rows[index.row()];
   auto const columnIndex = static_cast<FermentableTableModel::ColumnIndex>(index.column());
   switch (columnIndex) {
      case FermentableTableModel::ColumnIndex::Name:
      case FermentableTableModel::ColumnIndex::Type:
      case FermentableTableModel::ColumnIndex::IsWeight:
      case FermentableTableModel::ColumnIndex::IsMashed:
      case FermentableTableModel::ColumnIndex::AfterBoil:
      case FermentableTableModel::ColumnIndex::Yield:
      case FermentableTableModel::ColumnIndex::Color:
      case FermentableTableModel::ColumnIndex::Amount:
         return this->readDataFromModel(index, role);

      case FermentableTableModel::ColumnIndex::Inventory:
         if (role == Qt::DisplayRole) {
            return QVariant(
               Measurement::displayAmount(Measurement::Amount{
                                             row->inventory(),
                                             row->amountIsWeight() ? Measurement::Units::kilograms :
                                                                     Measurement::Units::liters
                                          },
                                          3,
                                          this->get_ColumnInfo(columnIndex).getForcedSystemOfMeasurement(),
                                          std::nullopt)
            );
         }
         break;
///      case FermentableTableModel::ColumnIndex::Amount:
///         if (role == Qt::DisplayRole) {
///            return QVariant(
///               Measurement::displayAmount(Measurement::Amount{
///                                             row->amount(),
///                                             row->amountIsWeight() ? Measurement::Units::kilograms :
///                                                                     Measurement::Units::liters
///                                          },
///                                          3,
///                                          this->get_ColumnInfo(columnIndex).getForcedSystemOfMeasurement(),
///                                          std::nullopt)
///            );
///         }
///         break;
      default:
         qCritical() << Q_FUNC_INFO << "Bad column: " << index.column();
         break;
   }
   return QVariant();
}

QVariant FermentableTableModel::headerData( int section, Qt::Orientation orientation, int role ) const {
   if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
      return this->getColumnLabel(section);
   }

   if (displayPercentages && orientation == Qt::Vertical && role == Qt::DisplayRole) {
      double perMass = 0.0;
      if (totalFermMass_kg > 0.0 ) {
         // .:TODO:. Work out what to do for amounts that are volumes
         if (this->rows[section]->amountIsWeight()) {
            perMass = this->rows[section]->amount()/totalFermMass_kg;
         } else {
            qWarning() << Q_FUNC_INFO << "Unhandled branch for liquid fermentables";
         }
      }
      return QVariant( QString("%1%").arg( static_cast<double>(100.0) * perMass, 0, 'f', 0 ) );
   }

   return QVariant();
}

Qt::ItemFlags FermentableTableModel::flags(QModelIndex const & index) const {
   Qt::ItemFlags constexpr defaults = Qt::ItemIsEnabled;
   auto row = this->rows[index.row()];

   auto const columnIndex = static_cast<FermentableTableModel::ColumnIndex>(index.column());
   switch (columnIndex) {
      case FermentableTableModel::ColumnIndex::IsMashed:
         // Ensure that being mashed and being a late addition are mutually exclusive.
         if (!row->addAfterBoil()) {
            return (defaults | Qt::ItemIsSelectable | (editable ? Qt::ItemIsEditable : Qt::NoItemFlags) | Qt::ItemIsDragEnabled);
         }
         return Qt::ItemIsSelectable | (editable ? Qt::ItemIsEditable : Qt::NoItemFlags) | Qt::ItemIsDragEnabled;
      case FermentableTableModel::ColumnIndex::AfterBoil:
         // Ensure that being mashed and being a late addition are mutually exclusive.
         if (!row->isMashed()) {
            return (defaults | Qt::ItemIsSelectable | (editable ? Qt::ItemIsEditable : Qt::NoItemFlags) | Qt::ItemIsDragEnabled);
         }
         return Qt::ItemIsSelectable | (editable ? Qt::ItemIsEditable : Qt::NoItemFlags) | Qt::ItemIsDragEnabled;
      case FermentableTableModel::ColumnIndex::Name:
         return (defaults | Qt::ItemIsSelectable);
      case FermentableTableModel::ColumnIndex::Inventory:
         return (defaults | (this->isInventoryEditable() ? Qt::ItemIsEditable : Qt::NoItemFlags));
      default:
         return (defaults | Qt::ItemIsSelectable | (editable ? Qt::ItemIsEditable : Qt::NoItemFlags) );
   }
}


bool FermentableTableModel::setData(QModelIndex const & index,
                                    QVariant const & value,
                                    [[maybe_unused]] int role) {
   if (index.row() >= static_cast<int>(this->rows.size())) {
      return false;
   }

   bool retVal = false;
   auto row = this->rows[index.row()];

   Measurement::PhysicalQuantity physicalQuantity =
      row->amountIsWeight() ? Measurement::PhysicalQuantity::Mass: Measurement::PhysicalQuantity::Volume;

   auto const columnIndex = static_cast<FermentableTableModel::ColumnIndex>(index.column());
   switch (columnIndex) {
      case FermentableTableModel::ColumnIndex::Name:
         retVal = value.canConvert(QVariant::String);
         if (retVal) {
            MainWindow::instance().doOrRedoUpdate(*row,
                                                  TYPE_INFO(Fermentable, NamedEntity, name),
                                                  value.toString(),
                                                  tr("Change Fermentable Name"));
         }
         break;
      case FermentableTableModel::ColumnIndex::Type:
         retVal = value.canConvert(QVariant::Int);
         if (retVal) {
            // Doing the set via doOrRedoUpdate() saves us from doing a static_cast<Fermentable::Type>() here (as the
            // Q_PROPERTY system will do the casting for us).
            MainWindow::instance().doOrRedoUpdate(*row,
                                                  TYPE_INFO(Fermentable, type),
                                                  value.toInt(),
                                                  tr("Change Fermentable Type"));
         }
         break;
      case FermentableTableModel::ColumnIndex::Inventory:
         retVal = value.canConvert(QVariant::String);
         if (retVal) {
            MainWindow::instance().doOrRedoUpdate(
               *row,
               TYPE_INFO(Fermentable, NamedEntityWithInventory, inventory),
               Measurement::qStringToSI(value.toString(),
                                        physicalQuantity,
                                        this->get_ColumnInfo(columnIndex).getForcedSystemOfMeasurement(),
                                        this->get_ColumnInfo(columnIndex).getForcedRelativeScale()).quantity(),
               tr("Change Fermentable Inventory Amount")
            );
         }
         break;
      case FermentableTableModel::ColumnIndex::Amount:
         retVal = value.canConvert(QVariant::String);
         if (retVal) {
            // This is where the amount of a fermentable in a recipe gets updated
            // We need to refer back to the MainWindow to make this an undoable operation
            MainWindow::instance().doOrRedoUpdate(
               *row,
               TYPE_INFO(Fermentable, amount),
               Measurement::qStringToSI(value.toString(),
                                        physicalQuantity,
                                        this->get_ColumnInfo(columnIndex).getForcedSystemOfMeasurement(),
                                        this->get_ColumnInfo(columnIndex).getForcedRelativeScale()).quantity(),
               tr("Change Fermentable Amount")
            );
            if (rowCount() > 0) {
               headerDataChanged( Qt::Vertical, 0, rowCount()-1 ); // Need to re-show header (grain percent).
            }
         }
         break;
      case FermentableTableModel::ColumnIndex::IsWeight:
         if (!value.canConvert(QVariant::Bool)) {
            return false;
         }
         MainWindow::instance().doOrRedoUpdate(*row,
                                               TYPE_INFO(Fermentable, amountIsWeight),
                                               value.toBool(),
                                               tr("Change Fermentable Amount Type"));
         break;
      case FermentableTableModel::ColumnIndex::IsMashed:
         retVal = value.canConvert(QVariant::Bool);
         if (retVal) {
            // Doing the set via doOrRedoUpdate() saves us from doing a static_cast<Fermentable::AdditionMethod>() here
            // (as the Q_PROPERTY system will do the casting for us).
            MainWindow::instance().doOrRedoUpdate(*row,
                                                  TYPE_INFO(Fermentable, isMashed),
                                                  value.toBool(),
                                                  tr("Change Fermentable Is Mashed"));
         }
         break;
      case FermentableTableModel::ColumnIndex::AfterBoil:
         retVal = value.canConvert(QVariant::Bool);
         if (retVal) {
            // Doing the set via doOrRedoUpdate() saves us from doing a static_cast<Fermentable::AdditionTime>() here
            // (as the Q_PROPERTY system will do the casting for us).
            MainWindow::instance().doOrRedoUpdate(*row,
                                                  TYPE_INFO(Fermentable, addAfterBoil),
                                                  value.toBool(),
                                                  tr("Change Add After Boil"));
         }
         break;
      case FermentableTableModel::ColumnIndex::Yield:
         retVal = value.canConvert(QVariant::Double);
         if (retVal) {
            MainWindow::instance().doOrRedoUpdate(*row,
                                                  TYPE_INFO(Fermentable, yield_pct),
                                                  value.toDouble(),
                                                  tr("Change Yield"));
         }
         break;
      case FermentableTableModel::ColumnIndex::Color:
         retVal = value.canConvert(QVariant::Double);
         if (retVal) {
            MainWindow::instance().doOrRedoUpdate(
               *row,
               TYPE_INFO(Fermentable, color_srm),
               Measurement::qStringToSI(value.toString(),
                                        Measurement::PhysicalQuantity::Color,
                                        this->get_ColumnInfo(columnIndex).getForcedSystemOfMeasurement(),
                                        this->get_ColumnInfo(columnIndex).getForcedRelativeScale()).quantity(),
               tr("Change Color")
            );
         }
         break;
      default:
         qWarning() << Q_FUNC_INFO << "Bad column: " << index.column();
         return false;
   }
   return retVal;
}

// Insert the boiler-plate stuff that we cannot do in TableModelBase
TABLE_MODEL_COMMON_CODE(Fermentable)

//=========================================== CLASS FermentableItemDelegate ============================================

// Insert the boiler-plate stuff that we cannot do in ItemDelegate
ITEM_DELEGATE_COMMON_CODE(Fermentable)
