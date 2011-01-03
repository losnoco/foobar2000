I'm not sure whether to call this GPL, or public domain, or whatever, since the original LHA
code was without an obvious English license, while the C++-ized stuff that makes up most
of this unlha code was ripped right out of Open ModPlug Tracker.

I think I'll just slap BSD on my own stuff, which is unpack_lha.cpp, and the modifications to
file_cached.h, which I used to speed up byte-per-byte reads from the LHA compressed
data blocks. I suppose I could have done that with my own implementation of buffering, but
this was on hand and easily adapted to my needs.

Please don't fork this if you have any feature suggestions. I would be happier if I saw any
improvements forwarded my way, perhaps by forum PM or e-mail. That way, this stays together
and such. :)

Oh, and not exactly the cleanest implementation of archive_impl service, or unpacker. I hope
to do better soon, and/or improve this implementation.
