#!../src/bltwish

package require BLT
source scripts/demo.tcl
#blt::bltdebug 100

set bg [blt::background create radial -lowcolor  grey70 -highcolor grey90 \
	-jitter 10 -relativeto .]

image create picture label1 -file ./images/folder.gif
image create picture label2 -file ./images/mini-book2.gif
image create picture testImage -file ./images/txtrflag.gif

blt::tabset .ts \
    -scrollcommand { .s set } \
    -scrollincrement 1  \
    -scrolltabs yes 

label .ts.l -image testImage

set attributes {
    "Graph \#1" pink	
    "Graph \#2" lightblue	
    "Graph \#3" orange
    "Graph \#5" yellow	
    "Barchart \#2" green
}

foreach { label color } $attributes {
    .ts insert end $label \
	-selectbackground ${color}3  \
	-background ${color}3 \
	-activebackground ${color}2
}

foreach item [.ts configure] {
    if { [llength $item] == 5 } {
	set switch [lindex $item 0]
	set config($switch) [lindex $item 4]
    }
}
.ts insert end -selectbackground salmon2 -background salmon3 \
    -selectbackground salmon3 -activebackground salmon2 -window .ts.l

set tabLabels { 
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
}
set extra {
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

foreach label $tabLabels {
    .ts insert end $label -image label1 -bg $bg
}

blt::tk::scrollbar .s -command { .ts view } -orient horizontal -borderwidth 1
blt::tk::label .side_l -text "-side" 
blt::combobutton .side -textvariable text(-side) -menu .side.m
blt::combomenu .side.m -textvariable text(-side)
array set side2rotate {
    top 0
    bottom 0
    left 90
    right 270
}
.side.m add -type radiobutton -text [.ts cget -side] 
.side.m add -type separator
.side.m add -type radiobutton -text "top" 
.side.m add -type radiobutton -text "bottom" 
.side.m add -type radiobutton -text "left" 
.side.m add -type radiobutton -text "right" 
.side.m item configure all -variable config(-side) \
    -command { .ts configure -side $config(-side) -rotate $side2rotate($config(-side)) }
.side.m select 0

blt::tk::label .iconpos_l -text "-iconposition" 
blt::combobutton .iconpos -textvariable text(-iconposition) -menu .iconpos.m 
blt::combomenu .iconpos.m -textvariable text(-iconposition)
.iconpos.m add -type radiobutton -text [.ts cget -iconposition]
.iconpos.m add -type separator
.iconpos.m add -type radiobutton -text "right" 
.iconpos.m add -type radiobutton -text "left"  
.iconpos.m add -type radiobutton -text "bottom"
.iconpos.m add -type radiobutton -text "top"   
.iconpos.m item configure all -variable config(-iconposition) \
    -command { .ts configure -iconposition $config(-iconposition) } 
.iconpos.m select 0

blt::tk::label .slant_l -text "-slant" 
blt::combobutton .slant -textvariable text(-slant) -menu .slant.m 
blt::combomenu .slant.m -textvariable text(-slant)
.slant.m add -type radiobutton -text [.ts cget -slant]
.slant.m add -type separator
.slant.m add -type radiobutton -text "none" 
.slant.m add -type radiobutton -text "left"
.slant.m add -type radiobutton -text "right"
.slant.m add -type radiobutton -text "both"
.slant.m item configure all -variable config(-slant) \
    -command { .ts configure -slant $config(-slant) }
.slant.m select 0
    
blt::tk::label .rotate_l -text "-rotate"
blt::combobutton .rotate -textvariable rotatelabel  -menu .rotate.m 
blt::combomenu .rotate.m -textvariable rotatelabel
.rotate.m add -type radiobutton -text [.ts cget -rotate]
.rotate.m add -type separator
.rotate.m add -type radiobutton -text "0" 
.rotate.m add -type radiobutton -text "90" 
.rotate.m add -type radiobutton -text "180" 
.rotate.m add -type radiobutton -text "270" 
.rotate.m add -type radiobutton -text "30" 
.rotate.m item configure all -variable rotate \
    -command { .ts configure -rotate $rotate }
.rotate.m select 0

blt::tk::label .scrolltabs_l -text "-scrolltabs" 
blt::combobutton .scrolltabs -textvariable text(-scrolltabs) \
    -menu .scrolltabs.m 
blt::combomenu .scrolltabs.m -textvariable text(-scrolltabs)
.scrolltabs.m add -type radiobutton -text [.ts cget -scrolltabs]
.scrolltabs.m add -type separator
.scrolltabs.m add -type radiobutton -text "No" -value "0" 
.scrolltabs.m add -type radiobutton -text "Yes" -value "1" 
.scrolltabs.m item configure all -variable config(-scrolltabs) \
    -command { .ts configure -scrolltabs $config(-scrolltabs) }
.scrolltabs.m select 1

blt::tk::label .xbutton_l -text "-xbutton" 
blt::combobutton .xbutton -textvariable text(-xbutton) \
    -menu .xbutton.m 
blt::combomenu .xbutton.m -textvariable text(-xbutton)
.xbutton.m add -type radiobutton -text [.ts cget -xbutton]
.xbutton.m add -type separator
.xbutton.m add -type radiobutton -text "No" -value "0" 
.xbutton.m add -type radiobutton -text "Yes" -value "1" 
.xbutton.m item configure all -variable config(-xbutton) \
    -command { .ts configure -xbutton $config(-xbutton) }
.xbutton.m select 0

if 0 {
blt::tk::label .showsingletab_l -text "-showsingletab" 
blt::combobutton .showsingletab -textvariable text(-showsingletab) \
    -menu .showsingletab.m 
blt::combomenu .showsingletab.m -textvariable text(-showsingletab)
.showsingletab.m add -type radiobutton -text [.ts cget -showsingletab]
.showsingletab.m add -type separator
.showsingletab.m add -type radiobutton -text "No" -value "0" 
.showsingletab.m add -type radiobutton -text "Yes" -value "1" 
.showsingletab.m item configure all -variable config(-showsingletab) \
    -command { .ts configure -showsingletab $config(-showsingletab) }
.showsingletab.m select 0
}

blt::tk::label .tiers_l -text "-tiers"
blt::combobutton .tiers -textvariable text(-tiers) -menu .tiers.m 
blt::combomenu .tiers.m -textvariable text(-tiers)
.tiers.m add -type radiobutton -text [.ts cget -tiers]
.tiers.m add -type separator
.tiers.m add -type radiobutton -text "1"
.tiers.m add -type radiobutton -text "2"
.tiers.m add -type radiobutton -text "3"
.tiers.m add -type radiobutton -text "4" 
.tiers.m add -type radiobutton -text "5"
.tiers.m add -type radiobutton -text "10" 
.tiers.m item configure all -variable tiers \
    -command { .ts configure -tiers $tiers }
.tiers.m select 0

blt::tk::label .tabwidth_l -text "-tabwidth"
blt::combobutton .tabwidth -textvariable text(-tabwidth) -menu .tabwidth.m 
blt::combomenu .tabwidth.m -textvariable text(-tabwidth)
.tabwidth.m add -type radiobutton -text [.ts cget -tabwidth]
.tabwidth.m add -type separator
.tabwidth.m add -type radiobutton -text "variable" 
.tabwidth.m add -type radiobutton -text "same"
.tabwidth.m add -type radiobutton -text ".5 inch" -value "0.5i" 
.tabwidth.m add -type radiobutton -text "1 inch" -value "1i" 
.tabwidth.m add -type radiobutton -text "2 inch" -value "2i" 
.tabwidth.m item configure all -variable config(-tabwidth) \
    -command { .ts configure -tabwidth $config(-tabwidth) }
.tabwidth.m select 0

blt::tk::label .justify_l -text "-justify" 
blt::combobutton .justify -textvariable text(-justify) -menu .justify.m 
blt::combomenu .justify.m -textvariable text(-justify)
.justify.m add -type radiobutton -text [.ts cget -justify]
.justify.m add -type separator
.justify.m add -type radiobutton -text "left" 
.justify.m add -type radiobutton -text "center"
.justify.m add -type radiobutton -text "right"
.justify.m item configure all -variable config(-justify) \
    -command { .ts configure -justify $config(-justify) }
.justify.m select 0

blt::tk::label .tearoff_l -text "-tearoff" 
blt::combobutton .tearoff -textvariable text(-tearoff) -menu .tearoff.m 
blt::combomenu .tearoff.m -textvariable text(-tearoff)
.tearoff.m add -type radiobutton -text [.ts cget -tearoff]
.tearoff.m add -type separator
.tearoff.m add -type radiobutton -text "No" -value "0" 
.tearoff.m add -type radiobutton -text "Yes" -value "1" 
.tearoff.m item configure all -variable config(-tearoff) \
    -command { .ts configure -tearoff $config(-tearoff) }
.tearoff.m select 0

blt::tk::label .highlightthickness_l -text "-highlightthickness" 
blt::combobutton .highlightthickness -textvariable text(-highlightthickness) \
    -menu .highlightthickness.m 
blt::combomenu .highlightthickness.m -textvariable text(-highlightthickness)
.highlightthickness.m add -type radiobutton -text [.ts cget -highlightthickness]
.highlightthickness.m add -type separator
.highlightthickness.m add -type radiobutton -text "0" 
.highlightthickness.m add -type radiobutton -text "1" 
.highlightthickness.m add -type radiobutton -text "2" 
.highlightthickness.m add -type radiobutton -text "3" 
.highlightthickness.m add -type radiobutton -text "10" 
.highlightthickness.m item configure all -variable config(-highlightthickness) \
    -command { .ts configure -highlightthickness $config(-highlightthickness) }
.highlightthickness.m select 0

blt::tk::label .outerpad_l -text "-outerpad" 
blt::combobutton .outerpad -textvariable text(-outerpad) -menu .outerpad.m 
blt::combomenu .outerpad.m -textvariable text(-outerpad)
.outerpad.m add -type radiobutton -text [.ts cget -outerpad]
.outerpad.m add -type separator
.outerpad.m add -type radiobutton -text "0" 
.outerpad.m add -type radiobutton -text "1" 
.outerpad.m add -type radiobutton -text "2" 
.outerpad.m add -type radiobutton -text "3" 
.outerpad.m add -type radiobutton -text "10" 
.outerpad.m item configure all -variable config(-outerpad) \
    -command { .ts configure -outerpad $config(-outerpad) }
.outerpad.m select 0

blt::tk::label .borderwidth_l -text "-borderwidth" 
blt::combobutton .borderwidth -textvariable text(-borderwidth) \
    -menu .borderwidth.m 
blt::combomenu .borderwidth.m -textvariable text(-borderwidth)
.borderwidth.m add -type radiobutton -text [.ts cget -borderwidth]
.borderwidth.m add -type separator
.borderwidth.m add -type radiobutton -text "0" 
.borderwidth.m add -type radiobutton -text "1" 
.borderwidth.m add -type radiobutton -text "2" 
.borderwidth.m add -type radiobutton -text "3" 
.borderwidth.m add -type radiobutton -text "10" 
.borderwidth.m item configure all -variable config(-borderwidth) \
    -command { .ts configure -borderwidth $config(-borderwidth) }
.borderwidth.m select 0

blt::tk::label .gap_l -text "-gap" 
blt::combobutton .gap -textvariable text(-gap) -menu .gap.m 
blt::combomenu .gap.m -textvariable text(-gap)
.gap.m add -type radiobutton -text [.ts cget -gap]
.gap.m add -type separator
.gap.m add -type radiobutton -text "0" 
.gap.m add -type radiobutton -text "1" 
.gap.m add -type radiobutton -text "2" 
.gap.m add -type radiobutton -text "3" 
.gap.m add -type radiobutton -text "10" 
.gap.m item configure all -variable config(-gap) \
    -command { .ts configure -gap $config(-gap) }
.gap.m select 0

blt::tk::label .relief_l -text "-relief" 
blt::combobutton .relief -textvariable text(-relief) -menu .relief.m 
blt::combomenu .relief.m -textvariable text(-relief)
.relief.m add -type radiobutton -text [.ts cget -relief]
.relief.m add -type separator
.relief.m add -type radiobutton -text "flat" 
.relief.m add -type radiobutton -text "sunken" 
.relief.m add -type radiobutton -text "raised" 
.relief.m add -type radiobutton -text "groove" 
.relief.m add -type radiobutton -text "ridge" 
.relief.m add -type radiobutton -text "solid" 
.relief.m item configure all -variable config(-relief) \
    -command { .ts configure -relief $config(-relief) }
.relief.m select 0

blt::tk::label .background_l -text "-background" 
blt::combobutton .background -textvariable text(-background) \
    -menu .background.m 
blt::combomenu .background.m -textvariable text(-background)
.background.m add -type radiobutton -text [.ts cget -background]
.background.m add -type separator
.background.m add -type radiobutton -text "grey" 
.background.m add -type radiobutton -text "white" 
.background.m add -type radiobutton -text "black" 
.background.m add -type radiobutton -text "lightblue" 
.background.m item configure all -variable config(-background) \
    -command { .ts configure -background $config(-background) }
.background.m select 0

blt::tk::label .activebackground_l -text "-activebackground" 
blt::combobutton .activebackground -textvariable text(-activebackground) \
    -menu .activebackground.m 
blt::combomenu .activebackground.m -textvariable text(-activebackground)
.activebackground.m add -type radiobutton -text [.ts cget -activebackground]
.activebackground.m add -type separator
.activebackground.m add -type radiobutton -text "grey" 
.activebackground.m add -type radiobutton -text "white" 
.activebackground.m add -type radiobutton -text "black" 
.activebackground.m add -type radiobutton -text "lightblue" 
.activebackground.m item configure all -variable config(-activebackground) \
    -command { .ts configure -activebackground $config(-activebackground) }
.activebackground.m select 0

blt::tk::label .activeforeground_l -text "-activeforeground" 
blt::combobutton .activeforeground -textvariable text(-activeforeground) \
    -menu .activeforeground.m 
blt::combomenu .activeforeground.m -textvariable text(-activeforeground)
.activeforeground.m add -type radiobutton -text [.ts cget -activeforeground]
.activeforeground.m add -type separator
.activeforeground.m add -type radiobutton -text "grey" 
.activeforeground.m add -type radiobutton -text "white" 
.activeforeground.m add -type radiobutton -text "black" 
.activeforeground.m add -type radiobutton -text "lightblue" 
.activeforeground.m item configure all -variable config(-activeforeground) \
    -command { .ts configure -activeforeground $config(-activeforeground) }
.activeforeground.m select 0

blt::tk::label .selectbackground_l -text "-selectbackground" 
blt::combobutton .selectbackground -textvariable text(-selectbackground) \
    -menu .selectbackground.m 
blt::combomenu .selectbackground.m -textvariable text(-selectbackground)
.selectbackground.m add -type radiobutton -text [.ts cget -selectbackground]
.selectbackground.m add -type separator
.selectbackground.m add -type radiobutton -text "grey" 
.selectbackground.m add -type radiobutton -text "white" 
.selectbackground.m add -type radiobutton -text "black" 
.selectbackground.m add -type radiobutton -text "lightblue" 
.selectbackground.m item configure all -variable config(-selectbackground) \
    -command { .ts configure -selectbackground $config(-selectbackground) }
.selectbackground.m select 0

blt::tk::label .troughcolor_l -text "-troughcolor" 
blt::combobutton .troughcolor -textvariable text(-troughcolor) \
    -menu .troughcolor.m 
blt::combomenu .troughcolor.m -textvariable text(-troughcolor)
.troughcolor.m add -type radiobutton -text [.ts cget -troughcolor]
.troughcolor.m add -type separator
.troughcolor.m add -type radiobutton -text "grey" 
.troughcolor.m add -type radiobutton -text "white" 
.troughcolor.m add -type radiobutton -text "black" 
.troughcolor.m add -type radiobutton -text "lightblue" 
.troughcolor.m item configure all -variable config(-troughcolor) \
    -command { .ts configure -troughcolor $config(-troughcolor) }
.troughcolor.m select 0

blt::tk::label .foreground_l -text "-foreground" 
blt::combobutton .foreground -textvariable text(-foreground) \
    -menu .foreground.m 
blt::combomenu .foreground.m -textvariable text(-foreground)
.foreground.m add -type radiobutton -text [.ts cget -foreground]
.foreground.m add -type separator
.foreground.m add -type radiobutton -text "grey" 
.foreground.m add -type radiobutton -text "white" 
.foreground.m add -type radiobutton -text "black" 
.foreground.m add -type radiobutton -text "lightblue" 
.foreground.m item configure all -variable config(-foreground) \
    -command { .ts configure -foreground $config(-foreground) }
.foreground.m select 0

blt::tk::label .outerborderwidth_l -text "-outerborderwidth" 
blt::combobutton .outerborderwidth -textvariable text(-outerborderwidth) \
    -menu .outerborderwidth.m 
blt::combomenu .outerborderwidth.m -textvariable text(-outerborderwidth)
.outerborderwidth.m add -type radiobutton -text [.ts cget -outerborderwidth]
.outerborderwidth.m add -type separator
.outerborderwidth.m add -type radiobutton -text "0" 
.outerborderwidth.m add -type radiobutton -text "1" 
.outerborderwidth.m add -type radiobutton -text "2" 
.outerborderwidth.m add -type radiobutton -text "3" 
.outerborderwidth.m add -type radiobutton -text "10" 
.outerborderwidth.m item configure all -variable config(-outerborderwidth) \
    -command { .ts configure -outerborderwidth $config(-outerborderwidth) }
.outerborderwidth.m select 0


blt::tk::label .outerrelief_l -text "-outerrelief" 
blt::combobutton .outerrelief -textvariable text(-outerrelief) \
    -menu .outerrelief.m 
blt::combomenu .outerrelief.m -textvariable text(-outerrelief)
.outerrelief.m add -type radiobutton -text [.ts cget -outerrelief]
.outerrelief.m add -type separator
.outerrelief.m add -type radiobutton -text "flat" 
.outerrelief.m add -type radiobutton -text "sunken" 
.outerrelief.m add -type radiobutton -text "raised" 
.outerrelief.m add -type radiobutton -text "groove" 
.outerrelief.m add -type radiobutton -text "ridge" 
.outerrelief.m add -type radiobutton -text "solid" 
.outerrelief.m item configure all -variable config(-outerrelief) \
    -command { .ts configure -outerrelief $config(-outerrelief) }
.outerrelief.m select 0

blt::tk::label .selectforeground_l -text "-selectforeground" 
blt::combobutton .selectforeground -textvariable text(-selectforeground) \
    -menu .selectforeground.m 
blt::combomenu .selectforeground.m -textvariable text(-selectforeground)
.selectforeground.m add -type radiobutton -text [.ts cget -selectforeground]
.selectforeground.m add -type separator
.selectforeground.m add -type radiobutton -text "grey" 
.selectforeground.m add -type radiobutton -text "white" 
.selectforeground.m add -type radiobutton -text "black" 
.selectforeground.m add -type radiobutton -text "lightblue" 
.selectforeground.m item configure all -variable config(-selectforeground) \
    -command { .ts configure -selectforeground $config(-selectforeground) }
.selectforeground.m select 0

blt::tk::label .selectpadx_l -text "-selectpadx" 
blt::combobutton .selectpadx -textvariable text(-selectpadx) \
    -menu .selectpadx.m 
blt::combomenu .selectpadx.m -textvariable text(-selectpadx)
.selectpadx.m add -type radiobutton -text [.ts cget -selectpadx]
.selectpadx.m add -type separator
.selectpadx.m add -type radiobutton -text "0" 
.selectpadx.m add -type radiobutton -text "1" 
.selectpadx.m add -type radiobutton -text "2" 
.selectpadx.m add -type radiobutton -text "3" 
.selectpadx.m add -type radiobutton -text "10" 
.selectpadx.m item configure all -variable config(-selectpadx) \
    -command { .ts configure -selectpadx $config(-selectpadx) }
.selectpadx.m select 0

blt::tk::label .selectpady_l -text "-selectpady" 
blt::combobutton .selectpady -textvariable text(-selectpady) \
    -menu .selectpady.m 
blt::combomenu .selectpady.m -textvariable text(-selectpady)
.selectpady.m add -type radiobutton -text [.ts cget -selectpady]
.selectpady.m add -type separator
.selectpady.m add -type radiobutton -text "0" 
.selectpady.m add -type radiobutton -text "1" 
.selectpady.m add -type radiobutton -text "2" 
.selectpady.m add -type radiobutton -text "3" 
.selectpady.m add -type radiobutton -text "10" 
.selectpady.m item configure all -variable config(-selectpady) \
    -command { .ts configure -selectpady $config(-selectpady) }
.selectpady.m select 0

blt::table . \
    .ts                   0,0 -fill both -rspan 25 \
    .activebackground_l   1,1 -anchor e \
    .activebackground     1,2 -fill x \
    .activeforeground_l   2,1 -anchor e \
    .activeforeground     2,2 -fill x \
    .background_l         3,1 -anchor e \
    .background           3,2 -fill x \
    .borderwidth_l        4,1 -anchor e \
    .borderwidth          4,2 -fill x \
    .gap_l                5,1 -anchor e \
    .gap                  5,2 -fill x \
    .foreground_l         6,1 -anchor e \
    .foreground           6,2 -fill x \
    .highlightthickness_l 7,1 -anchor e \
    .highlightthickness   7,2 -fill x \
    .iconpos_l            8,1 -anchor e \
    .iconpos              8,2 -fill x \
    .justify_l            9,1 -anchor e \
    .justify              9,2 -fill x \
    .outerborderwidth_l  10,1 -anchor e \
    .outerborderwidth    10,2 -fill x \
    .outerpad_l          11,1 -anchor e \
    .outerpad            11,2 -fill x \
    .outerrelief_l       12,1 -anchor e \
    .outerrelief         12,2 -fill x \
    .relief_l            13,1 -anchor e \
    .relief              13,2 -fill x \
    .rotate_l            14,1 -anchor e \
    .rotate              14,2 -fill x \
    .scrolltabs_l        15,1 -anchor e \
    .scrolltabs          15,2 -fill x \
    .selectbackground_l  16,1 -anchor e \
    .selectbackground    16,2 -fill x \
    .selectforeground_l  17,1 -anchor e \
    .selectforeground    17,2 -fill x \
    .selectpadx_l        18,1 -anchor e \
    .selectpadx          18,2 -fill x \
    .selectpady_l        19,1 -anchor e \
    .selectpady          19,2 -fill x \
    .side_l              20,1 -anchor e \
    .side                20,2 -fill x \
    .xbutton_l           21,1 -anchor e \
    .xbutton             21,2 -fill x \
    .slant_l             23,1 -anchor e \
    .slant               23,2 -fill x \
    .tabwidth_l          24,1 -anchor e \
    .tabwidth            24,2 -fill x \
    .tearoff_l           25,1 -anchor e \
    .tearoff             25,2 -fill x \
    .tiers_l             26,1 -anchor e \
    .tiers               26,2 -fill x \
    .troughcolor_l       27,1 -anchor e \
    .troughcolor         27,2 -fill x \
    .s                   27,0 -fill x 

foreach option { 
    borderwidth gap highlightthickness justify outerpad relief rotate 
    side slant tabwidth tearoff tiers iconposition
} {
    set $option [.ts cget -$option]
}

blt::table configure . r* c1 -resize none
blt::table configure . r24 -resize expand
focus .ts

.ts focus 0

if 0 {
set filecount 0
foreach file { graph1 graph2 graph3 graph5 barchart2 } {
    namespace eval $file {
	if { [string match graph* $file] } {
	    set graph [blt::graph .ts.$file]
	} else {
	    set graph [blt::barchart .ts.$file]
	}
	source scripts/$file.tcl
	.ts tab configure $filecount -window $graph -fill both 
	incr filecount
    }
}
}


.ts select 0
.ts activate 0
.ts focus 0
after 5000 {
    .ts tab configure 0 -state disabled
}
