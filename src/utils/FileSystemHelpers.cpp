/*======================================================================================================================
 * utils/FileSystemHelpers.cpp is part of Brewken, and is copyright the following authors 2024:
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
#include "utils/FileSystemHelpers.h"

#include <codecvt>
#include <locale>

QString FileSystemHelpers::toQString(std::filesystem::path const & path) {
   //
   // On Linux and Mac, native format for file paths is UTF-8, but on Windows it is double-byte characters (wchar_t)
   // which are not even UTF-16.
   //
   // Most of the std::filesystem::path functions for converting to strings leave things in native format (ie UTF-8 on
   // Linux/Mac and wchar_t on Windows).
   //
   // std::filesystem::path::generic_u8string() guarantees to give us UTF-8 on any platform, but using the new
   // std::u8string / char8_t types which are a bit of a pain to work with.
   //
   // In newer versions of Qt, a QString can be constructed directly from a null-terminated string of const char8_t*
   //
#if QT_VERSION < QT_VERSION_CHECK(6,1,0)
#if defined(Q_OS_WIN)
   std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
   return QString::fromStdString(converter.to_bytes(path.generic_wstring().c_str()));
#else
   return QString::fromStdString(path.generic_string().c_str());
#endif
#else
   return QString::fromUtf8(path.generic_u8string().c_str());
#endif
}
