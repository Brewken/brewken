#----------------------------------------------------------------------------------------------------------------------
# src/CMakeLists.txt is part of Brewken, and is copyright the following authors 2009-2021:
#   • Aidan Roberts <aidanr67@gmail.com>
#   • Daniel Moreno <danielm5@users.noreply.github.com>
#   • Mark de Wever <koraq@xs4all.nl>
#   • Mattias Måhl <mattias@kejsarsten.com>
#   • Matt Young<mfsy@yahoo.com>
#   • Maxime Lavigne <duguigne@gmail.com>
#   • Mik Firestone <mikfire@gmail.com>
#   • Philip Greggory Lee <rocketman768@gmail.com>
#   • Ryan Hoobler <rhoob@yahoo.com>
#   • Théophane Martin <theophane.m@gmail.com>
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
# See also ../CMakeLists.txt
#

#
# Uncomment the next line for slightly verbose build output
# Alternatively, for very verbose output, invoke make as follows:
#    $ make VERBOSE=1
#
#set(CMAKE_VERBOSE_MAKEFILE ON)

# The current directory in the original source.
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} )
# The current directory in the output build directory.
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR} )

# Put all the required Qt dirs in the includes.
#   That isn't quite what this include does. QT_USE_FILE references
#   a cmake macro include, which is good but not required.
#INCLUDE(${QT_USE_FILE})

# (ADD_PCH_RULE  _header_filename _src_list)
# Version 7/26/2010
#
# use this macro before "add_executable"
#
# _header_filename
#  header to make a .gch
#
# _src_list
#   the variable name (do not use ${..}) which contains a
#     a list of sources (a.cpp b.cpp c.cpp ...)
#  This macro will append a header file to it, then this src_list can be used in
#  "add_executable..."
#
#
# Now a .gch file should be generated and gcc should use it.
#        (add -Winvalid-pch to the cpp flags to verify)
#
# make clean should delete the pch file
#
# example : ADD_PCH_RULE(headers.h myprog_SRCS)
MACRO (ADD_PCH_RULE  _header_filename _src_list)
   SET(_gch_filename "${CMAKE_CURRENT_BINARY_DIR}/${_header_filename}.gch")
   SET(_header "${CMAKE_CURRENT_SOURCE_DIR}/${_header_filename}")

   LIST(APPEND ${_src_list} ${_gch_filename})

   SET (_args ${CMAKE_CXX_FLAGS})
   LIST(APPEND _args -c ${_header} -o ${_gch_filename})

   GET_DIRECTORY_PROPERTY(DIRINC INCLUDE_DIRECTORIES)
   FOREACH(_inc ${DIRINC})
      LIST(APPEND _args "-I" ${_inc})
   ENDFOREACH(_inc ${DIRINC})

   SEPARATE_ARGUMENTS(_args)

   ADD_CUSTOM_COMMAND(OUTPUT ${_gch_filename}
                      COMMAND rm -f ${_gch_filename}
                      COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_ARG1} ${_args}
                      DEPENDS ${_header})
ENDMACRO(ADD_PCH_RULE _src_list _header_filename)

