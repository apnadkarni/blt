
package require BLT

set imgData {
    R0lGODlhEAANAMIAAAAAAH9/f///////AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM8WBrM+rAEQWmIb5KxiWjNInCkV32AJHRlGQBgDA7vdN4vUa8tC78qlrCWmvRKsJTquHkp
    ZTKAsiCtWq0JADs=
}

set icon2 [image create picture -file images/blt98.gif]
set icon [image create picture -data $imgData]
set bg white

set blt::features(enable_xshm) 0
parray blt::features
set image ""
option add *ComboEntry.takeFocus 1

if { [file exists ../library] } {
    set blt_library ../library
}

#    -postcommand {.e.m configure -width [winfo width .e] ; update} \
set myIcon ""
blt::comboentry .e \
    -image $image \
    -textvariable myText1 \
    -iconvariable myIcon1 \
    -arrowrelief flat \
    -textwidth 0 \
    -menu .e.m \
    -exportselection yes \
    -xscrollcommand { .s set }  \
    -arrowbackground white \
    -clearbutton yes 
	
#    -bg $bg 

blt::combomenu .e.m  \
    -restrictwidth min \
    -height 200 \
    -textvariable myText1 \
    -iconvariable myIcon1 \
    -yscrollbar .e.m.ybar \
    -xscrollbar .e.m.xbar
blt::tk::scrollbar .e.m.xbar -style tk
blt::tk::scrollbar .e.m.ybar -style xp -arrowcolor grey20
.e.m sort configure -auto 1

set onOff 1
set wwho ""
foreach {item type } { 
    Undo	command 
    X1		checkbutton 
    Y1		radiobutton
    Redo	checkbutton
    Cut		command
    Copy	checkbutton
    X2		checkbutton
    Y2		radiobutton
    Paste	checkbutton
    "Edit Options" separator
    "Select All" checkbutton
    X3		checkbutton
    Y3		radiobutton
    Find	cascade
    Replace	checkbutton
} {
    set char [string range $item 0 0] 
    .e.m add \
	-type $type \
	-text $item \
	-accel "Ctrl+$char" \
	-underline 0 \
	-tag "$type [string tolower $char]" \
	-icon $icon \
	-variable x$item 
    if { $type == "radiobutton" } {
	.e.m item configure $item -value $item 
    }
}
set X1 1
set Redo 1

.e.m item configure Find -menu .e.m.m
#-state disabled
.e.m item configure x -state disabled 
.e.m item configure radiobutton -variable wwho 
.e.m item configure Y1 -state disabled
set wwho Y1
blt::combomenu .e.m.m  \
    -bg $bg \
    -textvariable myText1 \
    -iconvariable myIcon1 \
    -height { 0 500 } \
    -yscrollbar .e.m.m.ybar \
    -xscrollbar .e.m.m.xbar 

blt::tk::scrollbar .e.m.m.xbar 
blt::tk::scrollbar .e.m.m.ybar 

set onOff 0
foreach item { 
    Undo X1 Y1 Redo Cut Copy X2 Y2 Paste "Select All" X3 Y3 Find Replace 
} {
    set char [string range $item 0 0] 
    .e.m.m add \
	-type checkbutton \
	-text "$item" \
	-accel "Ctrl+$char" \
	-accel "" \
	-underline 0 \
	-tag [string tolower $char] \
	-icon $icon \
	-variable $item 

}
set wwho2 ""
.e.m.m item configure Undo -type command
.e.m.m item configure Cut -type command 
.e.m.m item configure Find -type cascade -menu .e.m.m.m
#-state disabled
.e.m.m item configure Y3 -type command -image $icon2
.e.m.m item configure Undo -type command 
.e.m.m item configure Paste -type separator 
.e.m.m item configure x -state disabled 
.e.m.m item configure y -type radiobutton -variable wwho2

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

blt::combomenu .e.m.m.m \
    -bg $bg \
    -textvariable myText1 \
    -iconvariable myIcon1 \
    -width  { 0 400 } \
    -height { 0 500 } \
    -yscrollbar .e.m.m.m.ybar \
    -xscrollbar .e.m.m.m.xbar

.e.m.m.m listadd $labels \
    -icon $icon 

blt::tk::scrollbar .e.m.m.m.xbar
blt::tk::scrollbar .e.m.m.m.ybar 

blt::tk::scrollbar .s -orient vertical -command { .e xview } 

bind BltComboEntry <3> {
    grab release [grab current]
}

blt::table . \
    0,0 .e -fill x -anchor n 

#blt::table configure . r0 -resize none
#blt::table configure . r1 -resize both
