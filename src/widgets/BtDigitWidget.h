/*======================================================================================================================
 * widgets/BtDigitWidget.h is part of Brewken, and is copyright the following authors 2009-2023:
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
#ifndef WIDGETS_BTDIGITWIDGET_H
#define WIDGETS_BTDIGITWIDGET_H
#pragma once

#include <memory> // For PImpl

#include <QLabel>
#include <QString>
#include <QWidget>

#include "BtFieldType.h"
#include "measurement/PhysicalQuantity.h"
#include "measurement/Unit.h"
#include "measurement/UnitSystem.h"
#include "UiAmountWithUnits.h"

/*!
 * \class BtDigitWidget
 *
 * \brief Widget that displays colored numbers, depending on if the number is ok, high, or low.  Currently only used in
 *        waterDialog.ui (ie Water Chemistry Dialog).
 *
 * \todo Make this thing directly accept signals from the model items it is supposed to watch.
 */
class BtDigitWidget : public QLabel {
   Q_OBJECT

public:
   enum ColorType{NONE, LOW, GOOD, HIGH, BLACK};

   BtDigitWidget(QWidget * parent,
                 BtFieldType fieldType);
   virtual ~BtDigitWidget();

   //! \brief Displays the given \c num with precision \c prec.
   void display(double num, int prec = 0);

   //! \brief Display a QString.
   void display(QString str);

   //! \brief Set the lower limit of the "good" range.
   void setLowLim(double num);

   //! \brief Set the upper limit of the "good" range.
   void setHighLim(double num);

   /**
    * \brief Always use a constant color. Use a constantColor of NONE to unset
    */
   void setConstantColor(ColorType c);

   //! \brief Convenience method to set high and low limits in one call
   void setLimits(double low, double high);

   //! \brief Methods to set the low, good and high messages
   void setLowMsg(QString msg);
   void setGoodMsg(QString msg);
   void setHighMsg(QString msg);

   //! \brief the array needs to be low, good, high
   void setMessages(QStringList msgs);

   void setText(QString amount, int precision = 2);
   void setText(double  amount, int precision = 2);

   /**
    * \brief Use this when you want to get the text as a number (and ignore any units or other trailling letters or
    *        symbols)
    */
   template<typename T> T getValueAs() const;

protected:
   int getPrecision() const;

   BtFieldType fieldType;

private:
   // Private implementation details - see https://herbsutter.com/gotw/_100/
   class impl;
   std::unique_ptr<impl> pimpl;
};

//
// See comment in BtLineEdit.h for why we need these trivial child classes to use in .ui files
//
class BtGenericDigit : public BtDigitWidget { Q_OBJECT public: BtGenericDigit(QWidget * parent); };

#endif