#
# Variable that contains all the .cpp files in this project.
#
# You can recreate the body of this list by running the following from the bash prompt in the build directory:
#    find ../src -name '*.cpp' | sort  | sed 's+^../src+    ${SRCDIR}+' | grep -v Testing.cpp | grep -v main.cpp
#
SET( Brewken_SRCS
    ${SRCDIR}/AboutDialog.cpp
    ${SRCDIR}/AlcoholTool.cpp
    ${SRCDIR}/Algorithms.cpp
    ${SRCDIR}/AncestorDialog.cpp
    ${SRCDIR}/BeerColorWidget.cpp
    ${SRCDIR}/boiltime.cpp
    ${SRCDIR}/BrewDayScrollWidget.cpp
    ${SRCDIR}/BrewDayWidget.cpp
    ${SRCDIR}/Brewken.cpp
    ${SRCDIR}/BrewNoteWidget.cpp
    ${SRCDIR}/BtDatePopup.cpp
    ${SRCDIR}/BtDigitWidget.cpp
    ${SRCDIR}/BtFolder.cpp
    ${SRCDIR}/BtLabel.cpp
    ${SRCDIR}/BtLineEdit.cpp
    ${SRCDIR}/BtPrintPreview.cpp
    ${SRCDIR}/BtSplashScreen.cpp
    ${SRCDIR}/BtTabWidget.cpp
    ${SRCDIR}/BtTextEdit.cpp
    ${SRCDIR}/BtTreeFilterProxyModel.cpp
    ${SRCDIR}/BtTreeItem.cpp
    ${SRCDIR}/BtTreeModel.cpp
    ${SRCDIR}/BtTreeView.cpp
    ${SRCDIR}/ColorMethods.cpp
    ${SRCDIR}/ConverterTool.cpp
    ${SRCDIR}/CustomComboBox.cpp
    ${SRCDIR}/database/Database.cpp
    ${SRCDIR}/database/DatabaseSchemaHelper.cpp
    ${SRCDIR}/database/DbTransaction.cpp
    ${SRCDIR}/database/ObjectStoreTyped.cpp
    ${SRCDIR}/database/ObjectStore.cpp
    ${SRCDIR}/EquipmentButton.cpp
    ${SRCDIR}/EquipmentEditor.cpp
    ${SRCDIR}/EquipmentListModel.cpp
    ${SRCDIR}/FermentableDialog.cpp
    ${SRCDIR}/FermentableEditor.cpp
    ${SRCDIR}/FermentableSortFilterProxyModel.cpp
    ${SRCDIR}/FermentableTableModel.cpp
    ${SRCDIR}/HeatCalculations.cpp
    ${SRCDIR}/HelpDialog.cpp
    ${SRCDIR}/HopDialog.cpp
    ${SRCDIR}/HopEditor.cpp
    ${SRCDIR}/HopSortFilterProxyModel.cpp
    ${SRCDIR}/HopTableModel.cpp
    ${SRCDIR}/Html.cpp
    ${SRCDIR}/HydrometerTool.cpp
    ${SRCDIR}/IbuGuSlider.cpp
    ${SRCDIR}/IbuMethods.cpp
    ${SRCDIR}/InstructionWidget.cpp
    ${SRCDIR}/InventoryFormatter.cpp
    ${SRCDIR}/Logging.cpp
    ${SRCDIR}/MainWindow.cpp
    ${SRCDIR}/MashButton.cpp
    ${SRCDIR}/MashComboBox.cpp
    ${SRCDIR}/MashDesigner.cpp
    ${SRCDIR}/MashEditor.cpp
    ${SRCDIR}/MashListModel.cpp
    ${SRCDIR}/MashStepEditor.cpp
    ${SRCDIR}/MashStepTableModel.cpp
    ${SRCDIR}/MashStepTableWidget.cpp
    ${SRCDIR}/MashWizard.cpp
    ${SRCDIR}/matrix.cpp
    ${SRCDIR}/MiscDialog.cpp
    ${SRCDIR}/MiscEditor.cpp
    ${SRCDIR}/MiscSortFilterProxyModel.cpp
    ${SRCDIR}/MiscTableModel.cpp
    ${SRCDIR}/model/BrewNote.cpp
    ${SRCDIR}/model/Equipment.cpp
    ${SRCDIR}/model/Fermentable.cpp
    ${SRCDIR}/model/Hop.cpp
    ${SRCDIR}/model/Instruction.cpp
    ${SRCDIR}/model/Inventory.cpp
    ${SRCDIR}/model/Mash.cpp
    ${SRCDIR}/model/MashStep.cpp
    ${SRCDIR}/model/Misc.cpp
    ${SRCDIR}/model/NamedEntity.cpp
    ${SRCDIR}/model/NamedEntityWithInventory.cpp
    ${SRCDIR}/model/NamedParameterBundle.cpp
    ${SRCDIR}/model/Recipe.cpp
    ${SRCDIR}/model/Salt.cpp
    ${SRCDIR}/model/Style.cpp
    ${SRCDIR}/model/Water.cpp
    ${SRCDIR}/model/Yeast.cpp
    ${SRCDIR}/NamedEntitySortProxyModel.cpp
    ${SRCDIR}/NamedMashEditor.cpp
    ${SRCDIR}/OgAdjuster.cpp
    ${SRCDIR}/OptionDialog.cpp
    ${SRCDIR}/PersistentSettings.cpp
    ${SRCDIR}/PitchDialog.cpp
    ${SRCDIR}/PreInstruction.cpp
    ${SRCDIR}/PrimingDialog.cpp
    ${SRCDIR}/QueuedMethod.cpp
    ${SRCDIR}/RadarChart.cpp
    ${SRCDIR}/RangedSlider.cpp
    ${SRCDIR}/RecipeExtrasWidget.cpp
    ${SRCDIR}/RecipeFormatter.cpp
    ${SRCDIR}/RefractoDialog.cpp
    ${SRCDIR}/SaltTableModel.cpp
    ${SRCDIR}/ScaleRecipeTool.cpp
    ${SRCDIR}/SimpleUndoableUpdate.cpp
    ${SRCDIR}/StrikeWaterDialog.cpp
    ${SRCDIR}/StyleButton.cpp
    ${SRCDIR}/StyleEditor.cpp
    ${SRCDIR}/StyleListModel.cpp
    ${SRCDIR}/StyleRangeWidget.cpp
    ${SRCDIR}/StyleSortFilterProxyModel.cpp
    ${SRCDIR}/TimerListDialog.cpp
    ${SRCDIR}/TimerMainDialog.cpp
    ${SRCDIR}/TimerWidget.cpp
    ${SRCDIR}/Unit.cpp
    ${SRCDIR}/UnitSystem.cpp
    ${SRCDIR}/WaterButton.cpp
    ${SRCDIR}/WaterDialog.cpp
    ${SRCDIR}/WaterEditor.cpp
    ${SRCDIR}/WaterListModel.cpp
    ${SRCDIR}/WaterSortFilterProxyModel.cpp
    ${SRCDIR}/WaterTableModel.cpp
    ${SRCDIR}/WaterTableWidget.cpp
    ${SRCDIR}/xml/BeerXml.cpp
    ${SRCDIR}/xml/BtDomErrorHandler.cpp
    ${SRCDIR}/xml/XercesHelpers.cpp
    ${SRCDIR}/xml/XmlCoding.cpp
    ${SRCDIR}/xml/XmlMashRecord.cpp
    ${SRCDIR}/xml/XmlMashStepRecord.cpp
    ${SRCDIR}/xml/XmlRecipeRecord.cpp
    ${SRCDIR}/xml/XmlRecordCount.cpp
    ${SRCDIR}/xml/XmlRecord.cpp
    ${SRCDIR}/YeastDialog.cpp
    ${SRCDIR}/YeastEditor.cpp
    ${SRCDIR}/YeastSortFilterProxyModel.cpp
    ${SRCDIR}/YeastTableModel.cpp
)

