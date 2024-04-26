/*======================================================================================================================
 * RecipeExtrasWidget.cpp is part of Brewken, and is copyright the following authors 2009-2024:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Peter Buelow <goballstate@gmail.com>
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
 =====================================================================================================================*/
#include "RecipeExtrasWidget.h"

#include <QDate>
#include <QWidget>
#include <QDebug>

#include "MainWindow.h"
#include "measurement/Unit.h"
#include "model/Recipe.h"
#include "utils/OptionalHelpers.h"

RecipeExtrasWidget::RecipeExtrasWidget(QWidget* parent) :
   QWidget(parent),
   recipe(nullptr),
   ratingChanged{false} {

   this->setupUi(this);

   // Note that label_primaryAge, label_secAge, label_tertAge, label_age are QLabel, not SmartLabel, as we're "forcing"
   // the measurement to be in days rather than allowing the usual units of PhysicalQuantity::Time
   SMART_FIELD_INIT(RecipeExtrasWidget, label_brewer     , lineEdit_brewer     , Recipe, PropertyNames::Recipe::brewer              );
   SMART_FIELD_INIT(RecipeExtrasWidget, label_asstBrewer , lineEdit_asstBrewer , Recipe, PropertyNames::Recipe::asstBrewer          );
///   SMART_FIELD_INIT(RecipeExtrasWidget, label_primaryAge , lineEdit_primaryAge , Recipe, PropertyNames::Recipe::primaryAge_days  , 0);
///   SMART_FIELD_INIT(RecipeExtrasWidget, label_primaryTemp, lineEdit_primaryTemp, Recipe, PropertyNames::Recipe::primaryTemp_c    , 1);
///   SMART_FIELD_INIT(RecipeExtrasWidget, label_secAge     , lineEdit_secAge     , Recipe, PropertyNames::Recipe::secondaryAge_days, 0);
///   SMART_FIELD_INIT(RecipeExtrasWidget, label_secTemp    , lineEdit_secTemp    , Recipe, PropertyNames::Recipe::secondaryTemp_c  , 1);
///   SMART_FIELD_INIT(RecipeExtrasWidget, label_tertAge    , lineEdit_tertAge    , Recipe, PropertyNames::Recipe::tertiaryAge_days , 0);
///   SMART_FIELD_INIT(RecipeExtrasWidget, label_tertTemp   , lineEdit_tertTemp   , Recipe, PropertyNames::Recipe::tertiaryTemp_c   , 1);
   SMART_FIELD_INIT(RecipeExtrasWidget, label_age        , lineEdit_age        , Recipe, PropertyNames::Recipe::age_days         , 0);
   SMART_FIELD_INIT(RecipeExtrasWidget, label_ageTemp    , lineEdit_ageTemp    , Recipe, PropertyNames::Recipe::ageTemp_c        , 1);
   SMART_FIELD_INIT(RecipeExtrasWidget, label_carbVols   , lineEdit_carbVols   , Recipe, PropertyNames::Recipe::carbonation_vols    );

   // See comment in model/Recipe.cpp about things we measure in days.  If we switched them from Dimensionless to Time,
   // we would need something like this
   // this->lineEdit_primaryAge->getSmartField().setForcedRelativeScale(Measurement::UnitSystem::RelativeScale::Large);
   // this->lineEdit_secAge    ->getSmartField().setForcedRelativeScale(Measurement::UnitSystem::RelativeScale::Large);
   // this->lineEdit_tertAge   ->getSmartField().setForcedRelativeScale(Measurement::UnitSystem::RelativeScale::Large);
   // this->lineEdit_age       ->getSmartField().setForcedRelativeScale(Measurement::UnitSystem::RelativeScale::Large);

   connect(this->lineEdit_age         , &SmartLineEdit::textModified               , this, &RecipeExtrasWidget::updateAge          );
   connect(this->lineEdit_ageTemp     , &SmartLineEdit::textModified               , this, &RecipeExtrasWidget::updateAgeTemp      );
   connect(this->lineEdit_asstBrewer  , &SmartLineEdit::textModified               , this, &RecipeExtrasWidget::updateBrewerAsst   );
   connect(this->lineEdit_brewer      , &SmartLineEdit::textModified               , this, &RecipeExtrasWidget::updateBrewer       );
   connect(this->lineEdit_carbVols    , &SmartLineEdit::textModified               , this, &RecipeExtrasWidget::updateCarbonation  );
///   connect(this->lineEdit_primaryAge  , &SmartLineEdit::textModified               , this, &RecipeExtrasWidget::updatePrimaryAge   );
///   connect(this->lineEdit_primaryTemp , &SmartLineEdit::textModified               , this, &RecipeExtrasWidget::updatePrimaryTemp  );
///   connect(this->lineEdit_secAge      , &SmartLineEdit::textModified               , this, &RecipeExtrasWidget::updateSecondaryAge );
///   connect(this->lineEdit_secTemp     , &SmartLineEdit::textModified               , this, &RecipeExtrasWidget::updateSecondaryTemp);
///   connect(this->lineEdit_tertAge     , &SmartLineEdit::textModified               , this, &RecipeExtrasWidget::updateTertiaryAge  );
///   connect(this->lineEdit_tertTemp    , &SmartLineEdit::textModified               , this, &RecipeExtrasWidget::updateTertiaryTemp );
   connect(this->spinBox_tasteRating  , QOverload<int>::of(&QSpinBox::valueChanged), this, &RecipeExtrasWidget::changeRatings      );
   connect(this->spinBox_tasteRating  , &QAbstractSpinBox::editingFinished         , this, &RecipeExtrasWidget::updateTasteRating  );
   connect(this->dateEdit_date        , &BtOptionalDateEdit::optionalDateChanged   , this, &RecipeExtrasWidget::updateDate         );
   connect(this->btTextEdit_notes     , &BtTextEdit::textModified                  , this, &RecipeExtrasWidget::updateNotes        );
   connect(this->btTextEdit_tasteNotes, &BtTextEdit::textModified                  , this, &RecipeExtrasWidget::updateTasteNotes   );
   return;
}

