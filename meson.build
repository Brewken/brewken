#-----------------------------------------------------------------------------------------------------------------------
# meson.build is part of Brewken, and is copyright the following authors 2022:
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
#
# ⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐
# ⭐⭐⭐ THIS IS EXPERIMENTAL - CARRY ON USING CMAKE TO BUILD THE PRODUCT UNLESS AND UNTIL WORK ON MESON BUILD COMES TO
# ⭐⭐⭐ FRUITION.
# ⭐⭐⭐
# ⭐⭐⭐ AS OF 2022-11-23 COMPILATION, LOCAL INSTALL AND UNIT TESTS SEEM MOSTLY WORKING ON LINUX, BUT NOT EXTENSIVELY TESTED.  WE
# ⭐⭐⭐ HAVE NOT YET DONE ANYTHING ON PACKAGING OR MUCH ON WINDOWS/MAC BUILDS
# ⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐
#
# STEP 1: Ensure Meson etc is installed:
# --------------------------------------
# On recent (ie >= 22.04) Ubuntu and other Debian-based versions of Linux:
#    sudo apt install python3 ninja meson cmake
#
# On older Ubuntu, you instead need:
#    sudo apt install python3 ninja meson cmake
#    sudo apt remove meson
#    sudo pip3 install meson
#
# This is because we need Meson 0.56 or newer, and Ubuntu 20.04 packages only provide 0.53.2.  Using the pip3 install
# gets a newer version BUT, as explained at https://mesonbuild.com/Getting-meson.html#installing-meson-with-pip it is
# not an ideal way to do the install on Linux.  We have to do the pip install as root (which is normally not
# recommended), otherwise running `meson install` (or even `sudo meson install`) will barf on Linux (because we need to
# be able to install files into system directories).  So, where a sufficiently recent version of Meson is available in
# the distro packages (eg `sudo apt install meson` on Ubuntu etc) it is much better to install this.
#
# Also, although it might seem odd that we need CMake installed to do a Meson build, this is because Meson uses CMake
# as a library locator tool.
#
#
# STEP 2: Set up the Meson build:
# -------------------------------
# All you have to do is run:
#    ./bt setup
#
# Amongst other things, this creates the 'mbuild' directory tree for the Meson build (so no clashes with a CMake build
# in the 'build' directory tree).
#
#
# STEP 3: Compile, test, install:
# -------------------------------
# Everything else is done from the 'mbuild' directory, so start with:
#    cd mbuild
#
# To compile:
#    meson compile
# Alternatively, to get more detailed output:
#    meson compile --verbose
#
# To run unit tests:
#    meson test
#
# Then to install:
#    meson install
# Or, on Linux, you can do:
#    sudo meson install
# which avoids the pop-up window
#
# To build source packages (in the meson-dist subdirectory of the build directory):
#    meson dist
# This will build a .tar.xz file and create a corresponding .tar.xz.sha256sum file for it.
#
# To build binary packages:
#    ../bt package             ⭐⭐⭐ This is the bit that is not yet working ⭐⭐⭐
#
#
# Finally, note that if you want to add new build targets or change the names of existing targets, you have to run the
# following command from the same directory as this file:
#   meson --reconfigure mbuild
# See https://stackoverflow.com/questions/63329923/how-to-make-meson-scan-for-new-target for more on this.
# Alternatively, you can run 'bt setup' again.
#
#-----------------------------------------------------------------------------------------------------------------------


#=======================================================================================================================
#================================================== Project Settings ===================================================
#=======================================================================================================================
#
# We'll get an error if 'project' isn't the first call in this file
#
# NB: Per https://mesonbuild.com/Reference-manual_functions.html#project the default_options settings "are only used
#     when running Meson for the first time"!  So if you change any of the default_options settings you *MUST* delete
#     the entire build directory and run
#        meson setup <build dir name>
#     again to recreate the build directory and all its contained config.  Eg, if you are in the mbuild directory, you
#     need to run:
#        cd ..
#        rm -r mbuild
#        meson setup mbuild
#        cd mbuild
#        meson compile
#     Otherwise, as explained at
#     https://mesonbuild.com/FAQ.html#why-are-changes-to-default-project-options-ignored, your changes WILL HAVE NO
#     EFFECT.  TLDR this is because you can change all the settings via the command line and it would be hard to keep
#     track of where a setting had last been modified, so the default_options are only ever read once.
#        See also https://github.com/mesonbuild/meson/issues/2193 for further discussion about this.
#
# Default options (mostly ultimately controlling compiler settings):
#
#   - cpp_std         We need C++17 or later for nested namespaces (and C++11 or later for lambdas)
#
#   - warning_level   3 is the highest level ie warns about the most things.  For gcc it translates to
#                        -Wall -Winvalid-pch -Wnon-virtual-dtor -Wextra -Wpedantic
#
#   - prefer_static   We want to try static linking before shared linking because it makes packaging a lot easier on
#                     Windows and Mac.   NB: This requires meson 0.63.0 or newer.  Current version of meson in Ubuntu
#                     22.04 repositories is 0.61.2.  For the moment, we do static setting on a library-by-library basis
#                     (by setting 'static : true' on all the dependency() calls.
#
#   - buildtype   For the moment at least, I'm not making a distinction between debug and release builds.  Unless we
#                 find some compelling performance etc reason to do otherwise, my instinct is to have as much diagnostic
#                 information in the build in "release" as we would in "development/debug", on the grounds that it can
#                 only help if an end user hits a core-dumping bug.
#                    Meson encourages you to use either the buildtype option or the debug and optimization options
#                 rather than setting compiler options directly.  However, this does not give us as much control as we
#                 would like over compiler flags.  Eg switching 'debug' to 'true' turns on the '-g' flag (equivalent to
#                 '-g2') on GCC, but there isn't a way via the meson  options to set '-g3' for GCC.  So, we set
#                 'buildtype=plain' and manage compiler flags directly.
#
# NOTE that we need Meson version 0.59.0 or later to use qt.compile_resources, qt.compile_ui and qt.compile_moc
#
project('brewken', 'cpp',
        version: '0.1.0',
        license: 'GPL-3.0-or-later',
        meson_version: '>=0.59.0',
        default_options : ['cpp_std=c++17',
                           'warning_level=3',
#                           'prefer_static=true',
                           'buildtype=plain'])

#
# By design, Meson uses a sub-set of Python -- see
# https://mesonbuild.com/FAQ.html#why-is-meson-not-just-a-python-module-so-i-could-code-my-build-setup-in-python and
# links therefrom for the rationale for this.
#
# Besides some variations in syntax, this also means that you sometimes have to do things in a slightly more cumbersome
# way than you would in a full-fat Python script.  Eg here, in regular Python, we would write:
#    capitalisedProjectName = projectName.capitalize()
# But meson.project_name() returns a Meson String, not a Python String, so there's a bit more work to do to get the same
# result.
#
# Note that we need Meson 0.56.0 or newer to use Meson's 'substring' call.
#
projectName = meson.project_name().to_lower()
capitalisedProjectName = projectName.substring(0, 1).to_upper() + projectName.substring(1)

message('⭐ Building', projectName, 'version', meson.project_version(), 'for', host_machine.system(),
        'on', build_machine.system(), 'in', meson.project_source_root(), '⭐')

mainExecutableName = projectName
testRunnerName = mainExecutableName + '_tests'