# List of all the *.ui
# TODO: can I somehow have a separate CMakeLists.txt
# in the ui/ directory instead of here?
#
# You can recreate the body of this list by running the following from the bash prompt in the build directory:
#    find ../ui -name '*.ui' | sort  | sed 's+^../ui+    ${SRCDIR}+'
#
SET( Brewken_UIS
    ${UIDIR}/ancestorDialog.ui
    ${UIDIR}/brewDayScrollWidget.ui
    ${UIDIR}/brewDayWidget.ui
    ${UIDIR}/brewNoteWidget.ui
    ${UIDIR}/equipmentEditor.ui
    ${UIDIR}/fermentableEditor.ui
    ${UIDIR}/hopEditor.ui
    ${UIDIR}/instructionWidget.ui
    ${UIDIR}/mainWindow.ui
    ${UIDIR}/mashDesigner.ui
    ${UIDIR}/mashEditor.ui
    ${UIDIR}/mashStepEditor.ui
    ${UIDIR}/mashWizard.ui
    ${UIDIR}/miscEditor.ui
    ${UIDIR}/namedMashEditor.ui
    ${UIDIR}/ogAdjuster.ui
    ${UIDIR}/optionsDialog.ui
    ${UIDIR}/pitchDialog.ui
    ${UIDIR}/primingDialog.ui
    ${UIDIR}/recipeExtrasWidget.ui
    ${UIDIR}/refractoDialog.ui
    ${UIDIR}/strikeWaterDialog.ui
    ${UIDIR}/styleEditor.ui
    ${UIDIR}/timerMainDialog.ui
    ${UIDIR}/timerWidget.ui
    ${UIDIR}/waterDialog.ui
    ${UIDIR}/waterEditor.ui
    ${UIDIR}/yeastEditor.ui
)

