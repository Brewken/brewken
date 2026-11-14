# Brewken Changelog

This change log is for high-level user-visible changes to Brewken, intended for consumption by the typical end-user.
Note however that we also process it into a Debian-compliant text change log, so we need to keep the format consistent.
In particular, the Release Timestamp section is needed as part of this (and you need to be meticulous about the date
format therein, otherwise you'll get, eg, no-changelogname-tag error from rpmlint).  You get problems if you set the
release timestamp to be a date in the future, and I'm guessing nobody cares about the exact time of day a release
happens, so I'm now setting it to a slightly arbitrary time early in the morning.

## Forthcoming in future releases
* Shopping Lists
* We'll list other new features here...

## v5.3.0
Several new features, and bug fixes for some long-standing issues.

### New Features
* Remainder of work on: Enhancement - allow per-field selection of displayed decimal places [1098](https://github.com/Brewtarget/brewtarget/issues/1098)
* Input values with more than 1 decimal round to 1 place [1142](https://github.com/Brewtarget/brewtarget/issues/1142)
* Additional methods for calculating IBU, including the SMPH IBU formula, which takes into account:
  * the boiling point set on the equipment (eg for brewing at altitude)
  * the expected clarity of the wort going into the fermenter (on the recipe's Boil tab)
  * what happens to the krausen (in the Fermentation Editor).
* The clarity actually seen on brew day can be recorded in the brew log.
* New fields (eg as mentioned above) are saved in dotBeer files (new in dotBeer 1.1.0).
* Mash water amounts and temperatures (strike water, other infusions, decoctions and sparge water) can now be adjusted to suit the recipe in the Mash Editor, replacing the Mash Wizard and Mash Designer.  Individual amounts and temperatures can be pinned so that only the rest are adjusted.  Optionally, the mash can adjust automatically whenever the recipe changes.
* Mash steps (and mash fields) that don't make sense for the recipe are highlighted
* Example mash profiles, which are locked so they can be shared between recipes
* Auto-Adjust on the Water tab works out the brewing salts needed to match the target water profile and, optionally, the acid needed to reach a target mash pH.  Optionally, the water agents can adjust automatically whenever the recipe changes.
* Clicking Auto-Adjust (for the mash or the water) can be undone and redone in one step
* Twelve style target water profiles (eg "Pale, Hoppy", "Black, Dry") that Auto-Adjust can match from RO water, and starting profiles for the tap water of eleven well-known cities
* Target mash pH is saved in dotBeer files (as `target_mash_pH` on recipes, new in dotBeer 1.1.0), and defaults to any pH set on the recipe's mash
* Optionally, estimated final gravity (and hence ABV etc) can take account of the mash profile -- temperatures, rest times and ramps, over any number of steps -- using a published kinetic model of starch conversion (Brandam et al 2003), as well as the diastatic power of the grain bill and the mash thickness (Brewtarget [779](https://github.com/Brewtarget/brewtarget/issues/779)).  Turn on "Use mash profile in FG calculation" on the recipe's Mash tab.  Existing recipes are unchanged; whether new and imported recipes use it is set in Options > Default Values.

### Bug Fixes
* Removing a hop addition sometimes causes an assert [1113](https://github.com/Brewtarget/brewtarget/issues/1113)
* Inventory updates don't show inside a recipe [756](https://github.com/Brewtarget/brewtarget/issues/756)
* Late Addition IBU calc wrong [438](https://github.com/Brewtarget/brewtarget/issues/438)
* Choosing equipment from the drop-down on the recipe tab now offers to set the recipe's batch size and pre-boil size to match it, as dragging the equipment onto the recipe already did (Brewtarget [531](https://github.com/Brewtarget/brewtarget/issues/531)).  Re-selecting the recipe's current equipment (or style, mash, etc) no longer adds an empty step to the undo history.
* Saving changes to equipment now offers to update the batch size and pre-boil size of recipes using it that still had the sizes it previously specified (Brewtarget [531](https://github.com/Brewtarget/brewtarget/issues/531)).  Previously, recipes' pre-boil sizes were changed without asking (even if the recipe's boil was locked or shared) and batch sizes were not changed at all.
* Closing the Mash, Boil or Fermentation Editor with Escape or the window's close button (or quitting with it open) no longer leaves behind the new, unsaved mash, boil or fermentation created by "Edit this mash" etc (Brewtarget [1182](https://github.com/Brewtarget/brewtarget/issues/1182))
* Clicking "New" in the Mash, Boil or Fermentation Editor while editing the new, unsaved one for a recipe now makes the new one for that recipe, rather than one not used by any recipe
* A whirlpool or hop stand step that stays above 81°C (eg 100°C → 92°C, or held at 92°C after a cooling step) is now treated as post-boil, rather than being counted as part of the boil (which made the boil time too long and treated hops added in it as boil additions).  The boil proper now ends as soon as the wort starts to cool, per the dotBeer 1.1.0 rules for boil steps, which also work at altitude.
* Editing a recipe's boil (eg the length of a boil step, the temperature of a hop stand, the pre-boil size, or adding or removing steps) now immediately updates the recipe's calculated volumes, gravities and IBUs.  Previously, these were only updated when something else in the recipe changed.  Also, changes to a boil, mash or fermentation created automatically for a new recipe are now picked up straight away, rather than only after restarting.
* Predicted mash pH on the Water tab was wildly wrong if the recipe had any lactic acid (or acidulated malt or phosphoric acid), as acid amounts were converted with the wrong units.  The mash pH model has also been corrected to scale by the weight of grain rather than mash thickness, and to count all specialty malts rather than just the last one.
* Water agent figures for gypsum (calcium overstated by about 26%), magnesium chloride (no chloride, and a possible crash) and pickling lime were wrong
* Water tab didn't update predicted mash pH when the grain bill changed, and could give wrong figures after switching between recipes

### Release Timestamp
Fri, 13 Nov 2026 05:03:00 +0100

## v5.2.3
More bug fixes.

### New Features
None

### Bug Fixes
* Fixed a regression in 5.2.2 that had the program looking in the wrong place for its config
* Second attempt at fixing "v5.2.0 not working on some older hardware" [1179](https://github.com/Brewtarget/brewtarget/issues/1179)

### Release Timestamp
Thu, 5 Nov 2026 05:02:03 +0100

## v5.2.2
Fix some key bugs in 5.2.0 release.  Start of work on improving decimal place handling.

### New Features
* Enhancement - allow per-field selection of displayed decimal places [1098](https://github.com/Brewtarget/brewtarget/issues/1098)

### Bug Fixes
* v5.2.0 not working on some older hardware [1179](https://github.com/Brewtarget/brewtarget/issues/1179)
* "Error Loading Data" and Recipe UI issue on 5.2.0 [1180](https://github.com/Brewtarget/brewtarget/issues/1180)

### Release Timestamp
Wed, 4 Nov 2026 05:02:02 +0100

## v5.2.1
Fix Mac packaging

### New Features
None

### Bug Fixes
* macOS release bundles fail to launch on clean Apple Silicon systems [1176](https://github.com/Brewtarget/brewtarget/issues/1176)

### Release Timestamp
Mon, 2 Nov 2026 05:02:01 +0100

## v5.2.0
This release changes the version numbering to match that of Brewtarget.  We include various fixes and enhancements, including:
* Improvements to inventory
* Equipment: new "Foundation Water" field for the mash tun, for the space below a false bottom or malt pipe (eg in
  all-in-one systems) that has to be filled before the grain is covered but which drains out with the wort
* Mash and lauter tun losses are now correctly allowed for in the mash wizard's total water
* BeerXML export and import now keep folders (via an extension tag)
* Importing a dotBeer file into a folder now keeps the subfolders given in the file
* When a recipe's pre-boil size is set from equipment whose default pre-boil volume was calculated (eg on import from
  dotBeer or BeerJSON), it is now adjusted for the recipe's boil time
* A boil step with no start/end temperature is now treated as being at boiling (Brewtarget
  [1165](https://github.com/Brewtarget/brewtarget/issues/1165))
* It is no longer possible to drag-and-drop a folder onto itself or one of its subfolders (Brewtarget
  [1157](https://github.com/Brewtarget/brewtarget/issues/1157))
* Exporting a hop, yeast or style with only one end of a range set (eg minimum but not maximum alpha acid) no longer
  creates a dotBeer or BeerJSON file that cannot be imported
* Yeast addition cell counts no longer need to be whole numbers

### Release Timestamp
Sat, 31 Oct 2026 05:02:00 +0100

## v2.0.0

Release 2.0
* Stock rotation support in inventory

### Release Timestamp
Fri, 12 Dec 2025 02:00:00 +0100

## v1.0.0

Release 1.0
* BeerJSON support

### Release Timestamp
Mon, 27 May 2024 05:30:01 +0100

## v0.1.0

New upstream release 0.1.0
* Initial release (Closes: #1)

### Release Timestamp
Sun, 28 Feb 2021 08:46:19 +0100