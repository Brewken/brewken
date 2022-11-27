#!/bin/bash
#-----------------------------------------------------------------------------------------------------------------------
# packaging/qtIfw/doPackaging.sh is part of Brewken, and is copyright the following authors 2022:
#   • Matt Young <mfsy@yahoo.com>
#
# Brewken is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# Brewken is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with this program.  If not, see
# <http://www.gnu.org/licenses/>.
#-----------------------------------------------------------------------------------------------------------------------

#-----------------------------------------------------------------------------------------------------------------------
# NB: This script is intended to be invoked from the Meson build (see ../meson.build).  Meson will automatically set the
# following environment variables for us:
#    MESON_SOURCE_ROOT
#    MESON_BUILD_ROOT
# Additionally, we use meson.build to set a whole bunch of CONFIG_ environment variables
#
# This whole bash script _should_ run fine on Windows (in the MSYS2 environment), Mac and Linux...
#-----------------------------------------------------------------------------------------------------------------------
echo "Packaging ${CONFIG_APPLICATION_NAME_UC} from source root ${MESON_SOURCE_ROOT} to build root ${MESON_BUILD_ROOT}"

#
# Create the subdirectories in the build directory - see
# https://doc.qt.io/qtinstallerframework/ifw-component-description.html for the required structure
#
qtIfwRoot="${MESON_BUILD_ROOT}/qtIfw"
mkdir -p "${qtIfwRoot}/config"
packageRoot="${qtIfwRoot}/packages/com.${CONFIG_APPLICATION_NAME_LC}.root"
mkdir -p "${packageRoot}/data"
mkdir -p "${packageRoot}/meta"

# Copy the installer and package configuration/information files
cp -f "${MESON_BUILD_ROOT}/config.xml" "${qtIfwRoot}/config/."
cp -f "${MESON_BUILD_ROOT}/package.xml" "${packageRoot}/meta/."

#
# Find the binarycreator (Mac and Linux) / binarycreator.exe (Windows) file
#
# In order of preference we want:
#   - Whatever is already first in the PATH
#   - If nothing is in the path, whatever the most recent version of Qt IFW is installed under the user's home directory
# But if we can't find anything we want to capture that error and bail out
#
# TODO: We probably need a variable to handle appending ".exe" on Windows
#
qtBinaryCreator=$(which binarycreator)
if [[ "$qtBinaryCreator" == "" ]]
then
   qtBinaryCreator=$(ls -1r ~/Qt/QtIFW-*/bin/binarycreator | head -1)
fi
if [[ "$qtBinaryCreator" == "" ]]
then
  echo "Could not locate Qt IFW binarycreator"
  exit 1
fi

echo "Running ${qtBinaryCreator}"
${qtBinaryCreator} -c "${qtIfwRoot}/config/config.xml" -p "${qtIfwRoot}/packages/" "${CONFIG_APPLICATION_NAME_UC}"
