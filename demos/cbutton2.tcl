
package require BLT

set imgData {
    R0lGODlhEAANAMIAAAAAAH9/f///////AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM8WBrM+rAEQWmIb5KxiWjNInCkV32AJHRlGQBgDA7vdN4vUa8tC78qlrCWmvRKsJTquHkp
    ZTKAsiCtWq0JADs=
}

set icon2 [image create picture -file images/blt98.gif]
set icon [image create picture -data $imgData]
set bg white
set image ""
option add *ComboEntry.takeFocus 1

if { [file exists ../library] } {
    set blt_library ../library
}

set myIcon ""
blt::combobutton .b \
    -image $image \
    -textvariable myText1 \
    -iconvariable myIcon1 \
    -arrowon yes \
    -menu .b.m \
    -menuanchor se \
    -command "puts {button pressed}"

blt::combomenu .b.m  \
    -bg $bg \
    -cursor crosshair \
    -activebackground skyblue4 \
    -activeforeground white \
    -textvariable myText1 \
    -iconvariable myIcon1 \
    -disabledforeground grey75  \
    -disabledbackground grey98  \
    -disabledacceleratorforeground grey75  \
    -yscrollbar .b.m.ybar \
    -xscrollbar .b.m.xbar

blt::tk::scrollbar .b.m.xbar
# -elementborderwidth 2 -borderwidth 0
blt::tk::scrollbar .b.m.ybar
#-elementborderwidth 2 -borderwidth 0

set wwho ""
foreach item { Undo X1 Y1 Redo Cut Copy X2 Y2 Paste "Select All" X3 Y3 
    Find Replace } {
    set ${item}Var 0
    set char [string range $item 0 0] 
    .b.m add \
	-text $item \
	-type checkbutton \
	-accel "Ctrl+$char" \
	-underline 0 \
	-tag [string tolower $char] \
	-icon $icon \
	-variable ${item}Var \
	-value $item \

}

.b.m item configure Undo -type command
.b.m item configure Cut -type command 
.b.m item configure Find -type cascade -menu .b.m.m
#-state disabled
.b.m item configure Y3 -type command -image $icon2 
.b.m item configure Undo -type command 
.b.m item configure Paste -type separator 
.b.m item configure x -state disabled 
.b.m item configure y -type radiobutton -variable wwho 
.b.m item configure Y1 -state disabled
set wwho Y1
blt::combomenu .b.m.m  \
    -bg $bg \
    -textvariable myText1 \
    -iconvariable myIcon1 \
    -disabledforeground grey75  \
    -disabledbackground grey98  \
    -disabledacceleratorforeground grey75  \
    -width { 0 400 } \
    -height { 0 500 } \
    -yscrollbar .b.m.m.ybar \
    -xscrollbar .b.m.m.xbar

blt::tk::scrollbar .b.m.m.xbar 
blt::tk::scrollbar .b.m.m.ybar 

set onOff 0
foreach item { Undo X1 Y1 Redo Cut Copy X2 Y2 Paste "Select All" X3 Y3 
    Find Replace } {
    set ${item}Var 0
    set char [string range $item 0 0] 
    .b.m.m add \
	-text $item \
	-type checkbutton \
	-accel "Ctrl+$char" \
	-accel "" \
	-underline 0 \
	-tag [string tolower $char] \
	-icon $icon \
	-variable ${item}Var \
	-value $item \

}

.b.m.m item configure Undo -type command
.b.m.m item configure Cut -type command 
.b.m.m item configure Find -type cascade -menu .b.m.m.m
#-state disabled
.b.m.m item configure Y3 -type command -image $icon2
.b.m.m item configure Undo -type command 
.b.m.m item configure Paste -type separator 
.b.m.m item configure x -state disabled 
.b.m.m item configure y -type radiobutton -variable wwho 

