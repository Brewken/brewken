# Brewken Changelog

This change log is for high-level user-visible changes to Brewken, intended for consumption by the typical end-user.
Note however that we also process it into a Debian-compliant text change log, so we need to keep the format consistent.
In particular, the Release Timestamp section is needed as part of this (and you need to be meticulous about the date
format therein).

## Forthcoming in v2.1.0

* Improvements to inventory
* Equipment: new "Foundation Water" field for the mash tun, for the space below a false bottom or malt pipe (eg in
  all-in-one systems) that has to be filled before the grain is covered but which drains out with the wort
* Mash and lauter tun losses are now correctly allowed for in the mash wizard's total water
* BeerXML export and import now keep folders (via an extension tag)
* Importing a dotBeer file into a folder now keeps the subfolders given in the file
* When a recipe's pre-boil size is set from equipment whose default pre-boil volume was calculated (eg on import from
  dotBeer or BeerJSON), it is now adjusted for the recipe's boil time

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