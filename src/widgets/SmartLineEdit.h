/*======================================================================================================================
 * widgets/SmartLineEdit.h is part of Brewken, and is copyright the following authors 2009-2023:
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
#ifndef WIDGETS_SMARTLINEEDIT_H
#define WIDGETS_SMARTLINEEDIT_H
#pragma once

#include <memory> // For PImpl

#include <QLineEdit>
#include <QString>
#include <QWidget>

#include "BtFieldType.h"
#include "utils/TypeLookup.h"
#include "UiAmountWithUnits.h"

class SmartLabel;

/*!
 * \class SmartLineEdit
 *
 * \brief Extends QLineEdit to handle unit transformations and formatting
 *
 *        A \c SmartLineEdit widget will usually have a corresponding \c SmartLabel. See comment in
 *        \c widgets/SmartLabel.h for more details on the relationship between the two classes.
 *
 *        Typically, each \c SmartLineEdit and \c SmartLabel instance are declared in a dialog's Qt Designer UI File
 *        (\c ui/hopEditor.ui).  After it is constructed, the needs to be configured via \c SmartLineEdit::init.
 *
 *        This two-step set-up is needed because AFAIK there is no way to pass constructor parameters to an object in a
 *        .ui file.  (If you want to do that, the advice seems to be to build the layout manually in C++ code.)  You can
 *        (and we do) set Qt properties from (the code generated from) a .ui file, but that means passing in everything
 *        as a string and losing a lot of compile-time checks.
 *
 *        Similarly, we might think to template \c SmartLineEdit, but the Qt Meta-Object Compiler (moc) doesn't
 *        understand C++ templates.  This means we can't template classes that need to use the \c Q_OBJECT macro
 *        (required for classes that declare their own signals and slots or that use other services provided by Qt's
 *        meta-object system).
 *
 *        Previously we had a class called \c BtLineEdit (on which this class is based) and used mostly trivial
 *        subclassing to determine the class behaviour.  This was a sort of trick to pass in a constructor parameter.
 *        Classes \c BtStringEdit, \c BtPercentageEdit, would all inherit from \c BtLnneEdit and all do no more than
 *        pass different parameters to the \c BtLineEdit constructor.  This sort of worked when we had relatively few
 *        \c Measurement::UnitSystem classes but was starting to get unwieldy when those were expanded in anticipation
 *        of lots of new field types for BeerJSON.  Also, it had the disadvantage that you had to think about field
 *        types when editing the .ui file rather than being able to leave such details to the corresponding .cpp file.
 */
class SmartLineEdit : public QLineEdit {
   Q_OBJECT

   // These properties are needed so that the .ui files can uniquely identify each field and so that, for fields
   // relating to physical quantities, the user can individually set the system of measurement and relative scale.
   //
   // They all pass through to UiAmountWithUnits
   Q_PROPERTY(QString configSection             READ getConfigSection                      WRITE setConfigSection                      STORED false)
   Q_PROPERTY(QString editField                 READ getEditField                          WRITE setEditField                          STORED false)
   Q_PROPERTY(QString forcedSystemOfMeasurement READ getForcedSystemOfMeasurementViaString WRITE setForcedSystemOfMeasurementViaString STORED false)
   Q_PROPERTY(QString forcedRelativeScale       READ getForcedRelativeScaleViaString       WRITE setForcedRelativeScaleViaString       STORED false)

public:

   SmartLineEdit(QWidget* parent = nullptr);
   virtual ~SmartLineEdit();

   /**
    * \brief This needs to be called before the object is used, typically in constructor of whatever editor is using the
    *        widget.  As well as passing in a bunch of info that cannot easily be given to the constructor (per comment
    *        above), it also ensures, if necessary, that the \c changedSystemOfMeasurementOrScale signal from the
    *        \c SmartLabel buddy is connected to the \c lineChanged slot of this \c SmartLineEdit.
    *
    *        This version is for a \c PhysicalQuantity (or \c Mixed2PhysicalQuantities) field.
    *
    *        Note, in reality, you actually use the \c SMART_LINE_EDIT_INIT macro (see below).
    *
    * \param name      Used only for logging, mostly for debugging.  (We have hundreds of instances of this object and
    *                  if we detect that one of them is misconfigured, it's very useful to be able to log which one!)
    *
    * \param typeInfo             Tells us what data type we use to store the contents of the field (when converted to
    *                             canonical units if it is a \c PhysicalQuantity) and, whether this is an optional
    *                             field (in which case we need to handle blank / empty string as a valid value).
    *
    * \param buddyLabel           Required if \c fieldType is \b not a \c NonPhysicalQuantity
    *
    * \param precision            For a decimal field, this determines the number of decimal places to show.  If not
    *                             specified, we show 3 decimal places.  TBD: IDK if one day we might need to be more
    *                             sophisticated about this, ie with number of decimal places dependent on the units that
    *                             the user has chosen, but for now we assume it's the same for everything.
    * \param maximalDisplayString Used for determining the width of the widget
    */
   void init(char       const * const   name,
             TypeInfo           const & typeInfo,
             SmartLabel               & buddyLabel,
             std::optional<int> const   precision = std::nullopt,
             QString            const & maximalDisplayString = "100.000 srm");

   /**
    * \brief As above, but for non-physical quantity such as \c NonPhysicalQuantity::Date,
    *        \c NonPhysicalQuantity::String, etc.
    *
    *        Note, in reality, you actually use the \c SMART_LINE_EDIT_INIT macro (see below).
    */
   void init(char       const * const   name,
             TypeInfo           const & typeInfo,
             std::optional<int> const   precision = std::nullopt,
             QString            const & maximalDisplayString = "100.000 srm");

