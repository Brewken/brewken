/*======================================================================================================================
 * utils/FilePermissionsToStream.h is part of Brewken, and is copyright the following authors 2024:
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
#ifndef UTILS_FILEPERMISSIONSTOSTREAM_H
#define UTILS_FILEPERMISSIONSTOSTREAM_H
#pragma once

#include <array>
#include <filesystem>

#include <QString>
#include <QTextStream>

namespace {
   struct permissionAndFlag {
      std::filesystem::perms permission;
      char                   flag;
   };
   constexpr std::array allPermsAndFlags{
      permissionAndFlag{std::filesystem::perms::owner_read  , 'r'},
      permissionAndFlag{std::filesystem::perms::owner_write , 'w'},
      permissionAndFlag{std::filesystem::perms::owner_exec  , 'x'},
      permissionAndFlag{std::filesystem::perms::group_read  , 'r'},
      permissionAndFlag{std::filesystem::perms::group_write , 'w'},
      permissionAndFlag{std::filesystem::perms::group_exec  , 'x'},
      permissionAndFlag{std::filesystem::perms::others_read , 'r'},
      permissionAndFlag{std::filesystem::perms::others_write, 'w'},
      permissionAndFlag{std::filesystem::perms::others_exec , 'x'},
      //
      // We just add extra output for these "special" bits, rather than trying to mimic the output of, say, ls -l
      //
      permissionAndFlag{std::filesystem::perms::set_uid     , 'u'},
      permissionAndFlag{std::filesystem::perms::set_gid     , 'g'},
      permissionAndFlag{std::filesystem::perms::sticky_bit  , 's'},
   };
}


/**
 * \brief Convenience function to allow output of \c std::filesystem::perms to \c QDebug or \c QTextStream stream
 *
 *        (For some reason, \c QDebug does not inherit from \c QTextStream so we template the stream class.)
 */
template<class S>
S & operator<<(S & stream, std::filesystem::perms const & permissions) {

   QString output;
   QTextStream outputAsStream{&output};

   for (auto ii : allPermsAndFlags) {
      char const outputChar{
         std::filesystem::perms::none == (ii.permission & permissions) ? '-' : ii.flag
      };
      outputAsStream << outputChar;
   }

   stream << output;

   return stream;
}

#endif
