#----------------------------------------------------------------------------------------------------------------------
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
#----------------------------------------------------------------------------------------------------------------------

#
# ⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐
# ⭐⭐⭐ THIS IS EXPERIMENTAL - CARRY ON USING CMAKE TO BUILD THE PRODUCT UNLESS AND UNTIL WORK ON MESON BUILD COMES TO
# ⭐⭐⭐ FRUITION.
# ⭐⭐⭐
# ⭐⭐⭐ AS OF 2022-11-23 COMPILATION AND LOCAL INSTALL SEEMS MOSTLY WORKING ON LINUX, BUT NOT EXTENSIVELY TESTED.  WE
# ⭐⭐⭐ HAVE NOT YET DONE ANYTHING ON UNIT TESTS, PACKAGING OR WINDOWS/MAC BUILDS
# ⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐
#
# On Linux, to install as recent a meson as possible:
#   sudo apt install python3 ninja
#   sudo apt remove meson
#   sudo pip3 install meson
#
# NB: As explained at https://mesonbuild.com/Getting-meson.html#installing-meson-with-pip this is not ideal.  We have to
# do the pip install as root, otherwise running meson install (or even sudo meson install) will barf on Linux (because
# our build needs to be able to install files into system directories).  In the long run, it would be preferable to
# install meson from distro packages (eg  sudo apt install meson  on Ubuntu etc).  But, for the moment at least, we need
# features that are too new to be in the distro packages.
#
# (At 2022-11-23, on Ubuntu 22.04, sudo apt install meson gives version 0.61.2, whereas sudo pip3 install meson gives
# version 0.64.1)
#
# We can periodically update to the latest version of meson with:
#   sudo pip install --upgrade meson
#
#
# Then to set up, say, mbuild, as your build directory:
#   mkdir mbuild
#   meson setup mbuild
# First command not strictly necessary, as meson will create the directory if it does not exist.
# Recommend using a separate directory from build for the moment, as want to be able to carry on building with CMake
# and don't want to get outputs from two different build systems muddled up.
#
# To compile:
#   cd mbuild
#   meson compile
#
# Then to install:
#   meson install
#
# Packaging is where Meson is currently weakest IMHO.  It seems to have no equivalent to CPack and there is not a lot of
# info about how to do things:
#   - https://stackoverflow.com/questions/60027455/is-there-a-tool-for-meson-similar-equivalent-to-cpack-for-cmake is
#     essentially unanswered (and the link it offered for how to build RPMs is broken because this module was removed
#     from Meson per https://mesonbuild.com/Release-notes-for-0-62-0.html#removal-of-the-rpm-module because it was
#     broken and badly designed etc
#   - https://blog.devgenius.io/how-to-build-debian-packages-from-meson-ninja-d1c28b60e709 gives some starting info on
#     how to build deb packages
#   - There is almost no info out there on how to do Windows or Mac packaging
#
#
#

# We'll get an error if project isn't the first call in this file
project('brewken', 'cpp',
        version: '0.1.0',
        license: 'GPL-3.0-or-later')

#
# By design, Meson uses a sub-set of Python -- see
# https://mesonbuild.com/FAQ.html#why-is-meson-not-just-a-python-module-so-i-could-code-my-build-setup-in-python and
# links therefrom for the rationale for this.
#
# One thing this means is that you sometimes have to do things in a slightly more cumbersome way than you would in a
# full-fat Python script.  Eg here, in regular Python, we would write:
#    capitalisedProjectName = projectName.capitalize()
# But meson.project_name() returns a Meson String, not a Python String, so there's a bit more work to do to get the same
# result.
#
projectName = meson.project_name().to_lower()
capitalisedProjectName = projectName.substring(0, 1).to_upper() + projectName.substring(1)

message('Building', projectName, 'version', meson.project_version(), 'for', host_machine.system(), 'on', build_machine.system() )

executableName = projectName

#=======================================================================================================================
#==================================================== Build options ====================================================
#=======================================================================================================================
prefix = get_option('prefix')

#=======================================================================================================================
#============================================== Frameworks and Libraries ===============================================
#=======================================================================================================================

# Import the Qt tools.  See https://mesonbuild.com/Qt5-module.html
qt = import('qt5')

# Tell meson which Qt modules we need
qtDependencies = dependency('qt5', modules: ['Core',
                                             'Gui',
                                             'Multimedia',
                                             'Network',
                                             'PrintSupport',
                                             'Sql',
                                             'Svg',
                                             'Widgets',
                                             'Xml'])

# Find the other libraries we depend on
#
# See https://mesonbuild.com/Reference-manual_returned_dep.html for what info we can pull from a dependency object
#
# For BeerXML processing we need Xerces-C++ and Xalan-C++.  Meson can find both of these automatically using CMake's
# find_package(), as long as CMake is installed(!) and we provide the right library names ('XercesC' per
# https://cmake.org/cmake/help/latest/module/FindXercesC.html and 'XalanC' per
# https://cmake.org/cmake/help/latest/module/FindXalanC.html)
#
# I looked at using ConanCenter (https://conan.io/center/) as a source of libraries, so that we could automate
# installing them, but it does not have all the ones we need.  Eg it has Xerces-C but not Xalan-C.  (Others have already
# requested Xalan-C, eg see https://github.com/conan-io/conan-center-index/issues/5546, but it doesn't seem imminent.)
#
# Meson has its own dependency management system, Wrap, but the list of pre-provided projects at
# https://mesonbuild.com/Wrapdb-projects.html does not include Boost, Xerces or Xalan.
#
# More hopeful is vcpkg (https://vcpkg.io/en/index.html) as it does have both Xerces-C and Xalan-C, along with Boost.
# There is an example here https://github.com/Neumann-A/meson-vcpkg of how to use vcpkg from Meson.
#
xercesDependency = dependency('XercesC')
message('Xerces Library:', xercesDependency.name(), 'found =', xercesDependency.found(), 'version =', xercesDependency.version())
xalanDependency = dependency('XalanC')
message('Xalan Library:', xalanDependency.name(), 'found =', xalanDependency.found(), 'version =', xalanDependency.version())

