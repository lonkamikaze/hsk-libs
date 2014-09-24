buf[<:byte:>] &= ~(<:msk:%#04x:> << <:align:>);
<?int8?>buf[<:byte:>] |= (((ubyte)(val) >> <:pos:>) & <:msk:%#04x:>) << <:align:>;
<?int16?>buf[<:byte:>] |= ((ubyte)((uword)(val) >> <:pos:>) & <:msk:%#04x:>) << <:align:>;
<?int32?>buf[<:byte:>] |= ((ubyte)((ulong)(val) >> <:pos:>) & <:msk:%#04x:>) << <:align:>;