#=======================================================================================================================
#==================================================== Meson modules ====================================================
#=======================================================================================================================
# Import the Qt tools.  See https://mesonbuild.com/Qt5-module.html
qt = import('qt5')
# File System module - see https://mesonbuild.com/Fs-module.html
fs = import('fs')

#=======================================================================================================================
#==================================================== Build options ====================================================
#=======================================================================================================================
prefix = get_option('prefix')

#=======================================================================================================================
#============================================== Frameworks and Libraries ===============================================
#=======================================================================================================================
# It would be neat to automate the _installation_ of libraries and frameworks on which we depend.  However, I have yet
# to find a universal pain-free solution.
#
# I looked at using ConanCenter (https://conan.io/center/) as a source of libraries, so that we could automate
# installing them, but it does not have all the ones we need.  Eg it has Boost, Qt, Xerces-C and Valijson, but not
# Xalan-C.  (Someone else has already requested Xalan-C, see https://github.com/conan-io/conan-center-index/issues/5546,
# but that request has been open a long time, so its fulfilment doesn't seem imminent.)
#
# Meson has its own dependency management system, Wrap, but the list of pre-provided projects at
# https://mesonbuild.com/Wrapdb-projects.html does not include Qt, Boost, Xerces, Xalan or Valijson.
#
# More hopeful is vcpkg (https://vcpkg.io/en/index.html) as it does have both Xerces-C and Xalan-C, along with Boost, Qt
# and Valijson.  There is an example here https://github.com/Neumann-A/meson-vcpkg of how to use vcpkg from Meson.
# However, it's pretty slow because it builds from source everything it installs (including tools it depends on such as
# CMake).
#
# So, for the moment, we don't try to automate installation of libraries here.

#========================================================= Qt ==========================================================
# We need not just the "core" bit of Qt but various "optional" elements.
#
# We try to keep the minimum Qt version we need as low as we can.
#
# Note that if you change the minimum Qt version, you need to make corresponding changes to the .github/workflows/*.yml
# files so that GitHub uses the appropriate version of Qt for the automated builds.
#
# For the moment, max version we can have here is 5.9.5, because that's what Ubuntu 18.04 topped out at

# Tell meson which Qt modules we need
qtCommonDependencies = dependency('qt5',
                                  version : '>=5.9.5',
                                  modules : ['Core',
                                              'Multimedia',
                                              'Network',
                                              'PrintSupport',
                                              'Sql',
                                              'Svg',
                                              'Widgets',
                                              'Xml'],
                                  include_type : 'system',
                                  static : true)
# The Qt Gui module is only needed for the main program.  (We don't want the tests to try to load it or it could barf
# in a GitHub action that does not have a display running.)
qtMainExeDependencies = dependency('qt5', version : '>=5.9.5', modules: ['Gui'])
# The Qt Test module is only needed for the unit tests
qtTestRunnerDependencies = dependency('qt5', version : '>=5.9.5', modules: ['Test'])

# On Windows and Mac there are platform-specific Qt tools for packaging
if host_machine.system() == 'windows'
   # Per https://doc.qt.io/qt-6/windows-deployment.html, the windeployqt executable creates all the necessary folder
   # tree "containing the Qt-related dependencies (libraries, QML imports, plugins, and translations) required to run
   # the application from that folder".
   windeployqt = find_program('windeployqt', required : true)
elif host_machine.system() == 'darwin'
   # The macdeployqt executable shipped with Qt does for Mac what windeployqt does for Windows -- see
   # https://doc.qt.io/qt-6/macos-deployment.html#the-mac-deployment-tool
   #
   # At first glance, you might thanks that, with a few name changes, we might share all the meson code for macdeployqt
   # and windeployqt.  However, as you will see below, the two programs share _only_ a top-level goal ("automate the
   # process of creating a deployable [folder / applicaiton bundle] that contains [the necessary Qt dependencies]" - ie
   # so that the end user does not have to install Qt to run our software).  They have completely different
   # implementations and command line options, so it would be unhelpful to try to treat them identically.
   macdeployqt = find_program('macdeployqt', required : true)
endif

#===================================================== Find Boost ======================================================
# Boost is a collection of separate libraries, some, but not all, of which are header-only.  We only specify the Boost
# libraries that we actually use.
#
# On Linux, there are cases where we need a more recent version of a Boost library than is readily-available in system-
# supplied packages.  I haven't found a slick way to solve this in CMake, though https://github.com/Orphis/boost-cmake
# looks promising.  (For header-only Boost libraries, you might think it would be relatively painless to pull them in
# from where they are hosted on GitHub (see https://github.com/boostorg), but this is not the case.  AFAICT you can't
# easily pull a specific release, and just pulling master doesn't guarantee that everything compiles.)  So, anyway, on
# Debian-based distros of Linux, such as Ubuntu, you need to do the following to install Boost 1.79 in place of whatever
# (if anything) is already installed:
#
#    $ sudo apt remove boost-all-dev
#    $ cd ~
#    $ mkdir boost-tmp
#    $ cd boost-tmp
#    $ wget https://boostorg.jfrog.io/artifactory/main/release/1.79.0/source/boost_1_79_0.tar.bz2
#    $ tar --bzip2 -xf boost_1_79_0.tar.bz2
#    $ cd boost_1_79_0
#    $ ./bootstrap.sh --prefix=/usr
#    $ sudo ./b2 install
#    $ cd ../..
#    $ sudo rm -rf boost-tmp
#
# (Obviously if you want to make the necessary change to install an even more recent version than Boost 1.79 then that
# should be fine.)
#
# We do the same in .github/workflows/linux-ubuntu.yml to make GitHub automated builds work.
#
# Note that this means we want to _statically_ link Boost rather than force end users to have to do all the palava above
#
#    ************************
#    *** Boost Stacktrace ***
#    ************************
#
#    We use this for diagnostics.  In certain error cases it's very helpful to be able to log the call stack.
#
#    On Windows, using MSYS2, the mingw-w64-boost packages do not include libboost_stacktrace_backtrace, but
#    https://www.boost.org/doc/libs/1_76_0/doc/html/stacktrace/configuration_and_build.html suggests it is not required
#    (because on Windows, if you have libbacktrace installed, you can set BOOST_STACKTRACE_USE_BACKTRACE in header-only
#    mode).
#
#    .:TODO:. Not sure how to get libboost_stacktrace_backtrace installed on Mac.  It doesn't seem to be findable by
#    CMake after installing Boost via Homebrew (https://brew.sh/).  For the moment, skip trying to use
#    libboost_stacktrace_backtrace on Mac
#
#    .:TODO:. So far don't have stacktraces working properly on Windows (everything shows as register_frame_ctor), so
#    that needs some more investigation.  (It could be that it's a bug in Boost, at least according to
#    https://stackoverflow.com/questions/54333608/boost-stacktrace-not-demangling-names-when-cross-compiled)
#
#    ******************
#    *** Boost JSON ***
#    ******************
#
#    Boost JSON is an (optionally) header-only library that was introduced in Boost 1.75 in December 2020.  One of the
#    features we use, JSON pointers (the equivalent of XML's XPaths) was only introduced in Boost 1.79.  As of March
#    2022, Ubunutu 20.04 LTS only has packages for Boost 1.71 from August 2019, hence the need to manually install a
#    newer Boost.
#
#    ******************
#    *** Boost.Core ***
#    ******************
#
#    Boost.Core, part of collection of the Boost C++ Libraries, is a collection of core utilities used by other Boost
#    libraries.  Boost JSON needs a more recent version than 1.71.
#
# For Boost, per https://mesonbuild.com/Dependencies.html#boost, we only need to supply module names for libraries we
# need to link against.  For the header-only Boost libraries, the 'boost' dependency suffices.
boostModules = []
if host_machine.system() == 'linux'
   boostModules += 'stacktrace_backtrace'