#
# The list of files that need to be moc'd.
# In other words, classes that have Q_OBJECT declarations and stuff.
# NB: This is NOT a list of ALL header files!
#
# You can recreate the body of this list by running the following from the bash prompt in the build directory:
#    grep -rl Q_OBJECT ../src | sort | sed 's+^../src+    ${SRCDIR}+' | grep -v Testing.h
#
SET( Brewken_MOC_HEADERS
    ${SRCDIR}/AboutDialog.h
    ${SRCDIR}/AlcoholTool.h
    ${SRCDIR}/AncestorDialog.h
    ${SRCDIR}/BeerColorWidget.h
    ${SRCDIR}/boiltime.h
    ${SRCDIR}/BrewDayScrollWidget.h
    ${SRCDIR}/BrewDayWidget.h
    ${SRCDIR}/Brewken.h
    ${SRCDIR}/BrewNoteWidget.h
    ${SRCDIR}/BtDatePopup.h
    ${SRCDIR}/BtDigitWidget.h
    ${SRCDIR}/BtFolder.h
    ${SRCDIR}/BtHorizontalTabs.h
    ${SRCDIR}/BtLabel.h
    ${SRCDIR}/BtLineEdit.h
    ${SRCDIR}/BtSplashScreen.h
    ${SRCDIR}/BtTabWidget.h
    ${SRCDIR}/BtTextEdit.h
    ${SRCDIR}/BtTreeFilterProxyModel.h
    ${SRCDIR}/BtTreeModel.h
    ${SRCDIR}/BtTreeView.h
    ${SRCDIR}/CMakeLists.txt
    ${SRCDIR}/ConverterTool.h
    ${SRCDIR}/CustomComboBox.h
    ${SRCDIR}/database/Database.h
    ${SRCDIR}/EquipmentButton.h
    ${SRCDIR}/EquipmentEditor.h
    ${SRCDIR}/EquipmentListModel.h
    ${SRCDIR}/FermentableDialog.h
    ${SRCDIR}/FermentableEditor.h
    ${SRCDIR}/FermentableSortFilterProxyModel.h
    ${SRCDIR}/FermentableTableModel.h
    ${SRCDIR}/HopDialog.h
    ${SRCDIR}/HopEditor.h
    ${SRCDIR}/HopSortFilterProxyModel.h
    ${SRCDIR}/HopTableModel.h
    ${SRCDIR}/HydrometerTool.h
    ${SRCDIR}/IbuGuSlider.h
    ${SRCDIR}/InstructionWidget.h
    ${SRCDIR}/MainWindow.h
    ${SRCDIR}/MashButton.h
    ${SRCDIR}/MashComboBox.h
    ${SRCDIR}/MashDesigner.h
    ${SRCDIR}/MashEditor.h
    ${SRCDIR}/MashListModel.h
    ${SRCDIR}/MashStepEditor.h
    ${SRCDIR}/MashStepTableModel.h
    ${SRCDIR}/MashStepTableWidget.h
    ${SRCDIR}/MashWizard.h
    ${SRCDIR}/MiscDialog.h
    ${SRCDIR}/MiscEditor.h
    ${SRCDIR}/MiscSortFilterProxyModel.h
    ${SRCDIR}/MiscTableModel.h
    ${SRCDIR}/model/BrewNote.h
    ${SRCDIR}/model/Equipment.h
    ${SRCDIR}/model/Fermentable.h
    ${SRCDIR}/model/Hop.h
    ${SRCDIR}/model/Instruction.h
    ${SRCDIR}/model/Inventory.h
    ${SRCDIR}/model/Mash.h
    ${SRCDIR}/model/MashStep.h
    ${SRCDIR}/model/Misc.h
    ${SRCDIR}/model/NamedEntity.h
    ${SRCDIR}/model/NamedEntityWithInventory.h
    ${SRCDIR}/model/Recipe.h
    ${SRCDIR}/model/Salt.h
    ${SRCDIR}/model/Style.h
    ${SRCDIR}/model/Water.h
    ${SRCDIR}/model/Yeast.h
    ${SRCDIR}/NamedEntitySortProxyModel.h
    ${SRCDIR}/NamedMashEditor.h
    ${SRCDIR}/OgAdjuster.h
    ${SRCDIR}/PitchDialog.h
    ${SRCDIR}/PrimingDialog.h
    ${SRCDIR}/QueuedMethod.h
    ${SRCDIR}/RangedSlider.h
    ${SRCDIR}/RecipeExtrasWidget.h
    ${SRCDIR}/RecipeFormatter.h
    ${SRCDIR}/RefractoDialog.h
    ${SRCDIR}/SaltTableModel.h
    ${SRCDIR}/ScaleRecipeTool.h
    ${SRCDIR}/SimpleUndoableUpdate.h
    ${SRCDIR}/StrikeWaterDialog.h
    ${SRCDIR}/StyleButton.h
    ${SRCDIR}/StyleEditor.h
    ${SRCDIR}/StyleListModel.h
    ${SRCDIR}/StyleRangeWidget.h
    ${SRCDIR}/StyleSortFilterProxyModel.h
    ${SRCDIR}/Testing.h
    ${SRCDIR}/TimerListDialog.h
    ${SRCDIR}/TimerMainDialog.h
    ${SRCDIR}/TimerWidget.h
    ${SRCDIR}/unit.h
    ${SRCDIR}/WaterButton.h
    ${SRCDIR}/WaterDialog.h
    ${SRCDIR}/WaterEditor.h
    ${SRCDIR}/WaterListModel.h
    ${SRCDIR}/WaterSortFilterProxyModel.h
    ${SRCDIR}/WaterTableModel.h
    ${SRCDIR}/WaterTableWidget.h
    ${SRCDIR}/YeastDialog.h
    ${SRCDIR}/YeastEditor.h
    ${SRCDIR}/YeastSortFilterProxyModel.h
    ${SRCDIR}/YeastTableModel.h
)