set labels { 
    Aarhus Aaron Ababa aback abaft abandon abandoned abandoning
    abandonment abandons abase abased abasement abasements abases
    abash abashed abashes abashing abasing abate abated abatement
    abatements abater abates abating Abba abbe abbey abbeys abbot
    abbots Abbott abbreviate abbreviated abbreviates abbreviating
    abbreviation abbreviations Abby abdomen abdomens abdominal
    abduct abducted abduction abductions abductor abductors abducts
    Abe abed Abel Abelian Abelson Aberdeen Abernathy aberrant
    aberration aberrations abet abets abetted abetter abetting
    abeyance abhor abhorred abhorrent abhorrer abhorring abhors
    abide abided abides abiding Abidjan Abigail Abilene abilities
    ability abject abjection abjections abjectly abjectness abjure
    abjured abjures abjuring ablate ablated ablates ablating
    ablation ablative ablaze able abler ablest ably Abner abnormal
    abnormalities abnormality abnormally Abo aboard abode abodes
    abolish abolished abolisher abolishers abolishes abolishing
    abolishment abolishments abolition abolitionist abolitionists
    abominable abominate aboriginal aborigine aborigines abort
    aborted aborting abortion abortions abortive abortively aborts
    Abos abound abounded abounding abounds about above aboveboard
    aboveground abovementioned abrade abraded abrades abrading
    Abraham Abram Abrams Abramson abrasion abrasions abrasive
    abreaction abreactions abreast abridge abridged abridges
    abridging abridgment abroad abrogate abrogated abrogates
    abrogating abrupt abruptly abruptness abscess abscessed
    abscesses abscissa abscissas abscond absconded absconding
    absconds absence absences absent absented absentee
    absenteeism absentees absentia absenting absently absentminded
    absents absinthe absolute absolutely absoluteness absolutes
    absolution absolve absolved absolves absolving absorb
    absorbed absorbency absorbent absorber absorbing absorbs
    absorption absorptions absorptive abstain abstained abstainer
    abstaining abstains abstention abstentions abstinence
    abstract abstracted abstracting abstraction abstractionism
    abstractionist abstractions abstractly abstractness
    abstractor abstractors abstracts abstruse abstruseness
    absurd absurdities absurdity absurdly Abu abundance abundant
    abundantly abuse abused abuses abusing abusive abut abutment
    abuts abutted abutter abutters abutting abysmal abysmally
    abyss abysses Abyssinia Abyssinian Abyssinians acacia
    academia academic academically academics academies academy
    Acadia Acapulco accede acceded accedes accelerate accelerated
    accelerates accelerating acceleration accelerations
    accelerator accelerators accelerometer accelerometers accent
    accented accenting accents accentual accentuate accentuated
    accentuates accentuating accentuation accept acceptability
    acceptable acceptably acceptance acceptances accepted
    accepter accepters accepting acceptor acceptors accepts
    access accessed accesses accessibility accessible accessibly
    accessing accession accessions accessories accessors
    accessory accident accidental accidentally accidently
    accidents acclaim acclaimed acclaiming acclaims acclamation
    acclimate acclimated acclimates acclimating acclimatization
    acclimatized accolade accolades accommodate accommodated
    accommodates accommodating accommodation accommodations
    accompanied accompanies accompaniment accompaniments
    accompanist accompanists accompany accompanying accomplice
    accomplices accomplish accomplished accomplisher accomplishers
    accomplishes accomplishing accomplishment accomplishments
    accord accordance accorded accorder accorders according
    accordingly accordion accordions accords accost accosted
    accosting accosts account accountability accountable accountably
    accountancy accountant accountants accounted accounting
    accounts Accra accredit accreditation accreditations
    accredited accretion accretions accrue accrued accrues
    accruing acculturate acculturated acculturates acculturating
    acculturation accumulate accumulated accumulates accumulating
    accumulation accumulations accumulator accumulators
    accuracies accuracy accurate accurately accurateness accursed
    accusal accusation accusations accusative accuse accused
    accuser accuses accusing accusingly accustom accustomed
    accustoming accustoms ace aces acetate acetone acetylene
    Achaean Achaeans ache ached aches achievable achieve achieved
    achievement achievements achiever achievers achieves achieving
    Achilles aching acid acidic acidities acidity acidly acids
    acidulous Ackerman Ackley acknowledge acknowledgeable
    acknowledged acknowledgement acknowledgements acknowledger
    acknowledgers acknowledges acknowledging acknowledgment
    acknowledgments acme acne acolyte acolytes acorn acorns
    acoustic acoustical acoustically acoustician acoustics
    acquaint acquaintance acquaintances acquainted acquainting
    acquaints acquiesce acquiesced acquiescence acquiescent
    acquiesces acquiescing acquirable acquire acquired acquires
    acquiring acquisition acquisitions
}

blt::combomenu .b.m.m.m \
    -bg $bg \
    -textvariable myText1 \
    -iconvariable myIcon1 \
    -disabledforeground grey75  \
    -disabledbackground grey98  \
    -disabledacceleratorforeground grey75  \
    -width { 0 400 } \
    -height { 0 500 } \
    -yscrollbar .b.m.m.m.ybar \
    -xscrollbar .b.m.m.m.xbar

.b.m.m.m listadd $labels \
    -icon $icon 

blt::tk::scrollbar .b.m.m.m.xbar  
blt::tk::scrollbar .b.m.m.m.ybar 

blt::tk::scrollbar .s -orient vertical -command { .b xview } 

bind BltComboEntry <3> {
    grab release [grab current]
}

blt::table . \
    0,0 .b -fill both 

blt::table configure . r0 -resize shrink

