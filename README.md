# Brewken
(https://github.com/Brewken/brewken)

Open source brewing software by and for microbrewers.

Hi, my name's Matt, and I run a small microbrewery in south-west France.  Although there are existing pieces of software aimed at homebrewers to do recipe design etc, there are extra things to worry about as a microbrewer.  Eg, since you're liable for duty on the beer you produce, you have to keep track more precisely of exactly what you brewed and what happened to it which, for me, includes calculating the volume of beer that left the fermentor by weighing kegs (empty then full) and dividing the weight by the specific gravity.  I started out using various spreadsheets, notebooks and documents to track this, but soon realised it would be much better to have a single integrated piece of software.

As a microbrewer, you don't have a lot of money to spend on commercial software.  So I thought I should write something open source.  There's no point reinventing the wheel, so I took existing open source homebrew software as a starting point to extend with additional features needed for commercial microbrewing.

Brewken is still at a very early stage of development, but the ultimate aim is to provide:
 - recipe design and scaling
 - inventory management (including cost tracking)
 - brew log
 - batch management
 - sales ledger and duty calculation

The code for recipe design and some other parts comes from Brewtarget (https://github.com/Brewtarget/brewtarget), an established open-source beer recipe creation tool.  Where possible I want to be able to share fixes and enhancements between the two projects.
