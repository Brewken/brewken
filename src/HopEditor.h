/*======================================================================================================================
 * HopEditor.h is part of Brewken, and is copyright the following authors 2009-2021:
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
 =====================================================================================================================*/
#ifndef HOPEDITOR_H
#define HOPEDITOR_H
#pragma once

#include "ui_hopEditor.h"
#include <QMetaProperty>
#include <QVariant>

// Forward declarations.
class Hop;

/*!
 * \class HopEditor
 *
 * \brief View/controller class for modifying hops.
 */
class HopEditor : public QDialog, private Ui::hopEditor
{
   Q_OBJECT

public:
   HopEditor( QWidget *parent=nullptr );
   virtual ~HopEditor() {}
   //! Edit the given hop.
   void setHop( Hop* h );
   //! Create a new hop
   void newHop(QString folder);

public slots:
   //! Save the changes.
   void save();
   //! Clear the dialog and close it.
   void clearAndClose();
   void changed(QMetaProperty,QVariant);
   void newHop();

private:
   Hop* obsHop;

   /*! Updates the UI elements based on \b prop.
    *  If null, updates all UI elements.
    */
   void showChanges(QMetaProperty* prop = nullptr);
};

#endif
