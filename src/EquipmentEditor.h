/*======================================================================================================================
 * EquipmentEditor.h is part of Brewken, and is copyright the following authors 2009-2021:
 *   • David Grundberg <individ@acc.umu.se>
 *   • Jeff Bailey <skydvr38@verizon.net>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mike Evans <mikee@saxicola.co.uk>
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
#ifndef EQUIPMENTEDITOR_H
#define EQUIPMENTEDITOR_H

#include "ui_equipmentEditor.h"

#include <QMetaProperty>
#include <QVariant>

// Forward declarations
class Equipment;
class EquipmentListModel;
class NamedEntitySortProxyModel;

/*!
 * \class EquipmentEditor
 *
 * \brief This is a dialog that edits an equipment record.
 */
class EquipmentEditor : public QDialog, private Ui::equipmentEditor {
   Q_OBJECT

public:
   //! \param singleEquipEditor true if you do not want the necessary elements for viewing all the database elements.
   EquipmentEditor( QWidget *parent=nullptr, bool singleEquipEditor=false );
   virtual ~EquipmentEditor() {}

   //! Edit the given equipment.
   void setEquipment( Equipment* e );

   void newEquipment(QString folder);

public slots:
   //! Save the changes to the equipment.
   void save();
   //! Create a new equipment record.
   void newEquipment();
   //! Delete the equipment from the database.
   void removeEquipment();
   //! Set the equipment to default values.
   void clear();
   //! Close the dialog, throwing away changes.
   void cancel();
   //! Set absorption back to default.
   void resetAbsorption();

   //! Edit the equipment currently selected in our combobox.
   void equipmentSelected();
   //! If state==Qt::Checked, set the "calculate boil volume" checkbox. Otherwise, unset.
   void updateCheckboxRecord();
   //! \brief set the default equipment, or unset the current equipment as the default
   void updateDefaultEquipment(int state);

   void changed(QMetaProperty,QVariant);

   double calcBatchSize();

protected:
   void closeEvent(QCloseEvent *event);

private:
   Equipment* obsEquip;
   EquipmentListModel* equipmentListModel;
   NamedEntitySortProxyModel* equipmentSortProxyModel;

   void showChanges();

};

#endif
