/*======================================================================================================================
 * ImportExport.cpp is part of Brewken, and is copyright the following authors 2013-2022:
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
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
#include "ImportExport.h"

#include <QFile>
#include <QMessageBox>
#include <QObject>

#include "BtTreeView.h"
#include "json/BeerJson.h"
#include "MainWindow.h"
#include "xml/BeerXml.h"

class Equipment;
class Fermentable;
class Hop;
class Misc;
class Recipe;
class Style;
class Water;
class Yeast;

namespace {
   enum class ImportOrExport {
      EXPORT,
      IMPORT
   };

   /**
    * \brief Display a file dialog for selecting file(s) for reading / writing
    */
   std::optional<QStringList> selectFiles(ImportOrExport const importOrExport) {
      //
      // Set up the file chooser dialog.  In previous versions of the code, this was created once and reused every time
      // we want to open a file.  The advantage of that is that, on subsequent uses, the file dialog is going to open
      // wherever you navigated to when you last opened a file.  However, as at 2020-12-30, there is a known bug in Qt
      // (https://bugreports.qt.io/browse/QTBUG-88971) which means you cannot make a QFileDialog "forget" previous
      // files you have selected with it.  So each time you you show it, the subsequent list returned from
      // selectedFiles() is actually all files _ever_ selected with this dialog object.  (The bug report is a bit bare
      // bones, but https://forum.qt.io/topic/121235/qfiledialog-has-memory has more detail.)
      //
      // Our workaround is to use a new QFileDialog each time, and manually keep track of the current directory.  This
      // also has the advantage that we remember the same directory for both reading and writing.
      //
      static QString fileChooserDirectory{QDir::homePath()};
      QFileDialog fileChooser{&MainWindow::instance(),
                              importOrExport == ImportOrExport::IMPORT ? QObject::tr("Open") : QObject::tr("Save"),
                              fileChooserDirectory,
                              QObject::tr("BeerJSON files (*.json);;BeerXML files (*.xml)")};
      fileChooser.setViewMode(QFileDialog::List);
      if (importOrExport == ImportOrExport::IMPORT) {
         fileChooser.setAcceptMode(QFileDialog::AcceptOpen);
         fileChooser.setFileMode(QFileDialog::ExistingFiles);
      } else {
         Q_ASSERT(importOrExport == ImportOrExport::EXPORT);
         fileChooser.setAcceptMode(QFileDialog::AcceptSave);
         fileChooser.setFileMode(QFileDialog::AnyFile);
         // If the user doesn't specify a suffix
         fileChooser.setDefaultSuffix(QString("xml"));
         // TBD what's the difference between filter and nameFilter?
//         QString filterStr = "BeerXML files (*.xml)";
//         fileChooser.setNameFilter(filterStr);
      }

      if (!fileChooser.exec() ) {
         // User clicked cancel, so nothing more to do
         return std::nullopt;
      }

      qDebug() << Q_FUNC_INFO << "Selected " << fileChooser.selectedFiles().length() << " files";
      qDebug() << Q_FUNC_INFO << "Directory " << fileChooser.directory();

      // Remember the directory for next time
      fileChooserDirectory = fileChooser.directory().canonicalPath();

      return fileChooser.selectedFiles();
   }

   /**
    * \brief Show a success/failure message to the user after we attempted to import one or more BeerXML files
    */
   void importExportMsg(ImportOrExport const importOrExport,
                        QString const & fileName,
                        bool succeeded,
                        QString const & userMessage) {
      // This will allow us to drop the directory path to the file, as it is often long and makes the message box a
      // "wall of text" that will put a lot of users off.
      QFileInfo fileInfo(fileName);

      QString messageBoxTitle{succeeded ? QObject::tr("Success!") : QObject::tr("ERROR")};
      QString messageBoxText;
      if (succeeded) {
         // The userMessage parameter will tell how many files were imported/exported and/or skipped (as duplicates)
         // Do separate messages for import and export as it makes translations easier
         if (ImportOrExport::IMPORT == importOrExport) {
            messageBoxText = QString(
               QObject::tr("Successfully read \"%1\"\n\n%2").arg(fileInfo.fileName()).arg(userMessage)
            );
         } else {
            messageBoxText = QString(
               QObject::tr("Successfully wrote \"%1\"\n\n%2").arg(fileInfo.fileName()).arg(userMessage)
            );
         }
      } else {
         if (ImportOrExport::IMPORT == importOrExport) {
            messageBoxText = QString(
               QObject::tr("Unable to import data from \"%1\"\n\n"
                           "%2\n\n"
                           "Log file may contain more details.").arg(fileInfo.fileName()).arg(userMessage)
            );
         } else {
            // Some write errors (eg nothing to export) are before the filename was chosen (in which case the name will
            // be blank).
            if (fileName == "") {
               messageBoxText = QString("%2").arg(userMessage);
            } else {
               messageBoxText = QString(
                  QObject::tr("Unable to write data to \"%1\"\n\n"
                              "%2\n\n"
                              "Log file may contain more details.").arg(fileInfo.fileName()).arg(userMessage)
               );
            }
         }
      }
      qDebug() << Q_FUNC_INFO << "Message box text : " << messageBoxText;
      QMessageBox msgBox{succeeded ? QMessageBox::Information : QMessageBox::Critical,
                         messageBoxTitle,
                         messageBoxText};
      msgBox.exec();
      return;
   }
}

