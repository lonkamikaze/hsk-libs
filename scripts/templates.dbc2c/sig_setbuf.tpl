buf[<:byte:>] &= ~(<:mask:> << <:align:>);
<?int8?>buf[<:byte:>] |= (((ubyte)(val) >> <:pos:>) & <:mask:>) << <:align:>;
<?int16?>buf[<:byte:>] |= ((ubyte)((uword)(val) >> <:pos:>) & <:mask:>) << <:align:>;
<?int32?>buf[<:byte:>] |= ((ubyte)((ulong)(val) >> <:pos:>) & <:mask:>) << <:align:>;