SET( FILES_TO_TRANSLATE
     ${Brewken_SRCS}
     ${Brewken_UIS}
)

#======Speed up compilation by using precompiled headers for development======
#IF( UNIX AND NOT APPLE )
#   SET( precompiled_h "equipment.h" "fermentable.h" "hop.h" "instruction.h" "mash.h" "mashstep.h" "misc.h" "recipe.h" "style.h" "unit.h" "water.h" "yeast.h" "database.h" "Brewken.h" )
#   FOREACH( header ${precompiled_h} )
#      ADD_PCH_RULE( ${header} Brewken_SRCS )
#   ENDFOREACH()
#ENDIF()

#==================================Qt Junk=====================================

# Create the ui_*.h files from the *.ui files.
QT5_WRAP_UI(Brewken_UIS_H ${Brewken_UIS})
# Generate the resource .cpp from the .qrc file.
QT5_ADD_RESOURCES(Brewken_QRC_CPP ${Brewken_QRC})
QT5_ADD_RESOURCES(Brewken_cfg_QRC_CPP ${Brewken_cfg_QRC})

# Translations. CREATE_TRANSLATION will update the .ts files (do not want)
#QT5_CREATE_TRANSLATION(${TRANSLATIONS_FILES} ${FILES_TO_TRANSLATE})
# Creates the .qm files from the .ts files
QT5_ADD_TRANSLATION(QM_FILES ${TRANSLATIONS_FILES})

