/*======================================================================================================================
 * widgets/BtAmountDigitWidget.h is part of Brewken, and is copyright the following authors 2009-2023:
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
#ifndef WIDGETS_BTAMOUNTDIGITWIDGET_H
#define WIDGETS_BTAMOUNTDIGITWIDGET_H
#pragma once

#include "widgets/BtDigitWidget.h"

/**
 * \brief Extends \c BtDigitWidget to show units
 *
 *        NB: Per https://doc.qt.io/qt-5/moc.html#multiple-inheritance-requires-qobject-to-be-first, "If you are using
 *        multiple inheritance, moc [Qt's Meta-Object Compiler] assumes that the first inherited class is a subclass of
 *        QObject. Also, be sure that only the first inherited class is a QObject."  In particular, this means we must
 *        put Q_PROPERTY declarations for UiAmountWithUnits attributes here rather than in UiAmountWithUnits itself.
 */
class BtAmountDigitWidget : public BtDigitWidget, public UiAmountWithUnits {
   Q_OBJECT
public:
///   Q_PROPERTY(int     type                      READ type                                  WRITE setType                               STORED false)
   Q_PROPERTY(QString configSection             READ getConfigSection                      WRITE setConfigSection                      STORED false)
   Q_PROPERTY(QString editField                 READ getEditField                          WRITE setEditField                          STORED false)
   Q_PROPERTY(QString forcedSystemOfMeasurement READ getForcedSystemOfMeasurementViaString WRITE setForcedSystemOfMeasurementViaString STORED false)
   Q_PROPERTY(QString forcedRelativeScale       READ getForcedRelativeScaleViaString       WRITE setForcedRelativeScaleViaString       STORED false)

   BtAmountDigitWidget(QWidget * parent,
                       Measurement::PhysicalQuantities const physicalQuantities);
   virtual ~BtAmountDigitWidget();

public slots:
   /**
    * \brief Received from \c BtLabel when the user has change \c UnitSystem
    *
    * This is mostly referenced in .ui files.  (NB this means that the signal connections are only checked at run-time.)
    */
   void displayChanged(PreviousScaleInfo previousScaleInfo);

};

//
// See comment in BtLineEdit.h for why we need these trivial child classes to use in .ui files
//
class BtMassDigit :    public BtAmountDigitWidget { Q_OBJECT public: BtMassDigit(QWidget * parent); };

#endif