endif
boostDependency = dependency('boost',
                             version : '>=1.79.0',
                             modules : boostModules,
                             static : true)
message('Boost:', boostDependency.name(), 'found =', boostDependency.found(), 'version =', boostDependency.version())

#
# Extra requirements for Boost Stacktrace
#
# Per https://www.boost.org/doc/libs/1_76_0/doc/html/stacktrace/configuration_and_build.html, by default
# Boost.Stacktrace is a header-only library.  However, you get better results by linking (either statically or
# dynamically) with a helper library.  Of the various options, it seems like boost_stacktrace_backtrace gives the most
# functionality over the most platforms.  This has dependencies on:
#   - libdl on POSIX platforms -- but see note below
#   - libbacktrace
# The latter is an external library on Windows.  On POSIX plaforms it's typically already either installed on the system
# (eg see https://man7.org/linux/man-pages/man3/backtrace.3.html) or built in to the compiler.  Fortunately, CMake knows
# how to do the right thing in either case, thanks to https://cmake.org/cmake/help/latest/module/FindBacktrace.html.
#
# Just to make things extra fun, in 2021, the GNU compilers did away with libdl and incorporated its functionality into
# libc, per section 2.3 of release info at https://sourceware.org/glibc/wiki/Release/2.34.  This means, if we're using
# the GNU tool chain and libc is v2.34 or newer, then we should NOT look for libdl, as we won't find it!  To find the
# version of libc, you execute 'ldd --version', which gives multi-line output, of which the first line will be something
# such as:
#    ldd (Ubuntu GLIBC 2.35-0ubuntu3.1) 2.35
# In this case, that means libc is version 2.35
#
# If we _don't_ need libdl, we just create an "unrequired" dependency.  This saves having to repeat a bunch of logic
# later on when we get to the main build.
#
needLibdl = false
if meson.get_compiler('cpp').get_id() == 'gcc'
   lddOutput = run_command('ldd', '--version', check: true).stdout().strip()
   lddOutputLine1 = lddOutput.split('\n')[0]
   message('ldd --version gives:', lddOutputLine1)
   libcVersion = lddOutputLine1.split(' ')[-1]
   message('libc version is:', libcVersion)
   if libcVersion.version_compare('<2.34')
      needLibdl = true
   endif
endif
dlDependency = dependency('dl', required : needLibdl, static : true)

#======================================== Find the other libraries we depend on ========================================
#
# See https://mesonbuild.com/Reference-manual_returned_dep.html for what info we can pull from a dependency object
#
# For BeerXML processing we need Xerces-C++ and Xalan-C++.  Meson can find both of these automatically using CMake's
# find_package(), as long as (a) CMake is installed(!) and (b) we provide the right library names ('XercesC' per
# https://cmake.org/cmake/help/latest/module/FindXercesC.html and 'XalanC' per
# https://cmake.org/cmake/help/latest/module/FindXalanC.html)
#
#
xercesDependency = dependency('XercesC', version : '>=3.2.2', static : true)
message('Xerces Library:', xercesDependency.name(), 'found =', xercesDependency.found(),
        'version =', xercesDependency.version())
xalanDependency = dependency('XalanC', version : '>=1.11.0', static : true)
message('Xalan Library:', xalanDependency.name(), 'found =', xalanDependency.found(),
        'version =', xalanDependency.version())

#====================================================== Valijson =======================================================
# Don't need to do anything special, other than set include directories below, as it's header-only and we pull it in as
# a Git submodule.

#==================================================== Other headers ====================================================
# Other directories to search in for headers.  Meson will barf an error if any of these directories does not exist.
includeDirs = include_directories('src', 'third-party/valijson/include')

#=======================================================================================================================
#================================================== Extra Apple stuff ==================================================
#=======================================================================================================================
# Statically linking Xalan, Xerces etc requires CFStringLowercase, CFStringUppercase, etc on Mac
corefoundationDependency = dependency(
   'appleframeworks',
   modules: ['CoreFoundation'],
   required: false,
)

#=======================================================================================================================
#===================================================== Input Files =====================================================
#=======================================================================================================================
# Sub-directories of the one containing this (meson.build) file are
#    src          = C++ source code
#    ui           = QML UI layout files
#    data         = Binary files, including sounds and default database
#    translations = Translated texts
#    mac          = Mac-specific files (desktop icon)
#    win          = Windows-specific files (desktop icon)
#    packaging    = Packaging-related config
#=======================================================================================================================