RecipeExtrasWidget::~RecipeExtrasWidget() = default;

void RecipeExtrasWidget::setRecipe(Recipe* rec) {
   if (this->recipe) {
      disconnect(this->recipe, 0, this, 0);
   }

   if (rec) {
      this->recipe = rec;
      connect(this->recipe, &NamedEntity::changed, this, &RecipeExtrasWidget::changed);
      this->showChanges();
   }
   return;
}

void RecipeExtrasWidget::updateBrewer() {
   if (!this->recipe) { return; }
   MainWindow::instance().doOrRedoUpdate(*recipe, TYPE_INFO(Recipe, brewer), lineEdit_brewer->text(), tr("Change Brewer"));
   return;
}

void RecipeExtrasWidget::updateBrewerAsst() {
   if (!this->recipe) { return; }
   if ( lineEdit_asstBrewer->isModified() ) {
      MainWindow::instance().doOrRedoUpdate(*recipe, TYPE_INFO(Recipe, asstBrewer), lineEdit_asstBrewer->text(), tr("Change Assistant Brewer"));
   }
   return;
}

void RecipeExtrasWidget::changeRatings([[maybe_unused]] int rating) { ratingChanged = true; }

void RecipeExtrasWidget::updateTasteRating() {
   if (!this->recipe) { return; }
   if ( ratingChanged )
   {
      MainWindow::instance().doOrRedoUpdate(*recipe, TYPE_INFO(Recipe, tasteRating), spinBox_tasteRating->value(), tr("Change Taste Rating"));
      ratingChanged = false;
   }
   return;
}

