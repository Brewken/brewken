/*======================================================================================================================
 * BtLineEdit.h is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Mark de Wever <koraq@xs4all.nl>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mike Evans <mikee@saxicola.co.uk>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Scott Peshak <scott@peshak.net>
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
#ifndef BTLINEEDIT_H
#define BTLINEEDIT_H
#pragma once

#include  <optional>

#include <QLineEdit>
#include <QString>
#include <QWidget>

#include "BtFieldType.h"

class NamedEntity;

/*!
 * \class BtLineEdit
 *
 * \brief This class and its subclasses extends QLineEdit such that the Object handles all the unit transformation we
 *        do, instead of each dialog.
 *
 *        It makes the code much nicer and prevents more cut'n'paste code.
 *
 *        A \c BtLineEdit (or subclass thereof) will usually have a corresponding \c BtLabel (or subclass thereof).
 *        See comment in BtLabel.h for more details on the relationship between the two classes.
 */
class BtLineEdit : public QLineEdit {
   Q_OBJECT

public:
   /*!
    * \brief Initialize the BtLineEdit with the parent and do some things with the type
    *
    * \param parent - QWidget* to the parent object
    * \param fieldType the type of input field; if it is not \c NonPhysicalQuantity then we should be being called
    *                  from \c BtAmountEdit or a subclass thereof
    * \param defaultPrecision
    * \param maximalDisplayString - an example of the widest string this widget would be expected to need to display
    *
    * \todo Not sure if I can get the name of the widget being created.
    *       Not sure how to signal the parent to redisplay
    */
   BtLineEdit(QWidget* parent = nullptr,
              BtFieldType fieldType = NonPhysicalQuantity::String,
              int const defaultPrecision = 3,
              QString const & maximalDisplayString = "100.000 srm");

   virtual ~BtLineEdit();

   BtFieldType const getFieldType() const;

   /**
    * \brief Set the amount for a decimal field
    *
    * \param amount is the amount to display, but the field should be blank if this is \b std::nullopt
    * \param precision is how many decimal places to show.  If not specified, the default will be used.
    */
   void setText(std::optional<double> amount, std::optional<int> precision = std::nullopt);

   /**
    * \brief .:TBD:. Do we need this to be able to parse numbers out of strings, or just to set string text?
    */
   void setText(QString               amount, std::optional<int> precision = std::nullopt);

   /**
    * \brief Use this when you want to get the text as a number (and ignore any units or other trailling letters or
    *        symbols)
    */
   template<typename T> T getValueAs() const;

public slots:
   /**
    * \brief This slot receives the \c QLineEdit::editingFinished signal
    */
   void onLineChanged();

signals:
   /**
    * \brief Where we want "instant updates", this signal should be picked up by the editor or widget object using this
    *        input field so it can read the changed value and update the underlying data model.
    *
    *        Where we want to defer updating the underlying data model until the user clicks "Save" etc, then this
    *        signal will typically be ignored.
    */
   void textModified();

protected:
   BtFieldType fieldType;

   int const defaultPrecision;

   void calculateDisplaySize(QString const & maximalDisplayString);
   void setDisplaySize(bool recalculate = false);

   int desiredWidthInPixels;
};

//
// These are trivial specialisations of BtLineEdit that make it possible to use specific types of BtLineEdit in .ui
// files.  It's a bit of a sledgehammer way to pass in a constructor parameter but seems necessary because of
// limitations in Qt.
//
// AFAIK there is no way to pass constructor parameters to an object in a .ui file.  (If you want to do that, the advice
// seems to be to build the layout manually in C++ code.)
//
// Similarly, we might think to template BtLineEdit, but the Qt Meta-Object Compiler (moc) doesn't understand C++
// templates.  This means we can't template classes that need to use the Q_OBJECT macro (required for classes that
// declare their own signals and slots or that use other services provided by Qt's meta-object system).
//
// TODO: Kill BtGenericEdit
//
// TBD: Can we think of a more elegant way of handing, eg, different numbers of decimal places for %
//
class BtGenericEdit       : public BtLineEdit { Q_OBJECT public: BtGenericEdit      (QWidget* parent); };
class BtStringEdit        : public BtLineEdit { Q_OBJECT public: BtStringEdit       (QWidget* parent); };
class BtPercentageEdit    : public BtLineEdit { Q_OBJECT public: BtPercentageEdit   (QWidget* parent); };
class BtDimensionlessEdit : public BtLineEdit { Q_OBJECT public: BtDimensionlessEdit(QWidget* parent); };

#endif