#
# List of the C++ source files that are common to the app and the unit tests - ie all .cpp files _except_ main.cpp and
# test.cpp
#
# See https://mesonbuild.com/FAQ.html#why-cant-i-specify-target-files-with-a-wildcard for why it is strongly recommended
# not to use wildcard specification.  (This is common to many build systems.)
#
# The files() wrapper around the array ensures that all the files exist and means you don't have to worry as much about
# subdirectories as you might otherwise -- see https://mesonbuild.com/Reference-manual_functions.html#files
#
commonSourceFiles = files([
   'src/AboutDialog.cpp',
   'src/AlcoholTool.cpp',
   'src/Algorithms.cpp',
   'src/AncestorDialog.cpp',
   'src/Application.cpp',
   'src/BeerColorWidget.cpp',
   'src/boiltime.cpp',
   'src/BrewDayScrollWidget.cpp',
   'src/BrewDayFormatter.cpp',
   'src/BrewNoteWidget.cpp',
   'src/BtColor.cpp',
   'src/BtDatePopup.cpp',
   'src/BtDigitWidget.cpp',
   'src/BtFieldType.cpp',
   'src/BtFolder.cpp',
   'src/BtLabel.cpp',
   'src/BtLineEdit.cpp',
   'src/BtSplashScreen.cpp',
   'src/BtTabWidget.cpp',
   'src/BtTextEdit.cpp',
   'src/BtTreeFilterProxyModel.cpp',
   'src/BtTreeItem.cpp',
   'src/BtTreeModel.cpp',
   'src/BtTreeView.cpp',
   'src/ConverterTool.cpp',
   'src/CustomComboBox.cpp',
   'src/database/BtSqlQuery.cpp',
   'src/database/Database.cpp',
   'src/database/DatabaseSchemaHelper.cpp',
   'src/database/DbTransaction.cpp',
   'src/database/ObjectStore.cpp',
   'src/database/ObjectStoreTyped.cpp',
   'src/EquipmentButton.cpp',
   'src/EquipmentEditor.cpp',
   'src/EquipmentListModel.cpp',
   'src/FermentableDialog.cpp',
   'src/FermentableEditor.cpp',
   'src/FermentableSortFilterProxyModel.cpp',
   'src/HeatCalculations.cpp',
   'src/HelpDialog.cpp',
   'src/HopDialog.cpp',
   'src/HopEditor.cpp',
   'src/HopSortFilterProxyModel.cpp',
   'src/Html.cpp',
   'src/HydrometerTool.cpp',
   'src/IbuGuSlider.cpp',
   'src/ImportExport.cpp',
   'src/InstructionWidget.cpp',
   'src/InventoryFormatter.cpp',
   'src/json/BeerJson.cpp',
   'src/json/JsonCoding.cpp',
   'src/json/JsonRecord.cpp',
   'src/json/JsonRecordDefinition.cpp',
   'src/json/JsonSchema.cpp',
   'src/json/JsonUtils.cpp',
   'src/json/JsonXPath.cpp',
   'src/Localization.cpp',
   'src/Logging.cpp',
   'src/MainWindow.cpp',
   'src/MashButton.cpp',
   'src/MashComboBox.cpp',
   'src/MashDesigner.cpp',
   'src/MashEditor.cpp',
   'src/MashListModel.cpp',
   'src/MashStepEditor.cpp',
   'src/MashStepTableWidget.cpp',
   'src/MashWizard.cpp',
   'src/matrix.cpp',
   'src/measurement/Amount.cpp',
   'src/measurement/ColorMethods.cpp',
   'src/measurement/IbuMethods.cpp',
   'src/measurement/Measurement.cpp',
   'src/measurement/PhysicalQuantity.cpp',
   'src/measurement/SucroseConversion.cpp',
   'src/measurement/SystemOfMeasurement.cpp',
   'src/measurement/Unit.cpp',
   'src/measurement/UnitSystem.cpp',
   'src/MiscDialog.cpp',
   'src/MiscEditor.cpp',
   'src/MiscSortFilterProxyModel.cpp',
   'src/model/BrewNote.cpp',
   'src/model/Equipment.cpp',
   'src/model/Fermentable.cpp',
   'src/model/Hop.cpp',
   'src/model/Instruction.cpp',
   'src/model/Inventory.cpp',
   'src/model/Mash.cpp',
   'src/model/MashStep.cpp',
   'src/model/Misc.cpp',
   'src/model/NamedEntity.cpp',
   'src/model/NamedEntityWithInventory.cpp',
   'src/model/NamedParameterBundle.cpp',
   'src/model/Recipe.cpp',
   'src/model/Salt.cpp',
   'src/model/Style.cpp',
   'src/model/Water.cpp',
   'src/model/Yeast.cpp',
   'src/NamedEntitySortProxyModel.cpp',
   'src/NamedMashEditor.cpp',
   'src/OgAdjuster.cpp',
   'src/OptionDialog.cpp',
   'src/PersistentSettings.cpp',
   'src/PitchDialog.cpp',
   'src/PreInstruction.cpp',
   'src/PrimingDialog.cpp',
   'src/PrintAndPreviewDialog.cpp',
   'src/RadarChart.cpp',
   'src/RangedSlider.cpp',
   'src/RecipeExtrasWidget.cpp',
   'src/RecipeFormatter.cpp',
   'src/RefractoDialog.cpp',
   'src/ScaleRecipeTool.cpp',
   'src/SimpleUndoableUpdate.cpp',
   'src/StrikeWaterDialog.cpp',
   'src/StyleButton.cpp',
   'src/StyleEditor.cpp',
   'src/StyleListModel.cpp',
   'src/StyleRangeWidget.cpp',
   'src/StyleSortFilterProxyModel.cpp',
   'src/tableModels/BtTableModel.cpp',
   'src/tableModels/BtTableModelInventory.cpp',
   'src/tableModels/FermentableTableModel.cpp',
   'src/tableModels/HopTableModel.cpp',
   'src/tableModels/MashStepTableModel.cpp',
   'src/tableModels/MiscTableModel.cpp',
   'src/tableModels/SaltTableModel.cpp',
   'src/tableModels/WaterTableModel.cpp',
   'src/tableModels/YeastTableModel.cpp',
   'src/TimerListDialog.cpp',
   'src/TimerMainDialog.cpp',
   'src/TimerWidget.cpp',
   'src/UiAmountWithUnits.cpp',
   'src/utils/BtException.cpp',
   'src/utils/BtStringConst.cpp',
   'src/utils/BtStringStream.cpp',
   'src/utils/EnumStringMapping.cpp',
   'src/utils/ImportRecordCount.cpp',
   'src/utils/TimerUtils.cpp',
   'src/WaterButton.cpp',
   'src/WaterDialog.cpp',
   'src/WaterEditor.cpp',
   'src/WaterListModel.cpp',
   'src/WaterSortFilterProxyModel.cpp',
   'src/WaterTableWidget.cpp',
   'src/widgets/Animator.cpp',
   'src/widgets/SelectionControl.cpp',
   'src/widgets/ToggleSwitch.cpp',
   'src/widgets/UnitAndScalePopUpMenu.cpp',
   'src/xml/BeerXml.cpp',
   'src/xml/BtDomErrorHandler.cpp',
   'src/xml/XercesHelpers.cpp',
   'src/xml/XmlCoding.cpp',
   'src/xml/XmlMashRecord.cpp',
   'src/xml/XmlMashStepRecord.cpp',
   'src/xml/XmlRecipeRecord.cpp',
   'src/xml/XmlRecord.cpp',
   'src/YeastDialog.cpp',
   'src/YeastEditor.cpp',
   'src/YeastSortFilterProxyModel.cpp'
])

#
# These are the headers that need to be processed by the Qt Meta Object Compiler (MOC).  Note that this is _not_ all the
# headers in the project.  Also, note that there is a separate (trivial) list of MOC headers for the unit test runner.
#
# You can recreate the body of this list by running the following from the bash prompt in the build directory:
#    grep -rl Q_OBJECT ../src | grep -v Testing.h | sort | sed "s+^../src/+   \'src/+; s/$/\',/"
#
mocHeaders = files([
   'src/AboutDialog.h',
   'src/AlcoholTool.h',
   'src/AncestorDialog.h',
   'src/BeerColorWidget.h',
   'src/boiltime.h',
   'src/BrewDayFormatter.h',
   'src/BrewDayScrollWidget.h',
   'src/BrewNoteWidget.h',
   'src/BtDatePopup.h',
   'src/BtDigitWidget.h',
   'src/BtFolder.h',
   'src/BtLabel.h',
   'src/BtLineEdit.h',
   'src/BtSplashScreen.h',
   'src/BtTabWidget.h',
   'src/BtTextEdit.h',
   'src/BtTreeFilterProxyModel.h',
   'src/BtTreeModel.h',
   'src/BtTreeView.h',
   'src/ConverterTool.h',
   'src/CustomComboBox.h',
   'src/database/ObjectStore.h',
   'src/EquipmentButton.h',
   'src/EquipmentEditor.h',
   'src/EquipmentListModel.h',
   'src/FermentableDialog.h',
   'src/FermentableEditor.h',
   'src/FermentableSortFilterProxyModel.h',
   'src/HelpDialog.h',
   'src/HopDialog.h',
   'src/HopEditor.h',
   'src/HopSortFilterProxyModel.h',
   'src/HydrometerTool.h',
   'src/IbuGuSlider.h',
   'src/InstructionWidget.h',
   'src/MainWindow.h',
   'src/MashButton.h',
   'src/MashComboBox.h',
   'src/MashDesigner.h',
   'src/MashEditor.h',
   'src/MashListModel.h',
   'src/MashStepEditor.h',
   'src/MashStepTableWidget.h',
   'src/MashWizard.h',
   'src/MiscDialog.h',
   'src/MiscEditor.h',
   'src/MiscSortFilterProxyModel.h',
   'src/model/BrewNote.h',
   'src/model/Equipment.h',
   'src/model/Fermentable.h',
   'src/model/Hop.h',
   'src/model/Instruction.h',
   'src/model/Inventory.h',
   'src/model/Mash.h',
   'src/model/MashStep.h',
   'src/model/Misc.h',
   'src/model/NamedEntity.h',
   'src/model/NamedEntityWithInventory.h',
   'src/model/Recipe.h',
   'src/model/Salt.h',
   'src/model/Style.h',
   'src/model/Water.h',
   'src/model/Yeast.h',
   'src/NamedEntitySortProxyModel.h',
   'src/NamedMashEditor.h',
   'src/OgAdjuster.h',
   'src/OptionDialog.h',
   'src/PitchDialog.h',
   'src/PrimingDialog.h',
   'src/PrintAndPreviewDialog.h',
   'src/RangedSlider.h',
   'src/RecipeExtrasWidget.h',
   'src/RecipeFormatter.h',
   'src/RefractoDialog.h',
   'src/ScaleRecipeTool.h',
   'src/SimpleUndoableUpdate.h',
   'src/StrikeWaterDialog.h',
   'src/StyleButton.h',
   'src/StyleEditor.h',
   'src/StyleListModel.h',
   'src/StyleRangeWidget.h',
   'src/StyleSortFilterProxyModel.h',
   'src/tableModels/BtTableModel.h',
   'src/tableModels/FermentableTableModel.h',
   'src/tableModels/HopTableModel.h',
   'src/tableModels/MashStepTableModel.h',
   'src/tableModels/MiscTableModel.h',
   'src/tableModels/SaltTableModel.h',
   'src/tableModels/WaterTableModel.h',
   'src/tableModels/YeastTableModel.h',
   'src/TimerListDialog.h',
   'src/TimerMainDialog.h',
   'src/TimerWidget.h',
   'src/WaterButton.h',
   'src/WaterDialog.h',
   'src/WaterEditor.h',
   'src/WaterListModel.h',
   'src/WaterSortFilterProxyModel.h',
   'src/WaterTableWidget.h',
   'src/widgets/Animator.h',
   'src/widgets/SelectionControl.h',
   'src/widgets/ToggleSwitch.h',
   'src/YeastDialog.h',
   'src/YeastEditor.h',
   'src/YeastSortFilterProxyModel.h',
])

