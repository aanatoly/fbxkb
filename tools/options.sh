#!/bin/bash

# creates tooltip with explanation for xkb group options

file=/usr/share/X11/xkb/rules/base.lst
var=`xprop -root _XKB_RULES_NAMES | sed -e 's/.*"\([^"]*\)"/\1/' -e 's/,/ /g'`
for i in $var; do  grep $i $file; done | sort -u | awk '
BEGIN {
    text="<tt>"
}
{
    #$1="<b>" $1 "</b>"
    sub("^[[:space:]]*", "<b>")
    sub("[[:space:]]", "</b> ")
    if (NR > 1) {
        text=text "\n"
    }
    text=text $0
}
END {
    text=text "</tt>"
    print text            
}'

