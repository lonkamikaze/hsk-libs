#!/usr/bin/awk -f

/^[ \t]+#/ {sub(/.*/, "")}
/\/\*\*/ {comment=1}
/\*\// {comment=0}
comment {sub(/^[ \t]*\* ?/, "")}
{print}

