/*======================================================================================================================
 * tableModels/MashStepTableModel.cpp is part of Brewken, and is copyright the following authors 2009-2022:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Maxime Lavigne <duguigne@gmail.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
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
#include "tableModels/MashStepTableModel.h"

#include <QAbstractTableModel>
#include <QComboBox>
#include <QHeaderView>
#include <QItemDelegate>
#include <QLineEdit>
#include <QModelIndex>
#include <QObject>
#include <QTableView>
#include <QVariant>
#include <QVector>
#include <QWidget>

#include "database/ObjectStoreWrapper.h"
#include "MainWindow.h"
#include "measurement/Measurement.h"
#include "measurement/Unit.h"
#include "model/MashStep.h"
#include "PersistentSettings.h"
#include "SimpleUndoableUpdate.h"

MashStepTableModel::MashStepTableModel(QTableView* parent) :
   BtTableModel{
      parent,
      false,
      {{MASHSTEPNAMECOL,       {tr("Name"),          NonPhysicalQuantity::String,                ""                                    }},
       {MASHSTEPTYPECOL,       {tr("Type"),          NonPhysicalQuantity::String,                ""                                    }},
       {MASHSTEPAMOUNTCOL,     {tr("Amount"),        Measurement::PhysicalQuantity::Volume,      "amount"                              }}, // Not a real property name
       {MASHSTEPTEMPCOL,       {tr("Infusion Temp"), Measurement::PhysicalQuantity::Temperature, *PropertyNames::MashStep::infuseTemp_c}},
       {MASHSTEPTARGETTEMPCOL, {tr("Target Temp"),   Measurement::PhysicalQuantity::Temperature, *PropertyNames::MashStep::stepTemp_c  }},
       {MASHSTEPTIMECOL,       {tr("Time"),          Measurement::PhysicalQuantity::Time,        *PropertyNames::Misc::time            }}}
   },
   mashObs(nullptr) {
   setObjectName("mashStepTableModel");

   QHeaderView* headerView = parentTableWidget->horizontalHeader();
   headerView->setContextMenuPolicy(Qt::CustomContextMenu);
   connect(headerView, &QWidget::customContextMenuRequested, this, &MashStepTableModel::contextMenu);
   connect(&ObjectStoreTyped<MashStep>::getInstance(), &ObjectStoreTyped<MashStep>::signalObjectInserted, this, &MashStepTableModel::addMashStep);
   connect(&ObjectStoreTyped<MashStep>::getInstance(), &ObjectStoreTyped<MashStep>::signalObjectDeleted,  this, &MashStepTableModel::removeMashStep);
   return;
}

MashStepTableModel::~MashStepTableModel() = default;

void MashStepTableModel::addMashStep(int mashStepId) {
   // A new MashStep has been inserted in the DB, so we've been sent a signal.  If the MashStep doesn't exist(!?!), or
   // if we already have it in our list, or if it's not for the the Mash we're watching, or we're not watching a Mash,
   // then there is nothing for us to do.
   MashStep * mashStep = ObjectStoreWrapper::getByIdRaw<MashStep>(mashStepId);
   if (mashStep == nullptr ||
       this->mashObs == nullptr ||
       this->steps.contains(mashStep) ||
       this->mashObs->key() != mashStep->getMashId()) {
      return;
   }

   int size {this->steps.size()};

   qDebug() <<
      Q_FUNC_INFO << "Instance @" << static_cast<void *>(this) << "Adding MashStep" << mashStep->name() << "(#" <<
      mashStepId << ") to existing list of " << size << "steps for Mash #" << this->mashObs->key();

   beginInsertRows( QModelIndex(), size, size );
   this->steps.append(mashStep);
   connect(mashStep, &NamedEntity::changed, this, &MashStepTableModel::mashStepChanged);
   //reset(); // Tell everybody that the table has changed.
   endInsertRows();
   return;
}

void MashStepTableModel::removeMashStep(int mashStepId, std::shared_ptr<QObject> object) {
   this->remove(std::static_pointer_cast<MashStep>(object).get());
   return;
}

bool MashStepTableModel::remove(MashStep * mashStep) {

   int i {this->steps.indexOf(mashStep)};
   if (i >= 0) {
      qDebug() << Q_FUNC_INFO << "Removing MashStep" << mashStep->name() << "(#" << mashStep->key() << ")";
      beginRemoveRows( QModelIndex(), i, i );
      disconnect( mashStep, nullptr, this, nullptr );
      this->steps.removeAt(i);
      //reset(); // Tell everybody the table has changed.
      endRemoveRows();

      return true;
   }

   return false;
}

void MashStepTableModel::setMash(Mash * m) {
   if (this->mashObs && this->steps.size() > 0) {
      qDebug() <<
         Q_FUNC_INFO << "Removing" << this->steps.size() << "MashStep rows for old Mash #" << this->mashObs->key();
      this->beginRemoveRows(QModelIndex(), 0, this->steps.size() - 1);
      // Remove mashObs and all steps.
      disconnect( mashObs, nullptr, this, nullptr );

      for (int i = 0; i < this->steps.size(); ++i) {
         disconnect(this->steps[i], nullptr, this, nullptr);
      }
      this->steps.clear();
      this->endRemoveRows();
   }

   this->mashObs = m;
   if (this->mashObs) {
      qDebug() << Q_FUNC_INFO << "Now watching Mash #" << this->mashObs->key();

      // This has to happen outside of the if{} block to make sure the mash
      // signal is connected. Otherwise, empty mashes will never be not empty.
      connect( mashObs, &Mash::mashStepsChanged, this, &MashStepTableModel::mashChanged );

      QList<MashStep*> tmpSteps = this->mashObs->mashSteps();
      if (tmpSteps.size() > 0) {
         qDebug() << Q_FUNC_INFO << "Inserting" << tmpSteps.size() << "MashStep rows";
         this->beginInsertRows(QModelIndex(), 0, tmpSteps.size() - 1);
         this->steps = tmpSteps;
         for (int i = 0; i < this->steps.size(); ++i ) {
            connect(this->steps[i], &NamedEntity::changed, this, &MashStepTableModel::mashStepChanged);
         }
         this->endInsertRows();
     }
   }

   if (parentTableWidget) {
      parentTableWidget->resizeColumnsToContents();
      parentTableWidget->resizeRowsToContents();
   }
   return;
}

Mash * MashStepTableModel::getMash() const {
   return this->mashObs;
}


void MashStepTableModel::reorderMashStep(MashStep* step, int current) {
   // doSomething will be -1 if we are moving up and 1 if we are moving down
   // and 0 if nothing is to be done (see next comment)
   int destChild   = step->stepNumber();
   int doSomething = destChild - current - 1;

   qDebug() << Q_FUNC_INFO << "Swapping" << destChild << "with" << current << ", so doSomething=" << doSomething;

   // Moving a step up or down generates two signals, one for each row
   // impacted. If we move row B above row A:
   //    1. The first signal is to move B above A, which will result in A
   //    being below B
   //    2. The second signal is to move A below B, which we just did.
   // Therefore, the second signal mostly needs to be ignored. In those
   // circusmtances, A->stepNumber() will be the same as it's position in the
   // steps list, modulo some indexing
   if (doSomething == 0) {
      return;
   }

   // beginMoveRows is a little odd. When moving rows within the same parent,
   // destChild points one beyond where you want to insert the row. Think of
   // it as saying "insert before destChild". If we are moving something up,
   // we need to be one less than stepNumber. If we are moving down, it just
   // works.
   if (doSomething < 0) {
      destChild--;
   }

   // We assert that we are swapping valid locations on the list as, to do otherwise implies a coding error
   qDebug() <<
      Q_FUNC_INFO << "Swap" << current + doSomething << "with" << current << ", in list of " << this->steps.size();
   Q_ASSERT(current >= 0);
   Q_ASSERT(current + doSomething >= 0);
   Q_ASSERT(current < this->steps.size());
   Q_ASSERT(current + doSomething < this->steps.size());

   this->beginMoveRows(QModelIndex(), current, current, QModelIndex(), destChild);

   // doSomething is -1 if moving up and 1 if moving down. swap current with
   // current -1 when moving up, and swap current with current+1 when moving
   // down
#if QT_VERSION < QT_VERSION_CHECK(5,13,0)
   this->steps.swap(current,current+doSomething);
#else
   this->steps.swapItemsAt(current,current+doSomething);
#endif
   this->endMoveRows();
   return;
}

MashStep* MashStepTableModel::getMashStep(unsigned int i) {
   if (i < static_cast<unsigned int>(this->steps.size())) {
      return this->steps[static_cast<int>(i)];
   }

   return nullptr;
}

void MashStepTableModel::mashChanged() {
   // Remove and re-add all steps.
   this->setMash(mashObs);
   return;
}

void MashStepTableModel::mashStepChanged(QMetaProperty prop, QVariant val) {
   qDebug() << Q_FUNC_INFO;

   MashStep* stepSender = qobject_cast<MashStep*>(sender());
   if (stepSender) {
      if (stepSender->getMashId() != this->mashObs->key()) {
         // It really shouldn't happen that we get a notification for a MashStep that's not in the Mash we're watching,
         // but, if we do, then stop trying to process the update.
         qCritical() <<
            Q_FUNC_INFO << "Instance @" << static_cast<void *>(this) << "received update for MashStep" <<
            stepSender->key() << "of Mash" << stepSender->getMashId() << "but we are watching Mash" <<
            this->mashObs->key();
         return;
      }

      int ii = this->steps.indexOf(stepSender);
      if (ii >= 0) {
         if (prop.name() == PropertyNames::MashStep::stepNumber) {
            this->reorderMashStep(stepSender, ii);
         }

         emit dataChanged( QAbstractItemModel::createIndex(ii, 0),
                           QAbstractItemModel::createIndex(ii, MASHSTEPNUMCOLS-1));
      }

   }

    if (this->parentTableWidget) {
      this->parentTableWidget->resizeColumnsToContents();
      this->parentTableWidget->resizeRowsToContents();
   }
   return;
}

int MashStepTableModel::rowCount(const QModelIndex& /*parent*/) const {
   return steps.size();
}

