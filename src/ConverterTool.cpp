/*======================================================================================================================
 * ConverterTool.cpp is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Matt Young <mfsy@yahoo.com>
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
#include "ConverterTool.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QVBoxLayout>

#include "measurement/Unit.h"

ConverterTool::ConverterTool(QWidget* parent) : QDialog(parent) {
   this->doLayout();

   connect(pushButton_convert, &QAbstractButton::clicked, this, &ConverterTool::convert);
   return;
}

void ConverterTool::convert() {
   this->outputLineEdit->setText(Measurement::Unit::convertWithoutContext(inputLineEdit->text(), outputUnitsLineEdit->text()));
   return;
}

void ConverterTool::changeEvent(QEvent* event) {
   if (event->type() == QEvent::LanguageChange) {
      this->retranslateUi();
   }
   QDialog::changeEvent(event);
   return;
}

void ConverterTool::doLayout() {
   this->resize(279, 96);
   QHBoxLayout* hLayout = new QHBoxLayout(this);
      QFormLayout* formLayout = new QFormLayout();
         inputLabel = new QLabel(this);
         inputLineEdit = new QLineEdit(this);
            inputLineEdit->setMinimumSize(QSize(100, 0));
            inputLineEdit->setMaximumSize(QSize(128, 16777215));
         outputUnitsLabel = new QLabel(this);
         outputUnitsLineEdit = new QLineEdit(this);
            outputUnitsLineEdit->setMinimumSize(QSize(40, 0));
            outputUnitsLineEdit->setMaximumSize(QSize(40, 16777215));
         outputLabel = new QLabel(this);
         outputLineEdit = new QLineEdit(this);
            outputLineEdit->setMinimumSize(QSize(100, 0));
            outputLineEdit->setMaximumSize(QSize(128, 16777215));
            outputLineEdit->setReadOnly(true);
         formLayout->setWidget(0, QFormLayout::LabelRole, inputLabel);
         formLayout->setWidget(0, QFormLayout::FieldRole, inputLineEdit);
         formLayout->setWidget(1, QFormLayout::LabelRole, outputUnitsLabel);
         formLayout->setWidget(1, QFormLayout::FieldRole, outputUnitsLineEdit);
         formLayout->setWidget(2, QFormLayout::LabelRole, outputLabel);
         formLayout->setWidget(2, QFormLayout::FieldRole, outputLineEdit);
         formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
      QVBoxLayout* vLayout = new QVBoxLayout();
         QSpacerItem* verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
         pushButton_convert = new QPushButton(this);
            pushButton_convert->setAutoDefault(false);
            pushButton_convert->setDefault(true);
         QSpacerItem* verticalSpacer2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
         vLayout->addItem(verticalSpacer);
         vLayout->addWidget(pushButton_convert);
         vLayout->addItem(verticalSpacer2);
   hLayout->addLayout(formLayout);
   hLayout->addLayout(vLayout);

   this->retranslateUi();
   return;
}

void ConverterTool::retranslateUi() {
   this->setWindowTitle(tr("Converter Tool"));
   this->inputLabel->setText(tr("Input"));
   this->outputUnitsLabel->setText(tr("Output Units"));
   this->outputLabel->setText(tr("Output"));
   this->pushButton_convert->setText(tr("Convert"));
#ifndef QT_NO_TOOLTIP
   this->inputLineEdit->setToolTip(tr("Amount and units to convert"));
   this->outputUnitsLineEdit->setToolTip(tr("Unit you want to convert to"));
   this->outputLineEdit->setToolTip(tr("Output conversion"));
#endif // QT_NO_TOOLTIP
   return;
}