void ImportExport::importFromFiles() {
   auto selectedFiles = selectFiles(ImportOrExport::IMPORT);
   if (!selectedFiles) {
      return;
   }

   for (QString filename : *selectedFiles) {
      //
      // I guess if the user were importing a lot of files in one go, it might be annoying to have a separate result
      // message for each one, but TBD whether that's much of a use case.  For now, we keep things simple.
      //
      qDebug() << Q_FUNC_INFO << "Importing " << filename;
      QString userMessage;
      QTextStream userMessageAsStream{&userMessage};
      bool succeeded = false;
      if (filename.endsWith("json", Qt::CaseInsensitive)) {
         succeeded = BeerJson::import(filename, userMessageAsStream);
      } else if (filename.endsWith("xml", Qt::CaseInsensitive)) {
         succeeded = BeerXML::getInstance().importFromXML(filename, userMessageAsStream);
      } else {
         qInfo() << Q_FUNC_INFO << "Don't understand file extension on" << filename << "so ignoring!";
      }
      qDebug() << Q_FUNC_INFO << "Import " << (succeeded ? "succeeded" : "failed");
      importExportMsg(ImportOrExport::IMPORT, filename, succeeded, userMessage);
   }

   MainWindow::instance().showChanges();

   return;
}


void ImportExport::exportToFile(QList<Recipe      const *> const * recipes,
                                QList<Equipment   const *> const * equipments,
                                QList<Fermentable const *> const * fermentables,
                                QList<Hop         const *> const * hops,
                                QList<Misc        const *> const * miscs,
                                QList<Style       const *> const * styles,
                                QList<Water       const *> const * waters,
                                QList<Yeast       const *> const * yeasts) {
   // It's the caller's responsibility to ensure that at least one list is supplied and that at least one of the
   // supplied lists is non-empty
   Q_ASSERT((recipes      && recipes     ->size() > 0) ||
            (equipments   && equipments  ->size() > 0) ||
            (fermentables && fermentables->size() > 0) ||
            (hops         && hops        ->size() > 0) ||
            (miscs        && miscs       ->size() > 0) ||
            (styles       && styles      ->size() > 0) ||
            (waters       && waters      ->size() > 0) ||
            (yeasts       && yeasts      ->size() > 0));

   auto selectedFiles = selectFiles(ImportOrExport::EXPORT);
   if (!selectedFiles) {
      return;
   }
   QString filename = (*selectedFiles)[0];

   QString userMessage;
   QTextStream userMessageAsStream{&userMessage};

   // Destructor will close the file if nec when we exit the function
   QFile outFile;
   outFile.setFileName(filename);

   if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      qWarning() << Q_FUNC_INFO << "Could not open" << filename << "for writing.";
      return;
   }

   if (filename.endsWith("json", Qt::CaseInsensitive)) {
      BeerJson::Exporter exporter(outFile, userMessageAsStream);
      if (hops         && hops        ->size() > 0) { exporter.add(*hops        ); }
      if (fermentables && fermentables->size() > 0) { exporter.add(*fermentables); }
      if (yeasts       && yeasts      ->size() > 0) { exporter.add(*yeasts      ); }
      if (miscs        && miscs       ->size() > 0) { exporter.add(*miscs       ); }
      if (waters       && waters      ->size() > 0) { exporter.add(*waters      ); }
      if (styles       && styles      ->size() > 0) { exporter.add(*styles      ); }
      if (recipes      && recipes     ->size() > 0) { exporter.add(*recipes     ); }
      if (equipments   && equipments  ->size() > 0) { exporter.add(*equipments  ); }

      exporter.close();
      return;
   }

   if (filename.endsWith("xml", Qt::CaseInsensitive)) {
      BeerXML & bxml = BeerXML::getInstance();
      // The slightly non-standard-XML format of BeerXML means the common bit (which gets written by createXmlFile) is
      // just at the start and there is no "closing" bit to write after we write all the data.
      bxml.createXmlFile(outFile);

      //
      // Not that it matters, but the order things are listed in the BeerXML 1.0 spec is:
      //    HOPS
      //    FERMENTABLES
      //    YEASTS
      //    MISCS
      //    WATERS
      //    STYLES
      //    MASH_STEPS
      //    MASHS
      //    RECIPES
      //    EQUIPMENTS
      //
      if (hops         && hops        ->size() > 0) { bxml.toXml(*hops,         outFile); }
      if (fermentables && fermentables->size() > 0) { bxml.toXml(*fermentables, outFile); }
      if (yeasts       && yeasts      ->size() > 0) { bxml.toXml(*yeasts,       outFile); }
      if (miscs        && miscs       ->size() > 0) { bxml.toXml(*miscs,        outFile); }
      if (waters       && waters      ->size() > 0) { bxml.toXml(*waters,       outFile); }
      if (styles       && styles      ->size() > 0) { bxml.toXml(*styles,       outFile); }
      if (recipes      && recipes     ->size() > 0) { bxml.toXml(*recipes,      outFile); }
      if (equipments   && equipments  ->size() > 0) { bxml.toXml(*equipments,   outFile); }

      return;
   }

   qInfo() << Q_FUNC_INFO << "Don't understand file extension on" << filename << "so ignoring!";

   return;
}