unitTestMocHeaders = files([
   'src/unitTests/Testing.h'
])

#
# List of UI files
#
# You can recreate the body of this list by running the following from the bash prompt in the build directory:
#    find ../ui -name '*.ui' | sort | sed "s+^../ui/+   \'ui/+; s/$/\',/"
#
uiFiles = files([
   'ui/ancestorDialog.ui',
   'ui/brewDayScrollWidget.ui',
   'ui/brewNoteWidget.ui',
   'ui/BtPrintAndPreview.ui',
   'ui/equipmentEditor.ui',
   'ui/fermentableEditor.ui',
   'ui/hopEditor.ui',
   'ui/instructionWidget.ui',
   'ui/mainWindow.ui',
   'ui/mashDesigner.ui',
   'ui/mashEditor.ui',
   'ui/mashStepEditor.ui',
   'ui/mashWizard.ui',
   'ui/miscEditor.ui',
   'ui/namedMashEditor.ui',
   'ui/ogAdjuster.ui',
   'ui/optionsDialog.ui',
   'ui/pitchDialog.ui',
   'ui/primingDialog.ui',
   'ui/recipeExtrasWidget.ui',
   'ui/refractoDialog.ui',
   'ui/strikeWaterDialog.ui',
   'ui/styleEditor.ui',
   'ui/timerDialog.ui',
   'ui/timerListDialog.ui',
   'ui/timerMainDialog.ui',
   'ui/timerWidget.ui',
   'ui/waterDialog.ui',
   'ui/waterEditor.ui',
   'ui/yeastEditor.ui'
])

#
# List of translation files to update (from translatable strings in the source code) and from which the binary .qm files
# will be generated and shipped.  Note that src/OptionDialog.cpp controls which languages are shown to the user as
# options for the UI
#
# .:TBD:. At the moment we are hitting a warning message similar to the one described at
# https://github.com/mesonbuild/meson/issues/5019.  I _think_ this is a minor Meson bug, but it might be that I've
# misunderstood how best to reference files in subdirectories.
#
translationSourceFiles = files([
   'translations/bt_ca.ts', # Catalan
   'translations/bt_cs.ts', # Czech
   'translations/bt_de.ts', # German
   'translations/bt_en.ts', # English
   'translations/bt_el.ts', # Greek
   'translations/bt_es.ts', # Spanish
   'translations/bt_et.ts', # Estonian
   'translations/bt_eu.ts', # Basque
   'translations/bt_fr.ts', # French
   'translations/bt_gl.ts', # Galician
   'translations/bt_nb.ts', # Norwegian Bokmal
   'translations/bt_it.ts', # Italian
   'translations/bt_lv.ts', # Latvian
   'translations/bt_nl.ts', # Dutch
   'translations/bt_pl.ts', # Polish
   'translations/bt_pt.ts', # Portuguese
   'translations/bt_hu.ts', # Hungarian
   'translations/bt_ru.ts', # Russian
   'translations/bt_sr.ts', # Serbian
   'translations/bt_sv.ts', # Swedish
   'translations/bt_tr.ts', # Turkish
   'translations/bt_zh.ts', # Chinese
])

# List of documentation files to be installed.  Note that ${repoDir}/COPYRIGHT is NOT included here as it needs special
# case handling below.

filesToInstall_docs = files([
  'README.md'
])

filesToInstall_data = files([
   'data/default_db.sqlite',
   'data/DefaultData.xml'
])

filesToInstall_desktop = files([
   'linux/' + projectName + '.desktop'
])

filesToInstall_icons = files([
   'images/' + projectName + '.svg'
])

filesToInstall_windowsIcon = files([
   'win/icon.rc'
])

filesToInstall_sounds = files([
   'data/sounds/45minLeft.wav',
   'data/sounds/addFuckinHops.wav',
   'data/sounds/aromaHops.wav',
   'data/sounds/beep.wav',
   'data/sounds/bitteringHops.wav',
   'data/sounds/checkBoil.wav',
   'data/sounds/checkFirstRunnings.wav',
   'data/sounds/checkGravity.wav',
   'data/sounds/checkHydrometer.wav',
   'data/sounds/checkMashTemps.wav',
   'data/sounds/checkTemp.wav',
   'data/sounds/clarifyingAgent.wav',
   'data/sounds/cleanup.wav',
   'data/sounds/closeFuckinValves.wav',
   'data/sounds/closeValves.wav',
   'data/sounds/doughIn.wav',
   'data/sounds/drinkAnotherHomebrew.wav',
   'data/sounds/drinkHomebrew.wav',
   'data/sounds/emptyMashTun.wav',
   'data/sounds/extraPropane.wav',
   'data/sounds/flameout.wav',
   'data/sounds/flavorHops.wav',
   'data/sounds/heatWater.wav',
   'data/sounds/mashHops.wav',
   'data/sounds/pitchYeast.wav',
   'data/sounds/sanitize.wav',
   'data/sounds/sparge.wav',
   'data/sounds/startBurner.wav',
   'data/sounds/startChill.wav',
   'data/sounds/stirMash.wav',
])

filesToInstall_macPropertyList = files([
   'mac/Info.plist'
])

filesToInstall_macIcons = files([
   'mac/' + capitalisedProjectName + 'Icon.icns'
])

# This has to be a string because we're going to pass it into a script.
# AFAICT Meson does not provide a way for you to extract, say, full path from a file object
filesToInstall_changeLogUncompressed = 'CHANGES.markdown'

