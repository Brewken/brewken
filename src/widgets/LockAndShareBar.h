/*======================================================================================================================
 * widgets/LockAndShareBar.h is part of Brewken, and is copyright the following authors 2026:
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
 =====================================================================================================================*/
#ifndef WIDGETS_LOCKANDSHAREBAR_H
#define WIDGETS_LOCKANDSHAREBAR_H
#pragma once

#include <memory>

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QString>

class Recipe;

/**
 * \brief A bar shown at the top of the editor for something that can be shared between recipes and/or locked (ie, at
 *        the moment, \c Mash, \c Boil and \c Fermentation -- see \c model/LockablePropertyBase.h).  It tells the user
 *        which recipes use the item and whether they can change it, and offers buttons to make a copy for the current
 *        recipe and to lock or unlock the item.
 *
 *        This class is just the UI.  The logic is in \c EditorBase, which connects to our signals and calls \c setState.
 *
 *        Editors that use this need, in their .ui file, a \c LockAndShareBar called \c lockAndShareBar.
 */
class LockAndShareBar : public QFrame {
   Q_OBJECT

public:
   explicit LockAndShareBar(QWidget * parent = nullptr);
   virtual ~LockAndShareBar();

   /**
    * \param text What to tell the user about who uses the item and whether it can be changed
    * \param needsAttention \c true if the item is locked or shared, ie the user needs to take note of \c text before
    *                       making changes.  We make the bar more prominent in this case.
    * \param offerCopy Whether to show the "Make a copy for this recipe" button
    * \param itemLocked Whether the item's own locked flag is set.  Determines whether we show "Lock" or "Unlock…"
    */
   void setState(QString const & text, bool const needsAttention, bool const offerCopy, bool const itemLocked);

signals:
   void makeCopyClicked();
   void lockClicked();
   void unlockClicked();

private:
   QLabel      * m_text;
   QPushButton * m_makeCopy;
   QPushButton * m_lockOrUnlock;
   bool          m_itemLocked = false;
};

namespace LockAndShare {
   /**
    * \brief Call this before a tool (eg the Mash Wizard) makes changes to \c recipe's \c NE (eg its \c Mash) on the
    *        user's behalf.  If the \c NE is not used by any other recipe and is not locked (see
    *        \c RecipeUtils::lockReason), it is returned as-is.  Otherwise, we explain the situation to the user and
    *        offer to make a copy for \c recipe (see \c RecipeUtils::useCopyFor).
    *
    *        Explicitly instantiated for \c Mash.  (Add others as needed.)
    *
    * \param parent For the message box
    * \param recipe
    * \param whyChange Translated text explaining what the tool wants to do, eg "The Mash Wizard changes the amounts and
    *                  temperatures of the steps of the recipe's mash."
    *
    * \return The \c NE that the tool can change (which might be a new copy), or \c nullptr if \c recipe has no
    *         \c NE, or the user declined to make a copy, or \c recipe is locked (so we cannot change which \c NE it
    *         uses).
    */
   template<class NE>
   std::shared_ptr<NE> getChangeable(QWidget * parent, Recipe & recipe, QString const & whyChange);
}

#endif
