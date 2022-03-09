/*======================================================================================================================
 * HelpDialog.h is part of Brewken, and is copyright the following authors 2021:
 *   • Matt Young <mfsy@yahoo.com>
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
#ifndef HELPDIALOG_H
#define HELPDIALOG_H
#pragma once

#include <memory> // For PImpl

#include <QDialog>
#include <QEvent>

/*!
 * \class HelpDialog
 *
 * \brief Gives user info on file locations and links to Brewken website(s).
 */
class HelpDialog : public QDialog {

public:
   HelpDialog(QWidget * parent = nullptr);
   ~HelpDialog();

   virtual void changeEvent(QEvent * event);

private:
   // Private implementation details - see https://herbsutter.com/gotw/_100/
   class impl;
   std::unique_ptr<impl> pimpl;
};

#endif