   BtFieldType const getFieldType() const;

   TypeInfo const & getTypeInfo() const;

   /**
    * \brief If our field type is \b not \c NonPhysicalQuantity, then this returns the \c UiAmountWithUnits for handling
    *        units.  (It is a coding error to call this function if our field type \c is \c NonPhysicalQuantity.)
    */
   UiAmountWithUnits & getUiAmountWithUnits() const;

   /**
    * \brief If our field type is \b not \c NonPhysicalQuantity, then this returns the field converted to canonical
    *        units for the relevant \c Measurement::PhysicalQuantity.  (It is a coding error to call this function if
    *        our field type \c is \c NonPhysicalQuantity.)
    */
   Measurement::Amount toCanonical() const;

   /**
    * \brief Set the amount for a numeric field
    *
    * \param amount is the amount to display, but the field should be blank if this is \b std::nullopt
    */
   template<typename T> void setAmount(std::optional<T> amount);
   template<typename T> void setAmount(T                amount);

//   /**
//    * \brief Set the text for a non-numeric field
//    */
//   void setText(QString               amount, std::optional<int> precision = std::nullopt);

   /**
    * \brief Use this when you want to get the text as a number (and ignore any units or other trailling letters or
    *        symbols)
    */
   template<typename T> T getValueAs() const;

   //========================================== Property Getters and Setters ===========================================
   void    setEditField(QString editField);
   void    setConfigSection(QString configSection);
   void    setForcedSystemOfMeasurementViaString(QString systemOfMeasurementAsString);
   void    setForcedRelativeScaleViaString(QString relativeScaleAsString);

   QString getEditField() const;
   QString getConfigSection(); // This does lazy-loading so isn't const
   QString getForcedSystemOfMeasurementViaString() const;
   QString getForcedRelativeScaleViaString() const;
   //===================================================================================================================


public slots:
   /**
    * \brief This slot receives the \c QLineEdit::editingFinished signal
    */
   void onLineChanged();

   /**
    * \brief This is called from \c onLineChanged and also directly receives the
    *        \c SmartLabel::changedSystemOfMeasurementOrScale signal when the user has changed units (eg from US
    *        Customary to Metric).
    *
    *        In previous versions of the code, this was mostly referenced in .ui files.  One disadvantage of that
    *        approach was that the signal connections were only checked at run-time.
    */
   void lineChanged(PreviousScaleInfo previousScaleInfo);

signals:
   /**
    * \brief Where we want "instant updates", this signal should be picked up by the editor or widget object using this
    *        input field so it can read the changed value and update the underlying data model.
    *
    *        Where we want to defer updating the underlying data model until the user clicks "Save" etc, then this
    *        signal will typically be ignored.
    */
   void textModified();

private:
   // Private implementation details - see https://herbsutter.com/gotw/_100/
   class impl;
   std::unique_ptr<impl> pimpl;
};

/**
 * \brief Helper macro for \c SMART_LINE_EDIT_INIT.  Essentially does concatenation, using the magic, that, for the
 *        compiler, there is no difference between writing a string literal as:
 *           "foobarhumbug"
 *        and writing it as:
 *           "foo" "bar" "humbug"
 */
#define SLE_LOG_NAME(editorClass, fieldName) #editorClass "->" #fieldName

/**
 * \brief This macro saves a bit of copy-and-paste when invoking \c SmartLineEdit::init.  Eg instead of writing:
 *
 *           this->lineEdit_color->init("FermentableEditor->lineEdit_color", Fermentable::typeLookup.getType(PropertyNames::Fermentable::color_srm), *this->label_color, 0);
 *
 *        you write:
 *
 *           SMART_LINE_EDIT_INIT(FermentableEditor, Fermentable, lineEdit_color, PropertyNames::Fermentable::color_srm, *this->label_color, 0);
 *
 * \param editorClass The class name of the class holding the field we're initialising, eg \c HopEditor.  (In theory we
 *                    could pick this up via \c staticMetaObject.className(), but then we wouldn't be able to do the
 *                    macro concatenation here.)
 * \param modelClass The subclass of \c NamedEntity that we're editing.  Eg in \c HopEditor, this will be \c Hop
 * \param fieldName  The name of the member variable for this field, eg in \c HopEditor, this could be \c lineEdit_name,
 *                   \c lineEdit_alpha, etc.  Note that:
 *                     - We deliberately don't try to do anything clever with automatically inserting the "lineEdit_"
 *                       prefix, as this would make the code harder to read and search.
 *                     - It is intentional that field names sometimes differ slightly from property names.  The latter
 *                       always include their canonical unit names (eg \c PropertyNames::Fermentable::color_srm) whereas
 *                       the former do not (eg \c lineEdit_color) because the user can enter data in any supported
 *                       units.
 * \param propertyName The name of the property to which this field relates, eg in \c HopEditor, this could be
 *                     \c PropertyNames::NamedEntity::name, \c PropertyNames::Hop::alpha_pct, etc.  (Note, as above, we
 *                     intentionally do \b not automatically insert the \c PropertyNames:: prefix.)
 * \param ...  Any remaining arguments are passed through to \c SmartLineEdit::init in third position and above
 *             Note that the introduction of __VA_OPT__ in C++20 makes our lives easier here.
 */
#define SMART_LINE_EDIT_INIT(editorClass, modelClass, fieldName, propertyName, ...) \
   this-> fieldName ->init(SLE_LOG_NAME(editorClass, fieldName), modelClass ::typeLookup.getType(propertyName) __VA_OPT__(, __VA_ARGS__))

#endif