filesToInstall_copyright = files([
   'COPYRIGHT'
])

# This is not wrapped in files() (a) because it's not yet created and (b) because, as with
# filesToInstall_changeLogUncompressed, we're going to pass it into a bash script
#
# See below for how this one gets created from filesToInstall_changeLogUncompressed
filesToInstall_changeLogCompressed = 'changelog.gz'

#=======================================================================================================================
#============================================ Installation sub-directories =============================================
#=======================================================================================================================
if host_machine.system() == 'linux'
   #============================================= Linux Install Directories ============================================
   installSubDir_data = 'share/' + projectName
   installSubDir_doc  = 'share/doc/' + projectName
   installSubDir_bin  = 'bin'
   # According to https://specifications.freedesktop.org/menu-spec/menu-spec-1.0.html#paths, .desktop files need to live
   # in one of the $XDG_DATA_DIRS/applications/.  (Note that $XDG_DATA_DIRS is a colon-separated list of directories,
   # typically defaulting to /usr/local/share/:/usr/share/. but on another system it might be
   # /usr/share/plasma:/usr/local/share:/usr/share:/var/lib/snapd/desktop:/var/lib/snapd/desktop).  When combined with
   # CMAKE_INSTALL_PREFIX, "share/applications" should end up being one of these.
   installSubDir_applications = 'share/applications'
   # It's a similar but slightly more complicated story for where to put icons.  (See
   # https://specifications.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html#directory_layout for all the
   # details.)
   installSubDir_icons        = 'share/icons'
elif host_machine.system() == 'windows'
   #============================================ Windows Install Directories ===========================================
   installSubDir_data = 'data'
   installSubDir_doc  = 'doc'
   installSubDir_bin  = 'bin'
elif host_machine.system() == 'darwin'
   #============================================== Mac Install Directories =============================================
   installSubDir_data = 'Contents/Resources'
   installSubDir_doc  = 'Contents/Resources/en.lproj'
   installSubDir_bin  = 'Contents/MacOS'
else
   error('Unrecognised target OS type:', host_machine.system())
endif

#============================================== Common Install Directories =============================================
installSubDir_translations = installSubDir_data + '/translations_qm'


#=======================================================================================================================
#=========================================== Qt Meta Object Compilation etc ============================================
#=======================================================================================================================

# Compile Qt's resources collection files (.qrc) into C++ files for compilation
generatedFromQrc = qt.compile_resources(sources : projectName + '.qrc')

# Compile Qt's ui files (.ui) into header files.
generatedFromUi = qt.compile_ui(sources : uiFiles)

# Compile Qt's moc files (.moc) into header and/or source files
generatedFromMoc = qt.compile_moc(headers : mocHeaders,
                                  dependencies : qtCommonDependencies)
generatedFromMocForUnitTests = qt.compile_moc(headers : unitTestMocHeaders,
                                              dependencies : qtCommonDependencies)

#
# We need to do two processes with Translation Source (.ts) XML files:
#   - Update them from the source code, ie to ensure they have all the tr(), QObject::tr() etc calls from the .cpp files
#     and all the translatable strings from the .ui files -- which can be done manually from the command line with
#     lupdate
#   - Generate the binary .qm files that ship with the application and are used at run time -- which can be done
#     manually from the command line with lrelease
# Calling qt.compile_translations will do only the latter, so we need to do the former directly
#

# Call lupdate to ensure the .ts files are synced with the source code.  We need:
#    lupdate meson.project_source_root()/src meson.project_source_root()/ui -ts [list of .ts files]
# This tells lupdate to recursively scan the src/ and ui/ subdirectories and update the specified ts files
# Fortunately, we can pass a list of File objects as a parameter to run_command and Meson does the right thing
#
# We make a point here of displaying the output of run_command because we want to show message emitted by lupdate about
# what it did.
message('Running lupdate on the following ts files:', run_command('ls', translationSourceFiles, check: true).stdout())
message(
   run_command('lupdate',
               meson.project_source_root() + '/src',
               meson.project_source_root() + '/ui',
               '-ts',
               translationSourceFiles,
               check: true).stdout()
)

# Now we can generate the necessary targets to build translation files with lrelease
# Setting install to true means we want to ship all the .qm files (so users can change language at run time).
translations = qt.compile_translations(ts_files         : translationSourceFiles,
                                       build_by_default : true,
                                       install          : true,
                                       install_dir      : installSubDir_translations)

#=======================================================================================================================
#=============================================== Lists of Dependencies =================================================
#=======================================================================================================================
commonDependencies = [qtCommonDependencies,
                      xercesDependency,
                      xalanDependency,
                      boostDependency,
                      dlDependency]
mainExeDependencies = commonDependencies + qtMainExeDependencies
testRunnerDependencies = commonDependencies + qtTestRunnerDependencies

#=======================================================================================================================
#================================================= Exported variables ==================================================
#=======================================================================================================================
#
# There are a number of places where we want to "export" variables from this file.  In a couple of instances we are
# generating a file (using configure_file()) and in other places we are running a shell script (with run_command() or
# run_target()).  Although it's not always the exact same set of variables that we need to export, there is, I think,
# enough overlap that it's worth defining all the exports once to avoid repeating ourselves.
#
# The file generation and script execution take different types of object for their "exported variables": a
# Configuration Cata (cfg_data) object (see https://mesonbuild.com/Reference-manual_returned_cfg_data.html) in for the
# former and an Environment (env) object (see https://mesonbuild.com/Reference-manual_returned_env.html) for the latter.
# Fortunately however, both types of object can be constructed from a Dictionary (see
# https://mesonbuild.com/Syntax.html#dictionaries), so that is what we define here.
#
exportedVariables = {
   'CONFIG_VERSION_STRING'      : meson.project_version(),
   'CONFIG_DATA_DIR'            : prefix + '/' + installSubDir_data, # This is a bit of a hack...
   'CONFIG_APPLICATION_NAME_UC' : capitalisedProjectName,
   'CONFIG_APPLICATION_NAME_LC' : projectName,
   # NB CMAKE_HOST_SYSTEM means something different than meson host_machine
   'CONFIG_BUILD_SYSTEM'        : build_machine.system(),
   'CONFIG_RUN_SYSTEM'          : host_machine.system(),
   'CONFIG_CXX_COMPILER_ID'     : meson.get_compiler('cpp').get_id(),
   # Meson doesn't directly give you a way to obtain the current date and time.  But it does allow you turn an external
   # command, so this is one way to get it - relying on the fact that MSYS2 on Windows, Linux and Mac all have date
   # available from the command line.
   'CONFIG_BUILD_TIMESTAMP'     : run_command('date', check: true).stdout().strip(),
   #
   # This block is specifically for generating the compressed changelog for building a Debian package
   #
   'CONFIG_CHANGE_LOG_UNCOMPRESSED' : join_paths(meson.current_source_dir(), filesToInstall_changeLogUncompressed),
   'CONFIG_CHANGE_LOG_COMPRESSED'   : join_paths(meson.current_build_dir(),  filesToInstall_changeLogCompressed  ),
   'CONFIG_PACKAGE_MAINTAINER'      : 'Matt Young <mfsy@yahoo.com>',

   # Info for InstallerWindowIcon which is "Filename for a custom window icon in PNG format for the Installer
   # application.
   # Used on Windows and Linux, no functionality on macOS."
   'CONFIG_INSTALLER_WINDOW_ICON'     : capitalisedProjectName + 'Logo.png',
   'CONFIG_INSTALLER_WINDOW_ICON_DIR' : 'images',

}

