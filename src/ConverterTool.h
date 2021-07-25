/**
 * ConverterTool.h is part of Brewken, and is copyright the following authors 2009-2015:
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
 */
#ifndef CONVERTERTOOL_H
#define CONVERTERTOOL_H
#pragma once

#include <QDialog>
#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QEvent>

/*!
 * \brief Dialog to convert units.
 *
 */
class ConverterTool : public QDialog
{
   Q_OBJECT
public:

   ConverterTool(QWidget* parent=0);

   //! \name Public UI Variables
   //! @{
   QPushButton* pushButton_convert;
   QLabel* inputLabel;
   QLineEdit* inputLineEdit;
   QLabel* outputLabel;
   QLineEdit* outputLineEdit;
   QLabel* outputUnitsLabel;
   QLineEdit* outputUnitsLineEdit;
   //! @}

public slots:
   void convert();

protected:

   virtual void changeEvent(QEvent* event)
   {
      if(event->type() == QEvent::LanguageChange)
         retranslateUi();
      QDialog::changeEvent(event);
   }

private:

   void doLayout();
   void retranslateUi();
};

#endif
