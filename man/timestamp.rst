===================
blt::timestamp
===================

--------------------------------------
Parse and format dates and timestamps.
--------------------------------------

:Author: George A Howlett <gahowlett@gmail.com>
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

.. TODO: authors and author with name <email>

SYNOPSIS
--------

**blt::timestamp format** *seconds* ?\ *switches* ...\ ?

**blt::timestamp parse** *timeStamp*

**blt::timestamp scan** *timeStamp* 

DESCRIPTION
-----------

This **blt::timestamp** command is for converting date/time timestamps to/from
double precision numbers.  It handles timestamps with fractional seconds,
IS08601 time formats (separated by the letter 'T'), and work week dates.
It is not designed to be a replacement for the TCL **clock** command.
There are differences (see the section `DIFFERENCES WITH TCL CLOCK`_ below).
This command is used internally by the **blt::datatable** data object to
parse timestamps where the format of the timestamp isn't known.

**blt::timestamp format** *seconds* ?\ *switches* ...\ ?
  Formats the time that is expressed in seconds into a date/time format.
  *Seconds* is a double precision number that represents the number of
  seconds since the epoch (typically January 1st 1970 UTC).  If no
  **-format** switch is given, the default format is used. For example
  if *seconds* is "0.0", the result would be "Thu Jan 01 00:00:00 +0000 1970".

  **-format** *substString*
    Specifies how to format the date.  *SubstString* is a string with
    various percent (%) sequences that are substituted with the desired
    value.  Most of the standand **clock** substitutions are available.
    See the section `PERCENT SUBSTITUTIONS`_ for a description of all the
    available sequences. The default format is "%a %b %d %H:%M:%S %z %Y". 

  **-timezone** *zoneName*
    Specifies that formatting is to be done according to the rules for
    the time zone specified by *zoneName*.  *ZoneName* may be a
    timezone abbreviation or offset. See the section `TIME ZONES`_ for a
    description of available names.

**blt::timestamp parse** *timeStamp*
  Parses the dates and returns its date and time components.  This is
  useful when you suspect that the timestamp format is not supported.
  *TimeStamp* is a string representing the date and/or time. This commands
  returns a list of key/value pairs.  The keys are "year", "month", "yday",
  "isleapyear", "hour", "minute" "second", "isdist", "tzoffset". Note that
  the value for "second" is a floating point number, not an integer.

**blt::timestamp scan** *timeStamp*
  Parses the date string given and returns a double precision number
  representing the number of seconds since the epoch (typically January 1st
  1970 UTC).    *TimeStamp* is a string representing the date and or time.
  The known formats for *timeStamp* are listed in section `KNOWN FORMATS`_. 

TIME ZONES
----------

The **blt::timestamp** command recognizes many different timezone names and
abbreviations.  If uses a TCL array variable **blt::timezones** to search
for timezones.  The array keyed by the timezone name or abbreviation.  The
value is a list of two strings representing the standard time and daylight
time offsets.  Each offset is in the form

  *sign* *HH* **:** *MM* 

where *sign* is either "+" or "-", *HH* are 2 digits representing the hours
and *MM* are 2 digits representing the minutes. There are separated by a
colon ":".  The first offset is the offset from UTC for the standard time.
The second is the offset from UTC for the daylight time. If the timezone
does not have daylight time, then the two entries are the same. For example,
the value for the timezone "US/Hawaii" is "-10:00 -10:00".

Unfortunately timezone abbreviations are not unique.  The are several
duplicates.  For example BST matches British Summer Time (UTC+01), Brazil
Standard Time (UTC-03), and Bering Summer Time (UTC-11).  EDT matches
Eastern Daylight Time USA" (UTC-04) and Eastern Daylight Time Australia
(UTC+11) EST matches Eastern Standard Time USA (UTC-05), Eastern Standard
Time Australia (UTC+10), and Eastern Brazil Standard Time (UTC-03).

If you don't find a timezone name or abbreviation in the **blt::timezones**
array or there is an entry with the wrong offsets, you can set the array
variable.

  ``set blt::timezones(BST) "+03:00 +02:00"``
  
The following are the default timezone abbreviations for known duplicates.
Ideally your timestamps will use the offset from UTC rather than a timezone
name or abbreviation.

KNOWN FORMATS
-------------

