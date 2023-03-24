/*======================================================================================================================
 * BtAmountEdit.h is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Mark de Wever <koraq@xs4all.nl>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mike Evans <mikee@saxicola.co.uk>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Scott Peshak <scott@peshak.net>
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
#ifndef BTAMOUNTEDIT_H
#define BTAMOUNTEDIT_H
#pragma once

#include "BtLineEdit.h"
#include "measurement/PhysicalQuantity.h"
#include "measurement/Unit.h"
#include "measurement/UnitSystem.h"
#include "UiAmountWithUnits.h"

/**
 * \brief Extends \c BtLineEdit for any numerical field where the user has a choice of units.  Handles all the unit
 *        transformations.
 *
 *        NB: Per https://doc.qt.io/qt-5/moc.html#multiple-inheritance-requires-qobject-to-be-first, "If you are using
 *        multiple inheritance, moc [Qt's Meta-Object Compiler] assumes that the first inherited class is a subclass of
 *        QObject. Also, be sure that only the first inherited class is a QObject."  In particular, this means we must
 *        put Q_PROPERTY declarations for UiAmountWithUnits attributes here rather than in UiAmountWithUnits itself.
 */
class BtAmountEdit : public BtLineEdit, public UiAmountWithUnits {
   Q_OBJECT

   Q_PROPERTY(QString configSection             READ getConfigSection                      WRITE setConfigSection                      STORED false)
   Q_PROPERTY(QString editField                 READ getEditField                          WRITE setEditField                          STORED false)
   Q_PROPERTY(QString forcedSystemOfMeasurement READ getForcedSystemOfMeasurementViaString WRITE setForcedSystemOfMeasurementViaString STORED false)
   Q_PROPERTY(QString forcedRelativeScale       READ getForcedRelativeScaleViaString       WRITE setForcedRelativeScaleViaString       STORED false)

public:
   BtAmountEdit(QWidget * parent,
                Measurement::PhysicalQuantities const physicalQuantities,
                Measurement::Unit const * units,
                int const defaultPrecision = 3,
                QString const & maximalDisplayString = "100.000 srm");
   virtual ~BtAmountEdit();

   /**
    * \see \c UiAmountWithUnits for what this member function needs to do
    */
   virtual QString getWidgetText() const;

   /**
    * \see \c UiAmountWithUnits for what this member function needs to do
    */
   virtual void setWidgetText(QString text);

   // Use one of these when you just want to set the text
   void setText(NamedEntity* element);
   void setText(NamedEntity* element, int precision);
   void setText(double amount);
   void setText(double amount, int precision);
   void setText(QString amount);
   void setText(QString amount, int precision);
   void setText(QVariant amount);
   void setText(QVariant amount, int precision);

public slots:
   void onLineChanged();

   /**
    * \brief Received from \c BtLabel when the user has change \c UnitSystem
    *
    * This is mostly referenced in .ui files.  (NB this means that the signal connections are only checked at run-time.)
    */
   void lineChanged(PreviousScaleInfo previousScaleInfo);
};

//
// See comment in BtLineEdit.h for why we need all these trivial child classes to use in .ui files
//
class BtMassEdit                 : public BtAmountEdit { Q_OBJECT public: BtMassEdit                (QWidget* parent); };
class BtVolumeEdit               : public BtAmountEdit { Q_OBJECT public: BtVolumeEdit              (QWidget* parent); };
class BtTimeEdit                 : public BtAmountEdit { Q_OBJECT public: BtTimeEdit                (QWidget* parent); };
class BtTemperatureEdit          : public BtAmountEdit { Q_OBJECT public: BtTemperatureEdit         (QWidget* parent); };
class BtColorEdit                : public BtAmountEdit { Q_OBJECT public: BtColorEdit               (QWidget* parent); };
class BtDensityEdit              : public BtAmountEdit { Q_OBJECT public: BtDensityEdit             (QWidget* parent); };
class BtDiastaticPowerEdit       : public BtAmountEdit { Q_OBJECT public: BtDiastaticPowerEdit      (QWidget* parent); };
class BtAcidityEdit              : public BtAmountEdit { Q_OBJECT public: BtAcidityEdit             (QWidget* parent); };
class BtBitternessEdit           : public BtAmountEdit { Q_OBJECT public: BtBitternessEdit          (QWidget* parent); };
class BtCarbonationEdit          : public BtAmountEdit { Q_OBJECT public: BtCarbonationEdit         (QWidget* parent); };
class BtMassConcentrationEdit    : public BtAmountEdit { Q_OBJECT public: BtMassConcentrationEdit   (QWidget* parent); };
class BtVolumeConcentrationEdit  : public BtAmountEdit { Q_OBJECT public: BtVolumeConcentrationEdit (QWidget* parent); };
class BtViscosityEdit            : public BtAmountEdit { Q_OBJECT public: BtViscosityEdit           (QWidget* parent); };
class BtSpecificHeatCapacityEdit : public BtAmountEdit { Q_OBJECT public: BtSpecificHeatCapacityEdit(QWidget* parent); };

// So-called "mixed" objects, ie ones where we accept two different types of measurement (eg Mass and Volume) are a pain
class BtMixedMassOrVolumeEdit : public BtAmountEdit {
   Q_OBJECT
public:
   BtMixedMassOrVolumeEdit(QWidget* parent);
public slots:
   void setIsWeight(bool state);
};

#endif
