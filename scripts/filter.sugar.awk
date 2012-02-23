#!/usr/bin/awk -f

/^[[:space:]]+#/ {sub(".*", "")}
{print}