The following are examples of known timestamp formats that can be parsed by
the **blt::timestamp** command.

  - 2006-01-02T15:04:05Z07:00

    The date and time are separated by 'T'.

  - 2006-01-02T15:04:05.999999999Z07:00"

    There are fractional seconds. 

  - Mon Jan 2 15:04:05 MST 2006

    The year trails the timezone.

  - 02 Jan 2006 15:04 -0700

    Timezone offset. 

  - Thu, 21 Jun 68 00:00:00 GMT

    Weekday name.

  - 1997-12-17 07:37:16-08

    ISO 8601/SQL standard 	

  - 12/17/1997 07:37:16.00 PST

    SQL traditional style.

  - Wed Dec 17 07:37:16 1997 PST

    POSTGRES original style.

  - 17.12.1997 07:37:16.00 PST

    German regional style.  The date is separated by periods.

  - 2004-W53-6  

    ISO 8601 work week.
    
PERCENT SUBSTITUTIONS
---------------------

The following substitutions may be used to format a timestamp.

  **%%**
	  Single percent sign (%)

  **%a**
	  Abbreviated weekday. Example: "Sun".

  **%A**
	  Weekday. Example: "Sunday".

  **%b**
	  Abbreviated month. Example: "Jan".

  **%h**
	  Month. Example: "January".

  **%B**
	  Month. Example: "Month".

  **%c**
	  Date and time. Example: "Thu Mar 3 23:05:25 2005".

  **%C**
	  Century without last 2 digits. Example: "20".

  **%d**
	  Day of month, 2 digits. Example: "01".

  **%D**
	  mm/dd/yy format. Example: "01/01/1970".

  **%e**
	  Day of month, space padded. Example: " 1".

  **%F**
	  Full date yyyy-mm-dd. Example: "1970-01-01".

  **%g**
	  Last 2 digits of ISO week year. Example: "70".

  **%G**
	  ISO week year. Example: "1970".

  **%H**
	  Hour (0-23). Example: "0".

  **%I**
	  Hour (0-12). Example: "0".

  **%j**
	  Day of year. Example: "0".

  **%k**
	  Hour (0-23), space padded. Example: " 1".

  **%l**
	  Hour (1-12), space padded. Example: " 1".

  **%m**
	  Month (01-12). Example: "01".

  **%M**
	  Minute (00-59). Example: "00".

  **%N**
	  Nanoseconds (000000000..999999999). Example: "00000000000000".

  **%P**
	  AM or PM.  Example "AM".

  **%p**
	  am or pm. Example "am".

  **%R**
	  24 hour clock time (hh:mm). Example "23:59".

  **%r**
	  12 hour clock time (hh:mm:ss AM or PM). Example: "01:59:00 AM".

  **%s**
	  Seconds since epoch, (may contain fraction). Example "".

  **%S**
	  Seconds (00-59). Example: "00".

  **%T**
	  The time as "**%H**:**%M**:**%S**". Example: "".

  **%w**
	  Day of week (0-6). Example: "0".

  **%u**
	  Day of week (1-7). Example "1".

  **%U**
	  Week number (0-53). Sunday is first day of week. Example "".

  **%W**
	  Week number (0-53)					"00"

  **%V**
	  ISO Week number. Monday is first day of week.	Example: "".

  **%x**
	  Date representation mm/dd/yy. Example: "".

  **%y**
	  Year, last 2 digits. Example: "70".

  **%Y**
	  Year. Example: "1970".

  **%z**
	  Numeric timezone (+hhmm). Example: "+0000".


EXAMPLE
-------

DIFFERENCES WITH TCL CLOCK
--------------------------

1. If no date is given (only the time), the **scan** and **parse**
   operations assume January 1st, 1970, not the current date.
2. If no timezone is given, the **scan** and **parse** operations assume
   GMT, not the local timezone.
3. For two-digit years (such as "25") the century is always assumed to be
   1900 not 2000. Don't use two-digit years.
   
KEYWORDS
--------

timestamp, datatable

COPYRIGHT
---------

2015 George A. Howlett. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

 1) Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2) Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the distribution.
 3) Neither the name of the authors nor the names of its contributors may
    be used to endorse or promote products derived from this software
    without specific prior written permission.
 4) Products derived from this software may not be called "BLT" nor may
    "BLT" appear in their names without specific prior written permission
    from the author.

THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