///void RecipeExtrasWidget::updatePrimaryAge() {
///   // See comment in model/Recipe.cpp for why age_days, primaryAge_days, secondaryAge_days, tertiaryAge_days properties
///   // are dimensionless
///   if (!this->recipe) { return; }
///   MainWindow::instance().doOrRedoUpdate(*recipe, TYPE_INFO(Recipe, primaryAge_days), lineEdit_primaryAge->getNonOptValue<double>(), tr("Change Primary Age"));
///}
///
///void RecipeExtrasWidget::updatePrimaryTemp() {
///   if (!this->recipe) { return; }
///   MainWindow::instance().doOrRedoUpdate(*recipe, TYPE_INFO(Recipe, primaryTemp_c), lineEdit_primaryTemp->getNonOptCanonicalQty(), tr("Change Primary Temp"));
///}
///
///void RecipeExtrasWidget::updateSecondaryAge() {
///   if (!this->recipe) { return; }
///   MainWindow::instance().doOrRedoUpdate(*recipe, TYPE_INFO(Recipe, secondaryAge_days), lineEdit_secAge->getNonOptValue<double>(), tr("Change Secondary Age"));
///}
///
///void RecipeExtrasWidget::updateSecondaryTemp() {
///   if (!this->recipe) { return; }
///   MainWindow::instance().doOrRedoUpdate(*recipe, TYPE_INFO(Recipe, secondaryTemp_c), lineEdit_secTemp->getNonOptCanonicalQty(), tr("Change Secondary Temp"));
///}
///
///void RecipeExtrasWidget::updateTertiaryAge() {
///   if (!this->recipe) { return; }
///   MainWindow::instance().doOrRedoUpdate(*recipe, TYPE_INFO(Recipe, tertiaryAge_days), lineEdit_tertAge->getNonOptValue<double>(), tr("Change Tertiary Age"));
///}
///
///void RecipeExtrasWidget::updateTertiaryTemp() {
///   if (!this->recipe) { return; }
///   MainWindow::instance().doOrRedoUpdate(*recipe, TYPE_INFO(Recipe, tertiaryTemp_c), lineEdit_tertTemp->getNonOptCanonicalQty(), tr("Change Tertiary Temp"));
///}

void RecipeExtrasWidget::updateAge() {
   if (!this->recipe) { return; }
   MainWindow::instance().doOrRedoUpdate(*recipe, TYPE_INFO(Recipe, age_days), lineEdit_age->getNonOptValue<double>(), tr("Change Age"));
}

void RecipeExtrasWidget::updateAgeTemp() {
   if (!this->recipe) { return; }
   MainWindow::instance().doOrRedoUpdate(*recipe, TYPE_INFO(Recipe, ageTemp_c), lineEdit_ageTemp->getNonOptCanonicalQty(), tr("Change Age Temp"));
}

void RecipeExtrasWidget::updateDate(std::optional<QDate> const date) {
   if (!this->recipe) { return; }

   qDebug() << Q_FUNC_INFO;
   if (date) {
      qDebug() << Q_FUNC_INFO << "date" << *date;
   }
   auto dateFromWidget = this->dateEdit_date->optionalDate();
   if (dateFromWidget) {
      qDebug() << Q_FUNC_INFO << "dateFromWidget" << *dateFromWidget;
   }

   // We have to be careful to avoid going round in circles here.  When we call
   // this->dateEdit_date->setOptionalDate(this->recipe->date()) to show the Recipe date in the UI, that will generate a
   // signal that ends up calling this function to say the date on the Recipe has changed, which it hasn't.
   if (date != this->recipe->date()) {
      qDebug() << Q_FUNC_INFO;
      MainWindow::instance().doOrRedoUpdate(*recipe, TYPE_INFO(Recipe, date), date, tr("Change Date"));
   }
   return;
}

void RecipeExtrasWidget::updateCarbonation() {
   if (!this->recipe) { return; }

   MainWindow::instance().doOrRedoUpdate(*recipe,
                                         TYPE_INFO(Recipe, carbonation_vols),
                                         lineEdit_carbVols->getNonOptCanonicalQty(),
                                         tr("Change Carbonation"));
}

void RecipeExtrasWidget::updateTasteNotes() {
   if (!this->recipe) { return; }

   MainWindow::instance().doOrRedoUpdate(*recipe,
                                         TYPE_INFO(Recipe, tasteNotes),
                                         btTextEdit_tasteNotes->toPlainText(),
                                         tr("Edit Taste Notes"));
}

void RecipeExtrasWidget::updateNotes() {
   if (!this->recipe) { return; }

   MainWindow::instance().doOrRedoUpdate(*recipe,
                                         TYPE_INFO(Recipe, notes),
                                         btTextEdit_notes->toPlainText(),
                                         tr("Edit Notes"));
}

