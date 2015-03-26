===================
blt::date
===================

-------------------------------------------------
Parse and format dates and timestamps.
-------------------------------------------------

:Author: gahowlett@gmail.com
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
        Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use, copy,
	modify, merge, publish, distribute, sublicense, and/or sell copies
	of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:
	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
	BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
	ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
	CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.

:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

.. TODO: authors and author with name <email>

SYNOPSIS
--------

**blt::date debug** *timeStamp*

**blt::date format** *seconds* ?\ *switches* ...\ ?

**blt::date scan** *timeStamp* 

DESCRIPTION
===========

This command is used by the **blt::tableview** widget to parse timestamps.

**blt::date debug** *timeStamp*

  Parses the date string given printing out debugging information.
  This is useful when you suspect that the date format is not supported.
  *TimeStamp* is a string representing the date and or time.

**blt::date format** *seconds* ?\ *switches* ...\ ?

  Formats the time that is expressed in seconds into a date/time format.
  *Seconds* is a double precision number that represents the number of
  seconds since the epoch (typically January 1st 1970 UTC).  If no
  **--format** switch is given, the default format is used. For example
  if *seconds* is "0.0", the result would be "Thu Jan 01 00:00:00 +0000 1970".

  **-format** *substString*
    Specifies how to format the date.  *SubstString* is a string with
    various percent (%) sequences that are substituted with the desired
    value.  Most of the standand **clock** substitutions are available.
    See the section PERCENT SUBSTITUTIONS for a description of all the
    available sequences. The default format is "%a %b %d %H:%M:%S %z %Y". 

  **-timezone** *zoneName*
    Specifies that formatting is to be done according to the rules for
    the time zone specified by *zoneName*.  *ZoneName* may be a
    timezone abbreviation or offset. See the section TIME ZONES for a
    description of available names.

**blt::date scan** *timeStamp*

  Parses the date string given and returns a double precision number
  representin the number of seconds since the epoch (typically January 1st
  1970 UTC).    *TimeStamp* is a string representing the date and or time.
  The known formats for *timeStamp* are listed in section KNOWN FORMATS. 

PERCENT SUBSTUTIONS
===================

%%
	Single percent sign (%)

%a
	Abbreviated weekday. Example: "Sun".

%A
	Weekday. Example: "Sunday".

%b
	Abbreviated month. Example: "Jan".

%h
	Month. Example: "January".

%B
	Month. Example: "Month".

%c
	Date and time. Example: "Thu Mar 3 23:05:25 2005".

%C
	Century without last 2 digits. Example: "20".

%d
	Day of month, 2 digits. Example: "01".

%D
	mm/dd/yy format. Example: "01/01/1970".

%e
	Day of month, space padded. Example: " 1".

%F
	Full date yyyy-mm-dd. Example: "1970-01-01".

%g
	Last 2 digits of ISO week year. Example: "70".

%G
	ISO week year. Example: "1970".

%H
	Hour (0-23). Example: "0".

%I
	Hour (0-12). Example: "0".

%j
	Day of year. Example: "0".

%k
	Hour (0-23), space padded. Example: " 1".

%l
	Hour (1-12), space padded. Example: " 1".

%m
	Month (01-12). Example: "01".

%M
	Minute (00-59). Example: "00".

%N
	Nanoseconds (000000000..999999999). Example: "00000000000000".

%P
	AM or PM.  Example "AM".

%p
	am or pm. Example "am".

%R
	24 hour clock time (hh:mm). Example "23:59".

%r
	12 hour clock time (hh:mm:ss AM or PM). Example: "01:59:00 AM".

%s
	Seconds since epoch, (may contain fraction). Example "".

%S
	Seconds (00-59). Example: "00".

%T
	The time as "%H:%M:%S". Example: "".

%w
	Day of week (0-6). Example: "0".

%u
	Day of week (1-7). Example "1".

%U
	Week number (0-53). Sunday is first day of week. Example "".

%W
	Week number (0-53)					"00"

%V
	ISO Week number. Monday is first day of week.	Example: "".

%x
	Date representation mm/dd/yy. Example: "".

%y
	Year, last 2 digits. Example: "70".

%Y
	Year. Example: "1970".

%z
	Numeric timezone (+hhmm). Example: "+0000".

TIME ZONES
==========

=========	================================
Zone Name	Description
=========	================================
gmt		Greenwich Mean
ut		Universal (Coordinated)
utc		Universal (Coordinated)
uct		Universal Coordinated Time
wet		Western European
bst		British Summer
wat		West Africa
at		Azores 
nft		Newfoundland 
nst		Newfoundland Standard 
ndt		Newfoundland Daylight 
ast		Atlantic Standard 
adt		Atlantic Daylight 
est		Eastern Standard 
edt		Eastern Daylight 
cst		Central Standard 
cdt		Central Daylight 
mst		Mountain Standard 
mdt		 Mountain Daylight 
pst		 Pacific Standard 
pdt		 Pacific Daylight 
yst		 Yukon Standard 
ydt		 Yukon Daylight 
hst		 Hawaii Standard 
hdt		 Hawaii Daylight 
cat		 Central Alaska 
ahst		 Alaska-Hawaii Standard 
nt		 Nome 
idlw		 International Date Line West 
cet		 Central European 
cest		 Central European Summer 
met		 Middle European 
mewt		 Middle European Winter 
mest		 Middle European Summer 
swt		 Swedish Winter 
sst		 Swedish Summer 
fwt		 French Winter 
fst		 French Summer 
eet		 Eastern Europe USSR Zone 1 
bt		 Baghdad, USSR Zone 2 
it		 Iran 
zp4		 USSR Zone 3 
zp5		 USSR Zone 4 
ist		 Indian Standard 
zp6		 USSR Zone 5 
wast		 West Australian Standard 
wadt		 West Australian Daylight 
jt		 Java (3pm in Cronusland!) 
cct		 China Coast, USSR Zone 7 
jst		 Japan Standard, USSR Zone 8 
jdt		 Japan Daylight 
kst		 Korea Standard 
kdt		 Korea Daylight 
cast		 Central Australian Standard 
cadt		 Central Australian Daylight 
east		 Eastern Australian Standard 
eadt		 Eastern Australian Daylight 
gst		 Guam Standard, USSR Zone 9 
nzt		 New Zealand 
nzst		 New Zealand Standard 
nzdt		 New Zealand Daylight 
idle		 International Date Line East 
dst		 DST on (hour is ignored) 
=========	================================

KNOWN FORMATS
==================

Timestamps 


EXAMPLE
=======

KEYWORDS
========

datatable, tableview