QVariant MashStepTableModel::data(QModelIndex const & index, int role) const {
   if (this->mashObs == nullptr) {
      return QVariant();
   }

   // Make sure we only respond to the DisplayRole role.
   if (role != Qt::DisplayRole) {
      return QVariant();
   }

   // Ensure the row is ok.
   if (index.row() >= static_cast<int>(this->steps.size())) {
      qWarning() << Q_FUNC_INFO << "Bad model index. row = " << index.row();
      return QVariant();
   }

   MashStep* row = this->steps[index.row()];

   int const column = index.column();
   switch (column) {
      case MASHSTEPNAMECOL:
         return QVariant(row->name());
      case MASHSTEPTYPECOL:
         return QVariant(row->typeStringTr());
      case MASHSTEPAMOUNTCOL:
         return QVariant(
            Measurement::displayAmount(
               Measurement::Amount{
                  row->type() == MashStep::Decoction ? row->decoctionAmount_l() : row->infuseAmount_l(),
                  Measurement::Units::liters
               },
               3,
               this->getForcedSystemOfMeasurementForColumn(column),
               this->getForcedRelativeScaleForColumn(column)
            )
         );
      case MASHSTEPTEMPCOL:
         if (row->type() == MashStep::Decoction) {
            return QVariant("---");
         }
         return QVariant(
            Measurement::displayAmount(Measurement::Amount{row->infuseTemp_c(), Measurement::Units::celsius},
                                       3,
                                       this->getForcedSystemOfMeasurementForColumn(column),
                                       std::nullopt)
         );
      case MASHSTEPTARGETTEMPCOL:
         return QVariant(
            Measurement::displayAmount(Measurement::Amount{row->stepTemp_c(), Measurement::Units::celsius},
                                       3,
                                       this->getForcedSystemOfMeasurementForColumn(column),
                                       std::nullopt)
         );
      case MASHSTEPTIMECOL:
         return QVariant(
            Measurement::displayAmount(Measurement::Amount{row->stepTime_min(), Measurement::Units::minutes},
                                       3,
                                       std::nullopt,
                                       this->getForcedRelativeScaleForColumn(column))
         );
      default :
         qWarning() << Q_FUNC_INFO << "Bad column: " << column;
         return QVariant();
   }
}