# Add a target for the QM_FILES so that we can add
# the translations as a dependency for the executable later.
ADD_CUSTOM_TARGET(translations DEPENDS ${QM_FILES})

#============================Icon for Windows==================================

SET(Brewken_ICON "")

IF( WIN32 AND MINGW )
  ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_BINARY_DIR}/src/icon.o
                     COMMAND windres.exe -I${CMAKE_CURRENT_SOURCE_DIR}
                     -i${RCFILE}
                     -o${CMAKE_BINARY_DIR}/src/icon.o
                     DEPENDS ${RCFILE}
  )
  SET(Brewken_ICON ${CMAKE_BINARY_DIR}/src/icon.o)
ELSEIF(WIN32)
  SET(Brewken_ICON ${RCFILE})
ENDIF()

#===========================================Requirements for Boost Stacktrace===========================================
#
# Per https://www.boost.org/doc/libs/1_76_0/doc/html/stacktrace/configuration_and_build.html, by default
# Boost.Stacktrace is a header-only library.  However, you get better results by linking (either statically or
# dynamically) with a helper library.  Of the various options, it seems like boost_stacktrace_backtrace gives the most
# functionality over the most platforms.  This has dependencies on:
#   - libdl on POSIX platforms
#   - libbacktrace
# The latter is an external library on Windows.  On POSIX plaforms it's typically already either installed on the system
# (eg see https://man7.org/linux/man-pages/man3/backtrace.3.html) or built in to the compiler.  Fortunately, CMake knows
# how to do the right thing in either case, thanks to https://cmake.org/cmake/help/latest/module/FindBacktrace.html.
#
IF(NOT WIN32)
   find_library(DL_LIBRARY dl)
ENDIF()
find_package(Backtrace REQUIRED)
# For the moment, leave default settings for Mac as can't work out how to get the backtrace version of stacktrace
# working.  (Should still get stack traces on Mac, but might not get as much info in them as we'd like.)
IF(NOT APPLE)
   IF(NOT WIN32)
      add_compile_definitions(BOOST_STACKTRACE_DYN_LINK)
   ENDIF()
   add_compile_definitions(BOOST_STACKTRACE_USE_BACKTRACE)
ENDIF()
#message("Backtrace libs " ${DL_LIBRARY} " | " ${Backtrace_LIBRARIES} " | " ${Boost_LIBRARIES})

#===========================Create the binary==================================

# This creates a "library" of object files so that we do not have to recompile
# the source files once per target, but rather, just once EVER.
ADD_LIBRARY(
   btobjlib
   OBJECT
   ${Brewken_SRCS}
   ${Brewken_MOC_SRCS}
   ${Brewken_QRC_CPP}
   ${Brewken_UIS_H}
)

