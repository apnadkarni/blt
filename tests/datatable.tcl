
package require BLT

if {[info procs test] != "test"} {
    source defs
}
if [file exists ../library] {
    set blt_library ../library
}

#set VERBOSE 1
test datatable.1 {datatable no args} {
    list [catch {blt::datatable} msg] $msg
} {1 {wrong # args: should be one of...
  blt::datatable create ?tableName?
  blt::datatable destroy ?tableName ...?
  blt::datatable exists tableName
  blt::datatable load tableName libpath
  blt::datatable names ?pattern ...?}}

test datatable.2 {datatable create #auto} {
    list [catch {blt::datatable create #auto} msg] $msg
} {0 ::datatable0}

test datatable.3 {datatable exists datatable0} {
    list [catch {blt::datatable exists datatable0} msg] $msg
} {0 1}

test datatable.4 {datatable destroy datatable0} {
    list [catch {blt::datatable destroy datatable0} msg] $msg
} {0 {}}

test datatable.5 {datatable exists datatable0} {
    list [catch {blt::datatable exists datatable0} msg] $msg
} {0 0}

test datatable.6 {datatable destroy datatable0} {
    list [catch {blt::datatable destroy datatable0} msg] $msg
} {1 {can't find table "datatable0"}}

test datatable.7 {datatable create #auto.suffix} {
    list [catch {
	blt::datatable create #auto.suffix
    } msg] $msg
} {0 ::datatable0.suffix}

test datatable.8 {datatable create prefix.#auto} {
    list [catch {
	blt::datatable create prefix.#auto
    } msg] $msg
} {0 ::prefix.datatable0}

test datatable.9 {datatable create prefix.#auto.suffix} {
    list [catch {
	blt::datatable create prefix.#auto.suffix
    } msg] $msg
} {0 ::prefix.datatable0.suffix}

test datatable.10 {datatable create prefix.#auto.suffix.#auto} {
    list [catch {
	blt::datatable create prefix.#auto.suffix.#auto
    } msg] $msg
} {0 ::prefix.datatable0.suffix.#auto}

test datatable.11 {blt::datatable destroy [blt::datatable names *datatable0*]} {
    list [catch {
	eval blt::datatable destroy [blt::datatable names *datatable0*]
    } msg] $msg
} {0 {}}

test datatable.12 {blt::datatable create} {
    list [catch {
	blt::datatable create
    } msg] $msg
} {0 ::datatable0}

test datatable.13 {blt::datatable create} {
    list [catch {
	blt::datatable create
    } msg] $msg
} {0 ::datatable1}

test datatable.14 {blt::datatable create fred} {
    list [catch {
	blt::datatable create fred
    } msg] $msg
} {0 ::fred}

test datatable.15 {blt::datatable create fred} {
    list [catch {
	blt::datatable create fred
    } msg] $msg
} {1 {a command "::fred" already exists}}

test datatable.16 {blt::datatable create if} {
    list [catch {
	blt::datatable create if
    } msg] $msg
} {1 {a command "::if" already exists}}

test datatable.17 {blt::datatable create (bad namespace)} {
    list [catch {
	blt::datatable create badNs::fred
    } msg] $msg
} {1 {unknown namespace "badNs"}}

test datatable.18 {blt::datatable create (wrong # args)} {
    list [catch {
	blt::datatable create a b
    } msg] $msg
} {1 {wrong # args: should be "blt::datatable create ?tableName?"}}

test datatable.19 {names} {
    list [catch {
	blt::datatable names
    } msg] [lsort $msg]
} {0 {::datatable0 ::datatable1 ::fred}}

test datatable.20 {names pattern)} {
    list [catch {
	blt::datatable names ::datatable*
    } msg] [lsort $msg]
} {0 {::datatable0 ::datatable1}}

test datatable.21 {names badPattern)} {
    list [catch {
	blt::datatable names badPattern*
    } msg] $msg
} {0 {}}

test datatable.22 {names pattern ::data* ::fred} {
    list [catch {
	blt::datatable names pattern ::data* ::fred
    } msg] $msg
} {0 {::fred ::datatable0 ::datatable1}}

test datatable.23 {destroy (no args)} {
    list [catch {
	blt::datatable destroy
    } msg] $msg
} {0 {}}

test datatable.24 {destroy badName} {
    list [catch {
	blt::datatable destroy badName
    } msg] $msg
} {1 {can't find table "badName"}}

test datatable.25 {destroy fred} {
    list [catch {
	blt::datatable destroy fred
    } msg] $msg
} {0 {}}

test datatable.26 {destroy datatable0 datatable1} {
    list [catch {
	blt::datatable destroy datatable0 datatable1
    } msg] $msg
} {0 {}}

test datatable.27 {create} {
    list [catch {
	blt::datatable create
    } msg] $msg
} {0 ::datatable0}

test datatable.28 {datatable0} {
    list [catch {
	datatable0
    } msg] $msg
} {1 {wrong # args: should be one of...
  datatable0 add tableName ?switches?
  datatable0 append rowName columnName ?value ...?
  datatable0 attach tableName
  datatable0 column op args...
  datatable0 copy tableName
  datatable0 dir path ?switches?
  datatable0 dump ?switches?
  datatable0 duplicate ?tableName?
  datatable0 emptyvalue ?newValue?
  datatable0 exists rowName columnName
  datatable0 export formatName args...
  datatable0 find exprString ?switches?
  datatable0 get rowName columnName ?defValue?
  datatable0 import formatName args...
  datatable0 keys ?columnName ...?
  datatable0 lappend rowName columnName ?value ...?
  datatable0 limits ?columnName?
  datatable0 lookup ?value...?
  datatable0 maximum ?columnName?
  datatable0 minimum ?columnName?
  datatable0 numcolumns ?numColumns?
  datatable0 numrows ?numRows?
  datatable0 restore ?switches?
  datatable0 row op args...
  datatable0 set ?rowName columnName value ...?
  datatable0 sort ?flags ...?
  datatable0 trace op args...
  datatable0 unset ?rowName columnName ...?
  datatable0 watch op args...}}

test datatable.29 {datatable0 badOp} {
    list [catch {
	datatable0 badOp
    } msg] $msg
} {1 {bad operation "badOp": should be one of...
  datatable0 add tableName ?switches?
  datatable0 append rowName columnName ?value ...?
  datatable0 attach tableName
  datatable0 column op args...
  datatable0 copy tableName
  datatable0 dir path ?switches?
  datatable0 dump ?switches?
  datatable0 duplicate ?tableName?
  datatable0 emptyvalue ?newValue?
  datatable0 exists rowName columnName
  datatable0 export formatName args...
  datatable0 find exprString ?switches?
  datatable0 get rowName columnName ?defValue?
  datatable0 import formatName args...
  datatable0 keys ?columnName ...?
  datatable0 lappend rowName columnName ?value ...?
  datatable0 limits ?columnName?
  datatable0 lookup ?value...?
  datatable0 maximum ?columnName?
  datatable0 minimum ?columnName?
  datatable0 numcolumns ?numColumns?
  datatable0 numrows ?numRows?
  datatable0 restore ?switches?
  datatable0 row op args...
  datatable0 set ?rowName columnName value ...?
  datatable0 sort ?flags ...?
  datatable0 trace op args...
  datatable0 unset ?rowName columnName ...?
  datatable0 watch op args...}}

test datatable.30 {datatable0 column (wrong \# args)} {
    list [catch {
	datatable0 column
    } msg] $msg
} {1 {wrong # args: should be "datatable0 column op args..."}}


test datatable.31 {datatable0 column badOp} {
    list [catch {
	datatable0 column badOp
    } msg] $msg
} {1 {bad operation "badOp": should be one of...
  datatable0 column copy srcColumn destColumn ?switches?
  datatable0 column create ?switches?
  datatable0 column delete ?columnName ...?
  datatable0 column duplicate ?columnName ...?
  datatable0 column empty columnName
  datatable0 column exists columnName
  datatable0 column extend numColumns ?switches?
  datatable0 column get columnName ?switches?
  datatable0 column index columnName
  datatable0 column indices ?pattern ...?
  datatable0 column join tableName ?switches?
  datatable0 column label columnName ?label?
  datatable0 column labels ?labelList?
  datatable0 column move fromColumn toColumn ?numColumns?
  datatable0 column names ?pattern ...?
  datatable0 column nonempty columnName
  datatable0 column set columnName rowName ?value ...?
  datatable0 column tag op args...
  datatable0 column type columnName ?typeName columnName typeName ...?
  datatable0 column unset columnName ?indices ...?
  datatable0 column values columnName ?valueList?}}

test datatable.32 {datatable0 row (wrong \# args)} {
    list [catch {
	datatable0 row
    } msg] $msg
} {1 {wrong # args: should be "datatable0 row op args..."}}


test datatable.33 {datatable0 row badOp} {
    list [catch {
	datatable0 row badOp
    } msg] $msg
} {1 {bad operation "badOp": should be one of...
  datatable0 row copy srcRow destRow ?switches?
  datatable0 row create ?switches...?
  datatable0 row delete ?rowName ...?
  datatable0 row duplicate ?rowName ...?
  datatable0 row empty rowName
  datatable0 row exists rowName
  datatable0 row extend numRows ?switches?
  datatable0 row get rowName ?switches?
  datatable0 row index rowName
  datatable0 row indices ?rowName ...?
  datatable0 row isheader rowName
  datatable0 row isnumeric rowName
  datatable0 row join srcTableName ?switches?
  datatable0 row label rowName ?label?
  datatable0 row labels ?labelList?
  datatable0 row move fromRow toRow ?numRows?
  datatable0 row names ?pattern ...?
  datatable0 row nonempty rowName
  datatable0 row set rowName columnName ?value...?
  datatable0 row tag op args...
  datatable0 row unset rowName ?indices...?
  datatable0 row values rowName ?valueList?}}

test datatable.34 {datatable0 numcolumns} {
    list [catch {datatable0 numcolumns} msg] $msg
} {0 0}

test datatable.35 {datatable0 column names} {
    list [catch {datatable0 column names} msg] $msg
} {0 {}}

test datatable.36 {datatable0 numcolumns badArg} {
    list [catch {datatable0 numcolumns badArg} msg] $msg
} {1 {expected integer but got "badArg"}}

test datatable.37 {datatable0 column -label xyz create} {
    list [catch {datatable0 column -label xyz create} msg] $msg
} {1 {bad operation "-label": should be one of...
  datatable0 column copy srcColumn destColumn ?switches?
  datatable0 column create ?switches?
  datatable0 column delete ?columnName ...?
  datatable0 column duplicate ?columnName ...?
  datatable0 column empty columnName
  datatable0 column exists columnName
  datatable0 column extend numColumns ?switches?
  datatable0 column get columnName ?switches?
  datatable0 column index columnName
  datatable0 column indices ?pattern ...?
  datatable0 column join tableName ?switches?
  datatable0 column label columnName ?label?
  datatable0 column labels ?labelList?
  datatable0 column move fromColumn toColumn ?numColumns?
  datatable0 column names ?pattern ...?
  datatable0 column nonempty columnName
  datatable0 column set columnName rowName ?value ...?
  datatable0 column tag op args...
  datatable0 column type columnName ?typeName columnName typeName ...?
  datatable0 column unset columnName ?indices ...?
  datatable0 column values columnName ?valueList?}}

test datatable.38 {column extend 5} {
    list [catch {datatable0 column extend 5} msg] $msg
} {0 {0 1 2 3 4}}

test datatable.39 {numcolumns} {
    list [catch {datatable0 numcolumns} msg] $msg
} {0 5}

test datatable.40 {column index @end} {
    list [catch {datatable0 column index @end} msg] $msg
} {0 4}

test datatable.41 {row extend 5} {
    list [catch {datatable0 row extend 5} msg] $msg
} {0 {0 1 2 3 4}}

test datatable.42 {numrows length} {
    list [catch {datatable0 numrows} msg] $msg
} {0 5}

test datatable.43 {column index @end} {
    list [catch {datatable0 row index @end} msg] $msg
} {0 4}

test datatable.44 {column index @all} {
    list [catch {datatable0 column index @all} msg] $msg
} {1 {multiple columns specified by "all"}}

test datatable.45 {column names} {
    list [catch {datatable0 column names} msg] $msg
} {0 {c1 c2 c3 c4 c5}}

test datatable.46 {column indices (no args) } {
    list [catch {datatable0 column indices} msg] $msg
} {0 {}}

test datatable.47 {column indices c1} {
    list [catch {datatable0 column indices c1} msg] $msg
} {0 0}

test datatable.48 {column indices c1 c2 c3} {
    list [catch {datatable0 column indices c1 c2 c3} msg] $msg
} {0 {0 1 2}}

test datatable.49 {column indices c3 c2 c1} {
    list [catch {datatable0 column indices c3 c2 c1} msg] $msg
} {0 {2 1 0}}

test datatable.50 {column indices c5 c4 c1} {
    list [catch {datatable0 column indices c5 c4 c1} msg] $msg
} {0 {4 3 0}}

test datatable.51 {column indices c5 c4 badLabel} {
    list [catch {datatable0 column indices c5 c4 badLabel} msg] $msg
} {0 {4 3 -1}}

test datatable.52 {column indices c5 c5 c5} {
    list [catch {datatable0 column indices c5 c5 c5} msg] $msg
} {0 {4 4 4}}

test datatable.53 {column create -label c5} {
    list [catch {datatable0 column create -label c5} msg] $msg
} {0 5}

test datatable.54 {column indices c5} {
    list [catch {datatable0 column indices c5} msg] [lsort $msg]
} {0 {4 5}}

test datatable.55 {column delete 4} {
    list [catch {datatable0 column delete 4} msg] $msg
} {0 {}}

test datatable.56 {column indices c5} {
    list [catch {datatable0 column indices c5} msg] [lsort $msg]
} {0 4}

test datatable.57 {column index @end} {
    list [catch {datatable0 column index @end} msg] $msg
} {0 4}

test datatable.58 {column index @end badArg} {
    list [catch {datatable0 column index @end badArg} msg] $msg
} {1 {wrong # args: should be "datatable0 column index columnName"}}

test datatable.59 {column label 0} {
    list [catch {datatable0 column label 0} msg] $msg
} {0 c1}

test datatable.60 {column label 0 myLabel} {
    list [catch {datatable0 column label 0 myLabel} msg] $msg
} {0 {}}

test datatable.61 {column label 0 myLabel} {
    list [catch {datatable0 column label 0 myLabel} msg] $msg
} {0 {}}

test datatable.62 {column label 1 myLabel} {
    list [catch {datatable0 column label 1 myLabel} msg] $msg
} {0 {}}

test datatable.63 {column label 1 myLabel2} {
    list [catch {datatable0 column label 1 myLabel2} msg] $msg
} {0 {}}

test datatable.64 {column label 1 myLabel2} {
    list [catch {datatable0 column label 1 myLabel2} msg] $msg
} {0 {}}

test datatable.65 {column labels} {
    list [catch {datatable0 column labels} msg] [lsort $msg]
} {0 {c3 c4 c5 myLabel myLabel2}}

test datatable.66 {column index myLabel} {
    list [catch {datatable0 column index myLabel} msg] $msg
} {0 0}

test datatable.67 {column label 0 newLabel} {
    list [catch {datatable0 column label 0 newLabel} msg] $msg
} {0 {}}

test datatable.68 {column labels} {
    list [catch {datatable0 column labels} msg] [lsort $msg]
} {0 {c3 c4 c5 myLabel2 newLabel}}

test datatable.69 {column index myLabel2} {
    list [catch {datatable0 column index myLabel2} msg] $msg
} {0 1}

test datatable.70 {column label 0} {
    list [catch {datatable0 column label 0} msg] $msg
} {0 newLabel}

test datatable.71 {column label @end end} {
    list [catch {datatable0 column label @end end} msg] $msg
} {0 {}}

test datatable.72 {column label @end endLabel} {
    list [catch {datatable0 column label @end endLabel} msg] $msg
} {0 {}}

test datatable.73 {column label @end label-with-minus} {
    list [catch {datatable0 column label @end label-with-minus} msg] $msg
} {0 {}}

test datatable.74 {column label @end 1abc} {
    list [catch {datatable0 column label @end 1abc} msg] $msg
} {0 {}}

test datatable.75 {column label @end -abc} {
    list [catch {datatable0 column label @end -abc} msg] $msg
} {1 {column label "-abc" can't start with a '-'.}}


test datatable.76 {column names} {
    list [catch {datatable0 column names} msg] $msg
} {0 {newLabel myLabel2 c3 c4 1abc}}

test datatable.77 {column indices newLabel} {
    list [catch {datatable0 column indices newLabel} msg] $msg
} {0 0}

test datatable.78 {column indices newLabel1} {
    list [catch {datatable0 column indices newLabel1} msg] $msg
} {0 -1}

test datatable.79 {column label 0-4 c1 } {
    list [catch {datatable0 column label 0-4 c1} msg] $msg
} {1 {multiple columns specified by "0-4"}}

test datatable.80 {column label 0 c1 1 c2 2 c3 3 c4 4 c5 } {
    list [catch {datatable0 column label 0 c1 1 c2 2 c3 3 c4 4 c5} msg] $msg
} {0 {}}

test datatable.81 {column label 0} {
    list [catch {datatable0 column label 0} msg] $msg
} {0 c1}

test datatable.82 {column label 1} {
    list [catch {datatable0 column label 1} msg] $msg
} {0 c2}

test datatable.83 {column label 2} {
    list [catch {datatable0 column label 2} msg] $msg
} {0 c3}

test datatable.84 {column label 3} {
    list [catch {datatable0 column label 3} msg] $msg
} {0 c4}

test datatable.85 {column label 4} {
    list [catch {datatable0 column label 4} msg] $msg
} {0 c5}

test datatable.86 {column label 5} {
    list [catch {datatable0 column label 5} msg] $msg
} {1 {bad column index "5"}}

test datatable.87 {column names} {
    list [catch {datatable0 column names} msg] $msg
} {0 {c1 c2 c3 c4 c5}}

test datatable.88 {column names c*} {
    list [catch {datatable0 column names c*} msg] $msg
} {0 {c1 c2 c3 c4 c5}}

test datatable.89 {column names {*[1-2]}} {
    list [catch {datatable0 column names {*[1-2]}} msg] $msg
} {0 {c1 c2}}

test datatable.90 {column names noMatch} {
    list [catch {datatable0 column names noMatch} msg] $msg
} {0 {}}

test datatable.91 {row label 0-4 r1} {
    list [catch {datatable0 row label 0-4 r1} msg] $msg
} {1 {multiple rows specified by "0-4"}}

test datatable.92 {row label 0 r1 1 r2 2 r3 3 r4 4 r5} {
    list [catch {datatable0 row label 0 r1 1 r2 2 r3 3 r4 4 r5} msg] $msg
} {0 {}}

test datatable.93 {row names} {
    list [catch {datatable0 row names} msg] $msg
} {0 {r1 r2 r3 r4 r5}}

test datatable.94 {row names r*} {
    list [catch {datatable0 row names r*} msg] $msg
} {0 {r1 r2 r3 r4 r5}}

test datatable.95 {row names noMatch} {
    list [catch {datatable0 row names noMatch} msg] $msg
} {0 {}}

test datatable.96 {column get} {
    list [catch {datatable0 column get} msg] $msg
} {1 {wrong # args: should be "datatable0 column get columnName ?switches?"}}

test datatable.97 {column get c1} {
    list [catch {datatable0 column get c1} msg] $msg
} {0 {0 {} 1 {} 2 {} 3 {} 4 {}}}

test datatable.98 {column get badColumn} {
    list [catch {datatable0 column get badColumn} msg] $msg
} {1 {unknown column specification "badColumn" in ::datatable0}}

test datatable.99 {column values} {
    list [catch {datatable0 column values} msg] $msg
} {1 {wrong # args: should be "datatable0 column values columnName ?valueList?"}}

test datatable.100 {column values c1} {
    list [catch {
	datatable0 column values c1
    } msg] $msg
} {0 {{} {} {} {} {}}}

test datatable.101 {column values c1 {1.01 2.01 3.01 4.01 5.01}} {
    list [catch {
	datatable0 column values c1 {1.01 2.01 3.01 4.01 5.01}
    } msg] $msg
} {0 {}}

test datatable.102 {column get 0} {
    list [catch {datatable0 column get 0} msg] $msg
} {0 {0 1.01 1 2.01 2 3.01 3 4.01 4 5.01}}

test datatable.103 {column set @all 0 1.0 1 2.0 2 3.0 3 4.0 4 5.0} {
    list [catch {
	datatable0 column set @all 0 1.0 1 2.0 2 3.0 3 4.0 4 5.0
    } msg] $msg
} {0 {}}

test datatable.104 {column get @all} {
    list [catch {datatable0 column get @all} msg] $msg
} {1 {multiple columns specified by "all"}}

test datatable.105 {column get 0-1} {
    list [catch {datatable0 column get 0-1} msg] $msg
} {1 {multiple columns specified by "0-1"}}

test datatable.106 {datatable0 column get 1} {
    list [catch {datatable0 column get 1} msg] $msg
} {0 {0 1.0 1 2.0 2 3.0 3 4.0 4 5.0}}

test datatable.107 {datatable0 column get 2} {
    list [catch {datatable0 column get 2} msg] $msg
} {0 {0 1.0 1 2.0 2 3.0 3 4.0 4 5.0}}

test datatable.108 {datatable0 column get @end} {
    list [catch {datatable0 column get @end} msg] $msg
} {0 {0 1.0 1 2.0 2 3.0 3 4.0 4 5.0}}

test datatable.109 {datatable0 column set 0 2 a 1 b 0 c} {
    list [catch {datatable0 column set 0 2 a 1 b 0 c} msg] $msg
} {0 {}}

test datatable.110 {column values 0} {
    list [catch {datatable0 column values 0} msg] $msg
} {0 {c b a 4.0 5.0}}

test datatable.111 {column set @end 0 x 1 y} {
    list [catch {datatable0 column set @end 0 x 1 y} msg] $msg
} {0 {}}

test datatable.112 {column values @end} {
    list [catch {datatable0 column values @end} msg] $msg
} {0 {x y 3.0 4.0 5.0}}

test datatable.113 {column index c5} {
    list [catch {datatable0 column index c5} msg] $msg
} {0 4}

test datatable.114 {column index -1} {
    list [catch {datatable0 column index -1} msg] $msg
} {0 -1}

test datatable.115 {column index 1000} {
    list [catch {datatable0 column index 1000} msg] $msg
} {0 -1}

test datatable.116 {column type 0} {
    list [catch {datatable0 column type 0} msg] $msg
} {0 string}

test datatable.117 {column type 1 integer} {
    list [catch {datatable0 column type 1 integer} msg] $msg
} {1 {expected integer but got "1.0"}}

test datatable.118 {column type 3 double} {
    list [catch {datatable0 column type 3 double} msg] $msg
} {0 {}}

test datatable.119 {column type 0 string} {
    list [catch {datatable0 column type 0 string} msg] $msg
} {0 {}}

test datatable.120 {column type 0 string badArg} {
    list [catch {datatable0 column type 0 string badArg} msg] $msg
} {1 {odd # of arguments: should ?index type?...}}

test datatable.121 {column type badTag string} {
    list [catch {datatable0 column type badTag string} msg] $msg
} {1 {unknown column specification "badTag" in ::datatable0}}

test datatable.122 {column tag names} {
    list [catch {datatable0 column tag names} msg] $msg
} {0 {all end}}

test datatable.123 {column type @all string} {
    list [catch {datatable0 column type @all string} msg] $msg
} {0 {}}

test datatable.124 {column tag badOp} {
    list [catch {datatable0 column tag badOp} msg] $msg
} {1 {bad tag operation "badOp": should be one of...
  datatable0 column tag add tagName ?columnName ...?
  datatable0 column tag delete tagName ?columnName ...?
  datatable0 column tag exists tagName ?columnName?
  datatable0 column tag forget ?tagName ...?
  datatable0 column tag get columnName ?pattern ...?
  datatable0 column tag indices ?tagName ...?
  datatable0 column tag labels ?tagName ...?
  datatable0 column tag names ?pattern ...?
  datatable0 column tag range from to ?tagName ...?
  datatable0 column tag set columnName ?tagName ...?
  datatable0 column tag unset columnName ?tagName ...?}}

test datatable.125 {column tag (missing args)} {
    list [catch {datatable0 column tag} msg] $msg
} {1 {wrong # args: should be one of...
  datatable0 column tag add tagName ?columnName ...?
  datatable0 column tag delete tagName ?columnName ...?
  datatable0 column tag exists tagName ?columnName?
  datatable0 column tag forget ?tagName ...?
  datatable0 column tag get columnName ?pattern ...?
  datatable0 column tag indices ?tagName ...?
  datatable0 column tag labels ?tagName ...?
  datatable0 column tag names ?pattern ...?
  datatable0 column tag range from to ?tagName ...?
  datatable0 column tag set columnName ?tagName ...?
  datatable0 column tag unset columnName ?tagName ...?}}

test datatable.126 {datatable0 column tag badOp} {
    list [catch {datatable0 column tag badOp} msg] $msg
} {1 {bad tag operation "badOp": should be one of...
  datatable0 column tag add tagName ?columnName ...?
  datatable0 column tag delete tagName ?columnName ...?
  datatable0 column tag exists tagName ?columnName?
  datatable0 column tag forget ?tagName ...?
  datatable0 column tag get columnName ?pattern ...?
  datatable0 column tag indices ?tagName ...?
  datatable0 column tag labels ?tagName ...?
  datatable0 column tag names ?pattern ...?
  datatable0 column tag range from to ?tagName ...?
  datatable0 column tag set columnName ?tagName ...?
  datatable0 column tag unset columnName ?tagName ...?}}

test datatable.127 {datatable0 column tag add} {
    list [catch {datatable0 column tag add} msg] $msg
} {1 {wrong # args: should be "datatable0 column tag add tagName ?columnName ...?"}}

test datatable.128 {datatable0 column tag add newTag (no columns)} {
    list [catch {datatable0 column tag add newTag} msg] $msg
} {0 {}}

test datatable.129 {datatable0 column tag add newTag badIndex} {
    list [catch {datatable0 column tag add newTag badIndex} msg] $msg
} {1 {unknown column specification "badIndex" in ::datatable0}}
    
test datatable.130 {datatable0 column tag add newTag 0} {
    list [catch {datatable0 column tag add newTag 0} msg] $msg
} {0 {}}

test datatable.131 {datatable0 column tag names} {
    list [catch {datatable0 column tag names} msg] $msg
} {0 {newTag all end}}


test datatable.132 {datatable0 column tag get 0} {
    list [catch {datatable0 column tag get 0} msg] [lsort $msg]
} {0 {all newTag}}

test datatable.133 {datatable0 column tag add newTag1 0 1 2} {
    list [catch {datatable0 column tag add newTag1 0 1 2} msg] $msg
} {0 {}}

test datatable.134 {datatable0 column tag names newTag1} {
    list [catch {datatable0 column tag names newTag1} msg] $msg
} {0 newTag1}

test datatable.135 {datatable0 column tag names newTag} {
    list [catch {datatable0 column tag names newTag} msg] $msg
} {0 newTag}

test datatable.136 {datatable0 column tag get 0} {
    list [catch {datatable0 column tag get 0} msg] $msg
} {0 {all newTag newTag1}}

test datatable.137 {datatable0 column tag add newTag2 @all} {
    list [catch {datatable0 column tag add newTag2 @all} msg] $msg
} {0 {}}

test datatable.138 {datatable0 column tag names} {
    list [catch {datatable0 column tag names} msg] $msg
} {0 {newTag2 newTag newTag1 all end}}

test datatable.139 {datatable0 column tag names newTag2} {
    list [catch {datatable0 column tag names newTag2} msg] $msg
} {0 newTag2}

test datatable.140 {datatable0 column tag names *Tag*} {
    list [catch {datatable0 column tag names *Tag*} msg] $msg
} {0 {newTag2 newTag newTag1}}

test datatable.141 {datatable0 column tag names all} {
    list [catch {datatable0 column tag names all} msg] $msg
} {0 all}

test datatable.142 {datatable0 column tag names end} {
    list [catch {datatable0 column tag names end} msg] $msg
} {0 end}

test datatable.143 {datatable0 column tag names all} {
    list [catch {datatable0 column tag names all} msg] $msg
} {0 all}

test datatable.144 {datatable0 column tag names e*} {
    list [catch {datatable0 column tag names e*} msg] $msg
} {0 end}

test datatable.145 {datatable0 column tag delete} {
    list [catch {datatable0 column tag delete} msg] $msg
} {1 {wrong # args: should be "datatable0 column tag delete tagName ?columnName ...?"}}

test datatable.146 {datatable0 column tag delete badTag} {
    list [catch {datatable0 column tag delete badTag} msg] $msg
} {0 {}}

test datatable.147 {column tag names} {
    list [catch {datatable0 column tag names} msg] $msg
} {0 {newTag2 newTag newTag1 all end}}

test datatable.148 {datatable0 column tag delete newTag1} {
    list [catch {datatable0 column tag delete newTag1} msg] $msg
} {0 {}}

test datatable.149 {datatable0 column tag delete newTags1 1} {
    list [catch {datatable0 column tag delete newTag1 1} msg] $msg
} {0 {}}

test datatable.150 {column tag delete newTag2 1} {
    list [catch {datatable0 column tag delete newTag2 1} msg] $msg
} {0 {}}

# Don't care if column tag doesn't exist.
test datatable.151 {column tag delete badTag 1} {
    list [catch {datatable0 column tag delete badTag 1} msg] $msg
} {0 {}}

test datatable.152 {column tag names} {
    list [catch {datatable0 column tag names} msg] $msg
} {0 {newTag2 newTag newTag1 all end}}


test datatable.153 {column tag delete someTag 1000} {
    list [catch {datatable0 column tag delete someTag 1000} msg] $msg
} {1 {bad column index "1000"}}

test datatable.154 {column tag delete end 1} {
    list [catch {datatable0 column tag delete end 1} msg] $msg
} {0 {}}

test datatable.155 {column tag delete all 1} {
    list [catch {datatable0 column tag delete all 1} msg] $msg
} {0 {}}

test datatable.156 {column tag forget} {
    list [catch {datatable0 column tag forget} msg] $msg
} {0 {}}

test datatable.157 {column tag forget all} {
    list [catch {datatable0 column tag forget all} msg] $msg
} {0 {}}

test datatable.158 {column tag forget newTag1} {
    list [catch {datatable0 column tag forget newTag1} msg] $msg
} {0 {}}

# Don't care if column tag doesn't exist.
test datatable.159 {column tag forget newTag1} {
    list [catch {datatable0 column tag forget newTag1} msg] $msg
} {0 {}}

test datatable.160 {column tag indices} {
    list [catch {datatable0 column tag indices} msg] $msg
} {0 {}}

test datatable.161 {column tag indices all} {
    list [catch {datatable0 column tag indices all} msg] $msg
} {0 {0 1 2 3 4}}

test datatable.162 {column tag indices end} {
    list [catch {datatable0 column tag indices end} msg] $msg
} {0 4}

test datatable.163 {column tag indices newTag} {
    list [catch {datatable0 column tag indices newTag} msg] $msg
} {0 0}

test datatable.164 {column tag range 0 2 midTag} {
    list [catch {datatable0 column tag range 0 2 midTag} msg] $msg
} {0 {}}

test datatable.165 {column tag indices midTag} {
    list [catch {datatable0 column tag indices midTag} msg] $msg
} {0 {0 1 2}}

test datatable.166 {column tag range 0 @end myTag} {
    list [catch {datatable0 column tag range 0 @end myTag} msg] $msg
} {0 {}}

test datatable.167 {column tag indices myTag} {
    list [catch {datatable0 column tag indices myTag} msg] $msg
} {0 {0 1 2 3 4}}

test datatable.168 {column tag range -1 0 myTag} {
    list [catch {datatable0 column tag range -1 0 myTag} msg] $msg
} {1 {unknown column specification "-1" in ::datatable0}}

test datatable.169 {column tag range 0 -1 myTag} {
    list [catch {datatable0 column tag range 0 -1 myTag} msg] $msg
} {1 {unknown column specification "-1" in ::datatable0}}

test datatable.170 {column tag range 0 1000 myTag} {
    list [catch {datatable0 column tag range 0 1000 myTag} msg] $msg
} {1 {bad column index "1000"}}

test datatable.171 {column tag forget myTag} {
    list [catch {datatable0 column tag forget myTag} msg] $msg
} {0 {}}

test datatable.172 {column tag set} {
    list [catch {datatable0 column tag set} msg] $msg
} {1 {wrong # args: should be "datatable0 column tag set columnName ?tagName ...?"}}

test datatable.173 {column tag set 0} {
    list [catch {datatable0 column tag set 0} msg] $msg
} {0 {}}

test datatable.174 {column tag set 0 myTag} {
    list [catch {datatable0 column tag set 0 myTag} msg] $msg
} {0 {}}

test datatable.175 {get 0 @myTag} {
    list [catch {datatable0 get 0 @myTag} msg] $msg
} {0 c}

test datatable.176 {get 0 0} {
    list [catch {datatable0 get 0 0} msg] $msg
} {0 c}

test datatable.177 {column get 0} {
    list [catch {datatable0 column get 0} msg] $msg
} {0 {0 c 1 b 2 a 3 4.0 4 5.0}}

test datatable.178 {column unset} {
    list [catch {datatable0 column unset} msg] $msg
} {1 {wrong # args: should be "datatable0 column unset columnName ?indices ...?"}}

test datatable.179 {column unset 0} {
    list [catch {datatable0 column unset 0} msg] $msg
} {0 {}}

test datatable.180 {column unset 0 @end} {
    list [catch {datatable0 column unset 0 @end} msg] $msg
} {0 {}}

test datatable.181 {column names} {
    list [catch {datatable0 column names} msg] $msg
} {0 {c1 c2 c3 c4 c5}}

test datatable.182 {column extend 4 badSwitch } {
    list [catch {datatable0 column extend 4 badSwitch} msg] $msg
} {1 {unknown switch "badSwitch"
following switches are available:
   -labels list}}

test datatable.183 {column names} {
    list [catch {datatable0 column names} msg] $msg
} {0 {c1 c2 c3 c4 c5}}

test datatable.184 {column extend} {
    list [catch {datatable0 column extend} msg] $msg
} {1 {wrong # args: should be "datatable0 column extend numColumns ?switches?"}}

test datatable.185 {column extend -1} {
    list [catch {datatable0 column extend -1} msg] $msg
} {1 {bad value "-1": can't be negative}}

test datatable.186 {column extend 0} {
    list [catch {datatable0 column extend 0} msg] $msg
} {0 {}}

if 0 {
test datatable.187 {column extend 10000000000 } {
    list [catch {datatable0 column extend 10000000000} msg] $msg
} {1 {can't extend table by 10000000000 columns: out of memory.}}
}
test datatable.188 {column extend -10 } {
    list [catch {datatable0 column extend -10} msg] $msg
} {1 {bad value "-10": can't be negative}}


test datatable.189 {column extend 10 } {
    list [catch {datatable0 column extend 10} msg] $msg
} {0 {5 6 7 8 9 10 11 12 13 14}}

test datatable.190 {column label 5 c6 6 c7 7 c8...} {
    list [catch {
	datatable0 column label \
		5 x6 6 x7 7 x8 8 x9 9 x10 10 x11 11 x12 12 x13 13 x14 14 x15
    } msg] $msg
} {0 {}}

test datatable.191 {column names} {
    list [catch {datatable0 column names} msg] $msg
} {0 {c1 c2 c3 c4 c5 x6 x7 x8 x9 x10 x11 x12 x13 x14 x15}}

test datatable.192 {column delete 9 } {
    list [catch {datatable0 column delete 9} msg] $msg
} {0 {}}

test datatable.193 {column names} {
    list [catch {datatable0 column names} msg] $msg
} {0 {c1 c2 c3 c4 c5 x6 x7 x8 x9 x11 x12 x13 x14 x15}}

test datatable.194 {column delete 10 } {
    list [catch {datatable0 column delete 10} msg] $msg
} {0 {}}

test datatable.195 {column names} {
    list [catch {datatable0 column names} msg] $msg
} {0 {c1 c2 c3 c4 c5 x6 x7 x8 x9 x11 x13 x14 x15}}

test datatable.196 {numcolumns} {
    list [catch {datatable0 numcolumns} msg] $msg
} {0 13}

test datatable.197 {column create} {
    list [catch {datatable0 column create} msg] $msg
} {0 13}

test datatable.198 {column names} {
    list [catch {datatable0 column names} msg] $msg
} {0 {c1 c2 c3 c4 c5 x6 x7 x8 x9 x11 x13 x14 x15 c17}}

test datatable.199 {column label @end fred} {
    list [catch {
	datatable0 column label @end fred
	datatable0 column names
    } msg] $msg
} {0 {c1 c2 c3 c4 c5 x6 x7 x8 x9 x11 x13 x14 x15 fred}}

test datatable.200 {column label @end c18} {
    list [catch {
	datatable0 column label @end c18
	datatable0 column names
    } msg] $msg
} {0 {c1 c2 c3 c4 c5 x6 x7 x8 x9 x11 x13 x14 x15 c18}}

test datatable.201 {column create -before 1 -badSwitch} {
    list [catch {datatable0 column create -badSwitch} msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -after column
   -after row
   -before column
   -before row
   -label string
   -tags tagList
   -type columnType}}

test datatable.202 {datatable0 column create -badSwitch -before 1} {
    list [catch {datatable0 column create -badSwitch} msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -after column
   -after row
   -before column
   -before row
   -label string
   -tags tagList
   -type columnType}}

test datatable.203 {datatable0 column create -before 1 -badSwitch arg} {
    list [catch {datatable0 column create -badSwitch arg} msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -after column
   -after row
   -before column
   -before row
   -label string
   -tags tagList
   -type columnType}}

test datatable.204 {datatable0 column create -before 1 -label nc1} {
    list [catch {datatable0 column create -before 1 -label nc1} msg] $msg
} {0 1}

test datatable.205 {datatable0 column create -before 2 -label nc2} {
    list [catch {datatable0 column create -before 2 -label nc2} msg] $msg
} {0 2}

test datatable.206 {datatable0 column create -after 2 -label nc3} {
    list [catch {datatable0 column create -after 2 -label nc3} msg] $msg
} {0 3}

test datatable.207 {datatable0 numcolumns} {
    list [catch {datatable0 numcolumns} msg] $msg
} {0 17}

test datatable.208 {datatable0 column index @end} {
    list [catch {datatable0 column index @end} msg] $msg
} {0 16}

test datatable.209 {datatable0 column names} {
    list [catch {datatable0 column names} msg] $msg
} {0 {c1 nc1 nc2 nc3 c2 c3 c4 c5 x6 x7 x8 x9 x11 x13 x14 x15 c18}}

test datatable.210 {datatable0 column create -after @end} {
    list [catch {datatable0 column create -after @end} msg] $msg
} {0 17}

test datatable.211 {datatable0 column create -after 0} {
    list [catch {datatable0 column create -after 0} msg] $msg
} {0 1}

test datatable.212 {datatable0 column create -label -one} {
    list [catch {datatable0 column create -label -one} msg] $msg
} {1 {column label "-one" can't start with a '-'.}}

test datatable.213 {datatable0 column create -label abc-one} {
    list [catch {datatable0 column create -label abc-one} msg] $msg
} {0 19}

test datatable.214 {datatable0 column create -label} {
    list [catch {datatable0 column create -label} msg] $msg
} {1 {value for "-label" missing}}


test datatable.215 {datatable0 column create -before -1} {
    list [catch {datatable0 column create -before -1} msg] $msg
} {1 {unknown column specification "-1" in ::datatable0}}

test datatable.216 {datatable0 numcolumns} {
    list [catch {datatable0 numcolumns} msg] $msg
} {0 20}

test datatable.217 {datatable0 column index fred} {
    list [catch {datatable0 column index fred} msg] $msg
} {0 -1}

test datatable.218 {datatable0 column index one} {
    list [catch {datatable0 column index one} msg] $msg
} {0 -1}

test datatable.219 {datatable0 column index @end} {
    list [catch {datatable0 column index @end} msg] $msg
} {0 19}

test datatable.220 {datatable0 column create -after 40} {
    list [catch {datatable0 column create -after 40} msg] $msg
} {1 {bad column index "40"}}

test datatable.221 {datatable0 column create -tags {myTag1 myTag2}} {
    list [catch {
	datatable0 column create -tags {myTag1 myTag2}
    } msg] $msg
} {0 20}

test datatable.222 {datatable0 column create -after @end -tags {myTag1 myTag2}} {
    list [catch {
	datatable0 column create -after @end -tags {myTag1 myTag2} 
    } msg] $msg
} {0 21}

test datatable.223 {datatable0 column tag indices myTag1 myTag2} {
    list [catch {
	datatable0 column tag indices myTag1 myTag2
    } msg] $msg
} {0 {20 21}}

test datatable.224 {datatable0 column tag indices myTag1 myTag2} {
    list [catch {
	datatable0 column tag indices myTag1 myTag2
    } msg] $msg
} {0 {20 21}}

test datatable.225 {blt::datatable create} {
    list [catch { blt::datatable create } msg] $msg
} {0 ::datatable1}

test datatable.226 {datatable1 numrows} {
    list [catch { datatable1 numrows } msg] $msg
} {0 0}

test datatable.227 {datatable1 numcolumns} {
    list [catch { datatable1 numrows } msg] $msg
} {0 0}

set orig [blt::datatable create]
$orig restore -data {
    i 5 6 0 0
    c 0 c1 string {}
    c 1 c2 string {}
    c 2 c3 string {}
    c 3 c4 string {}
    c 4 c5 string {}
    c 5 c6 string {}
    r 0 r1 {}
    r 1 r2 {}
    r 2 r3 {}
    r 3 r4 {}
    r 4 r5 {}
    d 0 0 1.0
    d 1 0 2.0
    d 2 0 3.0
    d 3 0 4.0
    d 4 0 5.0
    d 0 1 1.1
    d 1 1 2.1
    d 2 1 3.1
    d 3 1 4.1
    d 4 1 5.1
    d 0 2 1.2
    d 1 2 2.2
    d 2 2 3.2
    d 3 2 4.2
    d 4 2 5.2
    d 0 3 1.3
    d 1 3 2.3
    d 2 3 3.3
    d 3 3 4.3
    d 4 3 5.3
    d 0 4 1.4
    d 1 4 2.4
    d 2 4 3.4
    d 3 4 4.4
    d 4 4 5.4
    d 0 5 1.5
    d 1 5 2.5
    d 2 5 3.5
    d 3 5 4.5
    d 4 5 5.5
}

if 0 {
test datatable.228 {datatable1 dump} {
    list [catch {
	datatable1 import csv -file t2.csv
	datatable1 dump
    } msg] $msg
} {0 {i 5 6 0 0
c 0 c1 string {}
c 1 c2 string {}
c 2 c3 string {}
c 3 c4 string {}
c 4 c5 string {}
c 5 c6 string {}
r 0 r1 {}
r 1 r2 {}
r 2 r3 {}
r 3 r4 {}
r 4 r5 {}
d 0 0 1.0
d 1 0 2.0
d 2 0 3.0
d 3 0 4.0
d 4 0 5.0
d 0 1 1.1
d 1 1 2.1
d 2 1 3.1
d 3 1 4.1
d 4 1 5.1
d 0 2 1.2
d 1 2 2.2
d 2 2 3.2
d 3 2 4.2
d 4 2 5.2
d 0 3 1.3
d 1 3 2.3
d 2 3 3.3
d 3 3 4.3
d 4 3 5.3
d 0 4 1.4
d 1 4 2.4
d 2 4 3.4
d 3 4 4.4
d 4 4 5.4
d 0 5 1.5
d 1 5 2.5
d 2 5 3.5
d 3 5 4.5
d 4 5 5.5
}}

}
test datatable.229 {datatable1 restore -data} {
    list [catch {
	set table [blt::datatable create]
	$table restore -data {
	    i 5 6 0 0
	    c 0 c1 string {}
	    c 1 c2 string {}
	    c 2 c3 string {}
	    c 3 c4 string {}
	    c 4 c5 string {}
	    c 5 c6 string {}
	    r 0 r1 {}
	    r 1 r2 {}
	    r 2 r3 {}
	    r 3 r4 {}
	    r 4 r5 {}
	    d 0 0 1.0
	    d 1 0 2.0
	    d 2 0 3.0
	    d 3 0 4.0
	    d 4 0 5.0
	    d 0 1 1.1
	    d 1 1 2.1
	    d 2 1 3.1
	    d 3 1 4.1
	    d 4 1 5.1
	    d 0 2 1.2
	    d 1 2 2.2
	    d 2 2 3.2
	    d 3 2 4.2
	    d 4 2 5.2
	    d 0 3 1.3
	    d 1 3 2.3
	    d 2 3 3.3
	    d 3 3 4.3
	    d 4 3 5.3
	    d 0 4 1.4
	    d 1 4 2.4
	    d 2 4 3.4
	    d 3 4 4.4
	    d 4 4 5.4
	    d 0 5 1.5
	    d 1 5 2.5
	    d 2 5 3.5
	    d 3 5 4.5
	    d 4 5 5.5
	}
    } msg] $msg
} {0 {}}

test datatable.230 {datatable0 column move} {
    list [catch {datatable0 column move} msg] $msg
} {1 {wrong # args: should be "datatable0 column move fromColumn toColumn ?numColumns?"}}

test datatable.231 {datatable0 column move -1} {
    list [catch {datatable0 column move -1} msg] $msg
} {1 {wrong # args: should be "datatable0 column move fromColumn toColumn ?numColumns?"}}

test datatable.232 {datatable0 column move 50 50} {
    list [catch {datatable0 column move 50 50} msg] $msg
} {1 {bad column index "50"}}

test datatable.233 {datatable0 column move 10 50} {
    list [catch {datatable0 column move 10 50} msg] $msg
} {1 {bad column index "50"}}

test datatable.234 {datatable0 column move @all 10} {
    list [catch {datatable0 column move @all 10} msg] $msg
} {1 {multiple columns specified by "all"}}

test datatable.235 {column label} {
    list [catch {
	set nCols [datatable0 numcolumns]
	for { set i 0 } { $i < $nCols } { incr i } {
	    datatable0 column label $i c$i
	}
    } msg] $msg
} {0 {}}

test datatable.236 {duplicate} {
    list [catch { 
	set before [$orig column names]
	set dup [$orig duplicate]
	set after [$dup column names]
	blt::datatable destroy $dup
	expr { $before == $after }
    } msg] $msg
} {0 1}

test datatable.237 {duplicate} {
    list [catch { datatable1 duplicate datatable1 } msg] $msg
} {0 {}}

test datatable.238 {column names} {
    list [catch { $orig column names } msg] $msg
} {0 {c1 c2 c3 c4 c5 c6}}

test datatable.239 {row names} {
    list [catch { $orig row names } msg] $msg
} {0 {r1 r2 r3 r4 r5}}

test datatable.240 {duplicate} {
    list [catch { 
	set before [datatable1 column names]
	datatable1 duplicate datatable1 
	set after [datatable1 column names]
	expr { $before == $after }
    } msg] $msg
} {0 1}

test datatable.241 {duplicate} {
    list [catch { 
	set dup [$orig duplicate]
	set list [$dup column names]
	blt::datatable destroy $dup
	set list
    } msg] $msg
} {0 {c1 c2 c3 c4 c5 c6}}

test datatable.242 {duplicate} {
    list [catch { 
	datatable1 duplicate $orig
	datatable1 numcolumns
    } msg] $msg
} {0 6}

test datatable.243 {duplicate} {
    list [catch { 
	datatable1 duplicate $orig
	datatable1 numrows
    } msg] $msg
} {0 5}


test datatable.244 {duplicate} {
    list [catch { 
	datatable1 duplicate $orig
	datatable1 column names
    } msg] $msg
} {0 {c1 c2 c3 c4 c5 c6}}

test datatable.245 {datatable1 column move 0 9  0} {
    list [catch { 
	# This should be a no-op.
	datatable1 duplicate $orig
	set before [datatable1 column names]
	datatable0 column move 0 9 0
	set after [datatable1 column names]
	expr {$before == $after}
    } msg] $msg
} {0 1}

test datatable.246 {column move 0 0} {
    list [catch { 
	# This should be a no-op.
	datatable1 duplicate $orig
	set before [datatable1 column names]
	datatable0 column move 0 0
	set after [datatable1 column names]
	expr {$before == $after}
    } msg] $msg
} {0 1}

test datatable.247 {duplicate} {
    list [catch { 
	set before [$orig column names]
	set dup [$orig duplicate]
	set after [$dup column names]
	blt::datatable destroy $dup
	expr { $before == $after }
    } msg] $msg
} {0 1}

test datatable.248 {duplicate} {
    list [catch { datatable1 duplicate datatable1 } msg] $msg
} {0 {}}

test datatable.249 {column names} {
    list [catch { $orig column names } msg] $msg
} {0 {c1 c2 c3 c4 c5 c6}}

test datatable.250 {row names} {
    list [catch { $orig row names } msg] $msg
} {0 {r1 r2 r3 r4 r5}}

test datatable.251 {duplicate} {
    list [catch { 
	set before [datatable1 column names]
	datatable1 duplicate datatable1 
	set after [datatable1 column names]
	expr { $before == $after }
    } msg] $msg
} {0 1}

test datatable.252 {duplicate} {
    list [catch { 
	set dup [$orig duplicate]
	set list [$dup column names]
	blt::datatable destroy $dup
	set list
    } msg] $msg
} {0 {c1 c2 c3 c4 c5 c6}}

test datatable.253 {duplicate} {
    list [catch { 
	datatable1 duplicate $orig
	datatable1 numcolumns
    } msg] $msg
} {0 6}

test datatable.254 {duplicate} {
    list [catch { 
	datatable1 duplicate $orig
	datatable1 numrows
    } msg] $msg
} {0 5}


test datatable.255 {duplicate} {
    list [catch { 
	datatable1 duplicate $orig
	datatable1 column names
    } msg] $msg
} {0 {c1 c2 c3 c4 c5 c6}}


test datatable.256 {duplicate} {
    list [catch { 
	datatable1 duplicate $orig
	datatable1 column names
    } msg] $msg
} {0 {c1 c2 c3 c4 c5 c6}}

test datatable.257 {column move 0 2} {
    list [catch {
	datatable1 duplicate $orig
	datatable1 column move 0 2
	datatable1 column names
    } msg] $msg
} {0 {c2 c3 c1 c4 c5 c6}}

test datatable.258 {datatable0 column move -1 2} {
    list [catch {datatable0 column move -1 2} msg] $msg
} {1 {unknown column specification "-1" in ::datatable0}}

test datatable.259 {datatable0 column move 0 @end} {
    list [catch {
	datatable1 duplicate $orig
	datatable1 column move 0 @end
	datatable1 column names
    } msg] $msg
} {0 {c2 c3 c4 c5 c6 c1}}

test datatable.260 {find expr} {
    list [catch {
	datatable1 find { $c1 > 3.1 }
    } msg] $msg
} {0 {3 4}}

test datatable.261 {find expr} {
    list [catch {
	datatable1 find { $0 > 3.1 }
    } msg] $msg
} {0 {3 4}}

test datatable.262 {column values} {
    list [catch {
	datatable1 column values c1
    } msg] $msg
} {0 {1.0 2.0 3.0 4.0 5.0}}

test datatable.263 {label 3 columns same sameLabel} {
    list [catch {
	datatable1 column label 0 sameLabel
	datatable1 column label 1 sameLabel
	datatable1 column label 2 sameLabel
    } msg] $msg
} {0 {}}

test datatable.264 {column indices sameLabel} {
    list [catch {
	datatable1 column indices sameLabel
    } msg] [lsort $msg]
} {0 {0 1 2}}

test datatable.265 {datatable0 trace column} {
    list [catch {datatable0 trace column} msg] $msg
} {1 {wrong # args: should be "datatable0 trace column columnName how command"}}

test datatable.266 {trace column @all} {
    list [catch {datatable0 trace column @all} msg] $msg
} {1 {wrong # args: should be "datatable0 trace column columnName how command"}}
    
test datatable.267 {trace column @end} {
    list [catch {datatable0 trace column @end} msg] $msg
} {1 {wrong # args: should be "datatable0 trace column columnName how command"}}

test datatable.268 {trace column 1} {
    list [catch {datatable0 trace column 1} msg] $msg
} {1 {wrong # args: should be "datatable0 trace column columnName how command"}}

test datatable.269 {trace column 1 rwuc} {
    list [catch {datatable0 trace column 1 rwuc} msg] $msg
} {1 {wrong # args: should be "datatable0 trace column columnName how command"}}

test datatable.270 {trace column @all rwuc} {
    list [catch {datatable0 trace column @all rwuc} msg] $msg
} {1 {wrong # args: should be "datatable0 trace column columnName how command"}}

proc Doit { args } { 
    global mylist; lappend mylist $args 
}

test datatable.271 {trace column @all rwuc Doit} {
    list [catch {datatable1 trace column @all rwuc Doit} msg] $msg
} {0 trace0}

test datatable.272 {trace info trace0} {
    list [catch {datatable1 trace info trace0} msg] $msg
} {0 {name trace0 column all flags rwuc command {Doit ::datatable1}}}

test datatable.273 {test create trace} {
    list [catch {
	set mylist {}
	datatable1 set @all 0 20
	set mylist
    } msg] $msg
} {0 {{::datatable1 0 0 w} {::datatable1 1 0 w} {::datatable1 2 0 w} {::datatable1 3 0 w} {::datatable1 4 0 w}}}

test datatable.274 {test read trace} {
    list [catch {
	set mylist {}
	datatable1 column get 0
	set mylist
	} msg] $msg
} {0 {{::datatable1 0 0 r} {::datatable1 1 0 r} {::datatable1 2 0 r} {::datatable1 3 0 r} {::datatable1 4 0 r}}}

test datatable.275 {test write trace} {
    list [catch {
	set mylist {}
	datatable1 column set 1 1 a 2 b 3 c 4 d 5 e
	set mylist
	} msg] $msg
} {0 {{::datatable1 1 1 w} {::datatable1 2 1 w} {::datatable1 3 1 w} {::datatable1 4 1 w} {::datatable1 5 1 wc}}}

test datatable.276 {test unset trace} {
    list [catch {
	set mylist {}
	datatable1 column unset 0 @all
	set mylist
	} msg] $msg
} {0 {{::datatable1 0 0 u} {::datatable1 1 0 u} {::datatable1 2 0 u} {::datatable1 3 0 u} {::datatable1 4 0 u}}}

test datatable.277 {trace delete} {
    list [catch {datatable1 trace delete} msg] $msg
} {0 {}}

if 0 {
puts stderr [datatable0 column type @all double]
datatable0 column set 4 1 1.0 2 2.0 3 3.0 4 4.0 5 5.0
datatable0 column set 5 1 1.0 2 2.0 3 3.0 4 4.0 5 5.0
datatable0 column set 6 1 1.0 2 2.0 3 3.0 4 4.0 5 5.0
puts stderr [datatable0 export csv]
puts stderr [datatable0 export xml]
puts stderr 1,1=[datatable0 get 1 1]
}
test datatable.278 {trace delete badId} {
    list [catch {datatable1 trace delete badId} msg] $msg
} {1 {unknown trace "badId"}}

test datatable.279 {trace delete trace0} {
    list [catch {datatable1 trace delete trace0} msg] $msg
} {0 {}}

test datatable.280 {export -file} {
    list [catch {datatable1 export -file} msg] $msg
} {1 {can't export "-file": format not registered}}

test datatable.281 {export csv -file} {
    list [catch {datatable1 export csv -file} msg] $msg
} {1 {value for "-file" missing}}

test datatable.282 {exportfile csv -file /badDir/badFile } {
    list [catch {datatable1 export csv -file /badDir/badFile} msg] $msg
} {1 {couldn't open "/badDir/badFile": no such file or directory}}

test datatable.283 {exportfile csv -file @badChannel } {
    list [catch {datatable1 export csv -file @badChannel} msg] $msg
} {1 {can not find channel named "badChannel"}}

test datatable.284 {export csv -file table.csv} {
    list [catch {datatable1 export csv -file table.csv} msg] $msg
} {0 {}}

test datatable.285 {dup} {
    list [catch {datatable1 duplicate $orig} msg] $msg
} {0 {}}

test datatable.286 {column type @all double} {
    list [catch {datatable1 column type @all double} msg] $msg
} {0 {}}

test datatable.287 {export csv -rowlabels -columnlabels} {
    list [catch {datatable1 export csv -rowlabels -columnlabels} msg] $msg
} {0 {"*BLT*","c1","c2","c3","c4","c5","c6"
"r57",1.0,1.1,1.2,1.3,1.4,1.5
"r58",2.0,2.1,2.2,2.3,2.4,2.5
"r59",3.0,3.1,3.2,3.3,3.4,3.5
"r60",4.0,4.1,4.2,4.3,4.4,4.5
"r61",5.0,5.1,5.2,5.3,5.4,5.5
}}

test datatable.288 {column type @all string} {
    list [catch {datatable0 column type @all string} msg] $msg
} {0 {}}


test datatable.289 {dump -file table.dump} {
    list [catch {datatable0 dump -file table.dump} msg] $msg
} {0 {}}

test datatable.290 {datatable0 set (no args)} {
    list [catch { datatable0 set } msg] $msg
} {1 {wrong # args: should be "datatable0 set ?rowName columnName value ...?"}}

test datatable.291 {datatable0 set 1 (no column)} {
    list [catch { datatable0 set 1 } msg] $msg
} {1 {wrong # args: should be "datatable0 set ?rowName columnName value?..."}}

test datatable.292 {datatable0 set 1 1 (no value)} {
    list [catch { datatable0 set 1 1 } msg] $msg
} {1 {wrong # args: should be "datatable0 set ?rowName columnName value?..."}}

test datatable.293 {datatable0 set 1 1 1.0 3.0 (too many args)} {
    list [catch { datatable0 set 1 1 1.0 3.0} msg] $msg
} {1 {wrong # args: should be "datatable0 set ?rowName columnName value?..."}}

test datatable.294 {datatable0 set 1 1 1.0} {
    list [catch { datatable0 set 1 1 1.0 }  msg] $msg
} {0 {}}

test datatable.295 {datatable0 get 1 1} {
    list [catch { datatable0 get 1 1 }  msg] $msg
} {0 1.0}

test datatable.296 {datatable0 row exists newRow} {
    list [catch { datatable0 row exists newRow } msg] $msg
} {0 0}

test datatable.297 {datatable0 set newRow 1 abc} {
    list [catch { datatable0 set newRow 1 abc } msg] $msg
} {0 {}}

test datatable.298 {datatable0 get newRow 1} {
    list [catch { datatable0 get newRow 1 } msg] $msg
} {0 abc}

test datatable.299 {datatable0 row exists newRow} {
    list [catch { datatable0 row exists newRow } msg] $msg
} {0 1}

test datatable.300 {datatable0 column exists newColumn} {
    list [catch { datatable0 column exists newColumn } msg] $msg
} {0 0}

test datatable.301 {datatable0 set 1 newColumn def} {
    list [catch { datatable0 set 1 newColumn def } msg] $msg
} {0 {}}

test datatable.302 {datatable0 get 1 newColumn} {
    list [catch { datatable0 get 1 newColumn } msg] $msg
} {0 def}

test datatable.303 {datatable0 row delete newRow} {
    list [catch { datatable0 row delete newRow } msg] $msg
} {0 {}}

test datatable.304 {datatable0 row exists newRow} {
    list [catch { datatable0 row exists newRow } msg] $msg
} {0 0}

test datatable.305 {datatable0 column delete newColumn} {
    list [catch { datatable0 column delete newColumn } msg] $msg
} {0 {}}

test datatable.306 {datatable0 column exists newColumn} {
    list [catch { datatable0 row exists newRow } msg] $msg
} {0 0}

test datatable.307 {datatable0 set newRow newColumn abc} {
    list [catch { datatable0 set newRow newColumn abc } msg] $msg
} {0 {}}

test datatable.308 {datatable0 row delete newRow} {
    list [catch { datatable0 row delete newRow } msg] $msg
} {0 {}}

test datatable.309 {datatable0 column delete newColumn} {
    list [catch { datatable0 column delete newColumn } msg] $msg
} {0 {}}

test datatable.310 {datatable0 column type 1 double} {
    list [catch { datatable0 column type 1 double } msg] $msg
} {0 {}}

test datatable.311 {datatable0 set 1 1 1.0} {
    list [catch { 
	datatable0 set 1 1 1.0 
	datatable0 get 1 1
    } msg] $msg
} {0 1.0}

test datatable.312 {datatable0 set @end 1 1.0 @end 2 2.0 @end 3 3.0 @end 4 4.0 } {
    list [catch { 
	datatable0 set @end 1 1.0  @end 2 2.0 @end 3 3.0 @end 4 4.0 
    } msg] $msg
} {0 {}}

test datatable.313 {datatable0 set 6 1 1.0 6 } {
    list [catch { 
	datatable0 set 6 1 1.0  6
    } msg] $msg
} {1 {wrong # args: should be "datatable0 set ?rowName columnName value?..."}}

test datatable.314 {datatable0 set 1 1 abc} {
    list [catch { datatable0 set 1 1 abc } msg] $msg
} {1 {expected floating-point number but got "abc"}}

test datatable.315 {datatable0 column type 1 string} {
    list [catch { datatable0 column type 1 string } msg] $msg
} {0 {}}

test datatable.316 {datatable0 unset 1 1} {
    list [catch { datatable0 unset 1 1} msg] $msg
} {0 {}}

test datatable.317 {datatable0 append 1 1 abc (from empty)} {
    list [catch { datatable0 append 1 1 abc} msg] $msg
} {0 {}}

test datatable.318 {datatable0 get 1 1} {
    list [catch { datatable0 get 1 1 } msg] $msg
} {0 abc}

test datatable.319 {datatable0 append 1 1 def (from empty)} {
    list [catch { datatable0 append 1 1 def} msg] $msg
} {0 {}}

test datatable.320 {datatable0 get 1 1} {
    list [catch { datatable0 get 1 1 } msg] $msg
} {0 abcdef}

test datatable.321 {datatable0 get 1 2 defValue } {
    list [catch { datatable0 get 2 1 defValue } msg] $msg
} {0 defValue}

test datatable.322 {datatable0 get 1 2 defValue (too many args) } {
    list [catch { datatable0 get 2 1 defValue extraArg } msg] $msg
} {1 {wrong # args: should be "datatable0 get rowName columnName ?defValue?"}}

test datatable.323 {datatable0 append 1 1 123 456 789 (from empty)} {
    list [catch { datatable0 append 1 1 123 456 789 } msg] $msg
} {0 {}}

test datatable.324 {datatable0 get 1 1} {
    list [catch { datatable0 get 1 1 } msg] $msg
} {0 abcdef123456789}

test datatable.325 {datatable0 unset 1 1} {
    list [catch { datatable0 unset 1 1} msg] $msg
} {0 {}}

test datatable.326 {datatable0 unset 1 1} {
    list [catch { datatable0 unset 1 1} msg] $msg
} {0 {}}

test datatable.327 {datatable0 unset 1 1 1 } {
    list [catch { datatable0 unset 1 1 1 } msg] $msg
} {1 {wrong # args: should be "datatable0 unset ?rowName columnName ...?"}}

test datatable.328 {datatable0 unset @end 1 @end 2 @end 3 @end 4 } {
    list [catch { datatable0 unset @end 1  @end 2 @end 3 @end 4 } msg] $msg
} {0 {}}

test datatable.329 {datatable0 unset 0 0 } {
    list [catch { datatable0 unset 0 0 } msg] $msg
} {0 {}}

test datatable.330 {datatable0 unset 10000 10000 } {
    list [catch { datatable0 unset 10000 10000 } msg] $msg
} {0 {}}

test datatable.331 {datatable0 unset 10000 10000 } {
    list [catch { datatable0 unset 10000 10000 } msg] $msg
} {0 {}}

#---------------------

test datatable.332 {blt::datatable create} {
    list [catch {blt::datatable create} msg] $msg
} {0 ::datatable4}

test datatable.333 {column extend 5} {
    list [catch {datatable4 column extend 5} msg] $msg
} {0 {0 1 2 3 4}}

test datatable.334 {numrows} {
    list [catch {datatable4 numrows} msg] $msg
} {0 0}

test datatable.335 {numrows -10} {
    list [catch {datatable4 numrows -10} msg] $msg
} {1 {bad count "-10": # columns can't be negative.}}

test datatable.336 {numrows badArg} {
    list [catch {datatable4 numrows badArg} msg] $msg
} {1 {expected integer but got "badArg"}}

test datatable.337 {numrows 10} {
    list [catch {datatable4 numrows 10} msg] $msg
} {0 10}

test datatable.338 {numcolumns 10} {
    list [catch {datatable4 numrows 10} msg] $msg
} {0 10}

test datatable.339 {numrows 0} {
    list [catch {datatable4 numrows 0} msg] $msg
} {0 0}

test datatable.340 {numcolumns 0} {
    list [catch {datatable4 numcolumns 0} msg] $msg
} {0 0}

test datatable.341 {column names} {
    list [catch {datatable4 column names} msg] $msg
} {0 {}}

test datatable.342 {row names} {
    list [catch {datatable4 row names} msg] $msg
} {0 {}}

test datatable.343 {row -label xyz create} {
    list [catch {datatable4 row -label xyz create} msg] $msg
} {1 {bad operation "-label": should be one of...
  datatable4 row copy srcRow destRow ?switches?
  datatable4 row create ?switches...?
  datatable4 row delete ?rowName ...?
  datatable4 row duplicate ?rowName ...?
  datatable4 row empty rowName
  datatable4 row exists rowName
  datatable4 row extend numRows ?switches?
  datatable4 row get rowName ?switches?
  datatable4 row index rowName
  datatable4 row indices ?rowName ...?
  datatable4 row isheader rowName
  datatable4 row isnumeric rowName
  datatable4 row join srcTableName ?switches?
  datatable4 row label rowName ?label?
  datatable4 row labels ?labelList?
  datatable4 row move fromRow toRow ?numRows?
  datatable4 row names ?pattern ...?
  datatable4 row nonempty rowName
  datatable4 row set rowName columnName ?value...?
  datatable4 row tag op args...
  datatable4 row unset rowName ?indices...?
  datatable4 row values rowName ?valueList?}}

test datatable.344 {row extend 5} {
    list [catch {datatable4 row extend 5} msg] $msg
} {0 {0 1 2 3 4}}

test datatable.345 {numrows} {
    list [catch {datatable4 numrows} msg] $msg
} {0 5}

test datatable.346 {row index @end} {
    list [catch {datatable4 row index @end} msg] $msg
} {0 4}

test datatable.347 {row indices @all} {
    list [catch {datatable1 row indices @all} msg] $msg
} {0 {0 1 2 3 4}}

test datatable.348 {row indices 0-@end} {
    list [catch {datatable1 row indices "0-@end" } msg] $msg
} {0 {0 1 2 3 4}}

test datatable.349 {row indices 1-@all} {
    list [catch {datatable1 row indices "0-@all" } msg] $msg
} {1 {unknown row specification "0-@all" in ::datatable1}}

test datatable.350 {row indices 2-4} {
    list [catch {datatable1 row indices 2-4} msg] $msg
} {0 {2 3 4}}

test datatable.351 {row indices 2-5} {
    list [catch {datatable1 row indices 2-5} msg] $msg
} {1 {unknown row specification "2-5" in ::datatable1}}

test datatable.352 {row indices 5-2} {
    list [catch {datatable1 row indices 5-2} msg] $msg
} {1 {unknown row specification "5-2" in ::datatable1}}

test datatable.353 {row indices 4-2} {
    list [catch {datatable1 row indices 4-2} msg] $msg
} {0 {}}

test datatable.354 {row index @end} {
    list [catch {datatable1 row index @end} msg] $msg
} {0 4}

test datatable.355 {blt::datatable create} {
    list [catch {datatable4 duplicate $orig} msg] $msg
} {0 {}}


test datatable.356 {row index @end badArg} {
    list [catch {datatable1 row index @end badArg} msg] $msg
} {1 {wrong # args: should be "datatable1 row index rowName"}}

test datatable.357 {row label 0} {
    list [catch {datatable1 row label 0} msg] $msg
} {0 r57}

test datatable.358 {row label 0 myLabel} {
    list [catch {datatable1 row label 0 myLabel} msg] $msg
} {0 {}}

test datatable.359 {row label 0} {
    list [catch {datatable1 row label 0} msg] $msg
} {0 myLabel}

test datatable.360 {row label 1 myLabel} {
    list [catch {datatable1 row label 1 myLabel} msg] $msg
} {0 {}}

test datatable.361 {row label 0} {
    list [catch {datatable1 row label 0} msg] $msg
} {0 myLabel}

test datatable.362 {row label @end end} {
    list [catch {datatable1 row label @end end} msg] $msg
} {0 {}}

test datatable.363 {row label @end endLabel} {
    list [catch {datatable1 row label @end endLabel} msg] $msg
} {0 {}}

test datatable.364 {row label @end 1abc} {
    list [catch {datatable1 row label @end 1abc} msg] $msg
} {0 {}}

test datatable.365 {row label @end label-with-minus} {
    list [catch {datatable1 row label @end label-with-minus} msg] $msg
} {0 {}}

test datatable.366 {row label @end -abc} {
    list [catch {datatable1 row label @end -abc} msg] $msg
} {1 {row label "-abc" can't start with a '-'.}}

test datatable.367 {row names *Label} {
    list [catch {datatable1 row names *Label} msg] $msg
} {0 {myLabel myLabel}}

test datatable.368 {row names r*} {
    list [catch {datatable1 row names r*} msg] $msg
} {0 {r59 r60}}

test datatable.369 {row names *-with-*} {
    list [catch {datatable1 row names *-with-*} msg] $msg
} {0 label-with-minus}

test datatable.370 {datatable1 row names badPattern} {
    list [catch {datatable1 row names badPattern} msg] $msg
} {0 {}}

test datatable.371 {datatable1 row get myLabel} {
    list [catch {datatable1 row get myLabel} msg] $msg
} {1 {multiple rows specified by "myLabel"}}

test datatable.372 {datatable1 row get} {
    list [catch {datatable1 row get} msg] $msg
} {1 {wrong # args: should be "datatable1 row get rowName ?switches?"}}

test datatable.373 {datatable1 row set myLabel} {
    list [catch {datatable1 row set myLabel} msg] $msg
} {1 {wrong # args: should be "datatable1 row set rowName columnName ?value...?"}}

test datatable.374 {row set @all 1 1.0 2 2.0 3 3.0 4 4.0 5 5.0} {
    list [catch {
	datatable1 row set @all 1 1.0 2 2.0 3 3.0 4 4.0 5 5.0
    } msg] $msg
} {0 {}}

test datatable.375 {datatable1 row values 1} {
    list [catch {datatable1 row values 1} msg] $msg
} {0 {2.0 1.0 2.0 3.0 4.0 5.0}}

test datatable.376 {datatable1 row values @all} {
    list [catch {datatable1 row values @all} msg] $msg
} {1 {multiple rows specified by "all"}}

test datatable.377 {datatable1 row values 1-2} {
    list [catch {datatable1 row values 1-2} msg] $msg
} {1 {multiple rows specified by "1-2"}}

test datatable.378 {datatable1 row values 2} {
    list [catch {datatable1 row values 2} msg] $msg
} {0 {3.0 1.0 2.0 3.0 4.0 5.0}}

test datatable.379 {datatable1 row values 3} {
    list [catch {datatable1 row values 3} msg] $msg
} {0 {4.0 1.0 2.0 3.0 4.0 5.0}}

test datatable.380 {datatable1 row values @end} {
    list [catch {datatable1 row values @end} msg] $msg
} {0 {5.0 1.0 2.0 3.0 4.0 5.0}}

datatable1 duplicate $orig
datatable1 column type 0 double
test datatable.381 {datatable1 row values 0 { a b c }} {
    list [catch {datatable1 row values 0 { a b c }} msg] $msg
} {1 {expected floating-point number but got "a"}}

test datatable.382 {datatable1 row values 0 { 11.1 12.2 13.3 }} {
    list [catch {datatable1 row values 0 { 11.1 12.2 13.3 }} msg] $msg
} {0 {}}

test datatable.383 {datatable1 row values 1} {
    list [catch {datatable1 row values 1} msg] $msg
} {0 {2.0 2.1 2.2 2.3 2.4 2.5}}

test datatable.384 {datatable1 row values @end { -1 -2 }} {
    list [catch {datatable1 row values @end { -1 -2 }} msg] $msg
} {0 {}}

test datatable.385 {datatable1 row values @end} {
    list [catch {datatable1 row values @end} msg] $msg
} {0 {-1.0 -2 5.2 5.3 5.4 5.5}}

test datatable.386 {datatable1 row index label-with-minus} {
    list [catch {datatable1 row index label-with-minus} msg] $msg
} {0 -1}

test datatable.387 {datatable1 row indices @all} {
    list [catch {datatable1 row indices @all} msg] $msg
} {0 {0 1 2 3 4}}

test datatable.388 {datatable1 row index -1} {
    list [catch {datatable1 row index -1} msg] $msg
} {0 -1}

test datatable.389 {datatable1 row index 1000} {
    list [catch {datatable1 row index 1000} msg] $msg
} {0 -1}

test datatable.390 {datatable1 row tag badOp} {
    list [catch {datatable1 row tag badOp} msg] $msg
} {1 {bad tag operation "badOp": should be one of...
  datatable1 row tag add tagName ?rowName ...?
  datatable1 row tag delete tagName ?rowName ...?
  datatable1 row tag exists tagName ?rowName?
  datatable1 row tag forget ?tagName ...?
  datatable1 row tag get rowName ?pattern ...?
  datatable1 row tag indices ?tagName ...?
  datatable1 row tag labels ?tagName ...?
  datatable1 row tag names ?pattern ...?
  datatable1 row tag range fromRow toRow ?tagName ...?
  datatable1 row tag set rowName ?tagName ...?
  datatable1 row tag unset rowName ?tagName ...?}}

test datatable.391 {datatable1 row tag (missing args)} {
    list [catch {datatable1 row tag} msg] $msg
} {1 {wrong # args: should be one of...
  datatable1 row tag add tagName ?rowName ...?
  datatable1 row tag delete tagName ?rowName ...?
  datatable1 row tag exists tagName ?rowName?
  datatable1 row tag forget ?tagName ...?
  datatable1 row tag get rowName ?pattern ...?
  datatable1 row tag indices ?tagName ...?
  datatable1 row tag labels ?tagName ...?
  datatable1 row tag names ?pattern ...?
  datatable1 row tag range fromRow toRow ?tagName ...?
  datatable1 row tag set rowName ?tagName ...?
  datatable1 row tag unset rowName ?tagName ...?}}

test datatable.392 {datatable1 row tag badOp} {
    list [catch {datatable1 row tag badOp} msg] $msg
} {1 {bad tag operation "badOp": should be one of...
  datatable1 row tag add tagName ?rowName ...?
  datatable1 row tag delete tagName ?rowName ...?
  datatable1 row tag exists tagName ?rowName?
  datatable1 row tag forget ?tagName ...?
  datatable1 row tag get rowName ?pattern ...?
  datatable1 row tag indices ?tagName ...?
  datatable1 row tag labels ?tagName ...?
  datatable1 row tag names ?pattern ...?
  datatable1 row tag range fromRow toRow ?tagName ...?
  datatable1 row tag set rowName ?tagName ...?
  datatable1 row tag unset rowName ?tagName ...?}}

test datatable.393 {datatable1 row tag add} {
    list [catch {datatable1 row tag add} msg] $msg
} {1 {wrong # args: should be "datatable1 row tag add tagName ?rowName ...?"}}

test datatable.394 {datatable1 row tag add 1} {
    list [catch {datatable1 row tag add 1} msg] $msg
} {1 {tag "1" can't be a number.}}

test datatable.395 {datatable1 row tag add tag badIndex} {
    list [catch {datatable1 row tag add tag badIndex} msg] $msg
} {1 {unknown row specification "badIndex" in ::datatable1}}

test datatable.396 {datatable1 row tag add newTag 1} {
    list [catch {datatable1 row tag add newTag 1} msg] $msg
} {0 {}}

test datatable.397 {datatable1 row tag add newTag1 1} {
    list [catch {datatable1 row tag add newTag1 1} msg] $msg
} {0 {}}

test datatable.398 {datatable1 row tag add newTag2 1} {
    list [catch {datatable1 row tag add newTag2 1} msg] $msg
} {0 {}}

test datatable.399 {datatable1 row tag names} {
    list [catch {datatable1 row tag names} msg] $msg
} {0 {newTag2 tag newTag newTag1 all end}}


test datatable.400 {datatable1 row tag names *Tag*} {
    list [catch {datatable1 row tag names *Tag*} msg] [lsort $msg]
} {0 {newTag newTag1 newTag2}}

test datatable.401 {datatable1 row tag names all} {
    list [catch {datatable1 row tag names all} msg] $msg
} {0 all}

test datatable.402 {datatable1 row tag names end} {
    list [catch {datatable1 row tag names end} msg] $msg
} {0 end}

test datatable.403 {datatable1 row tag names all} {
    list [catch {datatable1 row tag names all} msg] $msg
} {0 all}

test datatable.404 {datatable1 row tag names end} {
    list [catch {datatable1 row tag names end} msg] $msg
} {0 end}

test datatable.405 {datatable1 row tag nameds e*} {
    list [catch {datatable1 row tag names e*} msg] $msg
} {0 end}

test datatable.406 {datatable1 row tag delete} {
    list [catch {datatable1 row tag delete} msg] $msg
} {1 {wrong # args: should be "datatable1 row tag delete tagName ?rowName ...?"}}

test datatable.407 {datatable1 row tag delete someTag} {
    list [catch {datatable1 row tag delete someTag} msg] $msg
} {0 {}}

test datatable.408 {datatable1 row tag delete newTag1 1} {
    list [catch {datatable1 row tag delete newTag1 1} msg] $msg
} {0 {}}

test datatable.409 {datatable1 row tag delete newTag1 1} {
    list [catch {datatable1 row tag delete newTag1 1} msg] $msg
} {0 {}}

test datatable.410 {datatable1 row tag delete newTag2 1} {
    list [catch {datatable1 row tag delete newTag2 1} msg] $msg
} {0 {}}

# It's okay to delete a row tag that doesn't exist.
test datatable.411 {datatable1 row tag delete badTag 1} {
    list [catch {datatable1 row tag delete badTag 1} msg] $msg
} {0 {}}

test datatable.412 {row tag names} {
    list [catch {datatable0 row tag names} msg] $msg
} {0 {all end}}

test datatable.413 {datatable1 row tag delete someTag 1000} {
    list [catch {datatable1 row tag delete someTag 1000} msg] $msg
} {1 {bad row index "1000"}}

test datatable.414 {datatable1 row tag delete end 1} {
    list [catch {datatable1 row tag delete end 1} msg] $msg
} {0 {}}

test datatable.415 {datatable1 row tag delete all 1} {
    list [catch {datatable1 row tag delete all 1} msg] $msg
} {0 {}}

test datatable.416 {datatable1 row tag forget} {
    list [catch {datatable1 row tag forget} msg] $msg
} {0 {}}

test datatable.417 {row tag forget all} {
    list [catch {datatable1 row tag forget all} msg] $msg
} {0 {}}

test datatable.418 {row tag forget newTag1} {
    list [catch {datatable1 row tag forget newTag1} msg] $msg
} {0 {}}

# It's okay to forget a row tag that doesn't exist.
test datatable.419 {row tag forget newTag1} {
    list [catch {datatable1 row tag forget newTag1} msg] $msg
} {0 {}}

test datatable.420 {row tag indices} {
    list [catch {datatable1 row tag indices} msg] $msg
} {0 {}}

test datatable.421 {row tag indices all} {
    list [catch {datatable1 row tag indices all} msg] $msg
} {0 {0 1 2 3 4}}

test datatable.422 {row tag indices end} {
    list [catch {datatable1 row tag indices end} msg] $msg
} {0 4}

test datatable.423 {row tag indices newTag} {
    list [catch {datatable1 row tag indices newTag} msg] $msg
} {0 1}

test datatable.424 {row tag range 1 3 midTag} {
    list [catch {datatable1 row tag range 1 3 midTag} msg] $msg
} {0 {}}

test datatable.425 {row tag indices midTag} {
    list [catch {datatable1 row tag indices midTag} msg] $msg
} {0 {1 2 3}}

test datatable.426 {row tag range 0 @end myTag} {
    list [catch {datatable1 row tag range 0 @end myTag} msg] $msg
} {0 {}}

test datatable.427 {row tag indices myTag} {
    list [catch {datatable1 row tag indices myTag} msg] $msg
} {0 {0 1 2 3 4}}

test datatable.428 {row tag range -1 0 myTag} {
    list [catch {datatable1 row tag range -1 0 myTag} msg] $msg
} {1 {unknown row specification "-1" in ::datatable1}}

test datatable.429 {row tag range 0 -1 myTag} {
    list [catch {datatable1 row tag range 0 -1 myTag} msg] $msg
} {1 {unknown row specification "-1" in ::datatable1}}

test datatable.430 {row tag range 0 1000 myTag} {
    list [catch {datatable1 row tag range 0 1000 myTag} msg] $msg
} {1 {bad row index "1000"}}

test datatable.431 {row unset} {
    list [catch {datatable1 row unset} msg] $msg
} {1 {wrong # args: should be "datatable1 row unset rowName ?indices...?"}}

test datatable.432 {row unset 0 @all} {
    list [catch {datatable1 row unset 0 @all} msg] $msg
} {0 {}}

test datatable.433 {row values 0} {
    list [catch {datatable1 row values 0} msg] $msg
} {0 {{} {} {} {} {} {}}}

test datatable.434 {row get 0} {
    list [catch {datatable1 row values 0} msg] $msg
} {0 {{} {} {} {} {} {}}}

test datatable.435 {dump } {
    list [catch {datatable1 dump} msg] $msg
} {0 {i 5 6 0 0
c 0 c1 double {}
c 1 c2 string {}
c 2 c3 string {}
c 3 c4 string {}
c 4 c5 string {}
c 5 c6 string {}
r 0 r62 {myTag}
r 1 r63 {myTag midTag newTag}
r 2 r64 {myTag midTag}
r 3 r65 {myTag midTag}
r 4 r66 {myTag}
d 1 0 2.0
d 2 0 3.0
d 3 0 4.0
d 4 0 -1
d 1 1 2.1
d 2 1 3.1
d 3 1 4.1
d 4 1 -2
d 1 2 2.2
d 2 2 3.2
d 3 2 4.2
d 4 2 5.2
d 1 3 2.3
d 2 3 3.3
d 3 3 4.3
d 4 3 5.3
d 1 4 2.4
d 2 4 3.4
d 3 4 4.4
d 4 4 5.4
d 1 5 2.5
d 2 5 3.5
d 3 5 4.5
d 4 5 5.5
}}

test datatable.436 {export csv} {
    list [catch {datatable1 export csv} msg] $msg
} {0 {"c1","c2","c3","c4","c5","c6"
,,,,,
2.0,"2.1","2.2","2.3","2.4","2.5"
3.0,"3.1","3.2","3.3","3.4","3.5"
4.0,"4.1","4.2","4.3","4.4","4.5"
-1,"-2","5.2","5.3","5.4","5.5"
}}

test datatable.437 {export xml} {
    list [catch {datatable1 export xml} msg] $msg
} {0 {<root>
  <row name="r62"/>
  <row name="r63" c1="2.0" c2="2.1" c3="2.2" c4="2.3" c5="2.4" c6="2.5"/>
  <row name="r64" c1="3.0" c2="3.1" c3="3.2" c4="3.3" c5="3.4" c6="3.5"/>
  <row name="r65" c1="4.0" c2="4.1" c3="4.2" c4="4.3" c5="4.4" c6="4.5"/>
  <row name="r66" c1="-1" c2="-2" c3="5.2" c4="5.3" c5="5.4" c6="5.5"/>
</root>
}}

test datatable.438 {dump} {
    list [catch {datatable1 dump} msg] $msg
} {0 {i 5 6 0 0
c 0 c1 double {}
c 1 c2 string {}
c 2 c3 string {}
c 3 c4 string {}
c 4 c5 string {}
c 5 c6 string {}
r 0 r62 {myTag}
r 1 r63 {myTag midTag newTag}
r 2 r64 {myTag midTag}
r 3 r65 {myTag midTag}
r 4 r66 {myTag}
d 1 0 2.0
d 2 0 3.0
d 3 0 4.0
d 4 0 -1
d 1 1 2.1
d 2 1 3.1
d 3 1 4.1
d 4 1 -2
d 1 2 2.2
d 2 2 3.2
d 3 2 4.2
d 4 2 5.2
d 1 3 2.3
d 2 3 3.3
d 3 3 4.3
d 4 3 5.3
d 1 4 2.4
d 2 4 3.4
d 3 4 4.4
d 4 4 5.4
d 1 5 2.5
d 2 5 3.5
d 3 5 4.5
d 4 5 5.5
}}

test datatable.439 {datatable1 row get 1 defValue} {
    list [catch {
	datatable1 emptyvalue defValue
	set out [datatable1 row get 1]
	datatable1 emptyvalue ""
	eval list $out
    } msg] $msg
} {0 {0 2.0 1 2.1 2 2.2 3 2.3 4 2.4 5 2.5}}

test datatable.440 {datatable1 row unset 1 @end} {
    list [catch {datatable1 row unset 1 @end} msg] $msg
} {0 {}}

test datatable.441 {datatable1 row extend 5 badArg } {
    list [catch {datatable1 row extend 5 badArg} msg] $msg
} {1 {unknown switch "badArg"
following switches are available:
   -labels list}}


test datatable.442 {datatable1 row extend} {
    list [catch {datatable1 row extend} msg] $msg
} {1 {wrong # args: should be "datatable1 row extend numRows ?switches?"}}

test datatable.443 {datatable1 row extend 1 -labels myRow} {
    list [catch {datatable1 row extend 1 -labels myRow} msg] $msg
} {0 5}

if 0 {
test datatable.444 {datatable1 row extend 10000000000 } {
    list [catch {datatable1 row extend 10000000000} msg] $msg
} {1 {can't extend table by 10000000000 rows: out of memory.}}
}

test datatable.445 {datatable1 row extend -10 } {
    list [catch {datatable1 row extend -10} msg] $msg
} {1 {bad value "-10": can't be negative}}

test datatable.446 {datatable1 row extend 10 } {
    list [catch {datatable1 row extend 10} msg] $msg
} {0 {6 7 8 9 10 11 12 13 14 15}}

test datatable.447 {datatable1 row names} {
    list [catch {datatable1 row names} msg] $msg
} {0 {r62 r63 r64 r65 r66 myRow r68 r69 r70 r71 r72 r73 r74 r75 r76 r77}}

test datatable.448 {datatable1 row delete 10 } {
    list [catch {datatable1 row delete 10} msg] $msg
} {0 {}}

test datatable.449 {datatable1 row names} {
    list [catch {datatable1 row names} msg] $msg
} {0 {r62 r63 r64 r65 r66 myRow r68 r69 r70 r71 r73 r74 r75 r76 r77}}


test datatable.450 {datatable1 row delete 10 } {
    list [catch {datatable1 row delete 10} msg] $msg
} {0 {}}

test datatable.451 {datatable1 numrows} {
    list [catch {datatable1 numrows} msg] $msg
} {0 14}

test datatable.452 {datatable1 row create} {
    list [catch {datatable1 row create} msg] $msg
} {0 14}

test datatable.453 {datatable1 row create -before 1 -badSwitch} {
    list [catch {datatable1 row create -badSwitch} msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -after column
   -after row
   -before column
   -before row
   -label string
   -tags tagList
   -type columnType}}

test datatable.454 {datatable1 row create -badSwitch -before 1} {
    list [catch {datatable1 row create -badSwitch} msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -after column
   -after row
   -before column
   -before row
   -label string
   -tags tagList
   -type columnType}}

test datatable.455 {datatable1 row create -before 1 -badSwitch arg} {
    list [catch {datatable1 row create -badSwitch arg} msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -after column
   -after row
   -before column
   -before row
   -label string
   -tags tagList
   -type columnType}}

test datatable.456 {datatable1 row create -before 1 -label one} {
    list [catch {datatable1 row create -before 1 -label one} msg] $msg
} {0 1}

test datatable.457 {datatable1 row create -before 2 -label two} {
    list [catch {datatable1 row create -before 2 -label two} msg] $msg
} {0 2}

test datatable.458 {datatable1 row create -after 3 -label three} {
    list [catch {datatable1 row create -after 3 -label three} msg] $msg
} {0 4}

test datatable.459 {datatable1 numrows} {
    list [catch {datatable1 numrows} msg] $msg
} {0 18}

test datatable.460 {datatable1 row names} {
    list [catch {datatable1 row names} msg] $msg
} {0 {r62 one two r63 three r64 r65 r66 myRow r68 r69 r70 r71 r74 r75 r76 r77 r78}}

test datatable.461 {datatable1 row index @end} {
    list [catch {datatable1 row index @end} msg] $msg
} {0 17}

test datatable.462 {datatable1 row names} {
    list [catch {datatable1 row names} msg] $msg
} {0 {r62 one two r63 three r64 r65 r66 myRow r68 r69 r70 r71 r74 r75 r76 r77 r78}}

test datatable.463 {datatable1 row create -after @end} {
    list [catch {datatable1 row create -after @end} msg] $msg
} {0 18}

test datatable.464 {datatable1 row create -after 1} {
    list [catch {datatable1 row create -after 1} msg] $msg
} {0 2}

test datatable.465 {datatable1 row create -label one} {
    list [catch {datatable1 row create -label one} msg] $msg
} {0 20}

test datatable.466 {datatable1 row create -label} {
    list [catch {datatable1 row create -label} msg] $msg
} {1 {value for "-label" missing}}

test datatable.467 {datatable1 row create -before 0} {
    list [catch {datatable1 row create -before 0} msg] $msg
} {0 0}

test datatable.468 {datatable1 numrows} {
    list [catch {datatable1 numrows} msg] $msg
} {0 22}

test datatable.469 {datatable1 row index fred} {
    list [catch {datatable1 row index fred} msg] $msg
} {0 -1}

test datatable.470 {datatable1 row index one} {
    list [catch {datatable1 row index one} msg] $msg
} {1 {multiple rows specified by "one"}}

test datatable.471 {datatable1 row indices one} {
    list [catch {datatable1 row indices one} msg] [lsort $msg]
} {0 {2 21}}

test datatable.472 {datatable1 row index @end} {
    list [catch {datatable1 row index @end} msg] $msg
} {0 21}

test datatable.473 {datatable1 row create -after 40} {
    list [catch {datatable1 row create -after 40} msg] $msg
} {1 {bad row index "40"}}

test datatable.474 {datatable1 row create -tags {myTag1 myTag2}} {
    list [catch {
	datatable1 row create -tags {myTag1 myTag2}
    } msg] $msg
} {0 22}

test datatable.475 {datatable1 row create -after @end -tags {myTag1 myTag2}} {
    list [catch {
	datatable1 row create -after @end -tags {myTag1 myTag2} 
    } msg] $msg
} {0 23}

test datatable.476 {datatable1 row tag indices myTag1 myTag2} {
    list [catch {
	datatable1 row tag indices myTag1 myTag2
    } msg] $msg
} {0 {22 23}}

test datatable.477 {datatable1 row tag indices myTag2 myTag1} {
    list [catch {
	datatable1 row tag indices myTag2 myTag1 
    } msg] $msg
} {0 {22 23}}

test datatable.478 {datatable1 row move} {
    list [catch {datatable1 row move} msg] $msg
} {1 {wrong # args: should be "datatable1 row move fromRow toRow ?numRows?"}}

test datatable.479 {datatable1 row move 0} {
    list [catch {datatable1 row move 0} msg] $msg
} {1 {wrong # args: should be "datatable1 row move fromRow toRow ?numRows?"}}

# Source equals destination.
test datatable.480 {datatable1 row move 0 0} {
    list [catch {datatable1 row move 0 0} msg] $msg
} {0 {}}

test datatable.481 {datatable1 row names} {
    list [catch {datatable1 row names} msg] $msg
} {0 {r85 r62 one r83 two r63 three r64 r65 r66 myRow r68 r69 r70 r71 r74 r75 r76 r77 r78 r82 one r86 r87}}

test datatable.482 {datatable1 row names} {
    list [catch {datatable1 row names} msg] $msg
} {0 {r85 r62 one r83 two r63 three r64 r65 r66 myRow r68 r69 r70 r71 r74 r75 r76 r77 r78 r82 one r86 r87}}

test datatable.483 {datatable1 row names} {
    list [catch {datatable1 row names} msg] $msg
} {0 {r85 r62 one r83 two r63 three r64 r65 r66 myRow r68 r69 r70 r71 r74 r75 r76 r77 r78 r82 one r86 r87}}

# Move 10 to 0. The row is labeled "myRow"
test datatable.484 {datatable1 row move 10 0} {
    list [catch {datatable1 row move 10 0} msg] $msg
} {0 {}}

test datatable.485 {datatable1 row names} {
    list [catch {datatable1 row names} msg] $msg
} {0 {myRow r85 r62 one r83 two r63 three r64 r65 r66 r68 r69 r70 r71 r74 r75 r76 r77 r78 r82 one r86 r87}}

# Can't move all rows.
test datatable.486 {datatable1 row move @all 10} {
    list [catch {datatable1 row move @all 10} msg] $msg
} {1 {multiple rows specified by "all"}}

# Move 1 to 10, 0 rows. Does nothing.
test datatable.487 {datatable1 row move 1 10 0} {
    list [catch { 
	datatable1 row move 1 10 0
	datatable1 row names
    } msg] $msg
} {0 {myRow r85 r62 one r83 two r63 three r64 r65 r66 r68 r69 r70 r71 r74 r75 r76 r77 r78 r82 one r86 r87}}

test datatable.488 {datatable1 row names} {
    list [catch {datatable1 row names} msg] $msg
} {0 {myRow r85 r62 one r83 two r63 three r64 r65 r66 r68 r69 r70 r71 r74 r75 r76 r77 r78 r82 one r86 r87}}


# Move 1 to 1. Does nothing.
test datatable.489 {datatable1 row move 1 1} {
    list [catch { 
	datatable1 row move 1 1 
	datatable1 row names
    } msg] $msg
} {0 {myRow r85 r62 one r83 two r63 three r64 r65 r66 r68 r69 r70 r71 r74 r75 r76 r77 r78 r82 one r86 r87}}

# Move 1 to 10 (1 row)
test datatable.490 {datatable1 row move 1 10} {
    list [catch { 
	datatable1 row move 1 10 
	datatable1 row names
    } msg] $msg
} {0 {myRow r62 one r83 two r63 three r64 r65 r66 r85 r68 r69 r70 r71 r74 r75 r76 r77 r78 r82 one r86 r87}}

# Move 1 to 2 (1 row)
test datatable.491 {datatable1 row move 1 2} {
    list [catch {
	datatable1 row move 1 2
	datatable1 row names
    } msg] $msg
} {0 {myRow one r62 r83 two r63 three r64 r65 r66 r85 r68 r69 r70 r71 r74 r75 r76 r77 r78 r82 one r86 r87}}

# Move 1 to 10 (5 rows)
test datatable.492 {datatable1 row move 1 10 5} {
    list [catch {
	datatable1 row move 1 10 5
	datatable1 row names
    } msg] $msg
} {0 {myRow three r64 r65 r66 r85 r68 r69 r70 r71 one r62 r83 two r63 r74 r75 r76 r77 r78 r82 one r86 r87}}

test datatable.493 {export csv} {
    list [catch {datatable1 export csv} msg] $msg
} {0 {"c1","c2","c3","c4","c5","c6"
,,,,,
,,,,,
3.0,"3.1","3.2","3.3","3.4","3.5"
4.0,"4.1","4.2","4.3","4.4","4.5"
-1,"-2","5.2","5.3","5.4","5.5"
,,,,,
,,,,,
,,,,,
,,,,,
,,,,,
,,,,,
,,,,,
,,,,,
,,,,,
2.0,"2.1","2.2","2.3","2.4",
,,,,,
,,,,,
,,,,,
,,,,,
,,,,,
,,,,,
,,,,,
,,,,,
,,,,,
}}

test datatable.494 {datatable1 row move 0 2} {
    list [catch {datatable1 row move 0 2} msg] $msg
} {0 {}}

test datatable.495 {datatable1 row move 1 17} {
    list [catch {datatable1 row move 1 17} msg] $msg
} {0 {}}

test datatable.496 {datatable1 trace row} {
    list [catch {datatable1 trace row} msg] $msg
} {1 {wrong # args: should be "datatable1 trace row rowName how command"}}

test datatable.497 {datatable1 trace row @all} {
    list [catch {datatable1 trace row @all} msg] $msg
} {1 {wrong # args: should be "datatable1 trace row rowName how command"}}
    
test datatable.498 {datatable1 trace row @end} {
    list [catch {datatable1 trace row @end} msg] $msg
} {1 {wrong # args: should be "datatable1 trace row rowName how command"}}

test datatable.499 {datatable1 trace row 1} {
    list [catch {datatable1 trace row 1} msg] $msg
} {1 {wrong # args: should be "datatable1 trace row rowName how command"}}

test datatable.500 {datatable1 trace row 1 rwuc} {
    list [catch {datatable1 trace row 1 rwuc} msg] $msg
} {1 {wrong # args: should be "datatable1 trace row rowName how command"}}

test datatable.501 {datatable1 trace row @all rwuc} {
    list [catch {datatable1 trace row @all rwuc} msg] $msg
} {1 {wrong # args: should be "datatable1 trace row rowName how command"}}

test datatable.502 {datatable1 trace names} {
    list [catch {datatable1 trace names} msg] $msg
} {0 {}}

proc Doit { args } { 
    global mylist; lappend mylist $args 
}

test datatable.503 {datatable1 trace row @all rwuc Doit} {
    list [catch {datatable1 trace row @all rwuc Doit} msg] $msg
} {0 trace1}

test datatable.504 {datatable1 trace names} {
    list [catch {datatable1 trace names} msg] $msg
} {0 trace1}


test datatable.505 {datatable1 trace info trace1} {
    list [catch {datatable1 trace info trace1} msg] $msg
} {0 {name trace1 row all flags rwuc command {Doit ::datatable1}}}

test datatable.506 {test create trace} {
    list [catch {
	set mylist {}
	datatable1 set @all 1 20
	set mylist
    } msg] $msg
} {0 {{::datatable1 0 1 wc} {::datatable1 1 1 wc} {::datatable1 2 1 w} {::datatable1 3 1 w} {::datatable1 4 1 wc} {::datatable1 5 1 wc} {::datatable1 6 1 wc} {::datatable1 7 1 wc} {::datatable1 8 1 wc} {::datatable1 9 1 wc} {::datatable1 10 1 wc} {::datatable1 11 1 wc} {::datatable1 12 1 wc} {::datatable1 13 1 w} {::datatable1 14 1 wc} {::datatable1 15 1 wc} {::datatable1 16 1 wc} {::datatable1 17 1 w} {::datatable1 18 1 wc} {::datatable1 19 1 wc} {::datatable1 20 1 wc} {::datatable1 21 1 wc} {::datatable1 22 1 wc} {::datatable1 23 1 wc}}}

test datatable.507 {test read trace} {
    list [catch {
	set mylist {}
	datatable1 row get 1
	set mylist
	} msg] $msg
} {0 {{::datatable1 1 0 r} {::datatable1 1 1 r} {::datatable1 1 2 r} {::datatable1 1 3 r} {::datatable1 1 4 r} {::datatable1 1 5 r}}}

test datatable.508 {test write trace} {
    list [catch {
	set mylist {}
	datatable1 row values 1 {101 102 103 104 105}
	set mylist
	} msg] $msg
} {0 {{::datatable1 1 0 wc} {::datatable1 1 1 w} {::datatable1 1 2 wc} {::datatable1 1 3 wc} {::datatable1 1 4 wc}}}

test datatable.509 {test unset trace} {
    list [catch {
	set mylist {}
	datatable1 row unset 1 @all
	set mylist
	} msg] $msg
} {0 {{::datatable1 1 0 u} {::datatable1 1 1 u} {::datatable1 1 2 u} {::datatable1 1 3 u} {::datatable1 1 4 u}}}

test datatable.510 {datatable1 trace delete} {
    list [catch {datatable1 trace delete} msg] $msg
} {0 {}}

#---------------------

test datatable.511 {datatable1 trace} {
    list [catch {datatable1 trace} msg] $msg
} {1 {wrong # args: should be one of...
  datatable1 trace cell rowName columnName how command
  datatable1 trace column columnName how command
  datatable1 trace delete ?traceName ...?
  datatable1 trace info traceName
  datatable1 trace names ?pattern ...?
  datatable1 trace row rowName how command}}

test datatable.512 {datatable1 trace cell} {
    list [catch {datatable1 trace cell} msg] $msg
} {1 {wrong # args: should be "datatable1 trace cell rowName columnName how command"}}

test datatable.513 {datatable1 trace cell 1} {
    list [catch {datatable1 trace cell 1} msg] $msg
} {1 {wrong # args: should be "datatable1 trace cell rowName columnName how command"}}

test datatable.514 {datatable1 trace cell 1 1 } {
    list [catch {datatable1 trace cell 1 1 } msg] $msg
} {1 {wrong # args: should be "datatable1 trace cell rowName columnName how command"}}

test datatable.515 {datatable1 trace cell 1 1 rwuc} {
    list [catch {datatable1 trace cell 1 1 rwuc} msg] $msg
} {1 {wrong # args: should be "datatable1 trace cell rowName columnName how command"}}

proc Doit args { global mylist; lappend mylist $args }

test datatable.516 {datatable1 trace names} {
    list [catch {datatable1 trace names} msg] $msg
} {0 trace1}

test datatable.517 {datatable1 trace cell 1 1 rwuc Doit} {
    list [catch {datatable1 trace cell 1 1 rwuc Doit} msg] $msg
} {0 trace2}

test datatable.518 {datatable1 trace names} {
    list [catch {datatable1 trace names} msg] $msg
} {0 {trace1 trace2}}

test datatable.519 {datatable1 trace info trace1} {
    list [catch {datatable1 trace info trace1} msg] $msg
} {0 {name trace1 row all flags rwuc command {Doit ::datatable1}}}

test datatable.520 {test create trace} {
    list [catch {
	set mylist {}
	datatable1 set 1 1 "newValue"
	set mylist
	} msg] $msg
} {0 {{::datatable1 1 1 wc} {::datatable1 1 1 wc}}}

test datatable.521 {test read trace} {
    list [catch {
	set mylist {}
	datatable1 get 1 1
	set mylist
	} msg] $msg
} {0 {{::datatable1 1 1 r} {::datatable1 1 1 r}}}

test datatable.522 {test write trace} {
    list [catch {
	set mylist {}
	datatable1 column values 1 { a b c e d }
	set mylist
	} msg] $msg
} {0 {{::datatable1 0 1 w} {::datatable1 1 1 w} {::datatable1 1 1 w} {::datatable1 2 1 w} {::datatable1 3 1 w} {::datatable1 4 1 w}}}

test datatable.523 {trace delete trace1} {
    list [catch {datatable1 trace delete trace1} msg] $msg
} {0 {}}

test datatable.524 {test write trace} {
    list [catch {
	set mylist {}
	datatable1 row values 1 { 1.01 2.02 3.03 4.04 5.05 }
	set mylist
	} msg] $msg
} {0 {{::datatable1 1 1 w}}}

test datatable.525 {test write trace} {
    list [catch {
	set mylist {}
	datatable1 set 1 1 "nextValue"
	set mylist
	} msg] $msg
} {0 {{::datatable1 1 1 w}}}

test datatable.526 {test unset trace} {
    list [catch {
	set mylist {}
	datatable1 unset 1 1
	set mylist
	} msg] $msg
} {0 {{::datatable1 1 1 u}}}

test datatable.527 {datatable1 trace delete} {
    list [catch {datatable1 trace delete} msg] $msg
} {0 {}}

test datatable.528 {datatable1 trace delete badId} {
    list [catch {datatable1 trace delete badId} msg] $msg
} {1 {unknown trace "badId"}}

test datatable.529 {datatable1 trace names} {
    list [catch {datatable1 trace names} msg] $msg
} {0 trace2}

test datatable.530 {datatable1 trace names trace*} {
    list [catch {datatable1 trace names trace*} msg] $msg
} {0 trace2}

test datatable.531 {datatable1 trace delete trace2} {
    list [catch {datatable1 trace delete trace2} msg] $msg
} {0 {}}

test datatable.532 {test create trace} {
    list [catch {
	set mylist {}
	datatable0 set @all newKey 20
	set mylist
	} msg] $msg
} {0 {}}

test datatable.533 {test unset trace} {
    list [catch {
	set mylist {}
	datatable0 unset @all newKey
	set mylist
	} msg] $msg
} {0 {}}

test datatable.534 {datatable0 dump -badSwitch} {
    list [catch {datatable0 dump -badSwitch} msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -rows rows
   -columns columns
   -file fileName}}


test datatable.535 {datatable0 dump -rows 1} {
    list [catch {datatable0 dump -rows 1} msg] $msg
} {0 {i 1 23 0 0
c 0 c0 string {newTag2 myTag midTag newTag}
c 1 c1 string {}
c 2 c2 string {}
c 3 c3 string {}
c 4 c4 string {}
c 5 c5 string {midTag}
c 6 c6 string {newTag2 midTag}
c 7 c7 string {newTag2}
c 8 c8 string {newTag2}
c 9 c9 string {}
c 10 c10 string {}
c 11 c11 string {}
c 12 c12 string {}
c 13 c13 string {}
c 14 c14 string {}
c 15 c15 string {}
c 16 c16 string {}
c 17 c17 string {}
c 18 c18 string {}
c 19 c19 string {}
c 20 c20 string {myTag2 myTag1}
c 21 c21 string {myTag2 myTag1}
c 22 newKey string {}
r 1 r2 {}
}}


test datatable.536 {datatable0 dump -columns 1} {
    list [catch {datatable0 dump -columns 1} msg] $msg
} {0 {i 5 1 0 0
c 1 c1 string {}
r 0 r1 {}
r 1 r2 {}
r 2 r3 {}
r 3 r4 {}
r 4 r5 {}
}}

test datatable.537 {dump -rows badTag} {
    list [catch {datatable0 dump -rows badTag} msg] $msg
} {1 {unknown row specification "badTag" in ::datatable0}}

test datatable.538 {column tag names} {
    list [catch {datatable0 column tag names} msg] $msg
} {0 {myTag2 newTag2 myTag midTag newTag myTag1 all end}}

test datatable.539 {dump -columns badTag} {
    list [catch {datatable0 dump -columns badTag} msg] $msg
} {1 {unknown column specification "badTag" in ::datatable0}}

test datatable.540 {dump -rows 1 -columns 1} {
    list [catch {datatable0 dump -rows 1 -columns 1} msg] $msg
} {0 {i 1 1 0 0
c 1 c1 string {}
r 1 r2 {}
}}

test datatable.541 {dump -file myout.dump} {
    list [catch {datatable0 dump -file myout.dump} msg] $msg
} {0 {}}

test datatable.542 {blt::datatable destroy datatable0} {
    list [catch {blt::datatable destroy datatable0} msg] $msg
} {0 {}}

test datatable.543 {blt::datatable create} {
    list [catch {blt::datatable create} msg] $msg
} {0 ::datatable0}

test datatable.544 {datatable0 column names} {
    list [catch {datatable0 column names} msg] $msg
} {0 {}}

test datatable.545 {datatable0 dump} {
    list [catch {datatable0 dump} msg] $msg
} {0 {i 0 0 0 0
}}

test datatable.546 {datatable0 restore} {
    list [catch {datatable0 restore} msg] $msg
} {1 {must set either -file and -data switch.}}

test datatable.547 {datatable1 dump} {
    list [catch {datatable1 dump} msg] $msg
} {0 {i 24 6 0 0
c 0 c1 double {}
c 1 c2 string {}
c 2 c3 string {}
c 3 c4 string {}
c 4 c5 string {}
c 5 c6 string {}
r 0 three {}
r 1 myRow {}
r 2 r65 {myTag midTag}
r 3 r66 {myTag}
r 4 r85 {}
r 5 r68 {}
r 6 r69 {}
r 7 r70 {}
r 8 r71 {}
r 9 one {}
r 10 r62 {myTag}
r 11 r83 {}
r 12 two {}
r 13 r63 {myTag midTag newTag}
r 14 r74 {}
r 15 r75 {}
r 16 r76 {}
r 17 r64 {myTag midTag}
r 18 r77 {}
r 19 r78 {}
r 20 r82 {}
r 21 one {}
r 22 r86 {myTag2 myTag1}
r 23 r87 {myTag2 myTag1}
d 1 0 1.01
d 2 0 4.0
d 3 0 -1
d 13 0 2.0
d 17 0 3.0
d 0 1 a
d 2 1 c
d 3 1 e
d 4 1 d
d 5 1 20
d 6 1 20
d 7 1 20
d 8 1 20
d 9 1 20
d 10 1 20
d 11 1 20
d 12 1 20
d 13 1 20
d 14 1 20
d 15 1 20
d 16 1 20
d 17 1 20
d 18 1 20
d 19 1 20
d 20 1 20
d 21 1 20
d 22 1 20
d 23 1 20
d 1 2 3.03
d 2 2 4.2
d 3 2 5.2
d 13 2 2.2
d 17 2 3.2
d 1 3 4.04
d 2 3 4.3
d 3 3 5.3
d 13 3 2.3
d 17 3 3.3
d 1 4 5.05
d 2 4 4.4
d 3 4 5.4
d 13 4 2.4
d 17 4 3.4
d 2 5 4.5
d 3 5 5.5
d 17 5 3.5
}}

test datatable.548 {dirtest dir /tmp} {
    list [catch {
	set table [blt::datatable create]
	file delete -force ./testdir
	file mkdir ./testdir
	file copy defs ./testdir
	$table dir ./testdir -permissions r -fields { type perms }
	set out [$table export csv]
	blt::datatable destroy $table
	file delete -force ./testdir
	set out
    } msg] $msg
} {0 {"name","type","perms"
"defs","file",420
}}



#----------------------
exit 0






