QVariant MashStepTableModel::headerData( int section, Qt::Orientation orientation, int role ) const {
   if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
      return this->getColumName(section);
   }
   return QVariant();
}

Qt::ItemFlags MashStepTableModel::flags(const QModelIndex& index ) const
{
   int col = index.column();
   switch(col)
   {
      case MASHSTEPNAMECOL:
         return Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled;
      default:
         return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled |
            Qt::ItemIsEnabled;
   }
}

bool MashStepTableModel::setData(QModelIndex const & index, QVariant const & value, int role) {

   if (this->mashObs == nullptr) {
      return false;
   }

   if (index.row() >= static_cast<int>(steps.size()) || role != Qt::EditRole ) {
      return false;
   }

   MashStep * row = this->steps[index.row()];

   int const column = index.column();
   switch (column) {
      case MASHSTEPNAMECOL:
         if (value.canConvert(QVariant::String)) {
            MainWindow::instance().doOrRedoUpdate(*row,
                                                     PropertyNames::NamedEntity::name,
                                                     value.toString(),
                                                     tr("Change Mash Step Name"));
            return true;
         }
         return false;

      case MASHSTEPTYPECOL:
         if (value.canConvert(QVariant::Int)) {
            MainWindow::instance().doOrRedoUpdate(*row,
                                                  PropertyNames::MashStep::type,
                                                  static_cast<MashStep::Type>(value.toInt()),
                                                  tr("Change Mash Step Type"));
            return true;
         }
         return false;

      case MASHSTEPAMOUNTCOL:
         if (value.canConvert(QVariant::String)) {
            if( row->type() == MashStep::Decoction ) {
               MainWindow::instance().doOrRedoUpdate(
                  *row,
                  PropertyNames::MashStep::decoctionAmount_l,
                  Measurement::qStringToSI(value.toString(),
                                           Measurement::PhysicalQuantity::Volume,
                                           this->getForcedSystemOfMeasurementForColumn(column),
                                           this->getForcedRelativeScaleForColumn(column)).quantity,
                  tr("Change Mash Step Decoction Amount")
               );
            } else {
               MainWindow::instance().doOrRedoUpdate(
                  *row,
                  PropertyNames::MashStep::infuseAmount_l,
                  Measurement::qStringToSI(value.toString(),
                                           Measurement::PhysicalQuantity::Volume,
                                           this->getForcedSystemOfMeasurementForColumn(column),
                                           this->getForcedRelativeScaleForColumn(column)).quantity,
                  tr("Change Mash Step Infuse Amount")
               );
            }
            return true;
         }
         return false;

      case MASHSTEPTEMPCOL:
         if (value.canConvert(QVariant::String) && row->type() != MashStep::Decoction) {
            MainWindow::instance().doOrRedoUpdate(
               *row,
               PropertyNames::MashStep::infuseTemp_c,
               Measurement::qStringToSI(value.toString(),
                                        Measurement::PhysicalQuantity::Temperature,
                                        this->getForcedSystemOfMeasurementForColumn(column),
                                        this->getForcedRelativeScaleForColumn(column)).quantity,
               tr("Change Mash Step Infuse Temp")
            );
            return true;
         }
         return false;

      case MASHSTEPTARGETTEMPCOL:
         if (value.canConvert(QVariant::String)) {
            // Two changes, but we want to group together as one undo/redo step
            //
            // We don't assign the pointer (returned by new) to second SimpleUndoableUpdate we create because, in the
            // constructor, it gets linked to the first one, which then "owns" it.
            auto targetTempUpdate = new SimpleUndoableUpdate(
               *row,
               PropertyNames::MashStep::stepTemp_c,
               Measurement::qStringToSI(value.toString(),
                                        Measurement::PhysicalQuantity::Temperature,
                                        this->getForcedSystemOfMeasurementForColumn(column),
                                        this->getForcedRelativeScaleForColumn(column)).quantity,
               tr("Change Mash Step Temp")
            );
            new SimpleUndoableUpdate(*row,
                                     PropertyNames::MashStep::endTemp_c,
                                     Measurement::qStringToSI(value.toString(),
                                                              Measurement::PhysicalQuantity::Temperature,
                                                              this->getForcedSystemOfMeasurementForColumn(column),
                                                              this->getForcedRelativeScaleForColumn(column)).quantity,
                                     tr("Change Mash Step End Temp"),
                                     targetTempUpdate);
            MainWindow::instance().doOrRedoUpdate(targetTempUpdate);
            return true;
         }
         return false;

      case MASHSTEPTIMECOL:
         if (value.canConvert(QVariant::String)) {
            MainWindow::instance().doOrRedoUpdate(
               *row,
               PropertyNames::MashStep::stepTime_min,
               Measurement::qStringToSI(value.toString(),
                                        Measurement::PhysicalQuantity::Time,
                                        this->getForcedSystemOfMeasurementForColumn(column),
                                        this->getForcedRelativeScaleForColumn(column)).quantity,
               tr("Change Mash Step Time")
            );
            return true;
         }
         return false;

      default:
         return false;
   }
}

