/*======================================================================================================================
 * BtDigitWidget.h is part of Brewken, and is copyright the following authors 2009-2021:
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
#ifndef BTDIGITWIDGET_H
#define BTDIGITWIDGET_H
#pragma once

#include <memory> // For PImpl

#include <QLabel>
#include <QString>
#include <QWidget>

#include "measurement/PhysicalQuantity.h"
#include "measurement/Unit.h"
#include "measurement/UnitSystem.h"

/*!
 * \class BtDigitWidget
 *
 * \brief Widget that displays colored numbers, depending on if the number is ok, high, or low.
 * \todo Make this thing directly accept signals from the model items it is supposed to watch.
 *
 * .:TBD:. This seems to share a lot with \c BtLineEdit. Could we pull out the common bits?
 */
class BtDigitWidget : public QLabel {
   Q_OBJECT
   Q_PROPERTY( int     type              READ type              WRITE setType              STORED false)
   Q_PROPERTY( QString configSection     READ configSection     WRITE setConfigSection     STORED false)
   Q_PROPERTY( QString editField         READ editField         WRITE setEditField         STORED false)
///   Q_PROPERTY( QString forcedUnit        READ forcedUnit        WRITE setForcedUnit        STORED false)
///   Q_PROPERTY( QString forcedScale       READ forcedScale       WRITE setForcedScale       STORED false)

public:
   enum ColorType{ NONE, LOW, GOOD, HIGH, BLACK };

   BtDigitWidget(QWidget* parent = 0,
                 Measurement::PhysicalQuantity type = Measurement::None,
                 Measurement::Unit const * units = nullptr);
   virtual ~BtDigitWidget();

   //! \brief Displays the given \c num with precision \c prec.
   void display( double num, int prec = 0 );

   //! \brief Display a QString.
   void display(QString str);

   //! \brief Set the lower limit of the "good" range.
   void setLowLim(double num);

   //! \brief Set the upper limit of the "good" range.
   void setHighLim(double num);

   //! \brief Always use a constant color. Use a constantColor of NONE to
   //!  unset
   void setConstantColor( ColorType c );

   //! \brief Convience method to set high and low limits in one call
   void setLimits(double low, double high);

   //! \brief Methods to set the low, good and high messages
   void setLowMsg(QString msg);
   void setGoodMsg(QString msg);
   void setHighMsg(QString msg);

   //! \brief the array needs to be low, good, high
   void setMessages(QStringList msgs);

   void setText(double amount, int precision = 2);
   void setText(QString amount, int precision = 2);

   // By defining the setters/getters, we can remove the need for
   // initializeProperties.
   QString editField() const;
   void setEditField( QString editField );

   QString configSection();
   void setConfigSection( QString configSection );

   int type() const;
   void setType(int type);

   Measurement::UnitSystem const * getForcedUnitSystem() const;
   void setForcedUnitSystem(Measurement::UnitSystem const * forcedUnitSystem);

   Measurement::UnitSystem::RelativeScale getForcedScale() const;
   void setForcedScale(Measurement::UnitSystem::RelativeScale forcedScale);

   QString displayAmount(double amount, int precision = 2);

/* LOOKS LIKE NOT USED
 * public slots:
   void displayChanged(Measurement::Unit::unitDisplay oldUnit, Measurement::UnitSystem::RelativeScale oldScale);*/

private:
   // Private implementation details - see https://herbsutter.com/gotw/_100/
   class impl;
   std::unique_ptr<impl> pimpl;
};

// TODO GET RID OF THESE CHILD CLASSES

class BtMassDigit: public BtDigitWidget {
   Q_OBJECT

public:
   BtMassDigit(QWidget* parent);
};

class BtGenericDigit: public BtDigitWidget {
   Q_OBJECT

public:
   BtGenericDigit(QWidget* parent);
};

#endif
