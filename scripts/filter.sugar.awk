#!/usr/bin/awk -f

/^[[:space:]]+#/ {sub(/.*/, "")}
/\/\*\*/ {comment=1}
/\*\// {comment=0}
comment {sub(/^[[:space:]]*\* ?/, "")}
{print}

