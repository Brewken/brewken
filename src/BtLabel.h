/*======================================================================================================================
 * BtLabel.h is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Mark de Wever <koraq@xs4all.nl>
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
#ifndef BTLABEL_H
#define BTLABEL_H
#pragma once

#include <QAction>
#include <QHash>
#include <QLabel>
#include <QMenu>
#include <QPoint>

#include "measurement/UnitSystem.h"

/*class BtColorLabel;
class BtDensityLabel;
class BtMassLabel;
class BtTemperatureLabel;
class BtVolumeLabel;
class BtTimeLabel;
class BtMixedLabel;
class BtDateLabel;
class BtDiastaticPowerLabel;*/

/*!
 * \class BtLabel
 *
 * \brief Performs the necessary magic to select display units for any label.  Specifically, this allows the user to
 *        right-click on the label for a field and select
 *           (a) which unit system to use for that field (eg US Customary (mass), Imperial (mass) or Metric/SI (mass)
 *               for a weight field)
 *           (b) which units within that system to use for the field (eg kg, g, mg if the user has selected Metric/SI on
 *               a weight field).
 *        Moreover, the settings for each label are remembered (via PersistentSettings) for future times the program is
 *        run.
 *
 *        .:TODO:. This is currently a rather hidden feature of the program as there is no visual clue that right-
 *                 clicking on a field label will bring up a useful menu (and it is not common behaviour in other
 *                 software).  It would be good to have some visual clue(s) to the user that this feature exists.  Eg,
 *                 perhaps the label could have an annotation and/or there could be a text or mouse pointer change on
 *                 mouse-over.  Also, why shouldn't left-click also bring up the context menu.
 *
 *        A \c BtLabel (or subclass thereof) will usually have a corresponding \c BtLineEdit (or subclass thereof).
 *        These two widgets will be Qt buddies, which just means that the \c BtLineEdit accepts the input focus on
 *        behalf of the \c BtLabel when the user types the label's shortcut key combination.
 *
 *        When the \c BtLabel needs to tell the \c BtLineEdit that the \c UnitSystem etc has changed, it sends a
 *        \c changedUnitSystemOrScale signal.   (Previously this signal was called \c labelChanged.)
 */
class BtLabel : public QLabel {
   Q_OBJECT
   Q_ENUMS(LabelType)

public:
   /*!
    * \enum LabelType What kinds of units are available for labels.  Mostly these are self-explanatory and correspond to
    *       values in \c Measurement::PhysicalQuantity.  However there are the following additional special values:
    *          MIXED - for fields that can either be weight or volume (depending on some other checkbox control), as
    *                  used for quantities in Misc Editor
    *          DATE - dates are not a measure of a physical quantity, so they don't have a unit system, but you can
    *                 right-click on a BtDateLabel to select the date format for the corresponding entry field
    *                 (.:TBD:. Is this really useful?  Would you actually want to be able to have different dates in
    *                  different formats?  It seems not worth all the extra complexity it creates.)
    *          NONE - there is no context menu for this label
    *
    */
   enum LabelType {
      NONE,
      COLOR,
      DENSITY,
      MASS,
      TEMPERATURE,
      VOLUME,
      TIME,
      MIXED,
      DATE,
      DIASTATIC_POWER
   };


   /*!
    * \brief Initialize the BtLabel with the parent and do some things with the type
    *
    * \param parent - QWidget* to the parent object
    * \param lType - the type of label: none, gravity, mass or volume
    *
    * \todo Not sure if I can get the name of the widget being created.
    *       Not sure how to signal the parent to redisplay
    */
   BtLabel(QWidget* parent = 0, LabelType lType = NONE);

   virtual ~BtLabel();

public slots:
   void popContextMenu(const QPoint &point);

signals:
   void changedUnitSystemOrScale(Measurement::UnitSystem const * oldUnitSystem,
                                 Measurement::UnitSystem::RelativeScale oldScale);

// Using protected instead of private allows me to not use the friends
// declaration
protected:
   LabelType whatAmI;
   QString propertyName;
   QString _section;
   QWidget *btParent;
   QMenu* _menu;

   void initializeSection();
   void initializeProperty();
   void initializeMenu();

};

//
// These are trivial specialisations of BtLabel that make it possible to use specific types of BtLabel in .ui files.
// It's a bit of a sledgehammer way to pass in a constructor parameter but seems necessary because of limitations in Qt.
//
// AFAIK there is no way to pass constructor parameters to an object in a .ui file.  (If you want to do that, the advice
// seems to be to build the layout manually in C++ code.)
//
// Similarly, we might think to template BtLabel, but the Qt Meta-Object Compiler (moc) doesn't understand C++
// templates, so we can't do that for classes that need to use the Q_OBJECT macro (required for classes that declare
// their own signals and slots or that use other services provided by Qt's meta-object system).
//
class BtColorLabel : public BtLabel {
   Q_OBJECT
public:
   BtColorLabel(QWidget* parent = 0);
};

class BtDensityLabel : public BtLabel {
   Q_OBJECT
public:
   BtDensityLabel(QWidget* parent = 0);
};

class BtMassLabel : public BtLabel {
   Q_OBJECT
public:
   BtMassLabel(QWidget* parent = 0);
};

class BtTemperatureLabel : public BtLabel {
   Q_OBJECT
public:
   BtTemperatureLabel(QWidget* parent = 0);
};

class BtVolumeLabel : public BtLabel {
   Q_OBJECT
public:
   BtVolumeLabel(QWidget* parent = 0);
};

class BtTimeLabel : public BtLabel {
   Q_OBJECT
public:
   BtTimeLabel(QWidget* parent = 0);
};

class BtMixedLabel : public BtLabel {
   Q_OBJECT
public:
   BtMixedLabel(QWidget* parent = 0);
};

class BtDateLabel : public BtLabel {
   Q_OBJECT
public:
   BtDateLabel(QWidget* parent = 0);
};

class BtDiastaticPowerLabel : public BtLabel {
   Q_OBJECT
public:
   BtDiastaticPowerLabel(QWidget* parent = 0);
};

#endif