IF( APPLE )
   # It seems I have to set these properties in this file, or things don't
   # get installed
   SET_SOURCE_FILES_PROPERTIES(
      ${Brewken_MAC_ICNS}
      PROPERTIES
      MACOSX_PACKAGE_LOCATION "Resources"
   )
   SET_SOURCE_FILES_PROPERTIES(
      ${Brewken_DATA}
      PROPERTIES
      MACOSX_PACKAGE_LOCATION "Resources"
   )
   SET_SOURCE_FILES_PROPERTIES(
      ${Brewken_DOCS}
      PROPERTIES
      MACOSX_PACKAGE_LOCATION "Resources/en.lproj"
   )
   SET_SOURCE_FILES_PROPERTIES(
      ${Brewken_SOUNDS}
      PROPERTIES
      MACOSX_PACKAGE_LOCATION "Resources/sounds"
   )
   SET_SOURCE_FILES_PROPERTIES(
      ${QM_FILES}
      PROPERTIES
      MACOSX_PACKAGE_LOCATION "Resources/translations_qm"
   )
   ADD_EXECUTABLE(
      ${Brewken_EXECUTABLE}
      MACOSX_BUNDLE
      ${SRCDIR}/main.cpp
      ${Brewken_MAC_ICNS}
      ${Brewken_DATA}
      ${Brewken_DOCS}
      ${Brewken_SOUNDS}
      ${QM_FILES}
      $<TARGET_OBJECTS:btobjlib>
   )
ELSE()
   ADD_EXECUTABLE(
      ${Brewken_EXECUTABLE}
      ${SRCDIR}/main.cpp
      ${Brewken_ICON}
      $<TARGET_OBJECTS:btobjlib>
   )
ENDIF()

ADD_DEPENDENCIES( ${Brewken_EXECUTABLE} translations )

# Link Brewken against appropriate libraries.
IF( WIN32 AND MINGW )
   ############################################################################
   # Need to set some linker flags that I don't know how to get
   # automatically.
   ############################################################################

   # MinGW-specific flags.
   #    '-Wl,-subsystem,windows' supresses the output command window.
   #    '-Wl,-s' strips the executable of symbols.
   SET_TARGET_PROPERTIES( ${Brewken_EXECUTABLE} PROPERTIES LINK_FLAGS "-Wl,-enable-stdcall-fixup -Wl,-enable-auto-import -Wl,-enable-runtime-pseudo-reloc -mthreads -Wl,-subsystem,windows")

   if(TARGET Qt5::windeployqt)
      # execute windeployqt in a tmp directory after build
      add_custom_command(TARGET ${Brewken_EXECUTABLE}
         POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/windeployqt"
         COMMAND Qt5::windeployqt --dir "${CMAKE_CURRENT_BINARY_DIR}/windeployqt" --no-translations --compiler-runtime "$<TARGET_FILE_DIR:${Brewken_EXECUTABLE}>/$<TARGET_FILE_NAME:${Brewken_EXECUTABLE}>"
   )

   # copy deployment directory during installation
   install(
      DIRECTORY
      "${CMAKE_CURRENT_BINARY_DIR}/windeployqt/"
      DESTINATION bin
   )
   endif()
ENDIF()

SET( QT5_USE_MODULES_LIST
   ${Brewken_EXECUTABLE}
   Qt5::Widgets
   Qt5::Network
   Qt5::PrintSupport
   Qt5::Sql
   Qt5::Svg
   Qt5::Xml
   )

IF( NOT ${NO_QTMULTIMEDIA})
   SET( QT5_USE_MODULES_LIST ${QT5_USE_MODULES_LIST} Qt5::Multimedia)
ENDIF()

#
# All the libraries
#
# ********************************************************************************************************************
# * Note that if you change this you need to make corresponding changes to this to target_link_libraries in the Tests
# * section below.
# *
# * .:TODO:. Work out how to avoid this copy-and-paste duplication
# ********************************************************************************************************************
#
target_link_libraries(
   ${QT5_USE_MODULES_LIST}
   ${XercesC_LIBRARIES}
   ${XalanC_LIBRARIES}
   ${Boost_LIBRARIES}
   ${DL_LIBRARY}
   ${Backtrace_LIBRARIES}
)