# We need to set TargetDir in config.xml for the Qt IFW installer
if host_machine.system() == 'linux'
   exportedVariables += {
      'CONFIG_TARGET_INSTALL_DIR' : '/'
   }
endif

# Info for InstallerApplicationIcon which is "Filename for a custom installer icon. The actual file is looked up by
# attaching a '.icns' (macOS), '.ico' (Windows) suffix. No functionality on Unix."
if host_machine.system() == 'windows'
   exportedVariables += {
      'CONFIG_INSTALLER_APPLICATION_ICON'      : capitalisedProjectName + 'Icon_96.ico',
      'CONFIG_INSTALLER_APPLICATION_ICON_DIR'  : 'win',
   }
elif host_machine.system() == 'darwin'
   exportedVariables += {
      'CONFIG_INSTALLER_APPLICATION_ICON'      : capitalisedProjectName + 'Icon.icns',
      'CONFIG_INSTALLER_APPLICATION_ICON_DIR'  : 'mac',
   }
elif host_machine.system() == 'linux'
   exportedVariables += {
      'CONFIG_INSTALLER_APPLICATION_ICON'      : '',
      'CONFIG_INSTALLER_APPLICATION_ICON_DIR'  : '',
   }
endif

#=======================================================================================================================
#===================================== Generate config.h etc from config.h.in etc ======================================
#=======================================================================================================================
# Taking src/config.h.in as input, we generate (in the build subdirectory only) config.h.  This is a way to inject build
# variables into the code.
#
# All variables written as "@VAR@" in src/config.h.in (input file) will be replaced in config.h (output file) by the
# value of VAR in the configuration_data dictionary we define here.
#
configure_file(input  : 'src/config.h.in',
               output : 'config.h',
               configuration : exportedVariables,
               install : false)
#
# Now do the same for the XML packaging files used by the "packages" target (see below).
#
# We ultimately want it in a subdirectory of the the build directory, but that is not permitted by configure_file, so,
# instead, it gets copied to the right location by packaging/doQtIFWpackaging.sh
#
configure_file(input  : 'packaging/qtIfw/config.xml.in',
               output : 'config.xml',
               configuration : exportedVariables,
               install : false)
configure_file(input  : 'packaging/qtIfw/package.xml.in',
               output : 'package.xml',
               configuration : exportedVariables,
               install : false)

#=======================================================================================================================
#=================================== Generate compressed changelog for Debian package ==================================
#=======================================================================================================================
if host_machine.system() == 'linux'
   # .:TBD:. Might be better to do this via custom_target (see https://mesonbuild.com/Custom-build-targets.html), though
   # the parameter passing is not as nice that way
   #
   # For now, if we're using QtIFW (see below), then we might not worry about it too much
   scriptResult = run_command('packaging/generateCompressedChangeLog.sh',
                              env : exportedVariables,
                              check : true)
   message('Generating compressed changlog output:', scriptResult.stdout().strip())
   if scriptResult.returncode() != 0
      error('Generating compressed changlog ERRORS:', scriptResult.stderr().strip())
   endif
endif

#=======================================================================================================================
#======================= Install files that we ship with the software (sounds, default DB, etc) ========================
#=======================================================================================================================
install_data(filesToInstall_data,   install_dir : installSubDir_data)
install_data(filesToInstall_docs,   install_dir : installSubDir_doc)
install_data(filesToInstall_sounds, install_dir : installSubDir_data + '/sounds')
if host_machine.system() == 'linux'
   # Install the icons
   # Per https://specifications.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html#install_icons, "installing a
   # svg icon in $prefix/share/icons/hicolor/scalable/apps means most desktops will have one icon that works for all
   # sizes".
   install_data(filesToInstall_icons, install_dir : installSubDir_icons + '/hicolor/scalable/apps/')

   # Install the .desktop file
   install_data(filesToInstall_desktop, install_dir : installSubDir_applications)

   # Install friendly-format change log aka release notes
   # Note that lintian does not like having a file called CHANGES.markdown in the doc directory, as it thinks it is a
   # misnamed changelog.Debian.gz (even when changelog.Debian.gz is also present!) so you get a
   # wrong-name-for-upstream-changelog warning.
   # The simplest way round this is to rename CHANGES.markdown to RelaseNotes.markdown
   install_data(filesToInstall_changeLogUncompressed, rename : 'RelaseNotes.markdown', install_dir : installSubDir_doc)

   # Debian packages need to have the copyright file in a particular place (/usr/share/doc/PACKAGE/copyright)
   # RPM packages don't like having duplicate files in the same directory (eg copyright and COPYRIGHT with same
   # contents).  So the simplest thing is to rename COPYRIGHT to copyright for both.
   install_data(filesToInstall_copyright, rename : 'copyright', install_dir : installSubDir_doc)

   # Each Debian package (which provides a /usr/share/doc/pkg directory) must install a Debian changelog file in
   # /usr/share/doc/pkg/changelog.Debian.gz
   install_data(join_paths(meson.current_build_dir(), filesToInstall_changeLogCompressed),
                rename : 'changelog.Debian.gz',
                install_dir : installSubDir_doc)
else
   #----------- Windows and Mac -----------
   install_data(filesToInstall_copyright, install_dir : installSubDir_doc)
endif

#=======================================================================================================================
#========================================= Compiler-specific settings & flags ==========================================
#=======================================================================================================================
if meson.get_compiler('cpp').get_id() == 'gcc'
   #
   # -g3   should give even more debugging information than -g (which is equivalent to -g2)
   #
   # -O2   is hopefully a sensible optimisation level.  It means "GCC performs nearly all supported optimizations that do
   #       not involve a space-speed tradeoff. As compared to -O, this option increases both compilation time and the
   #       performance of the generated code."
   #
   # -z noexecstack  Is, in theory at least, to ensure/assert we do not have an executable stack.  This is partly as a
   #                 good thing in itself, and partly because, by default, rpmlint with throw a
   #                 missing-PT_GNU_STACK-section error if we don't.
   #                    In theory, the compiler should work out automatically whether we need an executable stack,
   #                 decide the answer is "No" and pass all the right options to the linker.   In practice, it seems
   #                 this doesn't happen for reasons I have, as yet, to discover.
   #                    So, we attempt to assert manually that the stack should not be executable.  The "-z noexecstack"
   #                 should get passed through by gcc the linker (see
   #                 https://gcc.gnu.org/onlinedocs/gcc/Link-Options.html#Link-Options) and the GNU linker
   #                 (https://sourceware.org/binutils/docs/ld/Options.html) should recognise "-z noexecstack" as "Marks
   #                 the object as not requiring executable stack".
   #                    However, even this is not sufficient(!).  So, for the moment, we suppress the rpmlint error (see
   #                 packaging/rpmLintFilters.toml).
   #
   # -no-pie    } These are all needed for Boost stacktrace to work properly - at least according to comments at
   # -fno-pie   } https://stackoverflow.com/questions/52583544/boost-stack-trace-not-showing-function-names-and-line-numbers
   # -rdynamic  } But, for some reason, gcc on Windows does not accept -rdynamic
   #
   add_global_arguments(['-g3',
                         '-O2',
                         '-z', 'noexecstack', # NB Not '-z noexecstack' as otherwise will be passed to gcc in quotes!
                         '-no-pie',
                         '-fno-pie'], language : 'cpp')
   if host_machine.system() != 'windows'
      add_global_arguments('-rdynamic', language : 'cpp')
   endif
