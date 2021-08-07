/*======================================================================================================================
 * AlcoholTool.cpp is is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Matt Young <mfsy@yahoo.com>
 *   • Ryan Hoobler <rhoob@yahoo.com>
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
#include "AlcoholTool.h"

#include <QEvent>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QSpacerItem>
#include <QVBoxLayout>
#include <QWidget>

#include "Algorithms.h"
#include "Brewken.h"
#include "BtLineEdit.h"
#include "Unit.h"

// This private implementation class holds all private non-virtual members of AlcoholTool
class AlcoholTool::impl {
public:
   /**
    * Constructor
    */
   impl(AlcoholTool & self) :
      self              {self},
      pushButton_convert{new QPushButton  (&self)},
      label_og          {new QLabel       (&self)},
      input_og          {new BtDensityEdit(&self)},
      label_fg          {new QLabel       (&self)},
      input_fg          {new BtDensityEdit(&self)},
      label_result      {new QLabel       (&self)},
      output_result     {new QLabel       (&self)},
      hLayout           {new QHBoxLayout  (&self)},
      formLayout        {new QFormLayout  (&self)},
      vLayout           {new QVBoxLayout  (&self)},
      verticalSpacer    {new QSpacerItem{20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding}},
      verticalSpacer2   {new QSpacerItem{20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding}},
      verticalSpacer3   {new QSpacerItem{20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding}} {
      this->advancedInputsVisible = true; // .:TODO:. Pull this from persisetent settings
      this->doLayout();
      this->output_result->setText("%");
      return;
   }

   /**
    * Destructor
    *
    * Not much for us to do in the destructor.  Per https://doc.qt.io/qt-5/objecttrees.html, "When you create a QObject
    * with another object as parent, it's added to the parent's children() list, and is deleted when the parent is."
    *
    * I think, for similar reasons, we also do not need to delete QSpacerItem objects after they have been added to a
    * layout.
    */
   ~impl() = default;

   void doLayout() {

      this->input_og->setMinimumSize(QSize(80, 0));
      this->input_og->setProperty("forcedUnit", QVariant(QStringLiteral("displaySG")));

      this->input_fg->setMinimumSize(QSize(80, 0));
      this->input_fg->setProperty("forcedUnit", QVariant(QStringLiteral("displaySG")));

      this->label_result->setObjectName(QStringLiteral("label_results"));
      this->label_result->setContextMenuPolicy(Qt::CustomContextMenu);

      this->output_result->setMinimumSize(QSize(80, 0));
      this->output_result->setObjectName(QStringLiteral("output_result"));

      this->formLayout->setWidget(0, QFormLayout::LabelRole, label_og);
      this->formLayout->setWidget(0, QFormLayout::FieldRole, input_og);
      this->formLayout->setWidget(1, QFormLayout::LabelRole, label_fg);
      this->formLayout->setWidget(1, QFormLayout::FieldRole, input_fg);
      this->formLayout->setWidget(2, QFormLayout::LabelRole, label_result);
      this->formLayout->setWidget(2, QFormLayout::FieldRole, output_result);

      this->formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

      this->pushButton_convert->setAutoDefault(false);
      this->pushButton_convert->setDefault(true);

      this->vLayout->addItem(verticalSpacer);
      this->vLayout->addWidget(pushButton_convert);
      this->vLayout->addItem(verticalSpacer2);
      this->vLayout->addItem(verticalSpacer3);

      this->hLayout->addLayout(formLayout);
      this->hLayout->addLayout(vLayout);

      this->retranslateUi();
      return;
   }

   void retranslateUi() {
      self.setWindowTitle(tr("Alcohol Tool"));
      this->label_og->setText(tr("OG Reading"));
      this->label_result->setText(tr("ABV"));
      this->label_fg->setText(tr("FG Reading"));
      this->pushButton_convert->setText(tr("Calculate"));

   #ifndef QT_NO_TOOLTIP
      qDebug() << Q_FUNC_INFO << "Setting tooltips and What's This help texts";
      this->input_og->setToolTip(tr("Initial Reading"));
      this->input_fg->setToolTip(tr("Final Reading"));
      this->output_result->setToolTip(tr("Result"));
      this->output_result->setWhatsThis(
         tr("Calculated according to the formula set by the UK Laboratory of the Government Chemist")
      );
   #else
      qDebug() << Q_FUNC_INFO << "Tooltips not enabled in this build";
   #endif
      return;
   }

   // Member variables for impl
   AlcoholTool   & self;
   QPushButton   * pushButton_convert;
   QLabel        * label_og;
   BtDensityEdit * input_og;
   QLabel        * label_fg;
   BtDensityEdit * input_fg;
   QLabel        * label_result;
   QLabel        * output_result;
   QHBoxLayout   * hLayout;
   QFormLayout   * formLayout;
   QVBoxLayout   * vLayout;
   QSpacerItem   * verticalSpacer;
   QSpacerItem   * verticalSpacer2;
   QSpacerItem   * verticalSpacer3;
   bool            advancedInputsVisible;
};

AlcoholTool::AlcoholTool(QWidget* parent) : QDialog(parent),
                                            pimpl{std::make_unique<impl>(*this)} {
   connect(this->pimpl->pushButton_convert, SIGNAL(clicked()), this, SLOT(convert()) );
   connect(this->pimpl->input_og, &BtLineEdit::textModified, this, &AlcoholTool::convert );
   connect(this->pimpl->input_fg, &BtLineEdit::textModified, this, &AlcoholTool::convert );

   return;
}

AlcoholTool::~AlcoholTool() = default;

void AlcoholTool::convert() {
   double og = this->pimpl->input_og->toSI();
   double fg = this->pimpl->input_fg->toSI();
   if (og != 0.0 && fg != 0.0 && og >= fg) {
      double abv = Algorithms::abvFromOgAndFg(og, fg);
      //
      // We want to show two decimal places so that the user has the choice about rounding.  In the UK, for instance,
      // for tax purposes, it is acceptable to truncate (rather than round) ABV to 1 decimal place, eg if your ABV is
      // 4.19% you declare it as 4.1% not 4.2%.
      //
      // Note that we do not use QString::number() as it does not honour the user's locale and instead always uses
      // QLocale::C, i.e., English/UnitedStates
      //
      // So, if ABV is, say, 5.179% the call to QLocale::toString() below will correctly round it to 5.18% and the user
      // can decide whether to use 5.1% or 5.2% on labels etc.
      //
      this->pimpl->output_result->setText(QLocale().toString(abv, 'f', 2).append("%"));
   } else {
      this->pimpl->output_result->setText("? %");
   }
   return;
}

void AlcoholTool::changeEvent(QEvent* event) {
   if (event->type() == QEvent::LanguageChange) {
      this->pimpl->retranslateUi();
   }
   // Let base class do its work too
   this->QDialog::changeEvent(event);
   return;
}
