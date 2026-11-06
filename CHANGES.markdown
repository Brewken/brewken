# Brewken Changelog

This change log is for high-level user-visible changes to Brewken, intended for consumption by the typical end-user.
Note however that we also process it into a Debian-compliant text change log, so we need to keep the format consistent.
In particular, the Release Timestamp section is needed as part of this (and you need to be meticulous about the date
format therein, otherwise you'll get, eg, no-changelogname-tag error from rpmlint).  You get problems if you set the
release timestamp to be a date in the future, and I'm guessing nobody cares about the exact time of day a release
happens, so I'm now setting it to a slightly arbitrary time early in the morning.

## Forthcoming in v5.3.0

### New Features
* TODO: Additional methods for calculating IBU
* Shopping Lists
* Mash water calculation (strike water, other infusions, decoctions and sparge water) is now done in the Mash Editor, replacing the Mash Wizard and Mash Designer.  Individual amounts and temperatures can be marked as fixed so that only the rest are calculated.
* Example mash profiles, which are locked so they can be shared between recipes
* We'll list other new features here...

## v5.2.3
More bug fixes.

### New Features
* Remainder of work on: Enhancement - allow per-field selection of displayed decimal places [1098](https://github.com/Brewtarget/brewtarget/issues/1098)
* Input values with more than 1 decimal round to 1 place [1142](https://github.com/Brewtarget/brewtarget/issues/1142)

### Bug Fixes
* v5.2.0 not working on some older hardware [1179](https://github.com/Brewtarget/brewtarget/issues/1179)
* "Error Loading Data" and Recipe UI issue on 5.2.0 [1180](https://github.com/Brewtarget/brewtarget/issues/1180)

### Release Timestamp
Thu, 5 Nov 2026 05:02:05 +0100

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
Mon, 1 Nov 2026 05:02:01 +0100

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