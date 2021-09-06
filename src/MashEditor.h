/*======================================================================================================================
 * MashEditor.h is part of Brewken, and is copyright the following authors 2009-2021:
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
#ifndef MASHEDITOR_H
#define MASHEDITOR_H

class MashEditor;

#include <QDialog>
#include <QMetaProperty>
#include <QVariant>
#include "ui_mashEditor.h"

// Forward declarations.
class Recipe;
class Mash;
class Equipment;

/*!
 * \class MashEditor
 *
 * \brief View/controller dialog for editing a mash.
 *
 *        See also \c NamedMashEditor
 */
class MashEditor : public QDialog, public Ui::mashEditor {
   Q_OBJECT
public:
   MashEditor( QWidget *parent = nullptr );

public slots:
   void showEditor();
   void closeEditor();
   void saveAndClose();
   //! Get the tun mass and sp. heat from the equipment.
   void fromEquipment();
   //! Set the mash we wish to view/edit.
   void setMash(Mash* mash);
   void setRecipe(Recipe* r);

   void changed(QMetaProperty,QVariant);
private:
   void showChanges(QMetaProperty* prop = nullptr);
   void clear();
   Recipe* m_rec;
   Mash* mashObs;
   Equipment* m_equip;

};

#endif
