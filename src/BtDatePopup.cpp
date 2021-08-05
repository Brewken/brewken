/**
 * BtDatePopup.cpp is part of Brewken, and is copyright the following authors 2009-2014:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Mitch Lillie <mitch@mitchlillie.com>
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

#include <QtGui>
#include "BtDatePopup.h"

//! \brief set up the popup window.
BtDatePopup::BtDatePopup(QWidget* parent) : QDialog(parent, Qt::Popup)
{
   // No resizing a dialog
   setSizeGripEnabled(false);
   resize(260,230);

   widget = new QWidget(this);
   widget->setObjectName(QString("btDatePopup_widget"));
   widget->setGeometry(QRect(0,10,258,215));

   calendar = new QCalendarWidget(widget);
   calendar->setObjectName(QString("btDatePopup_calendar"));
   calendar->setNavigationBarVisible(true);
   calendar->setSelectedDate(QDate::currentDate());


   buttonbox = new QDialogButtonBox(widget);
   buttonbox->setObjectName(QString("btDatePopup_buttonbox"));
   buttonbox->setOrientation(Qt::Horizontal);
   buttonbox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

   vertical = new QVBoxLayout(widget);
   vertical->setObjectName(QString("btDatePopup_verticalbox"));
   vertical->setContentsMargins(0,0,0,0);

   vertical->addWidget(calendar);
   vertical->addWidget(buttonbox);

   connect(buttonbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
   connect(buttonbox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QDate BtDatePopup::selectedDate() const {
   return calendar->selectedDate();
}
