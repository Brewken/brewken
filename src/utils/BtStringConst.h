/*======================================================================================================================
 * utils/BtStringConst.h is part of Brewken, and is copyright the following authors 2021:
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
#ifndef UTILS_BTSTRINGCONST_H
#define UTILS_BTSTRINGCONST_H
#pragma once

/**
 * \brief Class for compile-time constant ASCII strings
 *
 *        This is a thin wrapper around char const * const that ensures the right thing happens when you do comparisons
 *        with ==, ie meaning you don't have to remember to use std::strcmp.  This is useful, because comparing two
 *        compile-time constant strings with == often "works" almost all the time (because two identical strings are
 *        just pointers to the same memory location) but not always (because one particular compiler will have done its
 *        optimisation differently and there are actually two different locations in memory holding identical strings).
 *
 *        In a Qt application such as ours, you might thing we could use QString const instead of char const * const.
 *        This has two disadvantages.  Firstly, we sometimes need the constant as a char const * -- eg to pass to Qt
 *        property functions such as QObject::property() and QObject::setProperty().  Although char const * into a
 *        QString is trivial, doing the reverse, ie getting getting char const * out of a QString, is a bit painful
 *        (because QString is inherently UTF-16 so you end up creating temporaries to hold char * data etc).
 *
 *        The second disadvantage of QString for string constants is that QString does clever reference counting
 *        internally (see https://doc.qt.io/qt-5/implicit-sharing.html).  In theory this is invisible to users of
 *        QString and never a problem.  In practice, you have to be careful about, say, a struct containing
 *        QString const &, as you can break the reference-counting logic and get a segfault (at least on Clang on Mac OS
 *        with Qt 5.9.5).
 */
class BtStringConst {
public:
   BtStringConst(char const * const cString = nullptr);
   //! Copy constructor OK
   BtStringConst(BtStringConst const &);
   ~BtStringConst();

   /**
    * \brief Compare two \c BtStringConst for equality using \c std::strcmp internally after checking for null pointers
    */
   bool operator==(BtStringConst const & rhs) const;

   /**
    * \brief Inverse of operator==.  (TODO: Replace both with spaceship operator once we're using C++20)
    */
   bool operator!=(BtStringConst const & rhs) const;

   /**
    * \brief Returns \c true if the contained char const * const pointer is null
    */
   bool isNull() const;

   /**
    * \brief Returns the contained char const * const pointer
    */
   char const * const operator*() const;

private:
   char const * const cString;

   //! No assignment operator
   BtStringConst & operator=(BtStringConst const &) = delete;
   //! No move constructor
   BtStringConst(BtStringConst &&) = delete;
   //! No move assignment
   BtStringConst & operator=(BtStringConst &&) = delete;
};

/**
 * \brief Generic output streaming for \c BtStringConst, including sensible output if the contained pointer is null
 */
template<class OS>
OS & operator<<(OS & outputStream, BtStringConst const & btStringConst) {
   if (btStringConst.isNull()) {
      outputStream << "[nullptr]";
   } else {
      outputStream << *btStringConst;
   }
   return outputStream;
}

/**
 * \brief Generic concatenation for \c BtStringConst, including sensible output if the contained pointer is null
 */
template<class T>
T operator+(T const & other, BtStringConst const & btStringConst) {
   if (btStringConst.isNull()) {
      return other + "[nullptr]";
   }
   return other + *btStringConst;
}

#endif
