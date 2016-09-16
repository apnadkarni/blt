
package require BLT

# Test switches
#
# -before, -after, -node for "insert" operation
#

if {[info procs test] != "test"} {
    source defs
}

if [file exists ../library] {
    set blt_library ../library
}

#set VERBOSE 1

test tree.1 {tree no args} {
    list [catch {blt::tree} msg] $msg
} {1 {wrong # args: should be one of...
  blt::tree create ?treeName?
  blt::tree destroy ?treeName ...?
  blt::tree exists treeName
  blt::tree load treeName libpath
  blt::tree names ?pattern ...?}}

test tree.2 {tree create #auto} {
    list [catch {blt::tree create #auto} msg] $msg
} {0 ::tree0}

test tree.3 {tree create #auto.suffix} {
    list [catch {blt::tree create #auto.suffix} msg] $msg
} {0 ::tree0.suffix}

test tree.4 {tree create prefix.#auto} {
    list [catch {blt::tree create prefix.#auto} msg] $msg
} {0 ::prefix.tree0}

test tree.5 {tree create prefix.#auto.suffix} {
    list [catch {blt::tree create prefix.#auto.suffix} msg] $msg
} {0 ::prefix.tree0.suffix}

test tree.6 {tree create prefix.#auto.suffix.#auto} {
    list [catch {blt::tree create prefix.#auto.suffix.#auto} msg] $msg
} {0 ::prefix.tree0.suffix.#auto}

test tree.7 {tree destroy [tree names *tree0*]} {
    list [catch {eval blt::tree destroy [blt::tree names *tree0*]} msg] $msg
} {0 {}}

test tree.8 {create} {
    list [catch {blt::tree create} msg] $msg
} {0 ::tree0}

test tree.9 {create} {
    list [catch {blt::tree create} msg] $msg
} {0 ::tree1}

test tree.10 {create fred} {
    list [catch {blt::tree create fred} msg] $msg
} {0 ::fred}

test tree.11 {create fred} {
    list [catch {blt::tree create fred} msg] $msg
} {1 {a command "::fred" already exists}}

test tree.12 {create if} {
    list [catch {blt::tree create if} msg] $msg
} {1 {a command "::if" already exists}}

test tree.13 {tree create (bad namespace)} {
    list [catch {blt::tree create badName::fred} msg] $msg
} {1 {unknown namespace "badName"}}

test tree.14 {tree create (wrong # args)} {
    list [catch {blt::tree create a b} msg] $msg
} {1 {wrong # args: should be "blt::tree create ?treeName?"}}

test tree.15 {tree names} {
    list [catch {blt::tree names} msg] [lsort $msg]
} {0 {::fred ::tree0 ::tree1}}

test tree.16 {tree names pattern)} {
    list [catch {blt::tree names ::tree*} msg] [lsort $msg]
} {0 {::tree0 ::tree1}}

test tree.17 {tree names badPattern)} {
    list [catch {blt::tree names badPattern*} msg] $msg
} {0 {}}

test tree.18 {tree names pattern arg (wrong # args)} {
    list [catch {blt::tree names pattern arg} msg] $msg
} {1 {wrong # args: should be "blt::tree names ?pattern ...?"}}

test tree.19 {tree destroy (no args)} {
    list [catch {blt::tree destroy} msg] $msg
} {0 {}}

test tree.20 {tree destroy badTree} {
    list [catch {blt::tree destroy badTree} msg] $msg
} {1 {can't find a tree named "badTree"}}

test tree.21 {tree destroy fred} {
    list [catch {blt::tree destroy fred} msg] $msg
} {0 {}}

test tree.22 {tree destroy tree0 tree1} {
    list [catch {blt::tree destroy tree0 tree1} msg] $msg
} {0 {}}

test tree.23 {create} {
    list [catch {blt::tree create} msg] $msg
} {0 ::tree0}

test tree.24 {tree0} {
    list [catch {tree0} msg] $msg
} {1 {wrong # args: should be one of...
  tree0 ancestor node1 node2
  tree0 append node key ?value ...?
  tree0 apply node ?switches ...?
  tree0 attach tree ?switches ...?
  tree0 children node ?first? ?last?
  tree0 copy parent ?tree? node ?switches ...?
  tree0 degree node
  tree0 delete ?node ...?
  tree0 depth ?node?
  tree0 dir node path ?switches ...?
  tree0 dump node ?switches ...?
  tree0 dup node
  tree0 exists node ?fileName?
  tree0 export format ?switches ...?
  tree0 find node ?switches ...?
  tree0 findchild node label
  tree0 firstchild node
  tree0 get node ?fieldName? ?defValue?
  tree0 import format ?switches ...?
  tree0 index label|list
  tree0 insert parent ?switches ...?
  tree0 isancestor node1 node2
  tree0 isbefore node1 node2
  tree0 isleaf node
  tree0 isroot node
  tree0 keys node ?node...?
  tree0 label node ?newLabel?
  tree0 lappend node fieldName ?value ...?
  tree0 lastchild node
  tree0 move node newParent ?switches ...?
  tree0 names node ?fieldName?
  tree0 next node
  tree0 nextsibling node
  tree0 notify args ...
  tree0 parent node
  tree0 path path ?args ...?
  tree0 position ?switches ...? node...
  tree0 previous node
  tree0 prevsibling node
  tree0 restore node ?switches ...?
  tree0 root 
  tree0 set node ?fieldName value ...?
  tree0 size node
  tree0 sort node ?switches ...?
  tree0 tag args ...
  tree0 trace args ...
  tree0 type node fieldName
  tree0 unset node ?fieldName ...?}}

test tree.25 {tree0 badOp} {
    list [catch {tree0 badOp} msg] $msg
} {1 {bad operation "badOp": should be one of...
  tree0 ancestor node1 node2
  tree0 append node key ?value ...?
  tree0 apply node ?switches ...?
  tree0 attach tree ?switches ...?
  tree0 children node ?first? ?last?
  tree0 copy parent ?tree? node ?switches ...?
  tree0 degree node
  tree0 delete ?node ...?
  tree0 depth ?node?
  tree0 dir node path ?switches ...?
  tree0 dump node ?switches ...?
  tree0 dup node
  tree0 exists node ?fileName?
  tree0 export format ?switches ...?
  tree0 find node ?switches ...?
  tree0 findchild node label
  tree0 firstchild node
  tree0 get node ?fieldName? ?defValue?
  tree0 import format ?switches ...?
  tree0 index label|list
  tree0 insert parent ?switches ...?
  tree0 isancestor node1 node2
  tree0 isbefore node1 node2
  tree0 isleaf node
  tree0 isroot node
  tree0 keys node ?node...?
  tree0 label node ?newLabel?
  tree0 lappend node fieldName ?value ...?
  tree0 lastchild node
  tree0 move node newParent ?switches ...?
  tree0 names node ?fieldName?
  tree0 next node
  tree0 nextsibling node
  tree0 notify args ...
  tree0 parent node
  tree0 path path ?args ...?
  tree0 position ?switches ...? node...
  tree0 previous node
  tree0 prevsibling node
  tree0 restore node ?switches ...?
  tree0 root 
  tree0 set node ?fieldName value ...?
  tree0 size node
  tree0 sort node ?switches ...?
  tree0 tag args ...
  tree0 trace args ...
  tree0 type node fieldName
  tree0 unset node ?fieldName ...?}}

test tree.26 {tree0 insert (wrong # args)} {
    list [catch {tree0 insert} msg] $msg
} {1 {wrong # args: should be "tree0 insert parent ?switches ...?"}}

test tree.27 {tree0 insert badParent} {
    list [catch {tree0 insert badParent} msg] $msg
} {1 {can't find tag or id "badParent" in ::tree0}}

test tree.28 {tree0 insert 1000} {
    list [catch {tree0 insert 1000} msg] $msg
} {1 {can't find tag or id "1000" in ::tree0}}

test tree.29 {tree0 insert 0} {
    list [catch {tree0 insert 0} msg] $msg
} {0 1}

test tree.30 {tree0 insert 0} {
    list [catch {tree0 insert 0} msg] $msg
} {0 2}

test tree.31 {tree0 insert root} {
    list [catch {tree0 insert root} msg] $msg
} {0 3}

test tree.32 {tree0 insert all} {
    list [catch {tree0 insert all} msg] $msg
} {1 {more than one node tagged as "all"}}

test tree.33 {tree0 insert 0 -at badPosition} {
    list [catch {tree0 insert 0 -at badPosition} msg] $msg
} {1 {expected integer but got "badPosition"}}

test tree.34 {tree0 insert 0 -at -1} {
    list [catch {tree0 insert 0 -at -1} msg] $msg
} {1 {bad value "-1": can't be negative}}

test tree.35 {tree0 insert 0 -at 1000} {
    list [catch {tree0 insert 0 -at 1000} msg] $msg
} {0 4}

test tree.36 {tree0 insert 0 -at (no arg)} {
    list [catch {tree0 insert 0 -at} msg] $msg
} {1 {value for "-at" missing}}

test tree.37 {tree0 insert 0 -tags myTag} {
    list [catch {tree0 insert 0 -tags myTag} msg] $msg
} {0 5}

test tree.38 {tree0 insert 0 -tags {myTag1 myTag2} } {
    list [catch {tree0 insert 0 -tags {myTag1 myTag2}} msg] $msg
} {0 6}

test tree.39 {tree0 insert 0 -tags root} {
    list [catch {tree0 insert 0 -tags root} msg] $msg
} {1 {can't add reserved tag "root"}}

test tree.40 {tree0 insert 0 -tags (missing arg)} {
    list [catch {tree0 insert 0 -tags} msg] $msg
} {1 {value for "-tags" missing}}

test tree.41 {tree0 insert 0 -label myLabel -tags thisTag} {
    list [catch {tree0 insert 0 -label myLabel -tags thisTag} msg] $msg
} {0 8}

test tree.42 {tree0 insert 0 -label (missing arg)} {
    list [catch {tree0 insert 0 -label} msg] $msg
} {1 {value for "-label" missing}}

test tree.43 {tree0 insert 1 -tags thisTag} {
    list [catch {tree0 insert 1 -tags thisTag} msg] $msg
} {0 9}

test tree.44 {tree0 insert 1 -data key (missing value)} {
    list [catch {tree0 insert 1 -data key} msg] $msg
} {1 {missing value for "key"}}

test tree.45 {tree0 insert 1 -data {key value}} {
    list [catch {tree0 insert 1 -data {key value}} msg] $msg
} {0 11}

test tree.46 {tree0 insert 1 -data {key1 value1 key2 value2}} {
    list [catch {tree0 insert 1 -data {key1 value1 key2 value2}} msg] $msg
} {0 12}

test tree.47 {get} {
    list [catch {
	tree0 get 12
    } msg] $msg
} {0 {key1 value1 key2 value2}}

test tree.48 {tree0 children} {
    list [catch {tree0 children} msg] $msg
} {1 {wrong # args: should be "tree0 children node ?first? ?last?"}}

test tree.49 {tree0 children 0} {
    list [catch {tree0 children 0} msg] $msg
} {0 {1 2 3 4 5 6 8}}

test tree.50 {tree0 children root} {
    list [catch {tree0 children root} msg] $msg
} {0 {1 2 3 4 5 6 8}}

test tree.51 {tree0 children 1} {
    list [catch {tree0 children 1} msg] $msg
} {0 {9 11 12}}

test tree.52 {tree0 insert myTag} {
    list [catch {tree0 insert myTag} msg] $msg
} {0 13}

test tree.53 {tree0 children myTag} {
    list [catch {tree0 children myTag} msg] $msg
} {0 13}

test tree.54 {tree0 children root 0 end} {
    list [catch {tree0 children root 0 end} msg] $msg
} {0 {1 2 3 4 5 6 8}}

test tree.55 {tree0 children root 2} {
    list [catch {tree0 children root 2} msg] $msg
} {0 3}

test tree.56 {tree0 children root 2 end} {
    list [catch {tree0 children root 2 end} msg] $msg
} {0 {3 4 5 6 8}}

test tree.57 {tree0 children root end end} {
    list [catch {tree0 children root end end} msg] $msg
} {0 8}

test tree.58 {tree0 children root 0 2} {
    list [catch {tree0 children root 0 2} msg] $msg
} {0 {1 2 3}}

test tree.59 {tree0 children root -1 -20} {
    list [catch {tree0 children root -1 -20} msg] $msg
} {0 {}}

test tree.60 {tree0 firstchild (missing arg)} {
    list [catch {tree0 firstchild} msg] $msg
} {1 {wrong # args: should be "tree0 firstchild node"}}

test tree.61 {tree0 firstchild root} {
    list [catch {tree0 firstchild root} msg] $msg
} {0 1}

test tree.62 {tree0 lastchild (missing arg)} {
    list [catch {tree0 lastchild} msg] $msg
} {1 {wrong # args: should be "tree0 lastchild node"}}

test tree.63 {tree0 lastchild root} {
    list [catch {tree0 lastchild root} msg] $msg
} {0 8}

test tree.64 {tree0 nextsibling (missing arg)} {
    list [catch {tree0 nextsibling} msg] $msg
} {1 {wrong # args: should be "tree0 nextsibling node"}}

test tree.65 {tree0 nextsibling 1)} {
    list [catch {tree0 nextsibling 1} msg] $msg
} {0 2}

test tree.66 {tree0 nextsibling 2)} {
    list [catch {tree0 nextsibling 2} msg] $msg
} {0 3}

test tree.67 {tree0 nextsibling 3)} {
    list [catch {tree0 nextsibling 3} msg] $msg
} {0 4}

test tree.68 {tree0 nextsibling 4)} {
    list [catch {tree0 nextsibling 4} msg] $msg
} {0 5}

test tree.69 {tree0 nextsibling 5)} {
    list [catch {tree0 nextsibling 5} msg] $msg
} {0 6}

test tree.70 {tree0 nextsibling 6)} {
    list [catch {tree0 nextsibling 6} msg] $msg
} {0 8}

test tree.71 {tree0 nextsibling 8)} {
    list [catch {tree0 nextsibling 8} msg] $msg
} {0 -1}

test tree.72 {tree0 nextsibling all)} {
    list [catch {tree0 nextsibling all} msg] $msg
} {1 {more than one node tagged as "all"}}

test tree.73 {tree0 nextsibling badTag)} {
    list [catch {tree0 nextsibling badTag} msg] $msg
} {1 {can't find tag or id "badTag" in ::tree0}}

test tree.74 {tree0 nextsibling -1)} {
    list [catch {tree0 nextsibling -1} msg] $msg
} {1 {can't find tag or id "-1" in ::tree0}}

test tree.75 {tree0 prevsibling 2)} {
    list [catch {tree0 prevsibling 2} msg] $msg
} {0 1}

test tree.76 {tree0 prevsibling 1)} {
    list [catch {tree0 prevsibling 1} msg] $msg
} {0 -1}

test tree.77 {tree0 prevsibling -1)} {
    list [catch {tree0 prevsibling -1} msg] $msg
} {1 {can't find tag or id "-1" in ::tree0}}

test tree.78 {tree0 root)} {
    list [catch {tree0 root} msg] $msg
} {0 0}

test tree.79 {tree0 root badArg)} {
    list [catch {tree0 root badArgs} msg] $msg
} {1 {wrong # args: should be "tree0 root "}}

test tree.80 {tree0 parent (missing arg))} {
    list [catch {tree0 parent} msg] $msg
} {1 {wrong # args: should be "tree0 parent node"}}

test tree.81 {tree0 parent root)} {
    list [catch {tree0 parent root} msg] $msg
} {0 -1}

test tree.82 {tree0 parent 1)} {
    list [catch {tree0 parent 1} msg] $msg
} {0 0}

test tree.83 {tree0 parent myTag)} {
    list [catch {tree0 parent myTag} msg] $msg
} {0 0}

test tree.84 {tree0 next (missing arg))} {
    list [catch {tree0 next} msg] $msg
} {1 {wrong # args: should be "tree0 next node"}}


test tree.85 {tree0 next (extra arg))} {
    list [catch {tree0 next root root} msg] $msg
} {1 {wrong # args: should be "tree0 next node"}}

test tree.86 {tree0 next root} {
    list [catch {tree0 next root} msg] $msg
} {0 1}

test tree.87 {tree0 next 1)} {
    list [catch {tree0 next 1} msg] $msg
} {0 9}

test tree.88 {tree0 next 2)} {
    list [catch {tree0 next 2} msg] $msg
} {0 3}

test tree.89 {tree0 next 3)} {
    list [catch {tree0 next 3} msg] $msg
} {0 4}

test tree.90 {tree0 next 4)} {
    list [catch {tree0 next 4} msg] $msg
} {0 5}

test tree.91 {tree0 next 5)} {
    list [catch {tree0 next 5} msg] $msg
} {0 13}

test tree.92 {tree0 next 6)} {
    list [catch {tree0 next 6} msg] $msg
} {0 8}

test tree.93 {tree0 next 8)} {
    list [catch {tree0 next 8} msg] $msg
} {0 -1}

test tree.94 {tree0 previous 1)} {
    list [catch {tree0 previous 1} msg] $msg
} {0 0}

test tree.95 {tree0 previous 0)} {
    list [catch {tree0 previous 0} msg] $msg
} {0 -1}

test tree.96 {tree0 previous 8)} {
    list [catch {tree0 previous 8} msg] $msg
} {0 6}

test tree.97 {tree0 depth (no arg))} {
    list [catch {tree0 depth} msg] $msg
} {0 2}

test tree.98 {tree0 depth root))} {
    list [catch {tree0 depth root} msg] $msg
} {0 0}

test tree.99 {tree0 depth myTag))} {
    list [catch {tree0 depth myTag} msg] $msg
} {0 1}

test tree.100 {tree0 depth myTag))} {
    list [catch {tree0 depth myTag} msg] $msg
} {0 1}

test tree.101 {tree0 dump (missing arg)))} {
    list [catch {tree0 dump} msg] $msg
} {1 {wrong # args: should be "tree0 dump node ?switches ...?"}}

test tree.102 {tree0 dump root} {
    list [catch {tree0 dump root} msg] $msg
} {0 {-1 0 {{}} {} {}
0 1 {{} node1} {} {}
1 9 {{} node1 node9} {} {thisTag}
1 11 {{} node1 node11} {key value} {}
1 12 {{} node1 node12} {key1 value1 key2 value2} {}
0 2 {{} node2} {} {}
0 3 {{} node3} {} {}
0 4 {{} node4} {} {}
0 5 {{} node5} {} {myTag}
5 13 {{} node5 node13} {} {}
0 6 {{} node6} {} {myTag2 myTag1}
0 8 {{} myLabel} {} {thisTag}
}}

test tree.103 {tree0 dump 1} {
    list [catch {tree0 dump 1} msg] $msg
} {0 {-1 1 {node1} {} {}
1 9 {node1 node9} {} {thisTag}
1 11 {node1 node11} {key value} {}
1 12 {node1 node12} {key1 value1 key2 value2} {}
}}

test tree.104 {tree0 dump this} {
    list [catch {tree0 dump myTag} msg] $msg
} {0 {-1 5 {node5} {} {myTag}
5 13 {node5 node13} {} {}
}}

test tree.105 {tree0 dump 1 badSwitch} {
    list [catch {tree0 dump 1 badSwitch} msg] $msg
} {1 {unknown switch "badSwitch"
following switches are available:
   -data data
   -file fileName}}

test tree.106 {tree0 dump 11} {
    list [catch {tree0 dump 11} msg] $msg
} {0 {-1 11 {node11} {key value} {}
}}

test tree.107 {tree0 dump all} {
    list [catch {tree0 dump all} msg] $msg
} {1 {more than one node tagged as "all"}}

test tree.108 {tree0 dump all} {
    list [catch {tree0 dump all} msg] $msg
} {1 {more than one node tagged as "all"}}

test tree.109 {tree0 dump 0 -file test.dump} {
    list [catch {tree0 dump 0 -file test.dump} msg] $msg
} {0 {}}

test tree.110 {tree0 get 9} {
    list [catch {tree0 get 9} msg] $msg
} {0 {}}

test tree.111 {tree0 get all} {
    list [catch {tree0 get all} msg] $msg
} {1 {more than one node tagged as "all"}}

test tree.112 {tree0 get root} {
    list [catch {tree0 get root} msg] $msg
} {0 {}}

test tree.113 {tree0 get 9 key} {
    list [catch {tree0 get root} msg] $msg
} {0 {}}

test tree.114 {tree0 get 12} {
    list [catch {tree0 get 12} msg] $msg
} {0 {key1 value1 key2 value2}}

test tree.115 {tree0 get 12 key1} {
    list [catch {tree0 get 12 key1} msg] $msg
} {0 value1}

test tree.116 {tree0 get 12 key2} {
    list [catch {tree0 get 12 key2} msg] $msg
} {0 value2}

test tree.117 {tree0 get 12 key1 defValue } {
    list [catch {tree0 get 12 key1 defValue} msg] $msg
} {0 value1}

test tree.118 {tree0 get 12 key100 defValue } {
    list [catch {tree0 get 12 key100 defValue} msg] $msg
} {0 defValue}

test tree.119 {tree0 index (missing arg) } {
    list [catch {tree0 index} msg] $msg
} {1 {wrong # args: should be "tree0 index label|list"}}

test tree.120 {tree0 index 0 10 (extra arg) } {
    list [catch {tree0 index 0 10} msg] $msg
} {1 {wrong # args: should be "tree0 index label|list"}}

test tree.121 {tree0 index 0} {
    list [catch {tree0 index 0} msg] $msg
} {0 0}

test tree.122 {tree0 index root} {
    list [catch {tree0 index root} msg] $msg
} {0 0}

test tree.123 {tree0 index all} {
    list [catch {tree0 index all} msg] $msg
} {0 -1}

test tree.124 {tree0 index myTag} {
    list [catch {tree0 index myTag} msg] $msg
} {0 5}

test tree.125 {tree0 index thisTag} {
    list [catch {tree0 index thisTag} msg] $msg
} {0 -1}

test tree.126 {tree0 is (no args)} {
    list [catch {tree0 is} msg] $msg
} {1 {ambiguous operation "is" matches:  isancestor isbefore isleaf isroot}}

test tree.127 {tree0 isbefore} {
    list [catch {tree0 isbefore} msg] $msg
} {1 {wrong # args: should be "tree0 isbefore node1 node2"}}

test tree.128 {tree0 isbefore 0 10 20} {
    list [catch {tree0 isbefore 0 10 20} msg] $msg
} {1 {wrong # args: should be "tree0 isbefore node1 node2"}}

test tree.129 {tree0 isbefore 0 12} {
    list [catch {tree0 isbefore 0 12} msg] $msg
} {0 1}

test tree.130 {tree0 isbefore 12 0} {
    list [catch {tree0 isbefore 12 0} msg] $msg
} {0 0}

test tree.131 {tree0 isbefore 0 0} {
    list [catch {tree0 isbefore 0 0} msg] $msg
} {0 0}

test tree.132 {tree0 isbefore root 0} {
    list [catch {tree0 isbefore root 0} msg] $msg
} {0 0}

test tree.133 {tree0 isbefore 0 all} {
    list [catch {tree0 isbefore 0 all} msg] $msg
} {1 {more than one node tagged as "all"}}

test tree.134 {tree0 isancestor} {
    list [catch {tree0 isancestor} msg] $msg
} {1 {wrong # args: should be "tree0 isancestor node1 node2"}}

test tree.135 {tree0 isancestor 0 12 20} {
    list [catch {tree0 isancestor 0 12 20} msg] $msg
} {1 {wrong # args: should be "tree0 isancestor node1 node2"}}

test tree.136 {tree0 isancestor 0 12} {
    list [catch {tree0 isancestor 0 12} msg] $msg
} {0 1}

test tree.137 {tree0 isancestor 12 0} {
    list [catch {tree0 isancestor 12 0} msg] $msg
} {0 0}

test tree.138 {tree0 isancestor 1 2} {
    list [catch {tree0 isancestor 1 2} msg] $msg
} {0 0}

test tree.139 {tree0 isancestor root 0} {
    list [catch {tree0 isancestor root 0} msg] $msg
} {0 0}

test tree.140 {tree0 isancestor 0 all} {
    list [catch {tree0 isancestor 0 all} msg] $msg
} {1 {more than one node tagged as "all"}}

test tree.141 {tree0 isroot (missing arg)} {
    list [catch {tree0 isroot} msg] $msg
} {1 {wrong # args: should be "tree0 isroot node"}}

test tree.142 {tree0 isroot 0 20 (extra arg)} {
    list [catch {tree0 isroot 0 20} msg] $msg
} {1 {wrong # args: should be "tree0 isroot node"}}

test tree.143 {tree0 isroot 0} {
    list [catch {tree0 isroot 0} msg] $msg
} {0 1}

test tree.144 {tree0 isroot 12} {
    list [catch {tree0 isroot 12} msg] $msg
} {0 0}

test tree.145 {tree0 isroot 1} {
    list [catch {tree0 isroot 1} msg] $msg
} {0 0}

test tree.146 {tree0 isroot root} {
    list [catch {tree0 isroot root} msg] $msg
} {0 1}

test tree.147 {tree0 isroot all} {
    list [catch {tree0 isroot all} msg] $msg
} {1 {more than one node tagged as "all"}}

test tree.148 {tree0 isleaf (missing arg)} {
    list [catch {tree0 isleaf} msg] $msg
} {1 {wrong # args: should be "tree0 isleaf node"}}

test tree.149 {tree0 isleaf 0 20 (extra arg)} {
    list [catch {tree0 isleaf 0 20} msg] $msg
} {1 {wrong # args: should be "tree0 isleaf node"}}

test tree.150 {tree0 isleaf 0} {
    list [catch {tree0 isleaf 0} msg] $msg
} {0 0}

test tree.151 {tree0 isleaf 12} {
    list [catch {tree0 isleaf 12} msg] $msg
} {0 1}

test tree.152 {tree0 isleaf 1} {
    list [catch {tree0 isleaf 1} msg] $msg
} {0 0}

test tree.153 {tree0 isleaf root} {
    list [catch {tree0 isleaf root} msg] $msg
} {0 0}

test tree.154 {tree0 isleaf all} {
    list [catch {tree0 isleaf all} msg] $msg
} {1 {more than one node tagged as "all"}}

test tree.155 {tree0 isleaf 1000} {
    list [catch {tree0 isleaf 1000} msg] $msg
} {1 {can't find tag or id "1000" in ::tree0}}

test tree.156 {tree0 isleaf badTag} {
    list [catch {tree0 isleaf badTag} msg] $msg
} {1 {can't find tag or id "badTag" in ::tree0}}

test tree.157 {tree0 set (missing arg)} {
    list [catch {tree0 set} msg] $msg
} {1 {wrong # args: should be "tree0 set node ?fieldName value ...?"}}

test tree.158 {tree0 set 0 (missing arg)} {
    list [catch {tree0 set 0} msg] $msg
} {0 {}}

test tree.159 {tree0 set 0 key (missing arg)} {
    list [catch {tree0 set 0 key} msg] $msg
} {1 {missing value for field "key"}}

test tree.160 {tree0 set 0 key value} {
    list [catch {tree0 set 0 key value} msg] $msg
} {0 {}}

test tree.161 {tree0 set 0 key1 value1 key2 value2 key3 value3} {
    list [catch {tree0 set 0 key1 value1 key2 value2 key3 value3} msg] $msg
} {0 {}}

test tree.162 {tree0 set 0 key1 value1 key2 (missing arg)} {
    list [catch {tree0 set 0 key1 value1 key2} msg] $msg
} {1 {missing value for field "key2"}}

test tree.163 {tree0 set 0 key value} {
    list [catch {tree0 set 0 key value} msg] $msg
} {0 {}}

test tree.164 {tree0 set 0 key1 value1 key2 (missing arg)} {
    list [catch {tree0 set 0 key1 value1 key2} msg] $msg
} {1 {missing value for field "key2"}}

test tree.165 {tree0 set all} {
    list [catch {tree0 set all} msg] $msg
} {0 {}}

test tree.166 {tree0 set all abc 123} {
    list [catch {tree0 set all abc 123} msg] $msg
} {0 {}}

test tree.167 {tree0 set root} {
    list [catch {tree0 set root} msg] $msg
} {0 {}}

test tree.168 {tree0 restore stuff} {
    list [catch {
	set data [tree0 dump root]
	blt::tree create
	tree1 restore root -data $data
	set data [tree1 dump root]
	blt::tree destroy tree1
	set data
	} msg] $msg
} {0 {-1 0 {{}} {key value key1 value1 key2 value2 key3 value3 abc 123} {}
0 1 {{} node1} {abc 123} {}
1 9 {{} node1 node9} {abc 123} {thisTag}
1 11 {{} node1 node11} {key value abc 123} {}
1 12 {{} node1 node12} {key1 value1 key2 value2 abc 123} {}
0 2 {{} node2} {abc 123} {}
0 3 {{} node3} {abc 123} {}
0 4 {{} node4} {abc 123} {}
0 5 {{} node5} {abc 123} {myTag}
5 13 {{} node5 node13} {abc 123} {}
0 6 {{} node6} {abc 123} {myTag2 myTag1}
0 8 {{} myLabel} {abc 123} {thisTag}
}}

test tree.169 {tree0 restore 0 -file test.dump} {
    list [catch {
	blt::tree create
	tree1 restore root -file test.dump
	set data [tree1 dump root]
	blt::tree destroy tree1
	file delete test.dump
	set data
	} msg] $msg
} {0 {-1 0 {{}} {} {}
0 1 {{} node1} {} {}
1 9 {{} node1 node9} {} {thisTag}
1 11 {{} node1 node11} {key value} {}
1 12 {{} node1 node12} {key1 value1 key2 value2} {}
0 2 {{} node2} {} {}
0 3 {{} node3} {} {}
0 4 {{} node4} {} {}
0 5 {{} node5} {} {myTag}
5 13 {{} node5 node13} {} {}
0 6 {{} node6} {} {myTag2 myTag1}
0 8 {{} myLabel} {} {thisTag}
}}


test tree.170 {tree0 unset 0 key1} {
    list [catch {tree0 unset 0 key1} msg] $msg
} {0 {}}

test tree.171 {tree0 get 0} {
    list [catch {tree0 get 0} msg] $msg
} {0 {key value key2 value2 key3 value3 abc 123}}

test tree.172 {tree0 unset 0 key2 key3} {
    list [catch {tree0 unset 0 key2 key3} msg] $msg
} {0 {}}

test tree.173 {tree0 get 0} {
    list [catch {tree0 get 0} msg] $msg
} {0 {key value abc 123}}

test tree.174 {tree0 unset 0} {
    list [catch {tree0 unset 0} msg] $msg
} {0 {}}

test tree.175 {tree0 get 0} {
    list [catch {tree0 get 0} msg] $msg
} {0 {}}

test tree.176 {tree0 unset all abc} {
    list [catch {tree0 unset all abc} msg] $msg
} {0 {}}

test tree.177 {tree0 restore stuff} {
    list [catch {
	set data [tree0 dump root]
	blt::tree create tree1
	tree1 restore root -data $data
	set data [tree1 dump root]
	blt::tree destroy tree1
	set data
	} msg] $msg
} {0 {-1 0 {{}} {} {}
0 1 {{} node1} {} {}
1 9 {{} node1 node9} {} {thisTag}
1 11 {{} node1 node11} {key value} {}
1 12 {{} node1 node12} {key1 value1 key2 value2} {}
0 2 {{} node2} {} {}
0 3 {{} node3} {} {}
0 4 {{} node4} {} {}
0 5 {{} node5} {} {myTag}
5 13 {{} node5 node13} {} {}
0 6 {{} node6} {} {myTag2 myTag1}
0 8 {{} myLabel} {} {thisTag}
}}

test tree.178 {tree0 restore (missing arg)} {
    list [catch {tree0 restore} msg] $msg
} {1 {wrong # args: should be "tree0 restore node ?switches ...?"}}

test tree.179 {tree0 restore 0 badSwitch} {
    list [catch {tree0 restore 0 badSwitch} msg] $msg
} {1 {unknown switch "badSwitch"
following switches are available:
   -data data
   -file fileName
   -notags 
   -overwrite }}


test tree.180 {tree0 restore 0 {} arg (extra arg)} {
    list [catch {tree0 restore 0 {} arg} msg] $msg
} {1 {unknown switch ""
following switches are available:
   -data data
   -file fileName
   -notags 
   -overwrite }}


test tree.181 {tree0 size (missing arg)} {
    list [catch {tree0 size} msg] $msg
} {1 {wrong # args: should be "tree0 size node"}}

test tree.182 {tree0 size 0} {
    list [catch {tree0 size 0} msg] $msg
} {0 12}

test tree.183 {tree0 size all} {
    list [catch {tree0 size all} msg] $msg
} {1 {more than one node tagged as "all"}}

test tree.184 {tree0 size 0 10 (extra arg)} {
    list [catch {tree0 size 0 10} msg] $msg
} {1 {wrong # args: should be "tree0 size node"}}

test tree.185 {tree0 delete (no args)} {
    list [catch {tree0 delete} msg] $msg
} {0 {}}

test tree.186 {tree0 delete 11} {
    list [catch {tree0 delete 11} msg] $msg
} {0 {}}

test tree.187 {tree0 delete 11} {
    list [catch {tree0 delete 11} msg] $msg
} {1 {can't find tag or id "11" in ::tree0}}

test tree.188 {tree0 delete 9 12} {
    list [catch {tree0 delete 9 12} msg] $msg
} {0 {}}

test tree.189 {tree0 dump 0} {
    list [catch {tree0 dump 0} msg] $msg
} {0 {-1 0 {{}} {} {}
0 1 {{} node1} {} {}
0 2 {{} node2} {} {}
0 3 {{} node3} {} {}
0 4 {{} node4} {} {}
0 5 {{} node5} {} {myTag}
5 13 {{} node5 node13} {} {}
0 6 {{} node6} {} {myTag2 myTag1}
0 8 {{} myLabel} {} {thisTag}
}}

test tree.190 {delete all} {
    list [catch {
	set data [tree0 dump root]
	blt::tree create
	tree1 restore root -data $data
	tree1 delete all
	set data [tree1 dump root]
	blt::tree destroy tree1
	set data
	} msg] $msg
} {0 {-1 0 {{}} {} {}
}}

test tree.191 {delete all all} {
    list [catch {
	set data [tree0 dump root]
	blt::tree create
	tree1 restore root -data $data
	tree1 delete all all
	set data [tree1 dump root]
	blt::tree destroy tree1
	set data
	} msg] $msg
} {0 {-1 0 {{}} {} {}
}}

test tree.192 {tree0 apply (missing arg)} {
    list [catch {tree0 apply} msg] $msg
} {1 {wrong # args: should be "tree0 apply node ?switches ...?"}}

test tree.193 {tree0 apply 0} {
    list [catch {tree0 apply 0} msg] $msg
} {0 {}}

test tree.194 {tree0 apply 0 -badSwitch} {
    list [catch {tree0 apply 0 -badSwitch} msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -precommand command
   -postcommand command
   -depth number
   -exact string
   -glob pattern
   -invert 
   -key pattern
   -keyexact string
   -keyglob pattern
   -keyregexp pattern
   -leafonly 
   -nocase 
   -path 
   -regexp pattern
   -tag tagList}}

test tree.195 {tree0 apply badTag} {
    list [catch {tree0 apply badTag} msg] $msg
} {1 {can't find tag or id "badTag" in ::tree0}}

test tree.196 {tree0 apply all} {
    list [catch {tree0 apply all} msg] $msg
} {1 {more than one node tagged as "all"}}

test tree.197 {tree0 apply myTag -precommand lappend} {
    list [catch {
	set mylist {}
	tree0 apply myTag -precommand {lappend mylist}
	set mylist
    } msg] $msg
} {0 {5 13}}

test tree.198 {tree0 apply root -precommand lappend} {
    list [catch {
	set mylist {}
	tree0 apply root -precommand {lappend mylist}
	set mylist
    } msg] $msg
} {0 {0 1 2 3 4 5 13 6 8}}

test tree.199 {tree0 apply -postcommand} {
    list [catch {
	set mylist {}
	tree0 apply root -postcommand {lappend mylist}
	set mylist
    } msg] $msg
} {0 {1 2 3 4 13 5 6 8 0}}

test tree.200 {tree0 apply -precommand -postcommand} {
    list [catch {
	set mylist {}
	tree0 apply root -precommand {lappend mylist} \
		-postcommand {lappend mylist}
	set mylist
    } msg] $msg
} {0 {0 1 1 2 2 3 3 4 4 5 13 13 5 6 6 8 8 0}}

test tree.201 {tree0 apply root -precommand lappend -depth 1} {
    list [catch {
	set mylist {}
	tree0 apply root -precommand {lappend mylist} -depth 1
	set mylist
    } msg] $msg
} {0 {0 1 2 3 4 5 6 8}}


test tree.202 {tree0 apply root -precommand -depth 0} {
    list [catch {
	set mylist {}
	tree0 apply root -precommand {lappend mylist} -depth 0
	set mylist
    } msg] $msg
} {0 0}

test tree.203 {tree0 apply root -precommand -tag myTag} {
    list [catch {
	set mylist {}
	tree0 apply root -precommand {lappend mylist} -tag myTag
	set mylist
    } msg] $msg
} {0 5}


test tree.204 {tree0 apply root -precommand -key key1} {
    list [catch {
	set mylist {}
	tree0 set myTag key1 0.0
	tree0 apply root -precommand {lappend mylist} -key key1
	tree0 unset myTag key1
	set mylist
    } msg] $msg
} {0 5}

test tree.205 {tree0 apply root -postcommand -regexp node.*} {
    list [catch {
	set mylist {}
	tree0 set myTag key1 0.0
	tree0 apply root -precommand {lappend mylist} -regexp {node5} 
	tree0 unset myTag key1
	set mylist
    } msg] $msg
} {0 5}

test tree.206 {tree0 find (missing arg)} {
    list [catch {tree0 find} msg] $msg
} {1 {wrong # args: should be "tree0 find node ?switches ...?"}}

test tree.207 {tree0 find 0} {
    list [catch {tree0 find 0} msg] $msg
} {0 {1 2 3 4 13 5 6 8 0}}

test tree.208 {tree0 find root} {
    list [catch {tree0 find root} msg] $msg
} {0 {1 2 3 4 13 5 6 8 0}}

test tree.209 {tree0 find 0 -glob node*} {
    list [catch {tree0 find root -glob node*} msg] $msg
} {0 {1 2 3 4 13 5 6}}

test tree.210 {tree0 find 0 -glob nobody} {
    list [catch {tree0 find root -glob nobody} msg] $msg
} {0 {}}

test tree.211 {tree0 find 0 -regexp {node[0-3]}} {
    list [catch {tree0 find root -regexp {node[0-3]}} msg] $msg
} {0 {1 2 3 13}}

test tree.212 {tree0 find 0 -regexp {.*[A-Z].*}} {
    list [catch {tree0 find root -regexp {.*[A-Z].*}} msg] $msg
} {0 8}

test tree.213 {tree0 find 0 -exact myLabel} {
    list [catch {tree0 find root -exact myLabel} msg] $msg
} {0 8}

test tree.214 {tree0 find 0 -exact myLabel -invert} {
    list [catch {tree0 find root -exact myLabel -invert} msg] $msg
} {0 {1 2 3 4 13 5 6 0}}


test tree.215 {tree0 find 3 -exact node3} {
    list [catch {tree0 find 3 -exact node3} msg] $msg
} {0 3}

test tree.216 {tree0 find 0 -nocase -exact mylabel} {
    list [catch {tree0 find 0 -nocase -exact mylabel} msg] $msg
} {0 8}

test tree.217 {tree0 find 0 -nocase} {
    list [catch {tree0 find 0 -nocase} msg] $msg
} {0 {1 2 3 4 13 5 6 8 0}}

test tree.218 {tree0 find 0 -path -nocase -glob *node1* } {
    list [catch {tree0 find 0 -path -nocase -glob *node1*} msg] $msg
} {0 {1 13}}

test tree.219 {tree0 find 0 -count 5 } {
    list [catch {tree0 find 0 -count 5} msg] $msg
} {0 {1 2 3 4 13}}

test tree.220 {tree0 find 0 -count -5 } {
    list [catch {tree0 find 0 -count -5} msg] $msg
} {1 {bad value "-5": can't be negative}}

test tree.221 {tree0 find 0 -count badValue } {
    list [catch {tree0 find 0 -count badValue} msg] $msg
} {1 {expected integer but got "badValue"}}

test tree.222 {tree0 find 0 -count badValue } {
    list [catch {tree0 find 0 -count badValue} msg] $msg
} {1 {expected integer but got "badValue"}}

test tree.223 {tree0 find 0 -leafonly} {
    list [catch {tree0 find 0 -leafonly} msg] $msg
} {0 {1 2 3 4 13 6 8}}

test tree.224 {tree0 find 0 -leafonly -glob {node[18]}} {
    list [catch {tree0 find 0 -glob {node[18]} -leafonly} msg] $msg
} {0 1}

test tree.225 {tree0 find 0 -depth 0} {
    list [catch {tree0 find 0 -depth 0} msg] $msg
} {0 0}

test tree.226 {tree0 find 0 -depth 1} {
    list [catch {tree0 find 0 -depth 1} msg] $msg
} {0 {1 2 3 4 5 6 8 0}}

test tree.227 {tree0 find 0 -depth 2} {
    list [catch {tree0 find 0 -depth 2} msg] $msg
} {0 {1 2 3 4 13 5 6 8 0}}

test tree.228 {tree0 find 0 -depth 20} {
    list [catch {tree0 find 0 -depth 20} msg] $msg
} {0 {1 2 3 4 13 5 6 8 0}}

test tree.229 {tree0 find 1 -depth 0} {
    list [catch {tree0 find 1 -depth 0} msg] $msg
} {0 1}

test tree.230 {tree0 find 1 -depth 1} {
    list [catch {tree0 find 1 -depth 1} msg] $msg
} {0 1}

test tree.231 {tree0 find 1 -depth 2} {
    list [catch {tree0 find 1 -depth 2} msg] $msg
} {0 1}

test tree.232 {tree0 find all} {
    list [catch {tree0 find all} msg] $msg
} {1 {more than one node tagged as "all"}}

test tree.233 {tree0 find badTag} {
    list [catch {tree0 find badTag} msg] $msg
} {1 {can't find tag or id "badTag" in ::tree0}}

test tree.234 {tree0 find 0 -addtag hi} {
    list [catch {tree0 find 0 -addtag hi} msg] $msg
} {0 {1 2 3 4 13 5 6 8 0}}

test tree.235 {tree0 find 0 -addtag all} {
    list [catch {tree0 find 0 -addtag all} msg] $msg
} {0 {1 2 3 4 13 5 6 8 0}}

test tree.236 {tree0 find 0 -addtag root} {
    list [catch {tree0 find 0 -addtag root} msg] $msg
} {1 {can't add reserved tag "root"}}

test tree.237 {tree0 find 0 -exec {lappend list} -leafonly} {
    list [catch {
	set list {}
	tree0 find 0 -exec {lappend list} -leafonly
	set list
	} msg] $msg
} {0 {1 2 3 4 13 6 8}}

test tree.238 {tree0 find 0 -tag root} {
    list [catch {tree0 find 0 -tag root} msg] $msg
} {0 0}

test tree.239 {tree0 find 0 -tag myTag} {
    list [catch {tree0 find 0 -tag myTag} msg] $msg
} {0 5}

test tree.240 {tree0 find 0 -tag badTag} {
    list [catch {tree0 find 0 -tag badTag} msg] $msg
} {0 {}}

test tree.241 {tree0 tag (missing args)} {
    list [catch {tree0 tag} msg] $msg
} {1 {wrong # args: should be "tree0 tag args ..."}}

test tree.242 {tree0 tag badOp} {
    list [catch {tree0 tag badOp} msg] $msg
} {1 {bad operation "badOp": should be one of...
  tree0 tag add tag ?node...?
  tree0 tag delete tag node...
  tree0 tag dump tag...
  tree0 tag exists tag ?node?
  tree0 tag forget tag...
  tree0 tag get node ?pattern...?
  tree0 tag names ?node...?
  tree0 tag nodes tag ?tag...?
  tree0 tag set node tag...
  tree0 tag unset node tag...}}

test tree.243 {tree0 tag add} {
    list [catch {tree0 tag add} msg] $msg
} {1 {wrong # args: should be "tree0 tag add tag ?node...?"}}

test tree.244 {tree0 tag add newTag} {
    list [catch {tree0 tag add newTag} msg] $msg
} {0 {}}

test tree.245 {tree0 tag add tag badNode} {
    list [catch {tree0 tag add tag badNode} msg] $msg
} {1 {can't find tag or id "badNode" in ::tree0}}

test tree.246 {tree0 tag add newTag root} {
    list [catch {tree0 tag add newTag root} msg] $msg
} {0 {}}

test tree.247 {tree0 tag add newTag all} {
    list [catch {tree0 tag add newTag all} msg] $msg
} {0 {}}

test tree.248 {tree0 tag add tag2 0 1 2 3 4} {
    list [catch {tree0 tag add tag2 0 1 2 3 4} msg] $msg
} {0 {}}

test tree.249 {tree0 tag exists tag2} {
    list [catch {tree0 tag exists tag2} msg] $msg
} {0 1}

test tree.250 {tree0 tag exists tag2 0} {
    list [catch {tree0 tag exists tag2 0} msg] $msg
} {0 1}

test tree.251 {tree0 tag exists tag2 5} {
    list [catch {tree0 tag exists tag2 5} msg] $msg
} {0 0}

test tree.252 {tree0 tag exists badTag} {
    list [catch {tree0 tag exists badTag} msg] $msg
} {0 0}

test tree.253 {tree0 tag exists badTag 1000} {
    list [catch {tree0 tag exists badTag 1000} msg] $msg
} {1 {can't find tag or id "1000" in ::tree0}}

test tree.254 {tree0 tag add tag2 0 1 2 3 4 1000} {
    list [catch {tree0 tag add tag2 0 1 2 3 4 1000} msg] $msg
} {1 {can't find tag or id "1000" in ::tree0}}

test tree.255 {tree0 tag names} {
    list [catch {tree0 tag names} msg] [lsort $msg]
} {0 {all hi myTag myTag1 myTag2 newTag root tag2 thisTag}}

test tree.256 {tree0 tag names badNode} {
    list [catch {tree0 tag names badNode} msg] $msg
} {1 {can't find tag or id "badNode" in ::tree0}}

test tree.257 {tree0 tag names all} {
    list [catch {tree0 tag names all} msg] $msg
} {1 {more than one node tagged as "all"}}

test tree.258 {tree0 tag names root} {
    list [catch {tree0 tag names root} msg] [lsort $msg]
} {0 {all hi newTag root tag2}}

test tree.259 {tree0 tag names 0 1} {
    list [catch {tree0 tag names 0 1} msg] [lsort $msg]
} {0 {all hi newTag root tag2}}

test tree.260 {tree0 tag nodes (missing arg)} {
    list [catch {tree0 tag nodes} msg] $msg
} {1 {wrong # args: should be "tree0 tag nodes tag ?tag...?"}}

test tree.261 {tree0 tag nodes root badTag} {
    # It's not an error to use bad tag.
    list [catch {tree0 tag nodes root badTag} msg] $msg
} {0 {}}

test tree.262 {tree0 tag nodes root tag2} {
    list [catch {tree0 tag nodes root tag2} msg] [lsort $msg]
} {0 {0 1 2 3 4}}

test tree.263 {tree0 ancestor (missing arg)} {
    list [catch {tree0 ancestor} msg] $msg
} {1 {wrong # args: should be "tree0 ancestor node1 node2"}}

test tree.264 {tree0 ancestor 0 (missing arg)} {
    list [catch {tree0 ancestor 0} msg] $msg
} {1 {wrong # args: should be "tree0 ancestor node1 node2"}}

test tree.265 {tree0 ancestor 0 10} {
    list [catch {tree0 ancestor 0 10} msg] $msg
} {1 {can't find tag or id "10" in ::tree0}}

test tree.266 {tree0 ancestor 0 4} {
    list [catch {tree0 ancestor 0 4} msg] $msg
} {0 0}

test tree.267 {tree0 ancestor 1 8} {
    list [catch {tree0 ancestor 1 8} msg] $msg
} {0 0}

test tree.268 {tree0 ancestor root 0} {
    list [catch {tree0 ancestor root 0} msg] $msg
} {0 0}

test tree.269 {tree0 ancestor 8 8} {
    list [catch {tree0 ancestor 8 8} msg] $msg
} {0 8}

test tree.270 {tree0 ancestor 0 all} {
    list [catch {tree0 ancestor 0 all} msg] $msg
} {1 {more than one node tagged as "all"}}

test tree.271 {tree0 ancestor 7 9} {
    list [catch {
	set n1 1; set n2 1;
	for { set i 0 } { $i < 4 } { incr i } {
	    set n1 [tree0 insert $n1]
	    set n2 [tree0 insert $n2]
	}
	tree0 ancestor $n1 $n2
	} msg] $msg
} {0 1}

test tree.272 {tree0 path (missing arg)} {
    list [catch {tree0 path} msg] $msg
} {1 {wrong # args: should be "tree0 path path ?args ...?"}}

test tree.273 {tree0 path badArg} {
    list [catch {tree0 path badArg} msg] $msg
} {1 {bad operation "badArg": should be one of...
  tree0 path create path ?switches ...?
  tree0 path parse path ?switches ...?
  tree0 path print node ?switches ...?
  tree0 path separator ?string?}}

test tree.274 {tree0 path print root} {
    list [catch {tree0 path print root} msg] $msg
} {0 {}}

test tree.275 {tree0 path print 0} {
    list [catch {tree0 path print 0} msg] $msg
} {0 {}}

test tree.276 {tree0 path print 15} {
    list [catch {tree0 path print 15} msg] $msg
} {0 {node1 node15}}

test tree.277 {tree0 path print all} {
    list [catch {tree0 path print all} msg] $msg
} {1 {more than one node tagged as "all"}}

test tree.278 {tree0 path print 0 badSwitch} {
    list [catch {tree0 path print 0 badSwitch} msg] $msg
} {1 {unknown switch "badSwitch"
following switches are available:
   -from node
   -separator char
   -noleadingseparator }}


test tree.279 {tree0 tag forget} {
    list [catch {tree0 tag forget} msg] $msg
} {1 {wrong # args: should be "tree0 tag forget tag..."}}

test tree.280 {tree0 tag forget badTag} {
    list [catch {
	tree0 tag forget badTag
	lsort [tree0 tag names]
    } msg] $msg
} {0 {all hi myTag myTag1 myTag2 newTag root tag2 thisTag}}

test tree.281 {tree0 tag forget hi} {
    list [catch {
	tree0 tag forget hi
	lsort [tree0 tag names]
    } msg] $msg
} {0 {all myTag myTag1 myTag2 newTag root tag2 thisTag}}

test tree.282 {tree0 tag forget tag1 tag2} {
    list [catch {
	tree0 tag forget myTag1 myTag2
	lsort [tree0 tag names]
    } msg] $msg
} {0 {all myTag newTag root tag2 thisTag}}

test tree.283 {tree0 tag forget all} {
    list [catch {
	tree0 tag forget all
	lsort [tree0 tag names]
    } msg] $msg
} {0 {all myTag newTag root tag2 thisTag}}

test tree.284 {tree0 tag forget root} {
    list [catch {
	tree0 tag forget root
	lsort [tree0 tag names]
    } msg] $msg
} {0 {all myTag newTag root tag2 thisTag}}

test tree.285 {tree0 tag delete} {
    list [catch {tree0 tag delete} msg] $msg
} {1 {wrong # args: should be "tree0 tag delete tag node..."}}

test tree.286 {tree0 tag delete tag} {
    list [catch {tree0 tag delete tag} msg] $msg
} {1 {wrong # args: should be "tree0 tag delete tag node..."}}

test tree.287 {tree0 tag delete tag 0} {
    list [catch {tree0 tag delete tag 0} msg] $msg
} {0 {}}

test tree.288 {tree0 tag delete root 0} {
    list [catch {tree0 tag delete root 0} msg] $msg
} {1 {can't delete reserved tag "root"}}

test tree.289 {tree0 move} {
    list [catch {tree0 move} msg] $msg
} {1 {wrong # args: should be "tree0 move node newParent ?switches ...?"}}

test tree.290 {tree0 move 0} {
    list [catch {tree0 move 0} msg] $msg
} {1 {wrong # args: should be "tree0 move node newParent ?switches ...?"}}

test tree.291 {tree0 move 0 0} {
    list [catch {tree0 move 0 0} msg] $msg
} {1 {can't move root node}}

test tree.292 {tree0 move 0 badNode} {
    list [catch {tree0 move 0 badNode} msg] $msg
} {1 {can't find tag or id "badNode" in ::tree0}}

test tree.293 {tree0 move 0 all} {
    list [catch {tree0 move 0 all} msg] $msg
} {1 {more than one node tagged as "all"}}

test tree.294 {tree0 move 1 0 -before 2} {
    list [catch {
	tree0 move 1 0 -before 2
	tree0 children 0
    } msg] $msg
} {0 {1 2 3 4 5 6 8}}

test tree.295 {tree0 move 1 0 -after 2} {
    list [catch {
	tree0 move 1 0 -after 2
	tree0 children 0
    } msg] $msg
} {0 {2 1 3 4 5 6 8}}

test tree.296 {tree0 move 1 2} {
    list [catch {
	tree0 move 1 2
	tree0 children 0
    } msg] $msg
} {0 {2 3 4 5 6 8}}

test tree.297 {tree0 move 0 2} {
    list [catch {tree0 move 0 2} msg] $msg
} {1 {can't move root node}}

test tree.298 {tree0 move 1 17} {
    list [catch {tree0 move 1 17} msg] $msg
} {1 {can't move node: "1" is an ancestor of "17"}}

test tree.299 {tree0 attach} {
    list [catch {tree0 attach} msg] $msg
} {1 {wrong # args: should be "tree0 attach tree ?switches ...?"}}

test tree.300 {tree0 attach tree2 badArg} {
    list [catch {tree0 attach tree2 badArg} msg] $msg
} {1 {unknown switch "badArg"
following switches are available:
   -newtags }}


test tree.301 {tree1 attach tree0 -newtags} {
    list [catch {
	blt::tree create
	tree1 attach tree0 -newtags
	tree1 dump 0
	} msg] $msg
} {0 {-1 0 {{}} {} {}
0 2 {{} node2} {} {}
2 1 {{} node2 node1} {} {}
1 14 {{} node2 node1 node14} {} {}
14 16 {{} node2 node1 node14 node16} {} {}
16 18 {{} node2 node1 node14 node16 node18} {} {}
18 20 {{} node2 node1 node14 node16 node18 node20} {} {}
1 15 {{} node2 node1 node15} {} {}
15 17 {{} node2 node1 node15 node17} {} {}
17 19 {{} node2 node1 node15 node17 node19} {} {}
19 21 {{} node2 node1 node15 node17 node19 node21} {} {}
0 3 {{} node3} {} {}
0 4 {{} node4} {} {}
0 5 {{} node5} {} {}
5 13 {{} node5 node13} {} {}
0 6 {{} node6} {} {}
0 8 {{} myLabel} {} {}
}}

test tree.302 {tree1 attach tree0} {
    list [catch {
	blt::tree create
	tree1 attach tree0
	tree1 dump 0
	} msg] $msg
} {0 {-1 0 {{}} {} {tag2 newTag}
0 2 {{} node2} {} {tag2 newTag}
2 1 {{} node2 node1} {} {tag2 newTag}
1 14 {{} node2 node1 node14} {} {}
14 16 {{} node2 node1 node14 node16} {} {}
16 18 {{} node2 node1 node14 node16 node18} {} {}
18 20 {{} node2 node1 node14 node16 node18 node20} {} {}
1 15 {{} node2 node1 node15} {} {}
15 17 {{} node2 node1 node15 node17} {} {}
17 19 {{} node2 node1 node15 node17 node19} {} {}
19 21 {{} node2 node1 node15 node17 node19 node21} {} {}
0 3 {{} node3} {} {tag2 newTag}
0 4 {{} node4} {} {tag2 newTag}
0 5 {{} node5} {} {newTag myTag}
5 13 {{} node5 node13} {} {newTag}
0 6 {{} node6} {} {newTag}
0 8 {{} myLabel} {} {thisTag newTag}
}}

test tree.303 {tree1 attach ""} {
    list [catch {tree1 attach ""} msg] $msg
} {0 {}}


test tree.304 {blt::tree destroy tree1} {
    list [catch {blt::tree destroy tree1} msg] $msg
} {0 {}}

test tree.305 {tree0 find root -badSwitch} {
    list [catch {tree0 find root -badSwitch} msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -addtag tagName
   -count number
   -depth number
   -exact string
   -excludes nodes
   -exec command
   -glob pattern
   -invert 
   -key string
   -keyexact string
   -keyglob pattern
   -keyregexp pattern
   -leafonly 
   -nocase 
   -order order
   -path 
   -regexp pattern
   -tag tagList}}

test tree.306 {tree0 find root -order} {
    list [catch {tree0 find root -order} msg] $msg
} {1 {value for "-order" missing}}

test tree.307 {tree0 find root ...} {
    list [catch {tree0 find root -order preorder -order postorder -order inorder} msg] $msg
} {0 {20 18 16 14 1 21 19 17 15 2 0 3 4 13 5 6 8}}

test tree.308 {tree0 find root -order preorder} {
    list [catch {tree0 find root -order preorder} msg] $msg
} {0 {0 2 1 14 16 18 20 15 17 19 21 3 4 5 13 6 8}}

test tree.309 {tree0 find root -order postorder} {
    list [catch {tree0 find root -order postorder} msg] $msg
} {0 {20 18 16 14 21 19 17 15 1 2 3 4 13 5 6 8 0}}

test tree.310 {tree0 find root -order inorder} {
    list [catch {tree0 find root -order inorder} msg] $msg
} {0 {20 18 16 14 1 21 19 17 15 2 0 3 4 13 5 6 8}}

test tree.311 {tree0 find root -order breadthfirst} {
    list [catch {tree0 find root -order breadthfirst} msg] $msg
} {0 {0 2 3 4 5 6 8 1 13 14 15 16 17 18 19 20 21}}

test tree.312 {tree0 set all key1 myValue} {
    list [catch {tree0 set all key1 myValue} msg] $msg
} {0 {}}

test tree.313 {tree0 set 15 key1 123} {
    list [catch {tree0 set 15 key1 123} msg] $msg
} {0 {}}

test tree.314 {tree0 set 16 key1 1234 key2 abc} {
    list [catch {tree0 set 16 key1 123 key2 abc} msg] $msg
} {0 {}}

test tree.315 {tree0 find root -key } {
    list [catch {tree0 find root -key} msg] $msg
} {1 {value for "-key" missing}}

test tree.316 {tree0 find root -key noKey} {
    list [catch {tree0 find root -key noKey} msg] $msg
} {0 {}}

test tree.317 {tree0 find root -key key1} {
    list [catch {tree0 find root -key key1} msg] $msg
} {0 {20 18 16 14 21 19 17 15 1 2 3 4 13 5 6 8 0}}

test tree.318 {tree0 find root -key key2} {
    list [catch {tree0 find root -key key2} msg] $msg
} {0 16}

test tree.319 {tree0 find root -key key2 -exact notThere } {
    list [catch {tree0 find root -key key2 -exact notThere } msg] $msg
} {0 {}}

test tree.320 {tree0 find root -key key1 -glob notThere } {
    list [catch {tree0 find root -key key2 -exact notThere } msg] $msg
} {0 {}}

test tree.321 {tree0 find root -key badKey -regexp notThere } {
    list [catch {tree0 find root -key key2 -exact notThere } msg] $msg
} {0 {}}

test tree.322 {tree0 find root -key key1 -glob 12*} {
    list [catch {tree0 find root -key key1 -glob 12*} msg] $msg
} {0 {16 15}}

test tree.323 {tree0 sort} {
    list [catch {tree0 sort} msg] $msg
} {1 {wrong # args: should be "tree0 sort node ?switches ...?"}}

test tree.324 {tree0 sort all} {
    list [catch {tree0 sort all} msg] $msg
} {1 {more than one node tagged as "all"}}

test tree.325 {tree0 sort -recurse} {
    list [catch {tree0 sort -recurse} msg] $msg
} {1 {can't find tag or id "-recurse" in ::tree0}}

test tree.326 {tree0 sort 0} {
    list [catch {tree0 sort 0} msg] $msg
} {0 {8 2 3 4 5 6}}

test tree.327 {tree0 sort 0 -recurse} {
    list [catch {tree0 sort 0 -recurse} msg] $msg
} {0 {0 8 1 2 3 4 5 6 13 14 15 16 17 18 19 20 21}}

test tree.328 {tree0 sort 0 -decreasing -key} {
    list [catch {tree0 sort 0 -decreasing -key} msg] $msg
} {1 {value for "-key" missing}}

test tree.329 {tree0 sort 0 -re} {
    list [catch {tree0 sort 0 -re} msg] $msg
} {1 {ambiguous switch "-re"
following switches are available:
   -ascii 
   -command command
   -decreasing 
   -dictionary 
   -integer 
   -key string
   -path 
   -real 
   -recurse 
   -reorder }}


test tree.330 {tree0 sort 0 -decreasing} {
    list [catch {tree0 sort 0 -decreasing} msg] $msg
} {0 {6 5 4 3 2 8}}

test tree.331 {tree0 sort 0} {
    list [catch {
	set list {}
	foreach n [tree0 sort 0] {
	    lappend list [tree0 label $n]
	}	
	set list
    } msg] $msg
} {0 {myLabel node2 node3 node4 node5 node6}}

test tree.332 {tree0 sort 0 -decreasing} {
    list [catch {tree0 sort 0 -decreasing} msg] $msg
} {0 {6 5 4 3 2 8}}


test tree.333 {tree0 sort 0 -decreasing -key} {
    list [catch {tree0 sort 0 -decreasing -key} msg] $msg
} {1 {value for "-key" missing}}

test tree.334 {tree0 sort 0 -decreasing -key key1} {
    list [catch {tree0 sort 0 -decreasing -key key1} msg] $msg
} {0 {8 6 5 4 3 2}}

test tree.335 {tree0 sort 0 -decreasing -recurse -key key1} {
    list [catch {tree0 sort 0 -decreasing -recurse -key key1} msg] $msg
} {0 {15 16 0 1 2 3 4 5 6 8 13 14 17 18 19 20 21}}

test tree.336 {tree0 sort 0 -decreasing -key key1} {
    list [catch {
	set list {}
	foreach n [tree0 sort 0 -decreasing -key key1] {
	    lappend list [tree0 get $n key1]
	}
	set list
    } msg] $msg
} {0 {myValue myValue myValue myValue myValue myValue}}


test tree.337 {tree0 index 1->firstchild} {
    list [catch {tree0 index 1->firstchild} msg] $msg
} {0 14}

test tree.338 {tree0 index root->firstchild} {
    list [catch {tree0 index root->firstchild} msg] $msg
} {0 2}

test tree.339 {tree0 label root->parent} {
    list [catch {tree0 label root->parent} msg] $msg
} {1 {can't find tag or id "root->parent" in ::tree0}}

test tree.340 {tree0 index root->parent} {
    list [catch {tree0 index root->parent} msg] $msg
} {0 -1}

test tree.341 {tree0 index root->lastchild} {
    list [catch {tree0 index root->lastchild} msg] $msg
} {0 8}

test tree.342 {tree0 index root->next} {
    list [catch {tree0 index root->next} msg] $msg
} {0 2}

test tree.343 {tree0 index root->previous} {
    list [catch {tree0 index root->previous} msg] $msg
} {0 -1}

test tree.344 {tree0 label root->previous} {
    list [catch {tree0 label root->previous} msg] $msg
} {1 {can't find tag or id "root->previous" in ::tree0}}

test tree.345 {tree0 index 1->previous} {
    list [catch {tree0 index 1->previous} msg] $msg
} {0 2}

test tree.346 {tree0 label root->badModifier} {
    list [catch {tree0 label root->badModifier} msg] $msg
} {1 {can't find tag or id "root->badModifier" in ::tree0}}

test tree.347 {tree0 index root->badModifier} {
    list [catch {tree0 index root->badModifier} msg] $msg
} {0 -1}

test tree.348 {tree0 index root->firstchild->parent} {
    list [catch {tree0 index root->firstchild->parent} msg] $msg
} {0 0}

test tree.349 {tree0 trace} {
    list [catch {tree0 trace} msg] $msg
} {1 {wrong # args: should be one of...
  tree0 trace create node key how command ?-whenidle?
  tree0 trace delete traceName ...
  tree0 trace info traceName
  tree0 trace names ?pattern ...?}}

test tree.350 {tree0 trace create} {
    list [catch {tree0 trace create} msg] $msg
} {1 {wrong # args: should be "tree0 trace create node key how command ?-whenidle?"}}

test tree.351 {tree0 trace create root} {
    list [catch {tree0 trace create root} msg] $msg
} {1 {wrong # args: should be "tree0 trace create node key how command ?-whenidle?"}}

test tree.352 {tree0 trace create root * } {
    list [catch {tree0 trace create root * } msg] $msg
} {1 {wrong # args: should be "tree0 trace create node key how command ?-whenidle?"}}

test tree.353 {tree0 trace create root * rwuc} {
    list [catch {tree0 trace create root * rwuc} msg] $msg
} {1 {wrong # args: should be "tree0 trace create node key how command ?-whenidle?"}}

proc Doit args { global mylist; lappend mylist $args }

test tree.354 {tree0 trace create all newKey rwuc Doit} {
    list [catch {tree0 trace create all newKey rwuc Doit} msg] $msg
} {0 trace0}

test tree.355 {tree0 trace info trace0} {
    list [catch {tree0 trace info trace0} msg] $msg
} {0 {all newKey rwuc Doit}}

test tree.356 {test create trace} {
    list [catch {
	set mylist {}
	tree0 set all newKey 20
	set mylist
	} msg] $msg
} {0 {{::tree0 0 newKey wc} {::tree0 2 newKey wc} {::tree0 1 newKey wc} {::tree0 14 newKey wc} {::tree0 16 newKey wc} {::tree0 18 newKey wc} {::tree0 20 newKey wc} {::tree0 15 newKey wc} {::tree0 17 newKey wc} {::tree0 19 newKey wc} {::tree0 21 newKey wc} {::tree0 3 newKey wc} {::tree0 4 newKey wc} {::tree0 5 newKey wc} {::tree0 13 newKey wc} {::tree0 6 newKey wc} {::tree0 8 newKey wc}}}

test tree.357 {test read trace} {
    list [catch {
	set mylist {}
	tree0 get root newKey
	set mylist
	} msg] $msg
} {0 {{::tree0 0 newKey r}}}

test tree.358 {test write trace} {
    list [catch {
	set mylist {}
	tree0 set all newKey 21
	set mylist
	} msg] $msg
} {0 {{::tree0 0 newKey w} {::tree0 2 newKey w} {::tree0 1 newKey w} {::tree0 14 newKey w} {::tree0 16 newKey w} {::tree0 18 newKey w} {::tree0 20 newKey w} {::tree0 15 newKey w} {::tree0 17 newKey w} {::tree0 19 newKey w} {::tree0 21 newKey w} {::tree0 3 newKey w} {::tree0 4 newKey w} {::tree0 5 newKey w} {::tree0 13 newKey w} {::tree0 6 newKey w} {::tree0 8 newKey w}}}

test tree.359 {test unset trace} {
    list [catch {
	set mylist {}
	tree0 set all newKey 21
	set mylist
	} msg] $msg
} {0 {{::tree0 0 newKey w} {::tree0 2 newKey w} {::tree0 1 newKey w} {::tree0 14 newKey w} {::tree0 16 newKey w} {::tree0 18 newKey w} {::tree0 20 newKey w} {::tree0 15 newKey w} {::tree0 17 newKey w} {::tree0 19 newKey w} {::tree0 21 newKey w} {::tree0 3 newKey w} {::tree0 4 newKey w} {::tree0 5 newKey w} {::tree0 13 newKey w} {::tree0 6 newKey w} {::tree0 8 newKey w}}}

test tree.360 {tree0 trace delete} {
    list [catch {tree0 trace delete} msg] $msg
} {0 {}}

test tree.361 {tree0 trace delete badId} {
    list [catch {tree0 trace delete badId} msg] $msg
} {1 {unknown trace "badId"}}

test tree.362 {tree0 trace delete trace0} {
    list [catch {tree0 trace delete trace0} msg] $msg
} {0 {}}

test tree.363 {test create trace} {
    list [catch {
	set mylist {}
	tree0 set all newKey 20
	set mylist
	} msg] $msg
} {0 {}}

test tree.364 {test unset trace} {
    list [catch {
	set mylist {}
	tree0 unset all newKey
	set mylist
	} msg] $msg
} {0 {}}


test tree.365 {tree0 notify} {
    list [catch {tree0 notify} msg] $msg
} {1 {wrong # args: should be one of...
  tree0 notify create ?switches ...? command
  tree0 notify delete ?notifyName ...?
  tree0 notify info notifyName
  tree0 notify names ?pattern ...?}}

test tree.366 {tree0 notify create} {
    list [catch {tree0 notify create} msg] $msg
} {1 {wrong # args: should be "tree0 notify create ?switches ...? command"}}

test tree.367 {tree0 notify create -allevents} {
    list [catch {tree0 notify create -allevents Doit} msg] $msg
} {0 notify0}

test tree.368 {tree0 notify info notify0} {
    list [catch {tree0 notify info notify0} msg] $msg
} {0 {notify0 {-create -delete -move -sort -relabel} {Doit}}}

test tree.369 {tree0 notify info badId} {
    list [catch {tree0 notify info badId} msg] $msg
} {1 {unknown notify name "badId"}}

test tree.370 {tree0 notify info} {
    list [catch {tree0 notify info} msg] $msg
} {1 {wrong # args: should be "tree0 notify info notifyName"}}

test tree.371 {tree0 notify names} {
    list [catch {tree0 notify names} msg] $msg
} {0 notify0}


test tree.372 {test create notify} {
    list [catch {
	set mylist {}
	tree0 insert 1 -tags test
	set mylist
	} msg] $msg
} {0 {{-create 22}}}

test tree.373 {test move notify} {
    list [catch {
	set mylist {}
	tree0 move 8 test
	set mylist
	} msg] $msg
} {0 {{-move 8}}}

test tree.374 {test sort notify} {
    list [catch {
	set mylist {}
	tree0 sort 0 -reorder 
	set mylist
	} msg] $msg
} {0 {{-sort 0}}}

test tree.375 {test relabel notify} {
    list [catch {
	set mylist {}
	tree0 label test "newLabel"
	set mylist
	} msg] $msg
} {0 {{-relabel 22}}}

test tree.376 {test delete notify} {
    list [catch {
	set mylist {}
	tree0 delete test
	set mylist
	} msg] $msg
} {0 {{-delete 8} {-delete 22}}}


test tree.377 {tree0 notify delete badId} {
    list [catch {tree0 notify delete badId} msg] $msg
} {1 {unknown notify name "badId"}}


test tree.378 {test create notify} {
    list [catch {
	set mylist {}
	tree0 set all newKey 20
	set mylist
	} msg] $msg
} {0 {}}

test tree.379 {test delete notify} {
    list [catch {
	set mylist {}
	tree0 unset all newKey
	set mylist
	} msg] $msg
} {0 {}}

test tree.380 {test delete notify} {
    list [catch {
	set mylist {}
	tree0 unset all newKey
	set mylist
	} msg] $msg
} {0 {}}

test tree.381 {tree0 copy} {
    list [catch {tree0 copy} msg] $msg
} {1 {wrong # args: should be "tree0 copy parent ?tree? node ?switches ...?"}}

test tree.382 {tree0 copy root} {
    list [catch {tree0 copy root} msg] $msg
} {1 {wrong # args: should be "tree0 copy parent ?tree? node ?switches ...?"}}

test tree.383 {tree0 copy root 14} {
    list [catch {tree0 copy root 14} msg] $msg
} {0 23}

test tree.384 {tree0 copy 14 root} {
    list [catch {tree0 copy 14 root} msg] $msg
} {0 24}

test tree.385 {tree0 copy 14 root -recurse} {
    list [catch {tree0 copy 14 root -recurse} msg] $msg
} {1 {can't make cyclic copy: source node is an ancestor of the destination}}

test tree.386 {tree0 copy 3 2 -recurse -tags} {
    list [catch {tree0 copy 3 2 -recurse -tags} msg] $msg
} {0 25}

test tree.387 {copy tree to tree -recurse} {
    list [catch {
	blt::tree create tree1
	foreach node [tree0 children root] {
	    tree1 copy root tree0 $node -recurse 
	}
	foreach node [tree0 children root] {
	    tree1 copy root tree0 $node -recurse 
	}
	tree1 dump root
    } msg] $msg
} {0 {-1 0 {{}} {} {}
0 1 {{} node2} {key1 myValue} {}
1 2 {{} node2 node1} {key1 myValue} {}
2 3 {{} node2 node1 node14} {key1 myValue} {}
3 4 {{} node2 node1 node14 node16} {key1 123 key2 abc} {}
4 5 {{} node2 node1 node14 node16 node18} {key1 myValue} {}
5 6 {{} node2 node1 node14 node16 node18 node20} {key1 myValue} {}
3 7 {{} node2 node1 node14 {}} {key1 myValue} {}
2 8 {{} node2 node1 node15} {key1 123} {}
8 9 {{} node2 node1 node15 node17} {key1 myValue} {}
9 10 {{} node2 node1 node15 node17 node19} {key1 myValue} {}
10 11 {{} node2 node1 node15 node17 node19 node21} {key1 myValue} {}
0 12 {{} node3} {key1 myValue} {}
12 13 {{} node3 node2} {key1 myValue} {}
13 14 {{} node3 node2 node1} {key1 myValue} {}
14 15 {{} node3 node2 node1 node14} {key1 myValue} {}
15 16 {{} node3 node2 node1 node14 node16} {key1 123 key2 abc} {}
16 17 {{} node3 node2 node1 node14 node16 node18} {key1 myValue} {}
17 18 {{} node3 node2 node1 node14 node16 node18 node20} {key1 myValue} {}
15 19 {{} node3 node2 node1 node14 {}} {key1 myValue} {}
14 20 {{} node3 node2 node1 node15} {key1 123} {}
20 21 {{} node3 node2 node1 node15 node17} {key1 myValue} {}
21 22 {{} node3 node2 node1 node15 node17 node19} {key1 myValue} {}
22 23 {{} node3 node2 node1 node15 node17 node19 node21} {key1 myValue} {}
0 24 {{} node4} {key1 myValue} {}
0 25 {{} node5} {key1 myValue} {}
25 26 {{} node5 node13} {key1 myValue} {}
0 27 {{} node6} {key1 myValue} {}
0 28 {{} node14} {key1 myValue} {}
0 29 {{} node2} {key1 myValue} {}
29 30 {{} node2 node1} {key1 myValue} {}
30 31 {{} node2 node1 node14} {key1 myValue} {}
31 32 {{} node2 node1 node14 node16} {key1 123 key2 abc} {}
32 33 {{} node2 node1 node14 node16 node18} {key1 myValue} {}
33 34 {{} node2 node1 node14 node16 node18 node20} {key1 myValue} {}
31 35 {{} node2 node1 node14 {}} {key1 myValue} {}
30 36 {{} node2 node1 node15} {key1 123} {}
36 37 {{} node2 node1 node15 node17} {key1 myValue} {}
37 38 {{} node2 node1 node15 node17 node19} {key1 myValue} {}
38 39 {{} node2 node1 node15 node17 node19 node21} {key1 myValue} {}
0 40 {{} node3} {key1 myValue} {}
40 41 {{} node3 node2} {key1 myValue} {}
41 42 {{} node3 node2 node1} {key1 myValue} {}
42 43 {{} node3 node2 node1 node14} {key1 myValue} {}
43 44 {{} node3 node2 node1 node14 node16} {key1 123 key2 abc} {}
44 45 {{} node3 node2 node1 node14 node16 node18} {key1 myValue} {}
45 46 {{} node3 node2 node1 node14 node16 node18 node20} {key1 myValue} {}
43 47 {{} node3 node2 node1 node14 {}} {key1 myValue} {}
42 48 {{} node3 node2 node1 node15} {key1 123} {}
48 49 {{} node3 node2 node1 node15 node17} {key1 myValue} {}
49 50 {{} node3 node2 node1 node15 node17 node19} {key1 myValue} {}
50 51 {{} node3 node2 node1 node15 node17 node19 node21} {key1 myValue} {}
0 52 {{} node4} {key1 myValue} {}
0 53 {{} node5} {key1 myValue} {}
53 54 {{} node5 node13} {key1 myValue} {}
0 55 {{} node6} {key1 myValue} {}
0 56 {{} node14} {key1 myValue} {}
}}

test tree.388 {tree dir (no recurse flag)} {
    list [catch {
	file delete -force ./testdir
	file mkdir ./testdir/dir1
	file mkdir ./testdir/dir2
	file mkdir ./testdir/dir3
	file copy defs ./testdir/dir1
	set tree [blt::tree create]
	$tree dir 0 ./testdir \
	    -fields { perms type } \
	    -pattern defs \
	    -recurse
	set contents [$tree dump 0]
	blt::tree destroy $tree
	file delete -force ./testdir
	set contents
    } msg] $msg
} {0 {-1 0 {{}} {} {}
0 2 {{} dir1} {perms 493 type directory} {}
2 3 {{} dir1 defs} {perms 420 type file} {}
}}


test tree.389 {tree dir -recurse} {
    list [catch {
	file delete -force ./testdir
	file mkdir ./testdir/.dir0
	file mkdir ./testdir/dir1/dir2
	file copy defs ./testdir/dir1/dir2
	set tree [blt::tree create]
	$tree dir 0 ./testdir  -recurse -fields { perms type } -type f
	set contents [$tree dump 0]
	blt::tree destroy $tree
	set contents
    } msg] $msg
} {0 {-1 0 {{}} {} {}
0 1 {{} dir1} {perms 493 type directory} {}
1 2 {{} dir1 dir2} {perms 493 type directory} {}
2 3 {{} dir1 dir2 defs} {perms 420 type file} {}
}}



test tree.390 {tree dir -recurse} {
    list [catch {
	file delete -force ./testdir
	file mkdir ./testdir
	file copy defs ./testdir
	set tree [blt::tree create]
	$tree dir 0 ./testdir -recurse -fields { size perms type }
	set contents [$tree dump 0]
	blt::tree destroy $tree
	file delete -force ./testdir
	set contents
    } msg] $msg
} {0 {-1 0 {{}} {} {}
0 1 {{} defs} {size 2894 perms 420 type file} {}
}}

test tree.391 {tree dir (default settings, no -recurse)} {
    list [catch {
	file delete -force ./testdir
	file mkdir ./testdir/dir1
	file copy defs ./testdir/dir1
	set tree [blt::tree create]
	$tree dir 0 ./testdir -fields { perms type }
	set contents [$tree dump 0]
	blt::tree destroy $tree
	file delete -force ./testdir
	set contents
    } msg] $msg
} {0 {-1 0 {{}} {} {}
0 1 {{} dir1} {perms 493 type directory} {}
}}

test tree.392 {tree dir -type "file pipe"} {
    list [catch {
	set tree [blt::tree create]
	$tree dir 0 /dev -fields { size perms type } -type "file pipe"
	set contents [$tree dump 0]
	blt::tree destroy $tree
	set contents
    } msg] $msg
} {0 {-1 0 {{}} {} {}
0 1 {{} core} {size 140737477881856 perms 256 type file} {}
0 2 {{} stderr} {size 0 perms 384 type fifo} {}
0 3 {{} stdout} {size 0 perms 384 type fifo} {}
0 4 {{} initctl} {size 0 perms 384 type fifo} {}
}}

test tree.393 {tree dir -type link -recurse} {
    list [catch {
	file delete -force ./testdir
	file mkdir ./testdir/dir1
	file copy defs ./testdir/dir1
	file link -symbolic [pwd]/testdir/mylink [pwd]/testdir/dir1/defs 
	set tree [blt::tree create]
	$tree dir 0 ./testdir -fields { size perms type } -type "link" -recurse
	set contents [$tree dump 0]
	blt::tree destroy $tree
	file delete -force ./testdir
	set contents
    } msg] $msg
} {0 {-1 0 {{}} {} {}
0 2 {{} mylink} {size 2894 perms 420 type file} {}
}}

exit 0