void MashStepTableModel::moveStepUp(int i) {
   if (this->mashObs == nullptr || i == 0 || i >= this->steps.size()) {
      return;
   }

   this->mashObs->swapMashSteps(*this->steps[i], *this->steps[i-1]);
   return;
}

void MashStepTableModel::moveStepDown(int i) {
   if (this->mashObs == nullptr ||  i+1 >= steps.size()) {
      return;
   }

   this->mashObs->swapMashSteps(*this->steps[i], *this->steps[i+1]);
   return;
}

//==========================CLASS MashStepItemDelegate===============================

MashStepItemDelegate::MashStepItemDelegate(QObject* parent) : QItemDelegate(parent) {
   return;
}

QWidget* MashStepItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/, const QModelIndex &index) const
{
   if( index.column() == MASHSTEPTYPECOL )
   {
      QComboBox *box = new QComboBox(parent);

      foreach( QString mtype, MashStep::types )
         box->addItem(mtype);

      box->setSizeAdjustPolicy(QComboBox::AdjustToContents);

      return box;
   }
   else
      return new QLineEdit(parent);
}

void MashStepItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
   if( index.column() == MASHSTEPTYPECOL )
   {
      QComboBox* box = qobject_cast<QComboBox*>(editor);
      QString text = index.model()->data(index, Qt::DisplayRole).toString();

      int index = box->findText(text);
      box->setCurrentIndex(index);
   }
   else
   {
      QLineEdit* line = qobject_cast<QLineEdit*>(editor);

      line->setText(index.model()->data(index, Qt::DisplayRole).toString());
   }

}

void MashStepItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
   QStringList typesTr = QStringList() << QObject::tr("Infusion") << QObject::tr("Temperature") << QObject::tr("Decoction");
   if( index.column() == MASHSTEPTYPECOL )
   {
      QComboBox* box = qobject_cast<QComboBox*>(editor);
      int ndx = box->currentIndex();
      int curr  = typesTr.indexOf(model->data(index,Qt::DisplayRole).toString());

      if ( ndx != curr )
         model->setData(index, ndx, Qt::EditRole);
   }
   else
   {
      QLineEdit* line = qobject_cast<QLineEdit*>(editor);

      if ( line->isModified() )
         model->setData(index, line->text(), Qt::EditRole);
   }
}

void MashStepItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex& /*index*/) const
{
   editor->setGeometry(option.rect);
}