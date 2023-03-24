/*======================================================================================================================
 * widgets/SmartLineEdit.cpp is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Mark de Wever <koraq@xs4all.nl>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mike Evans <mikee@saxicola.co.uk>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Théophane Martin <theophane.m@gmail.com>
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
#include "widgets/SmartLineEdit.h"

// This private implementation class holds all private non-virtual members of SmartLineEdit
class SmartLineEdit::impl {
public:
   impl(SmartLineEdit & self) :
      self{self},
      initialised{false} {
      return;
   }

   ~impl() = default;

   SmartLineEdit & self;
   bool initialised;
};

SmartLineEdit::SmartLineEdit(QWidget * parent) : QLineEdit(parent), pimpl{std::make_unique<impl>(*this)} {
   return;
}

SmartLineEdit::~SmartLineEdit() = default;

void SmartLineEdit::configure(BtFieldType const fieldType,
                              int const defaultPrecision,
                              QString const & maximalDisplayString) {
   // TODO Write this!
   return;
}