#=================================Tests========================================

#QT4_WRAP_CPP( testing_MOC_SRCS ${SRCDIR}/Testing.h )
ADD_EXECUTABLE(
   Brewken_tests
   ${SRCDIR}/Testing.cpp
   ${testing_MOC_SRCS}
   $<TARGET_OBJECTS:btobjlib>
)

SET( QT5_USE_MODULES_LIST
   Brewken_tests
   Qt5::Widgets
   Qt5::Network
   Qt5::PrintSupport
   Qt5::Sql
   Qt5::Svg
   Qt5::Xml
   Qt5::Test
   )

IF( NOT ${NO_QTMULTIMEDIA})
SET( QT5_USE_MODULES_LIST ${QT5_USE_MODULES_LIST} Qt5::Multimedia)
ENDIF()

# NB Needs to be same as the one a few lines above. Sigh.
target_link_libraries(
   ${QT5_USE_MODULES_LIST}
   ${XercesC_LIBRARIES}
   ${XalanC_LIBRARIES}
   ${Boost_LIBRARIES}
   ${DL_LIBRARY}
   ${Backtrace_LIBRARIES}
)

ADD_TEST(
   NAME pstdintTest
   COMMAND Brewken_tests pstdintTest
)
ADD_TEST(
   NAME recipeCalcTest_allGrain
   COMMAND Brewken_tests recipeCalcTest_allGrain
)
ADD_TEST(
   NAME postBoilLossOgTest
   COMMAND Brewken_tests postBoilLossOgTest
)
add_test(
   NAME testLogRotation
   COMMAND Brewken_tests testLogRotation
)
#=================================Installs=====================================

# Install executable.
INSTALL( TARGETS ${Brewken_EXECUTABLE}
         BUNDLE DESTINATION .
         RUNTIME DESTINATION ${TARGETPATH}
         COMPONENT ${RUNTIME_INSTALL_COMPONENT} )

IF( WIN32 )
      install(CODE
      "
         file(GET_RUNTIME_DEPENDENCIES
            RESOLVED_DEPENDENCIES_VAR deps
            UNRESOLVED_DEPENDENCIES_VAR confdeps
            EXECUTABLES src/${Brewken_EXECUTABLE}.exe
            PRE_EXCLUDE_REGEXES \"^api-ms-win.*\$|^ext-ms.*\$\"
            POST_EXCLUDE_REGEXES \".*system32.*\"
            DIRECTORIES ${Mingw_Path}
         )
         #message(\"Installing: \${deps}\")
         #message(\"Not found: \${confdeps}\")
         FOREACH (fdep IN LISTS deps)
            message( \"\${fdep}\" )
            file( INSTALL
               DESTINATION bin
               TYPE SHARED_LIBRARY
               FOLLOW_SYMLINK_CHAIN
               FILES \"\${fdep}\")
         ENDFOREACH()
      ")
   ENDIF()

# Install the translations.
INSTALL(FILES ${QM_FILES}
        DESTINATION "${DATAPATH}/translations_qm"
        COMPONENT ${DATA_INSTALL_COMPONENT} )

# http://pmarinc-tidylib.googlecode.com/hg/src/Sigil/CMakeLists.txt?r=8276c61e05bc385d5ffbcc58e1f007f84b0c52df
IF( APPLE )
   SET ( BOOST_GNU_SOURCE_FLAG "-D_GNU_SOURCE" )
   SET ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${BOOST_GNU_SOURCE_FLAG}" )
   # make addframeworks should copy the Qt libraries to the app.
   SET(QT_BINARY_DIR "${_qt5Core_install_prefix}/bin")
   ADD_CUSTOM_TARGET(
      addframeworks ALL
      COMMAND ${QT_BINARY_DIR}/macdeployqt "Brewken.app" -dmg
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      DEPENDS ${Brewken_EXECUTABLE}
   )
ENDIF()
