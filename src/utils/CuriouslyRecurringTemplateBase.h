/*======================================================================================================================
 * utils/CuriouslyRecurringTemplateBase.h is part of Brewken, and is copyright the following authors 2023:
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
#ifndef UTILS_CURIOUSLYRECURRINGTEMPLATEBASE_H
#define UTILS_CURIOUSLYRECURRINGTEMPLATEBASE_H
#pragma once

/**
 * \brief Trivial base class to implement the common functionality of Curiously Recurring Template Pattern (CRTP).
 *
 *        CRTP base classes can inherit from this class to avoid having to implement the standard \c derived member
 *        function.  (The inheritance needs to be public, otherwise you'll get compiler errors about inaccessible
 *        bases.)
 */
template<class Derived>
class CuriouslyRecurringTemplateBase {
protected:
   /**
    * \brief We need const and non-const versions of the function to downcast 'this' pointer to the derived class, which
    *        allows us to call non-virtual member functions in the derived class from the templated base class.
    */
   Derived       & derived()       { return static_cast<Derived      &>(*this); }
   Derived const & derived() const { return static_cast<Derived const&>(*this); }
};

#endif