void RecipeExtrasWidget::changed(QMetaProperty prop, QVariant /*val*/) {
   if (sender() != this->recipe) {
      return;
   }

   this->showChanges(&prop);
   return;
}

void RecipeExtrasWidget::saveAll() {
   //recObs->disableNotification();

   this->updateBrewer();
   this->updateBrewerAsst();
   this->updateTasteRating();
///   this->updatePrimaryAge();
///   this->updatePrimaryTemp();
///   this->updateSecondaryAge();
///   this->updateSecondaryTemp();
///   this->updateTertiaryAge();
///   this->updateTertiaryTemp();
   this->updateAge();
   this->updateAgeTemp();
   this->updateDate(dateEdit_date->optionalDate());
   this->updateCarbonation();
   this->updateTasteNotes();
   this->updateNotes();

   //recObs->reenableNotification();
   //recObs->forceNotify();

   this->hide();
   return;
}

void RecipeExtrasWidget::showChanges(QMetaProperty* prop) {
   if (!this->recipe) { return; }

   bool updateAll = true;
   QString propName;
   if (prop) {
      updateAll = false;
      propName = prop->name();
   }

   // I think we may be going circular here? LineEdit says "change is made",
   // which signals the widget which changes the db, which signals "change is
   // made" which signals the widget, which changes the LineEdit, which says
   // "change is made" ... rinse, lather, repeat
   // Unlike other editors, this one needs to read from recipe when it gets an
   // updateAll
   if (updateAll || propName == PropertyNames::Recipe::age_days         ) { this->lineEdit_age         ->setQuantity (recipe->age_days         ()); if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Recipe::ageTemp_c        ) { this->lineEdit_ageTemp     ->setQuantity (recipe->ageTemp_c        ()); if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Recipe::asstBrewer       ) { this->lineEdit_asstBrewer  ->setText     (recipe->asstBrewer       ()); if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Recipe::brewer           ) { this->lineEdit_brewer      ->setText     (recipe->brewer           ()); if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Recipe::carbonation_vols ) { this->lineEdit_carbVols    ->setQuantity (recipe->carbonation_vols ()); if (!updateAll) { return; } }
///   if (updateAll || propName == PropertyNames::Recipe::primaryAge_days  ) { this->lineEdit_primaryAge  ->setQuantity   (recipe->primaryAge_days  ()); if (!updateAll) { return; } }
///   if (updateAll || propName == PropertyNames::Recipe::primaryTemp_c    ) { this->lineEdit_primaryTemp ->setQuantity   (recipe->primaryTemp_c    ()); if (!updateAll) { return; } }
///   if (updateAll || propName == PropertyNames::Recipe::secondaryAge_days) { this->lineEdit_secAge      ->setQuantity   (recipe->secondaryAge_days()); if (!updateAll) { return; } }
///   if (updateAll || propName == PropertyNames::Recipe::secondaryTemp_c  ) { this->lineEdit_secTemp     ->setQuantity   (recipe->secondaryTemp_c  ()); if (!updateAll) { return; } }
///   if (updateAll || propName == PropertyNames::Recipe::tertiaryAge_days ) { this->lineEdit_tertAge     ->setQuantity   (recipe->tertiaryAge_days ()); if (!updateAll) { return; } }
///   if (updateAll || propName == PropertyNames::Recipe::tertiaryTemp_c   ) { this->lineEdit_tertTemp    ->setQuantity   (recipe->tertiaryTemp_c   ()); if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Recipe::tasteRating      ) { this->spinBox_tasteRating  ->setValue       (recipe->tasteRating      ()); if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Recipe::date             ) { this->dateEdit_date        ->setOptionalDate(recipe->date             ()); if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Recipe::notes            ) { this->btTextEdit_notes     ->setPlainText(recipe->notes            ()); if (!updateAll) { return; } }
   if (updateAll || propName == PropertyNames::Recipe::tasteNotes       ) { this->btTextEdit_tasteNotes->setPlainText(recipe->tasteNotes       ()); if (!updateAll) { return; } }

   return;
}
