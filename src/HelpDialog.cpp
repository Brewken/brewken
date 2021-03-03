/**
 * HelpDialog.cpp is part of Brewken, and is copyright the following authors 2021:
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
 */
#include "HelpDialog.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QTextStream>
#include <QLabel>

#include "config.h"

#include "Brewken.h"
#include "Logging.h"


// This private implementation class holds all private non-virtual members of BeerXML
class HelpDialog::impl {
public:

   /**
    * Constructor
    *
    * It should be safe to pass in a reference to HelpDialog from its constructor because there is nothing else in that
    * class to initialise by the time this pimpl constructor is being called.
    */
   impl(HelpDialog & helpDialog) : label{ new QLabel{} },
                                   layout{ new QVBoxLayout{&helpDialog} } {
      // Create the layout
      this->layout->addWidget(this->label.get());

      // We want they hyperlinks in the text to be clickable (opening in the user's default web browser)
      this->label->setOpenExternalLinks(true);

      this->setText(helpDialog);

      return;
   }

   /**
    * Destructor
    */
   ~impl() = default;

   /**
    * Set the text.  This is a separate function because we want to be able to redisplay in a different language.
    */
   void setText(HelpDialog & helpDialog) {
      QString mainText;
      QTextStream mainTextAsStream{&mainText};
      mainTextAsStream <<
         "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">"
         "<html>"
         "<head>"
         "<style type=\"text/css\">"
         "</style>"
         "</head>"
         ""
         "<h1>Brewken</h1>"
         "version " << VERSIONSTRING << " " << tr("for") << " " << QSysInfo::prettyProductName() <<
         "<h2>" << tr("Online Help") << "</h2>"
         "<p>" <<
         tr("<p>The Brewken wiki is at ") << "<a href=\"https://github.com/Brewken/brewken/wiki\">https://github.com/Brewken/brewken/wiki</a>.</p>"
         "<p>" <<
         tr("If you find a bug, or have an idea for an enhancement, please raise an issue at ") << "<br/>"
         "<a href=\"https://github.com/Brewken/brewken/issues\">https://github.com/Brewken/brewken/issues</a>.<br/>"
         "<br/>" <<
         tr("<em>If it's Brewken, we can fix it...</em>") <<
         "</p>"
         "<h2>" << tr("Your Data") << "</h2>"
         "<p>" <<
         tr("Important data is stored in the following folders (which are configurable via the 'Tools > Options' menu):") <<
         "</p>"
         "<ul>"
         "<li>" << tr("Configuration:") << " <pre>" << this->makeClickableDirLink(Brewken::getConfigDir().canonicalPath()) << "</pre></li>"
         "<li>" << tr("Data:") << " <pre>" << this->makeClickableDirLink(Brewken::getUserDataDir().canonicalPath()) << "</pre></li>"
         "</ul>"
         "<p>" << tr("It is a good idea to take regular backups of the contents of these directories.") << "</p>"
         "<h2>" << tr("Log files") << "</h2>"
         "<p>" <<
         tr("Log files are in the following folder (this is also configurable via the 'Tools > Options' menu):") <<
         "<ul>"
         "<li><pre>" << this->makeClickableDirLink(Logging::getDirectory().absolutePath()) << "</pre></li>"
         "</ul>" <<
         tr("The contents of the log files can be helpful for diagnosing problems.") <<
         "</p>"
         "</html>";
      this->label->setText(mainText);

      helpDialog.setWindowTitle(tr("Help"));
      return;
   }

   /**
    * Given a path to a directory, make a link that will allow the the user to open that directory in
    * Explorer/Finder/Dolphin/etc
    */
   QString makeClickableDirLink(QString const & canonicalPath) {
      return QString{"<a href=\"file:///%1\">%1</a>"}.arg(canonicalPath);
   }

   std::unique_ptr<QLabel> label;
   std::unique_ptr<QVBoxLayout> layout;


};


HelpDialog::HelpDialog(QWidget * parent) : QDialog(parent),
                                           pimpl{ new impl{*this} } {
   this->setObjectName("helpDialog");
   this->pimpl->setText(*this);
   return;
}

// See https://herbsutter.com/gotw/_100/ for why we need to explicitly define the destructor here (and not in the
// header file)
HelpDialog::~HelpDialog() = default;


void HelpDialog::changeEvent(QEvent* event) {
   if (event->type() == QEvent::LanguageChange) {
      this->pimpl->setText(*this);
   }
   // Pass the event down to the base class
   QDialog::changeEvent(event);
   return;
}
