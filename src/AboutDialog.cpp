/**
 * AboutDialog.cpp is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Matt Young <mfsy@yahoo.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
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
#include "AboutDialog.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSpacerItem>
#include <QVBoxLayout>
#include <QWidget>

#include "config.h"

AboutDialog::AboutDialog(QWidget * parent) :
   QDialog(parent),
   label(0) {
   setObjectName("aboutDialog");
   doLayout();

   // Do not translate this. It is important that the copyright/license
   // text is not altered.
   label->setText(
      QString::fromUtf8(
         "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">"
         "<html>"
         " <head>"
         "  <style type=\"text/css\">"
         "  </style>"
         " </head>"
         ""
         " <h1>Brewken %1</h1>"
         " <p>"
         "  Brewken, free software for developing beer recipes, is copyright 2009-2021<br/>"
         "  by the following developers:"
         " </p>"
         " <ul>"
         "  <li>Adam Hawes &lt;ach@hawes.net.au&gt;</li>"
         "  <li>Aidan Roberts &lt;aidanr67@gmail.com&gt;</li>"
         "  <li>A.J. Drobnich &lt;aj.drobnich@gmail.com&gt;</li>"
         "  <li>Blair Bonnett &lt;blair.bonnett@gmail.com&gt;</li>"
         "  <li>Brian Rower &lt;brian.rower@gmail.com&gt;</li>"
         "  <li>Carles Muñoz Gorriz &lt;carlesmu@internautas.org&gt;</li>"
         "  <li>Charles Fourneau &lt;plut0nium&gt;</li>"
         "  <li>Chris Hamilton &lt;marker5a@gmail.com&gt;</li>"
         "  <li>Chris Pavetto &lt;chrispavetto@gmail.com&gt;</li>"
         "  <li>Chris Speck &lt;cgspeck@gmail.com&gt;</li>"
         "  <li>Dan Cavanagh &lt;dan@dancavanagh.com&gt;</li>"
         "  <li>Daniel Moreno &lt;danielm5@users.noreply.github.com&gt;</li>"
         "  <li>Daniel Pettersson &lt;pettson81@gmail.com&gt;</li>"
         "  <li>David Grundberg &lt;individ@acc.umu.se&gt;</li>"
         "  <li>Eric Tamme &lt;etamme@gmail.com&gt;</li>"
         "  <li>Gregg Meess &lt;Daedalus12@gmail.com&gt;</li>"
         "  <li>Greg Greenaae &lt;ggreenaae@gmail.com&gt;</li>"
         "  <li>Idar Lund &lt;idarlund@gmail.com&gt;</li>"
         "  <li>Jamie Daws &lt;jdelectronics1@gmail.com&gt;</li>"
         "  <li>Jean-Baptiste Wons &lt;wonsjb@gmail.com&gt;</li>"
         "  <li>Jeff Bailey &lt;skydvr38@verizon.net&gt;</li>"
         "  <li>Jerry Jacobs &lt;jerry@xor-gate.org&gt;</li>"
         "  <li>Jonatan Pålsson &lt;jonatan.p@gmail.com&gt;</li>"
         "  <li>Jonathon Harding &lt;github@jrhardin.net&gt;</li>"
         "  <li>Julein &lt;j2bweb@gmail.com&gt;</li>"
         "  <li>Julian Volodia &lt;julianvolodia@gmail.com&gt;</li>"
         "  <li>Kregg Kemper &lt;gigatropolis@yahoo.com&gt;</li>"
         "  <li>Luke Vincent &lt;luke.r.vincent@gmail.com&gt;</li>"
         "  <li>Mark de Wever &lt;koraq@xs4all.nl&gt;</li>"
         "  <li>Markus Mårtensson &lt;mackan.90@gmail.com&gt;</li>"
         "  <li>Matt Anderson &lt;matt.anderson@is4s.com&gt;</li>"
         "  <li>Mattias Måhl &lt;mattias@kejsarsten.com&gt;</li>"
         "  <li>Matt Young &lt;mfsy@yahoo.com&gt;</li>"
         "  <li>Maxime Lavigne &lt;duguigne@gmail.com&gt;</li>"
         "  <li>Medic Momcilo &lt;medicmomcilo@gmail.com&gt;</li>"
         "  <li>Mike Evans &lt;mikee@saxicola.co.uk&gt;</li>"
         "  <li>Mik Firestone &lt;mikfire@gmail.com&gt;</li>"
         "  <li>Mikhail Gorbunov &lt;mikhail@sirena2000.ru&gt;</li>"
         "  <li>Mitch Lillie &lt;mitch@mitchlillie.com&gt;</li>"
         "  <li>Peter Buelow &lt;goballstate@gmail.com&gt;</li>"
         "  <li>Philip Greggory Lee &lt;rocketman768@gmail.com&gt;</li>"
         "  <li>Piotr Przybyla &lt;przybysh&gt;</li>"
         "  <li>Priceless Brewing &lt;shadowchao99@gmail.com&gt;</li>"
         "  <li>Rob Taylor &lt;robtaylor@floopily.org&gt;</li>"
         "  <li>Ryan Hoobler &lt;rhoob@yahoo.com&gt;</li>"
         "  <li>Samuel Östling &lt;MrOstling@gmail.com&gt;</li>"
         "  <li>Scott Peshak &lt;scott@peshak.net&gt;</li>"
         "  <li>Ted Wright &lt;tedwright@users.sourceforge.net&gt;</li>"
         "  <li>Théophane Martin &lt;theophane.m@gmail.com&gt;</li>"
         "  <li>Tim Payne &lt;swstim@gmail.com&gt;</li>"
         "  <li>Tyler Cipriani &lt;tcipriani@wikimedia.org&gt;</li>"
         " </ul>"
         ""
         " <h2>License (GPLv3)</h2>"
         " <p>"
         "  Brewken is free software: you can redistribute it and/or modify<br/>"
         "  it under the terms of the GNU General Public License as published by<br/>"
         "  the Free Software Foundation, either version 3 of the License, or<br/>"
         "  (at your option) any later version."
         "  <br/><br/>"
         "  Brewken is distributed in the hope that it will be useful,<br/>"
         "  but WITHOUT ANY WARRANTY; without even the implied warranty of<br/>"
         "  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the<br/>"
         "  GNU General Public License for more details."
         "  <br/><br/>"
         "  You should have received a copy of the GNU General Public License<br/>"
         "  along with Brewken.  If not, see &lt;<a href=\"http://www.gnu.org/licenses/\">http://www.gnu.org/licenses/</a>&gt;"
         " </p>"
         ""
         " <h2>Source Code</h2>"
         " <p>"
         "  Brewken's source code is available at <a href=\"https://github.com/Brewken/brewken\">github.com/Brewken/brewken</a>"
         " </p>"
         "</html>"
      )
      .arg(VERSIONSTRING)
   );
   return;
}


void AboutDialog::changeEvent(QEvent* event) {
   if (event->type() == QEvent::LanguageChange) {
      retranslateUi();
   }
   QDialog::changeEvent(event);
   return;
}


void AboutDialog::doLayout()  {
   QVBoxLayout* verticalLayout = new QVBoxLayout(this);
      QScrollArea* scrollArea = new QScrollArea(this);
         label = new QLabel(scrollArea);
         scrollArea->setWidgetResizable(true);
         scrollArea->setWidget(label);
      QHBoxLayout* horizontalLayout = new QHBoxLayout;
         QSpacerItem* horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
         horizontalLayout->addItem(horizontalSpacer);
      verticalLayout->addWidget(scrollArea);
      verticalLayout->addLayout(horizontalLayout);
   this->retranslateUi();
   return;
}

void AboutDialog::retranslateUi() {
   setWindowTitle(tr("About Brewken"));
   return;
}
