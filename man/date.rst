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

This **blt::date** command is for converting date/time timestamps to/from
double precision numbers.  It handles timestamps with fractional seconds,
IS08601 time formats (separated by the letter 'T'), and work week dates.
It is not designed to be a replacement for the TCL **clock** command.
There are differences (see the section DIFFERENCES WITH TCL CLOCK below).
This command is used internally by the **blt::tableview** widget to parse
timestamps where the format of the timestamp isn't known.

**blt::date debug** *timeStamp*

  Parses the dates and returns its date and time components.  This is
  useful when you suspect that the timestamp format is not supported.
  *TimeStamp* is a string representing the date and/or time. This commands
  returns a list of "year", "month", "yday", "isleapyear", "hour", "minute"
  "second", "isdist", "tzoffset" and their values. Note that the value for
  "second" is a floating point number, not an integer.

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
  representing the number of seconds since the epoch (typically January 1st
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


  ACDT
    Australian Central Daylight Time 	UTC + 10:30
  ACST
    Australian Central Standard Time 	UTC + 9:30
  ACWT
    Australian Central Western Time 	UTC + 8:45
  ADT
    Atlantic Daylight Time (Caribbean, North America) 	UTC - 3
  ADT
    Arabia Daylight Time 	UTC + 3
  ACT
    Acre Time (Brazil) 	UTC - 5
  AEDT
    Australian Eastern Daylight Time 	UTC + 11
  AEST
    Australian Eastern Standard Time 	UTC + 10
  AFT
    Afghanistan Time 	UTC + 4:30
  AKDT
    Alaska Daylight Time 	UTC - 8
  AKST
    Alaska Standard Time 	UTC - 9
  ALMT
    Alma-Ata Time 	UTC + 6
  AMT
    Armenia Time 	UTC + 4
  AMT
    Amazon Time 	UTC - 4
  AMST
    Amazon Summer Time 	UTC - 3
  AMST
    Armenia Summer Time UTC + 5
  ANAT
    Anadyr Time (Russia) 	UTC + 12
  ANAST
    Anadyr Summer Time (Russia) 	UTC + 12
  AQTT
    Aqtobe Time (Kazakhstan) 	UTC + 5
  ART
    Argentina Time 	UTC - 3
  AST
    Atlantic Standard Time (Caribbean, North America) 	UTC - 4
  AST
    Arab Standard Time 	UTC + 3
  AST
    Arabia Standard Time 	UTC + 3
  AWDT
    Australian Western Daylight Time 	UTC + 9
  AWST
    Australian Western Standard Time 	UTC + 8
  AZOT
    Azores Time 	UTC - 1
  AZOST
    Azores Summer Time 	UTC + 0
  AZT
    Azerbaijan Time 	UTC + 4
  AZST
    Azerbaijan Summer Time 	UTC + 5
  BNT
    Brunei Time 	UTC + 8
  BDT
    Bangladesh Time (also see BST) 	UTC + 6
  BOT
    Bolivia Time 	UTC - 4
  BRT
    Brasilia Time 	UTC - 3
  BRST
    Brasilia Summer Time 	UTC - 2
  BST
    British Summer Time 	UTC + 1
  BST
    Bangladesh Standard Time (also see BDT) 	UTC + 6
  BTT
    Bhutan Time 	UTC + 6
  CAST
    Casey Time (Antarctica) 	UTC + 8
  CAT
    Central Africa Time 	UTC + 2
  CCT
    Cocos Islands Time 	UTC + 6:30
  CDT
    Central Daylight Time (Australia) 	UTC + 10:30
  CDT
    Central Daylight Time (North America & Caribbean) 	UTC - 5
  CEDT
    Central European Daylight Time 	UTC + 2
  CEST
    Central European Summer Time 	UTC + 2
  CET
    Central European Time (standard time) 	UTC + 1
  CHADT
    Chatham Island Daylight Time 	UTC + 13:45
  CHAST
    Chatham Island Standard Time 	UTC + 12:45
  CHOT
    Choibalsan Time (Mongolia) 	UTC + 8
  CHOST
    Choibalsan Summer Time (Mongolia) 	UTC + 9
  CHST
    Chamorro Standard Time 	UTC + 10
  CHUT
    Chuuk Time 	UTC + 10
  CIT
    Central Indonesian Time (see abbreviation WITA) 	UTC + 8
  CKT
    Cook Island Time 	UTC - 10
  CLST
    Chile Summer Time 	UTC - 3
  CLT
    Chile Standard Time 	UTC - 4
  COT
    Columbia Time 	UTC - 5
  CST
    Central Standard Time (Australia) 	UTC + 9:30
  CST
    Central Standard Time (North America & Central America) 	UTC - 6
  CST
    Cuba Standard Time 	UTC - 5
  CST
    China Standard Time 	UTC + 8
CVT 	CAPE VERDE TIME 	UTC - 1
CWST 	CENTRAL WESTERN AUSTRALIA TIME (Eucla) 	UTC + 8:45
CXT 	CHRISTMAS ISLAND TIME 	UTC + 7
DAVT 	DAVIS TIME (Antarctica) 	UTC + 7
DDUT 	DUMONT D 'URVILLE TIME 	UTC + 10
DST 	DAYLIGHT SAVING TIME 	VARIES
EASST 	EASTERN ISLAND SUMMER TIME 	UTC - 5
EAST 	EASTERN ISLAND STANDARD TIME 	UTC - 6
EAT 	EAST AFRICA TIME 	UTC + 3
ECT 	ECUADOR TIME 	UTC - 5
EDT 	EASTERN DAYLIGHT TIME (Australia and Pacific) 	UTC + 11
EDT 	EASTERN DAYLIGHT TIME (North America and Caribbean) 	UTC - 4
EEDT 	EASTERN EUROPEAN DAYLIGHT TIME 	UTC + 3
EEST 	EASTERN EUROPEAN SUMMER TIME 	UTC + 3
EET 	EASTERN EUROPEAN TIME 	UTC + 2
EGT 	EASTERN GREENLAND TIME 	UTC - 1
EGST 	EASTERN GREENLAND SUMMER TIME 	UTC + 0
EST 	EASTERN STANDARD TIME (Australia and Pacific) 	UTC + 10
EST 	EASTERN STANDARD TIME (North America and Caribbean) 	UTC - 5
EIT 	EASTERN INDONESIAN TIME (see WIT) 	UTC + 9
FET 	FURTHER-EASTERN EUROPEAN TIME 	UTC + 3
FJT 	FIJI TIME 	UTC + 12
FJST 	FIJI SUMMER TIME 	UTC + 13
FKST 	FALKLAND ISLANDS SUMMER TIME 	UTC - 3
FKT 	FALKLAND ISLANDS TIME 	UTC - 4
FNT 	FERNANDO de NORONHA 	UTC - 2
GALT 	GALAPAGOS TIME 	UTC - 6
GAMT 	GAMBIER TIME 	UTC - 9
GET 	GEORGIA STANDARD TIME 	UTC + 4
GFT 	FRENCH GUIANA TIME 	UTC - 3
GILT 	GILBERT ISLAND TIME 	UTC + 12
GMT 	GREENWICH MEAN TIME 	UTC + 0
GST 	GULF STANDARD TIME 	UTC + 4
GST 	SOUTH GEORGIA TIME (South Georgia and the South Sandwich Islands) 	UTC - 2
GYT 	GUYANA TIME 	UTC - 4
HADT 	HAWAII-ALEUTIAN DAYLIGHT TIME 	UTC - 9
HAST 	HAWAII-ALEUTIAN STANDARD TIME 	UTC - 10
HKT 	HONG KONG TIME 	UTC + 8
HOVT 	HOVD TIME (Mongolia) 	UTC + 7
HOVST 	HOVD SUMMER TIME (Mongolia) 	UTC + 8
HST 	HAWAII STANDARD TIME 	UTC - 10
ICT 	INDOCHINA TIME 	UTC + 7
IDT 	ISRAEL DAYLIGHT TIME 	UTC + 3
IOT 	INDIAN CHAGOS TIME (British Indian Ocean Territory) 	UTC + 6
IRDT 	IRAN DAYLIGHT TIME 	UTC + 4:30
IRKT 	IRKUTSK TIME 	UTC + 8
IRKST 	IRKUTSK SUMMER TIME 	UTC + 9
IRST 	IRAN STANDARD TIME 	UTC + 3:30
IST 	INDIAN STANDARD TIME 	UTC + 5:30
IST 	ISRAEL STANDARD TIME 	UTC + 2
IST 	IRISH STANDARD TIME (IST is used during daylight saving time) 	UTC + 1
JST 	JAPAN STANDARD TIME 	UTC + 9
KGT 	KYRGYZSTAN TIME 	UTC + 6
KOST 	KOSRAE TIME (Micronesia) 	UTC + 11
KRAT 	KRASNOYARSK TIME 	UTC + 7
KRAST 	KRASNOYARSK SUMMER TIME 	UTC + 8
KST 	KOREA STANDARD TIME 	UTC + 9
KUYT 	KUYBYSHEV TIME (Samara Time as of 1991) 	UTC + 4
LHDT 	LORD HOWE DAYLIGHT TIME 	UTC + 11
LHST 	LORD HOWE STANDARD TIME 	UTC + 10:30
LINT 	LINE ISLANDS TIME 	UTC + 14
MAGT 	MAGADAN TIME 	UTC + 10
MAGST 	MAGADAN SUMMER TIME 	UTC + 12
MART 	MARQUESAS TIME 	UTC - 9:30
MAWT 	MAWSON STATION TIME (Antarctic) 	UTC + 5
MDT 	MOUNTAIN DAYLIGHT TIME (North America) 	UTC - 6
MeST 	METLAKATLA (Alaska Indian Community) 	UTC - 8
MHT 	MARSHALL ISLANDS TIME 	UTC + 12
MIST 	MACQUARIE ISLAND STATION TIME 	UTC + 11
MMT 	MYANMAR TIME 	UTC + 6:30
MSD 	MOSCOW SUMMER TIME 	UTC + 4
MSK 	MOSCOW STANDARD TIME 	UTC + 3
MST 	MOUNTAIN STANDARD TIME (North America) 	UTC - 7
MUT 	MAURITIUS TIME 	UTC + 4
MVT 	MALDIVES TIME 	UTC + 5
MYT 	MALAYSIA TIME 	UTC + 8
NCT 	NEW CALEDONIA TIME 	UTC + 11
NDT 	NEWFOUNDLAND DAYLIGHT TIME 	UTC - 2:30
NFT 	NORFOLK TIME 	UTC + 11:30
NOVT 	NOVOSIBIRSK TIME 	UTC + 6
NOVST 	NOVOSIBIRSK SUMMER TIME 	UTC + 7
NPT 	NEPAL TIME 	UTC + 5:45
NRT 	NAURU TIME 	UTC + 12
NST 	NEWFOUNDLAND STANDARD TIME 	UTC - 3:30
NT 	NEWFOUNDLAND TIME 	UTC - 3:30
NUT 	NIUE TIME 	UTC - 11
NZDT 	NEW ZEALAND DAYLIGHT TIME 	UTC + 13
NZST 	NEW ZEALAND STANDARD TIME 	UTC + 12
OMST 	OMSK STANDARD TIME 	UTC + 6
OMSST 	OMSK SUMMER TIME 	UTC + 7
ORAT 	ORAL TIME 	UTC + 5
PDT 	PACIFIC DAYLIGHT TIME (North America) 	UTC - 7
PET 	PERU TIME 	UTC - 5
PETT 	KAMCHATKA TIME 	UTC + 12
PETST 	KAMCHATKA SUMMER TIME 	UTC + 12
PGT 	PAPUA NEW GUINEA TIME 	UTC + 10
PHT 	PHILIPPINE TIME 	UTC + 8
PHOT 	PHOENIX ISLAND TIME 	UTC + 13
PKT 	PAKISTAN STANDARD TIME 	UTC + 5
PMDT 	PIERRE & MIQUELON DAYLIGHT TIME 	UTC - 2
PMST 	PIERRE & MIQUELON STANDARD TIME 	UTC - 3
PONT 	POHNPEI TIME (Formerly Ponape) 	UTC+ 11
PST 	PACIFIC STANDARD TIME (North America) 	UTC - 8
PST 	PITCAIRN TIME 	UTC - 8
PWT 	PALAU TIME 	UTC + 9
PYT 	PARAGUAY TIME 	UTC - 4
PYST 	PARAGUAY SUMMER TIME 	UTC - 3
QYZT 	QYZYLORDA TIME (Kazakhstan) 	UTC +6
RET 	REUNION TIME 	UTC + 4
ROTT 	ROTHERA (RESEARCH STATION) TIME (Antarctica) 	UTC - 3
SAKT 	SAKHALIN TIME 	UTC + 10
SAKST 	SAKHALIN SUMMER TIME 	UTC + 12
SAMT 	SAMARA TIME 	UTC + 4
SAST 	SOUTH AFRICA STANDARD TIME 	UTC + 2
SBT 	SOLOMON ISLANDS TIME 	UTC + 11
SCT 	SEYCHELLES TIME 	UTC + 4
SGT 	SINGAPORE TIME 	UTC + 8
SRT 	SURINAME TIME 	UTC - 3
SLT 	SRI LANKA TIME 	UTC + 5:30
SLST 	SRI LANKA TIME 	UTC + 5:30
SRET 	SREDNEKOLYMSK TIME 	UTC + 11
SST 	SAMOA STANDARD TIME (American Samoa) 	UTC - 11
SYOT 	SYOWA (RESEARCH STATION) TIME (Antarctica) 	UTC + 3
TAHT 	TAHITI TIME 	UTC - 10
TFT 	FRENCH SOUTHERN AND ANTARCTIC TERRITORIES TIME 	UTC + 5
TJT 	TAJIKISTAN TIME 	UTC + 5
TKT 	TOKELAU TIME 	UTC + 13
TLT 	EAST TIMOR TIME (Timor-Leste Time) 	UTC + 9
TMT 	TURKMENISTAN TIME 	UTC + 5
TOT 	TONGA TIME 	UTC + 13
TRUT 	TRUK TIME (Micronesia) 	UTC + 10
TVT 	TUVALU TIME 	UTC + 12
ULAT 	ULAANBAATAR TIME 	UTC + 8
ULAST 	ULAANBAATAR SUMMER TIME 	UTC + 9
UTC 	COORDINATED UNIVERSAL TIME 	UTC + 0
UYST 	URUGUAY SUMMER TIME 	UTC - 2
UYT 	URUGUAY STANDARD TIME 	UTC - 3
UZT 	UZBEKISTAN TIME 	UTC + 5
VET 	VENEZUELAN STANDARD TIME 	UTC - 4:30
VLAT 	VLADIVOSTOK TIME 	UTC + 10
VLAST 	VLADIVOSTOK SUMMER TIME 	UTC + 11
VOLT 	VOLGOGRAD TIME 	UTC + 4
VUT 	VANUATU TIME 	UTC + 11
WAKT 	WAKE ISLAND TIME 	UTC + 12
WAT 	WEST AFRICA TIME 	UTC + 1
WART 	WEST ARGENTINA TIME 	UTC - 4
WAST 	WEST AFRICA SUMMER TIME 	UTC + 2
WDT 	WESTERN DAYLIGHT TIME (Australia) 	UTC + 9
WEDT 	WESTERN EUROPEAN DAYLIGHT TIME 	UTC + 1
WEST 	WESTERN EUROPEAN SUMMER TIME 	UTC + 1
WET 	WESTERN EUROPEAN TIME 	UTC + 0
WFT 	WALLIS AND FUTUNA TIME 	UTC + 12
WGT 	WESTERN GREENLAND TIME 	UTC - 3
WGST 	WESTERN GREENLAND SUMMER TIME 	UTC - 2
WIB 	WESTERN INDONESIAN TIME 	UTC + 7
WIT 	EASTERN INDONESIAN TIME 	UTC + 9
WITA 	CENTRAL INDONESIAN TIME 	UTC + 8
WST 	WESTERN SAHARA SUMMER TIME 	UTC + 1
WST 	WESTERN STANDARD TIME (Australia) 	UTC + 8
WST 	WESTERN SAMOA TIME (standard time) 	UTC + 13
WST 	WESTERN SAMOA TIME (*also used during daylight saving time) 	UTC + 14
WT 	WESTERN SAHARA STANDARD TIME 	UTC + 0
YAKT 	YAKUTSK TIME 	UTC + 9
YAKST 	YAKUTSK SUMMER TIME 	UTC + 10
YAP 	YAP TIME (Micronesia) 	UTC + 10
YEKT 	YEKATERINBURG TIME 	UTC + 5
YEKST 	YEKATERINBURG SUMMER TIME 	UTC + 6



  gmt
    Greenwich Mean Time 	UTC
  ut
	  Universal (Coordinated)
  utc
    Coordinated Universal Time 	UTC
  uct
    Coordinated Universal Time 	UTC
  wet
    Western European Time 	UTC
  bst
    British Summer Time (British Standard Time from Feb 1968 to Oct 1971) UTC+01
  wat
    West Africa Time 	UTC+01
  azost
    Azores Standard Time 	UTC−01
  at
	  Azores 
  nt
    Newfoundland Time 	UTC−03:30
  nft
	  Newfoundland 
  nst
    Newfoundland Standard Time 	UTC−03:30
  ndt
    Newfoundland Daylight Time 	UTC−02:30
  ast
    Atlantic Standard Time 	UTC−04
  adt
    Atlantic Daylight Time 	UTC−03
  est
    Eastern Standard Time (North America) 	UTC−05
  edt
    Eastern Daylight Time (North America) 	UTC−04
  cst
    Central Standard Time (North America) 	UTC−06
  cdt
    Central Daylight Time (North America) 	UTC−05
  mst
    Mountain Standard Time (North America) 	UTC−07
  mdt
    Mountain Daylight Time (North America) 	UTC−06
  pst
    Pacific Standard Time (North America) 	UTC−08
  pdt
    Pacific Daylight Time (North America) 	UTC−07
  yst
	  Yukon Standard 
  ydt
	  Yukon Daylight 
  hst
    Hawaii Standard Time 	UTC−10
  hdt
	  Hawaii Daylight 
  cat
	  Central Alaska 
  ahst
	  Alaska-Hawaii Standard 
  nt
	  Nome 
  idlw
	  International Date Line West 
  cet
    Central European Time 	UTC+01
  cest
    Central European Summer Time (Cf. HAEC) 	UTC+02
  met
    Middle European Time Same zone as CET 	UTC+01
  mewt
	  Middle European Winter 
  mest
    Middle European Saving Time Same zone as CEST 	UTC+02
  swt
	  Swedish Winter 
  sst
	  Swedish Summer 
  fwt
	  French Winter 
  fst
	  French Summer 
  eet
    Eastern European Time 	UTC+02
  bt
	  Baghdad, USSR Zone 2 
  it
	  Iran 
  zp4
	  USSR Zone 3 
  zp5
	  USSR Zone 4 
  ist
    Indian Standard Time 	UTC+05:30
  zp6
	  USSR Zone 5 
  wast
	  West Australian Standard 
  wadt
	  West Australian Daylight 
  jt
	  Java (3pm in Cronusland!) 
  cct
	  China Coast, USSR Zone 7 
  jst
	  Japan Standard, USSR Zone 8 
  jdt
	  Japan Daylight 
  kst
	  Korea Standard 
  kdt
	  Korea Daylight 
  cast
	  Central Australian Standard 
  cadt
	  Central Australian Daylight 
  east
	  Eastern Australian Standard 
  eadt
	  Eastern Australian Daylight 
  gst
	  Guam Standard, USSR Zone 9 
  nzt
	  New Zealand 
  nzst
	  New Zealand Standard 
  nzdt
	  New Zealand Daylight 
  idle
	  International Date Line East 
  dst
	  DST on (hour is ignored)
  a
     Alpha Time Zone 	UTC+01:00
  b
     Bravo Time Zone 	UTC+02:00
  c
    Charlie Time Zone	UTC+03:00
  d
    Delta Time Zone 	UTC+04:00
  e
    Echo Time Zone 	UTC+05:00
  f
    Foxtrot Time Zone 	UTC+06:00
  g
    Golf Time Zone 	UTC+07:00
  h
    Hotel Time Zone 	UTC+08:00
  i
    India Time Zone 	UTC+09:00
  k
    Kilo Time Zone 	UTC+10:00
  l
    Lima Time Zone 	UTC+11:00
  m
    Mike Time Zone 	UTC+12:00
  n
    November Time Zone 	UTC−01:00
  o
    Oscar Time Zone 	UTC−02:00
  p
    Papa Time Zone 	UTC−03:00
  q
    Quebec Time Zone 	UTC−04:00
  r
    Romeo Time Zone 	UTC−05:00
  s
    Sierra Time Zone 	UTC−06:00
  t
    Tango Time Zone 	UTC−07:00
  u
    Uniform Time Zone 	UTC−08:00
  v
    Victor Time Zone 	UTC−09:00
  w
    Whiskey Time Zone 	UTC−10:00
  x     
    X-ray Time Zone 	UTC−11:00
  y
    Yankee Time Zone 	UTC−12:00
  z
    Zulu Time Zone	UTC

  acdt
    Australian Central Daylight Savings Time 	UTC+10:30
  acst
    Australian Central Standard Time 	UTC+09:30
  act
    Acre Time 	UTC−05
  act
    ASEAN Common Time 	UTC+08
  aedt
    Australian Eastern Daylight Savings Time 	UTC+11
  aest
    Australian Eastern Standard Time 	UTC+10
  aft
    Afghanistan Time 	UTC+04:30
  akdt
    Alaska Daylight Time 	UTC−08
  akst
    Alaska Standard Time 	UTC−09
  amst
    Amazon Summer Time (Brazil)[1] 	UTC−03
  amst
    Armenia Summer Time 	UTC+05
  amt
    Amazon Time (Brazil)[2] 	UTC−04
  amt
    Armenia Time 	UTC+04
  art
    Argentina Time 	UTC−03
  ast
    Arabia Standard Time 	UTC+03
  awdt
    Australian Western Daylight Time 	UTC+09
  awst
    Australian Western Standard Time 	UTC+08
  azt
   Azerbaijan Time 	UTC+04
  bdt
    Brunei Time 	UTC+08
  biot
    British Indian Ocean Time 	UTC+06
  bit
    Baker Island Time 	UTC−12
  bot
    Bolivia Time 	UTC−04
  brst
    Brasilia Summer Time 	UTC−02
  brt
    Brasilia Time 	UTC−03
  bst
    Bangladesh Standard Time 	UTC+06
UTC+01
  btt
    Bhutan Time 	UTC+06
  cat
    Central Africa Time 	UTC+02
  cct
    Cocos Islands Time 	UTC+06:30
  cdt
    Cuba Daylight Time[3] 	UTC−04
  cedt
    Central European Daylight Time 	UTC+02
  chadt
    Chatham Daylight Time 	UTC+13:45
  chast
    Chatham Standard Time 	UTC+12:45
  chot
    Choibalsan 	UTC+08
  chst
    Chamorro Standard Time 	UTC+10
  chut
    Chuuk Time 	UTC+10
  cist
    Clipperton Island Standard Time 	UTC−08
  cit
    Central Indonesia Time 	UTC+08
  ckt
    Cook Island Time 	UTC−10
  clst
    Chile Summer Time 	UTC−03
  clt
    Chile Standard Time 	UTC−04
  cost
    Colombia Summer Time 	UTC−04
  cot
    Colombia Time 	UTC−05
  cst
    China Standard Time 	UTC+08
  cst
    Central Standard Time (Australia) 	UTC+09:30
  cst
    Central Summer Time (Australia) 	UTC+10:30
  ct
    China time 	UTC+08
  cvt
    Cape Verde Time 	UTC−01
  cwst
    Central Western Standard Time (Australia) unofficial 	UTC+08:45
  cxt
    Christmas Island Time 	UTC+07
  davt
    Davis Time 	UTC+07
  ddut
    Dumont d'Urville Time 	UTC+10
  dft
    AIX specific equivalent of Central European Time[4] 	UTC+01
  easst
    Easter Island Standard Summer Time 	UTC−05
  east
    Easter Island Standard Time 	UTC−06
  eat
    East Africa Time 	UTC+03
  ect
    Eastern Caribbean Time (does not recognise DST) 	UTC−04
  ect
    Ecuador Time 	UTC−05
  eedt
    Eastern European Daylight Time 	UTC+03
  eest
    Eastern European Summer Time 	UTC+03
  egst
    Eastern Greenland Summer Time 	UTC+00
  egt
    Eastern Greenland Time 	UTC−01
  eit
    Eastern Indonesian Time 	UTC+09
  est
    Eastern Standard Time (Australia) 	UTC+10
  fet
    Further-eastern European Time 	UTC+03
  fjt
    Fiji Time 	UTC+12
  fkst
    Falkland Islands Standard Time 	UTC−03
  fkst
    Falkland Islands Summer Time 	UTC−03
  fkt
    Falkland Islands Time 	UTC−04
  fnt
    Fernando de Noronha Time 	UTC−02
  galt
    Galapagos Time 	UTC−06
  gamt
    Gambier Islands 	UTC−09
  get
    Georgia Standard Time 	UTC+04
  gft
    French Guiana Time 	UTC−03
  gilt
    Gilbert Island Time 	UTC+12
  git
    Gambier Island Time 	UTC−09
  gst
    South Georgia and the South Sandwich Islands 	UTC−02
  gst
    Gulf Standard Time 	UTC+04
  gyt
    Guyana Time 	UTC−04
  hadt
    Hawaii-Aleutian Daylight Time 	UTC−09
  haec
    Heure Avancée d'Europe Centrale francised name for CEST 	UTC+02
  hast
    Hawaii-Aleutian Standard Time 	UTC−10
  hkt
    Hong Kong Time 	UTC+08
  hmt
   Heard and McDonald Islands Time 	UTC+05
  hovt
    Khovd Time 	UTC+07
  ict
    Indochina Time 	UTC+07
  idt
    Israel Daylight Time 	UTC+03
  iot
    Indian Ocean Time 	UTC+03
  irdt
    Iran Daylight Time 	UTC+04:30
  irkt
    Irkutsk Time 	UTC+08
  irst
    Iran Standard Time 	UTC+03:30
  ist
    Irish Standard Time[5] 	UTC+01
  ist
    Israel Standard Time 	UTC+02
  jst
    Japan Standard Time 	UTC+09
  kgt
    Kyrgyzstan time 	UTC+06
  kost
    Kosrae Time 	UTC+11
  krat
    Krasnoyarsk Time 	UTC+07
  kst
    Korea Standard Time 	UTC+09
  lhst
    Lord Howe Standard Time 	UTC+10:30
  lhst
    Lord Howe Summer Time 	UTC+11
  lint
    Line Islands Time 	UTC+14
  magt
    Magadan Time 	UTC+12
  mart
    Marquesas Islands Time 	UTC−09:30
  mawt
    Mawson Station Time 	UTC+05
  met
    Middle European Time Same zone as CET 	UTC+01
  mht
    Marshall Islands 	UTC+12
  mist
    Macquarie Island Station Time 	UTC+11
  mit
    Marquesas Islands Time 	UTC−09:30
  mmt
    Myanmar Time 	UTC+06:30
  msk
    Moscow Time 	UTC+03
  mst
    Malaysia Standard Time 	UTC+08
  mst
    Myanmar Standard Time 	UTC+06:30
  mut
    Mauritius Time 	UTC+04
  mvt
    Maldives Time 	UTC+05
  myt
    Malaysia Time 	UTC+08
  nct
    New Caledonia Time 	UTC+11
  nft
    Norfolk Time 	UTC+11:30
  npt
    Nepal Time 	UTC+05:45
  nut
    Niue Time 	UTC−11
  nzdt
    New Zealand Daylight Time 	UTC+13
  nzst
    New Zealand Standard Time 	UTC+12
  omst
    Omsk Time 	UTC+06
  orat
    Oral Time 	UTC+05
  pet
    Peru Time 	UTC−05
  pett
    Kamchatka Time 	UTC+12
  pgt
    Papua New Guinea Time 	UTC+10
  phot
    Phoenix Island Time 	UTC+13
  pkt
    Pakistan Standard Time 	UTC+05
  pmdt
    Saint Pierre and Miquelon Daylight time 	UTC−02
  pmst
    Saint Pierre and Miquelon Standard Time 	UTC−03
  pont
    Pohnpei Standard Time 	UTC+11
  pst
    Philippine Standard Time 	UTC+08
  pyst
    Paraguay Summer Time (South America)[6] 	UTC−03
  pyt
    Paraguay Time (South America)[7] 	UTC−04
  ret
    Réunion Time 	UTC+04
  rott
    Rothera Research Station Time 	UTC−03
  sakt
    Sakhalin Island time 	UTC+11
  samt
    Samara Time 	UTC+04
  sast
    South African Standard Time 	UTC+02
  sbt
    Solomon Islands Time 	UTC+11
  sct
    Seychelles Time 	UTC+04
  sgt
    Singapore Time 	UTC+08
  slst
    Sri Lanka Time 	UTC+05:30
  sret
    Srednekolymsk Time 	UTC+11
  srt
    Suriname Time 	UTC−03
  sst
    Samoa Standard Time 	UTC−11
  sst
    Singapore Standard Time 	UTC+08
  syot
    Showa Station Time 	UTC+03
  taht
    Tahiti Time 	UTC−10
  tha
    Thailand Standard Time 	UTC+07
  tft
    Indian/Kerguelen 	UTC+05
  tjt
    Tajikistan Time 	UTC+05
  tkt
    Tokelau Time 	UTC+13
  tlt
    Timor Leste Time 	UTC+09
  tmt
    Turkmenistan Time 	UTC+05
  tot
    Tonga Time 	UTC+13
  tvt
   Tuvalu Time 	UTC+12
  ulat
    Ulaanbaatar Time 	UTC+08
  usz1
    Kaliningrad Time 	UTC+02
  uyst
    Uruguay Summer Time 	UTC−02
  uyt
    Uruguay Standard Time 	UTC−03
  uzt
    Uzbekistan Time 	UTC+05
  vet
    Venezuelan Standard Time 	UTC−04:30
  vlat
    Vladivostok Time 	UTC+10
  volt
    Volgograd Time 	UTC+04
  vost
    Vostok Station Time 	UTC+06
  vut
    Vanuatu Time 	UTC+11
  wakt
    Wake Island Time 	UTC+12
  wast
    West Africa Summer Time 	UTC+02
  wat
    West Africa Time 	UTC+01
  wedt
    Western European Daylight Time 	UTC+01
  west
    Western European Summer Time 	UTC+01
  wit
    Western Indonesian Time 	UTC+07
  wst
    Western Standard Time 	UTC+08
  yakt
    Yakutsk Time 	UTC+09
  yekt
    Yekaterinburg Time 	UTC+05
  Z
    Zulu Time (Coordinated Universal Time)

KNOWN FORMATS
==================

Timestamps 


EXAMPLE
=======

DIFFERENCES WITH TCL CLOCK
==========================

1. If no date is provided, **blt::date** assumes January 1st, 1970, not the
   current date.
2. For two digit years (such as "25") the century is always assumed to be
   1900 not 2000.
   
KEYWORDS
========

datatable, tableview