# Other directories to search in for headers.  Meson will barf an error if any of these directories does not exist.
includeDirs = include_directories('src', 'third-party/valijson/include')

#=======================================================================================================================
#===================================================== Input Files =====================================================
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
# headers in the project.
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
   projectName + '.desktop'
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

filesToInstall_changeLogUncompressed = files([
   'CHANGES.markdown'
])

filesToInstall_copyright = files([
   'COPYRIGHT'
])

# This is not wrapped in files() because it's not yet created!
# See below for how this one gets created from filesToInstall_changeLogUncompressed
filesToInstall_changeLogCompressed = [
   'changelog.gz'
]

#=======================================================================================================================
#============================================ Installation sub-directories =============================================
#=======================================================================================================================
installSubDir_data = 'share/' + projectName
installSubDir_doc  = 'share/doc/' + projectName
installSubDir_bin  = 'bin'
# According to https://specifications.freedesktop.org/menu-spec/menu-spec-1.0.html#paths, .desktop files need to live
# in one of the $XDG_DATA_DIRS/applications/.  (Note that $XDG_DATA_DIRS is a colon-separated list of directories, typically
# defaulting to /usr/local/share/:/usr/share/. but on another system it might be
# /usr/share/plasma:/usr/local/share:/usr/share:/var/lib/snapd/desktop:/var/lib/snapd/desktop).  When combined with
# CMAKE_INSTALL_PREFIX, "share/applications" should end up being one of these.
installSubDir_applications = 'share/applications'
# It's a similar but slightly more complicated story for where to put icons.  (See
# https://specifications.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html#directory_layout for all the
# details.)
installSubDir_icons        = 'share/icons'
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
                                  dependencies : qtDependencies)

# Generate the necessary targets to build translation files with lrelease
translations = qt.compile_translations(ts_files         : translationSourceFiles,
                                       build_by_default : true,
                                       install          : true,
                                       install_dir      : installSubDir_translations)

#=======================================================================================================================
#=========================================== Generate config.h from config.in ==========================================
#=======================================================================================================================
# Taking src/config.in as input, we generate (in the build subdirectory only) config.h.  This is a way to inject CMake
# variables into the code.
#
#### All variables written as "${VAR}" in config.in  will be replaced by the value of VAR in config.h.
#### Eg "#define CONFIG_DATA_DIR ${CONFIG_DATA_DIR}" in config.in will be replaced by the below corresponding value in
#### ${CONFIG_DATA_DIR} below when configure_file() is called.
####
###set(CONFIG_DATA_DIR "${CMAKE_INSTALL_PREFIX}/${installSubDir_data}/")
###string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S (UTC)" UTC)
###configure_file(src/config.in src/config.h)

#### meson.project_version() goes in here somewhere!
conf_data = configuration_data()
conf_data.set('PROJECT_VERSION',        meson.project_version())
conf_data.set('CONFIG_DATA_DIR',        prefix + installSubDir_data) # This is a bit of a hack...
conf_data.set('capitalisedProjectName', capitalisedProjectName)
conf_data.set('CMAKE_PROJECT_NAME',     projectName)
# NB CMAKE_HOST_SYSTEM means something different than meson host_machine
conf_data.set('CMAKE_HOST_SYSTEM',      build_machine.system())
conf_data.set('CMAKE_SYSTEM',           host_machine.system())
conf_data.set('CMAKE_CXX_COMPILER_ID',  meson.get_compiler('cpp').get_id())
# Meson doesn't directly give you a way to obtain the current date and time.  But it does allow you turn an external
# command, so this is one way to get it - relying on the fact that MSYS2 on Windows, Linux and Mac all have date
# available from the command line.
conf_data.set('BUILD_TIMESTAMP',        run_command('date', check: true).stdout().strip())
configure_file(input : 'config.in',
               output : 'config.h',
               configuration : conf_data)

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
   install_data(filesToInstall_icons, install_dir : installSubDir_icons + '/hicolor/scalable/apps/)')

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

   # TODO: Need to generate this file
   # Each Debian package (which provides a /usr/share/doc/pkg directory) must install a Debian changelog file in
   # /usr/share/doc/pkg/changelog.Debian.gz
   # install_data(filesToInstall_changeLogCompressed, rename : 'changelog.Debian.gz', install_dir : installSubDir_doc)
else
   #----------- Windows and Mac -----------
   install_data(filesToInstall_copyright, install_dir : installSubDir_doc)
endif

#=======================================================================================================================
#====================================================== Main build =====================================================
#=======================================================================================================================
executable(executableName,
           'src/main.cpp', commonSourceFiles, generatedFromQrc, generatedFromUi, generatedFromMoc, translations,
           include_directories : includeDirs,
           dependencies: [qtDependencies, xercesDependency, xalanDependency],
           install : true)