endif

#=======================================================================================================================
#===================================================== Main builds =====================================================
#=======================================================================================================================
#
# To keep things simple, we share almost all code between the actual executable and the unit test runner.  However, we
# don't want to compile everything twice.  So, as a trick we compile into a static library everything except the code
# that differs between actual executable and unit test runner, then this library is linked into both programs.
#
# Note however that you cannot put generatedFromQrc in the static_library as it doesn't work there.
#
commonCodeStaticLib = static_library('common_code',
                                     commonSourceFiles, generatedFromUi, generatedFromMoc, translations,
                                     include_directories : includeDirs,
                                     dependencies: commonDependencies,
                                     install : false)

mainExecutable = executable(mainExecutableName,
                            'src/main.cpp', generatedFromQrc,
                            include_directories : includeDirs,
                            dependencies : mainExeDependencies,
                            link_with : commonCodeStaticLib,
                            install : true)

testRunner = executable(testRunnerName, generatedFromQrc,
                        'src/unitTests/Testing.cpp', generatedFromMocForUnitTests,
                        include_directories : includeDirs,
                        dependencies : testRunnerDependencies,
                        link_with : commonCodeStaticLib,
                        install : false)

#=======================================================================================================================
#===================================================== Unit Tests ======================================================
#=======================================================================================================================
test('Test integer sizes',                   testRunner, args : ['pstdintTest'])
test('Test recipe calculations - all grain', testRunner, args : ['recipeCalcTest_allGrain'])
test('Test post boil loss OG',               testRunner, args : ['postBoilLossOgTest'])
test('Test unit conversions',                testRunner, args : ['testUnitConversions'])
test('Test NamedParameterBundle',            testRunner, args : ['testNamedParameterBundle'])
test('Test algorithms',                      testRunner, args : ['testAlgorithms'])
test('Test log rotation',                    testRunner, args : ['testLogRotation'])

#=======================================================================================================================
#===================================================== Packaging =======================================================
#=======================================================================================================================
#
# Note that it is not intended that this target be invoked directly from the command line.  Rather, the 'bt' script will
# do some preparation, then call meson with a suitable install target, then call target to extract various build
# parameters into a final invocation of the bt script to do the main work.
#
# Meson does not offer a huge amount of help on creating installable packages.  It has no equivalent to CMake's CPack
# and there is generally not a lot of info out there about how to do packaging in Meson.  OTOH, although CPack has lots
# of features it is also very painful to use.  So maybe taking a completely different approach is a good thing.
#
# Have had a bit of a look at Qt-Installer-Framework (QtIFW).  Upsides are:
#   - in principle we could write one set of install config that would then create install packages for Windows, Mac and
#     Linux
#   - it should already know how to package Qt libraries(!)
#   - it's the same licence as the rest of Qt
#   - we can use it in GitHub actions (courtesy of https://github.com/jurplel/install-qt-action)
#   - it can handle in-place upgrades (including the check for whether an upgraded version is available), per
#     https://doc.qt.io/qtinstallerframework/ifw-updates.html
# Downsides are:
#   - outside of packaging Qt itself, I'm not sure that it's hugely widely used
#   - it's not a great advert for itself -- eg when I installed it locally on Kubuntu by downloading directly from
#     https://download.qt.io/official_releases/qt-installer-framework/, it didn't put its own tools in the PATH,
#     so I had to manually add ~/Qt/QtIFW-4.5.0/bin/ to my PATH
#   - it usually necessary to link against a static build of Qt, which is a bit of a pain as you have to download the
#     source files for Qt and compile it locally -- see eg
#     https://stackoverflow.com/questions/14932315/how-to-compile-qt-5-under-windows-or-linux-32-or-64-bit-static-or-dynamic-on-v
#     for the whole process
#   - it's a change of installation method for people who have previously downloaded deb packages, RPMs, Mac DMG files,
#     etc
#
# The alternative approach would be to do Windows, Mac and Linux packaging separately:
#   - For Mac, there is some info at https://mesonbuild.com/Creating-OSX-packages.html on creating app bundles
#   - For Linux, there is some mention in the Meson manual of building deb and rpm packages eg
#     https://mesonbuild.com/Installing.html#destdir-support, but I think you have to do most of the work yourself.
#     https://blog.devgenius.io/how-to-build-debian-packages-from-meson-ninja-d1c28b60e709 gives some sketchy starting info on
#     how to build deb packages.  Maybe we could find the equivalent for creating RPMs.  (There used to be a Meson
#     module for creating RPMs, but it was discontinued per
#     https://mesonbuild.com/Release-notes-for-0-62-0.html#removal-of-the-rpm-module because it was broken and badly
#     designed etc.)  Also look at https://openbuildservice.org/.
#   - For Windows, we could probably use NSIS (Nullsoft Scriptable Install System -- see
#     https://nsis.sourceforge.io/) to create a Windows installer.
#
# For now we'll see if we can get QtIFW working.
#

#
# Notes on building debian package -- see https://unix.stackexchange.com/questions/30303/how-to-create-a-deb-file-manually
#
# You need to install some extras:
#    sudo apt-get install debhelper build-essential dh-make
#
# Then:
#    # Create the source tarball
#    meson dist
#    # Create the debian/ subtree in the build directory
#    # I think we only want to do this once, because you end up with a lot of example files that you need to edit etc
#    # Or we could have a look at that brewtarget debian project...
#    dh_make --copyright gpl3 --email mfsy@yahoo.com --single --packagename brewken_0.1.0-1 --file meson-dist/*.tar.xz --yes
#
# To create a deb package, we create the following directory structure (where items marked ✅ are installed by meson
# install with --destdir option, and items marked ✴ are things we need to generate):
# TODO Mark things with ❌ that are generated by dh_make and not needed...
#    [projectName]-[versionNumber]_x86_64/
#    ├── control          ✴  # Contains info about dependencies, maintainer, etc
#    ├── control.tar.gz   ✴  # Hopefully just a compressed version of the above
#    ├── data.tar.gz      ✴  # Compressed archive of the usr tree below
#    ├── debian-binary    ✴  # A text file containing just "2.0"
#    ├── md5sums          ✴  # = checksums of all the files in the usr tree below
#    └── usr
#        ├── bin
#        │   └── [projectName] ✅   <-- the executable
#        └── share
#            ├── [projectName]
#            │   ├── applications
#            │   │   └── [projectName].desktop ✅  <-- [filesToInstall_desktop]
#            │   ├── DefaultData.xml           ✅  <----- [filesToInstall_data]
#            │   ├── default_db.sqlite         ✅  <--┘
#            │   ├── icons
#            │   │   └── hicolor
#            │   │       └── scalable
#            │   │           └── apps
#            │   │               └── [projectName].svg ✅  <-- [filesToInstall_icons]
#            │   ├── sounds
#            │   │   └── [All the filesToInstall_sounds .wav files] ✅
#            │   └── translations_qm
#            │       └── [All the .qm files generated by qt.compile_translations] ✅
#            └── doc
#                └── [projectName]
#                    ├── changelog.Debian.gz  ✅
#                    ├── copyright            ✅
#                    ├── README.md            ✅
#                    └── RelaseNotes.markdown ✅
#
#

packagesTarget = run_target('part_2_of_packaging',
                            command : ['bt', 'part_3_of_packaging'],
                            env : exportedVariables)
