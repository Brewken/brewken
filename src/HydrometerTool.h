/**
 * HydrometerTool.h is part of Brewken, and is copyright the following authors 2016-2021:
 *   • Jamie Daws <jdelectronics1@gmail.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Ryan Hoobler <rhoob@yahoo.com>
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
 */
#ifndef HYDROMETERTOOL_H
#define HYDROMETERTOOL_H
#pragma once

#include <QDialog>

class QEvent;
class QGroupBox;
class QPushButton;
class QWidget;


#include "BtLabel.h"
#include "BtLineEdit.h"


class HydrometerTool : public QDialog {
   Q_OBJECT
public:

   HydrometerTool(QWidget* parent = nullptr);

   //! \name Public UI Variables
   //! @{
   QPushButton* pushButton_convert;
   BtDensityLabel* label_inputSg;
   BtDensityEdit* lineEdit_inputSg;
   BtDensityLabel* label_outputSg;
   BtLineEdit* lineEdit_outputSg;


   BtTemperatureLabel *label_inputTemp;
   BtTemperatureEdit *lineEdit_inputTemp;
   BtTemperatureLabel *label_calibratedTemp;
   BtTemperatureEdit *lineEdit_calibratedTemp;
   QGroupBox *groupBox_inputSg;
   //! @}

public slots:
   void convert();

protected:
   virtual void changeEvent(QEvent* event);

private:

   void doLayout();
   void retranslateUi();
};

#endif
