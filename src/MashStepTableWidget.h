/*======================================================================================================================
 * MashStepTableWidget.h is part of Brewken, and is copyright the following authors 2009-2014:
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
#ifndef MASHSTEPTABLEWIDGET_H
#define MASHSTEPTABLEWIDGET_H
#pragma once

#include <QTableView>
#include <QWidget>
class MashStepTableModel;

/*!
 * \class MashStepTableWidget
 *
 * \brief View class that displays a table of mash steps.
 * NOTE: This class seems completely
 * irrelevant to me. We should remove it and replace it with QTableView.
 */
class MashStepTableWidget : public QTableView
{
   Q_OBJECT
   friend class MainWindow;
public:
   MashStepTableWidget(QWidget* parent=nullptr);
   MashStepTableModel* getModel();

public slots:
   void moveSelectedStepUp();
   void moveSelectedStepDown();

private:
   MashStepTableModel* model;
};

#endif   /* _MASHSTEPTABLEWIDGET_H */
