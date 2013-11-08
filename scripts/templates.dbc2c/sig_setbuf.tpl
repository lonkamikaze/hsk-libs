buf[<:byte:>] &= ~(<:mask:> << <:align:>);
buf[<:byte:>] |= (((val) >> <:pos:>) & <:mask:>) << <:align:>;
