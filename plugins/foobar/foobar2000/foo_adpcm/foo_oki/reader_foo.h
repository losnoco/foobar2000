struct STREAM_READER : public libpcm_IReader
{
	volatile uint32_t refcount;
	service_ptr_t<file> reader;
	abort_callback * m_abort;
	uint32_t size;
};

static void LIBPCM_CALLBACK reader_seek(void *pv, uint32_t bytes)
{
	STREAM_READER *s = static_cast <STREAM_READER *> (pv);
	s->reader->seek_e(bytes, *s->m_abort);
}
static uint_t LIBPCM_CALLBACK reader_read(void *pv, void *buf, uint_t nbytes)
{
	STREAM_READER *s = static_cast <STREAM_READER *> (pv);
	return s->reader->read_e(buf, nbytes, *s->m_abort);
}
static uint32_t LIBPCM_CALLBACK reader_size(void *pv)
{
	STREAM_READER *s = static_cast <STREAM_READER *> (pv);
	return s->size;
}
static void * LIBPCM_CALLBACK reader_addref(void *pv)
{
	STREAM_READER *s = static_cast <STREAM_READER *> (pv);
	s->refcount++;
	return s;
}
static void LIBPCM_CALLBACK reader_release(void *pv)
{
	STREAM_READER *s = static_cast <STREAM_READER *> (pv);
	if (--s->refcount == 0)
		delete s;
}
static const char * LIBPCM_CALLBACK reader_path(void *pv)
{
	return "";
}
static const libpcm_IReaderVtbl FooReaderVtbl =
{
	reader_addref,
	reader_release,
	reader_size,
	reader_read,
	reader_seek,
	reader_path,
};

libpcm_IReader * libpcm_CreateFooReader( service_ptr_t<file> & preader, abort_callback & p_abort )
{
	if ( preader->get_size_e( p_abort ) <= 0) return 0;
	if (!preader->can_seek()) return 0;
	STREAM_READER *s = new STREAM_READER;
	if (!s) return 0;
	s->lpVtbl = &FooReaderVtbl;
	s->refcount = 1;
	s->reader = preader;
	s->m_abort = & p_abort;
	t_uint64 size = preader->get_size_e( p_abort );
	if ( size > UINT_MAX ) size = UINT_MAX;
	s->size = uint32_t(size);
	return s;
}

void libpcm_SetFooReaderAbort( libpcm_IReader * pv, abort_callback & p_abort )
{
	STREAM_READER *s = static_cast <STREAM_READER *> (pv);
	s->m_abort = &p_abort;
}