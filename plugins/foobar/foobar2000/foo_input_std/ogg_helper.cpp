#include "stdafx.h"

//originally ripped from vorbisfile.c, removed all vorbis-specific stuff



#include <stdlib.h>
#include <stdio.h>

#include <ogg/ogg.h>

#include "ogg_helper.h"

#define OV_FALSE      -1  
#define OV_EOF        -2
#define OV_HOLE       -3

#define OV_EREAD      -128
#define OV_EFAULT     -129
#define OV_EIMPL      -130
#define OV_EINVAL     -131
#define OV_ENOTVORBIS -132
#define OV_EBADHEADER -133
#define OV_EVERSION   -134
#define OV_ENOTAUDIO  -135
#define OV_EBADPACKET -136
#define OV_EBADLINK   -137
#define OV_ENOSEEK    -138



typedef struct {
  size_t (*read_func)  (void *ptr, size_t size, size_t nmemb, void *datasource);
  int    (*seek_func)  (void *datasource, ogg_int64_t offset, int whence);
  int    (*close_func) (void *datasource);
  ogg_int64_t (*tell_func)  (void *datasource);
} ov_callbacks;

#define  NOTOPEN   0
#define  PARTOPEN  1
#define  OPENED    2
#define  STREAMSET 3
#define  INITSET   4

typedef struct Ogg_File {
  void            *datasource; /* Pointer to a FILE *, etc. */
  int              seekable;
  ogg_int64_t      offset;
  ogg_int64_t      end;
  ogg_sync_state   oy; 

  /* If the FILE handle isn't seekable (eg, a pipe), only the current
     stream appears */
  int              links;
  ogg_int64_t     *offsets;
  ogg_int64_t     *dataoffsets;
  long            *serialnos;

  /* Decoding working state local storage */
  ogg_int64_t      pcm_offset;
  int              ready_state;
  long             current_serialno;
  int              current_link;

  double           bittrack;
  double           samptrack;

  ogg_stream_state os; /* take physical pages, weld into a logical
                          stream of packets */

  ov_callbacks callbacks;

} Ogg_File;

int ov_clear(Ogg_File *vf);



#define CHUNKSIZE 8500 /* a shade over 8k; anyone using pages well
                          over 8k gets what they deserve */
static long _get_data(Ogg_File *vf){
  errno=0;
  if(vf->datasource){
    char *buffer=ogg_sync_buffer(&vf->oy,CHUNKSIZE);
    long bytes=(vf->callbacks.read_func)(buffer,1,CHUNKSIZE,vf->datasource);
    if(bytes>0)ogg_sync_wrote(&vf->oy,bytes);
    if(bytes==0 && errno)return(-1);
    return(bytes);
  }else
    return(0);
}

/* save a tiny smidge of verbosity to make the code more readable */
static void _seek_helper(Ogg_File *vf,ogg_int64_t offset){
  if(vf->datasource){ 
    (vf->callbacks.seek_func)(vf->datasource, offset, SEEK_SET);
    vf->offset=offset;
    ogg_sync_reset(&vf->oy);
  }else{
    /* shouldn't happen unless someone writes a broken callback */
    return;
  }
}

/* The read/seek functions track absolute position within the stream */

/* from the head of the stream, get the next page.  boundary specifies
   if the function is allowed to fetch more data from the stream (and
   how much) or only use internally buffered data.

   boundary: -1) unbounded search
              0) read no additional data; use cached only
	      n) search for a new page beginning for n bytes

   return:   <0) did not find a page (OV_FALSE, OV_EOF, OV_EREAD)
              n) found a page at absolute offset n */

static ogg_int64_t _get_next_page(Ogg_File *vf,ogg_page *og,
				  ogg_int64_t boundary){
  if(boundary>0)boundary+=vf->offset;
  while(1){
    long more;

    if(boundary>0 && vf->offset>=boundary)return(OV_FALSE);
    more=ogg_sync_pageseek(&vf->oy,og);
    
    if(more<0){
      /* skipped n bytes */
      vf->offset-=more;
    }else{
      if(more==0){
	/* send more paramedics */
	if(!boundary)return(OV_FALSE);
	{
	  long ret=_get_data(vf);
	  if(ret==0)return(OV_EOF);
	  if(ret<0)return(OV_EREAD);
	}
      }else{
	/* got a page.  Return the offset at the page beginning,
           advance the internal offset past the page end */
	ogg_int64_t ret=vf->offset;
	vf->offset+=more;
	return(ret);
	
      }
    }
  }
}

/* find the latest page beginning before the current stream cursor
   position. Much dirtier than the above as Ogg doesn't have any
   backward search linkage.  no 'readp' as it will certainly have to
   read. */
/* returns offset or OV_EREAD, OV_FAULT */
static ogg_int64_t _get_prev_page(Ogg_File *vf,ogg_page *og){
  ogg_int64_t begin=vf->offset;
  ogg_int64_t end=begin;
  ogg_int64_t ret;
  ogg_int64_t offset=-1;

  while(offset==-1){
    begin-=CHUNKSIZE;
    if(begin<0)
      begin=0;
    _seek_helper(vf,begin);
    while(vf->offset<end){
      ret=_get_next_page(vf,og,end-vf->offset);
      if(ret==OV_EREAD)return(OV_EREAD);
      if(ret<0){
	break;
      }else{
	offset=ret;
      }
    }
  }

  /* we have the offset.  Actually snork and hold the page now */
  _seek_helper(vf,offset);
  ret=_get_next_page(vf,og,CHUNKSIZE);
  if(ret<0)
    /* this shouldn't be possible */
    return(OV_EFAULT);

  return(offset);
}

/* finds each bitstream link one at a time using a bisection search
   (has to begin by knowing the offset of the lb's initial page).
   Recurses for each link so it can alloc the link storage after
   finding them all, then unroll and fill the cache at the same time */
static int _bisect_forward_serialno(Ogg_File *vf,
				    ogg_int64_t begin,
				    ogg_int64_t searched,
				    ogg_int64_t end,
				    long currentno,
				    long m){
  ogg_int64_t endsearched=end;
  ogg_int64_t next=end;
  ogg_page og;
  ogg_int64_t ret;
  
  /* the below guards against garbage seperating the last and
     first pages of two links. */
  while(searched<endsearched){
    ogg_int64_t bisect;
    
    if(endsearched-searched<CHUNKSIZE){
      bisect=searched;
    }else{
      bisect=(searched+endsearched)/2;
    }
    
    _seek_helper(vf,bisect);
    ret=_get_next_page(vf,&og,-1);
    if(ret==OV_EREAD)return(OV_EREAD);
    if(ret<0 || ogg_page_serialno(&og)!=currentno){
      endsearched=bisect;
      if(ret>=0)next=ret;
    }else{
      searched=ret+og.header_len+og.body_len;
    }
  }

  _seek_helper(vf,next);
  ret=_get_next_page(vf,&og,-1);
  if(ret==OV_EREAD)return(OV_EREAD);
  
  if(searched>=end || ret<0){
    vf->links=m+1;
    vf->offsets=(t_int64*)_ogg_malloc((vf->links+1)*sizeof(*vf->offsets));
    vf->serialnos=(long*)_ogg_malloc(vf->links*sizeof(*vf->serialnos));
    vf->offsets[m+1]=searched;
  }else{
    ret=_bisect_forward_serialno(vf,next,vf->offset,
				 end,ogg_page_serialno(&og),m+1);
    if(ret==OV_EREAD)return(OV_EREAD);
  }
  
  vf->offsets[m]=begin;
  vf->serialnos[m]=currentno;
  return(0);
}

/* uses the local ogg_stream storage in vf; this is important for
   non-streaming input sources */
static int _fetch_headers(Ogg_File *vf,
			  long *serialno,ogg_page *og_ptr){
  ogg_page og;
  ogg_packet op;
  int i,ret;
  
  if(!og_ptr){
    ogg_int64_t llret=_get_next_page(vf,&og,CHUNKSIZE);
    if(llret==OV_EREAD)return(OV_EREAD);
    if(llret<0)return OV_ENOTVORBIS;
    og_ptr=&og;
  }

  ogg_stream_reset_serialno(&vf->os,ogg_page_serialno(og_ptr));
  if(serialno)*serialno=vf->os.serialno;
  vf->ready_state=STREAMSET;
  
  /* extract the initial header from the first page and verify that the
     Ogg bitstream is in fact Vorbis data */
  
  
  i=0;
  while(i<3){
    ogg_stream_pagein(&vf->os,og_ptr);
    while(i<3){
      int result=ogg_stream_packetout(&vf->os,&op);
      if(result==0)break;
      if(result==-1){
	ret=OV_EBADHEADER;
	goto bail_header;
      }
//      if((ret=vorbis_synthesis_headerin(vi,vc,&op))){
//	goto bail_header;
  //    }
      i++;
    }
    if(i<3)
      if(_get_next_page(vf,og_ptr,CHUNKSIZE)<0){
	ret=OV_EBADHEADER;
	goto bail_header;
      }
  }
  return 0; 

 bail_header:
  vf->ready_state=OPENED;

  return ret;
}

/* last step of the Ogg_File initialization; get all the
   vorbis_info structs and PCM positions.  Only called by the seekable
   initialization (local stream storage is hacked slightly; pay
   attention to how that's done) */

/* this is void and does not propogate errors up because we want to be
   able to open and use damaged bitstreams as well as we can.  Just
   watch out for missing information for links in the Ogg_File
   struct */
static void _prefetch_all_headers(Ogg_File *vf, ogg_int64_t dataoffset){
  ogg_page og;
  int i;
  ogg_int64_t ret;
  
  vf->dataoffsets=(t_int64*)_ogg_malloc(vf->links*sizeof(*vf->dataoffsets));
  
  for(i=0;i<vf->links;i++){
    if(i==0){
      /* we already grabbed the initial header earlier.  Just set the offset */
      vf->dataoffsets[i]=dataoffset;
      _seek_helper(vf,dataoffset);

    }else{

      /* seek to the location of the initial header */

      _seek_helper(vf,vf->offsets[i]);
      if(_fetch_headers(vf,NULL,NULL)<0){
    	vf->dataoffsets[i]=-1;
      }else{
	vf->dataoffsets[i]=vf->offset;
      }
    }

    /* fetch beginning PCM offset */

    if(vf->dataoffsets[i]!=-1){
      ogg_int64_t accumulated=0;
      int         result;

      ogg_stream_reset_serialno(&vf->os,vf->serialnos[i]);

      while(1){
	ogg_packet op;

	ret=_get_next_page(vf,&og,-1);
	if(ret<0)
	  /* this should not be possible unless the file is
             truncated/mangled */
	  break;
       
	if(ogg_page_serialno(&og)!=vf->serialnos[i])
	  break;
	
	/* count blocksizes of all frames in the page */
	ogg_stream_pagein(&vf->os,&og);
	while((result=ogg_stream_packetout(&vf->os,&op))){
	}

	if(ogg_page_granulepos(&og)!=-1){
	  /* pcm offset of last packet on the first audio page */
	  accumulated= ogg_page_granulepos(&og)-accumulated;
	  break;
	}
      }

      /* less than zero?  This is a stream with samples trimmed off
         the beginning, a normal occurrence; set the offset to zero */
      if(accumulated<0)accumulated=0;

    }

    /* get the PCM length of this link. To do this,
       get the last page of the stream */
    {
      ogg_int64_t end=vf->offsets[i+1];
      _seek_helper(vf,end);

      while(1){
	ret=_get_prev_page(vf,&og);
	if(ret<0){
	  /* this should not be possible */
	  break;
	}
	if(ogg_page_granulepos(&og)!=-1){

		break;
	}
	vf->offset=ret;
      }
    }
  }
}


static int _open_seekable2(Ogg_File *vf){
  long serialno=vf->current_serialno;
  ogg_int64_t dataoffset=vf->offset, end;
  ogg_page og;

  /* we're partially open and have a first link header state in
     storage in vf */
  /* we can seek, so set out learning all about this file */
  (vf->callbacks.seek_func)(vf->datasource,0,SEEK_END);
  vf->offset=vf->end=(vf->callbacks.tell_func)(vf->datasource);
  
  /* We get the offset for the last page of the physical bitstream.
     Most OggVorbis files will contain a single logical bitstream */
  end=_get_prev_page(vf,&og);
  if(end<0)return((int)end);

  /* more than one logical bitstream? */
  if(ogg_page_serialno(&og)!=serialno){

    /* Chained bitstream. Bisect-search each logical bitstream
       section.  Do so based on serial number only */
    if(_bisect_forward_serialno(vf,0,0,end+1,serialno,0)<0)return(OV_EREAD);

  }else{

    /* Only one logical bitstream */
    if(_bisect_forward_serialno(vf,0,end,end+1,serialno,0))return(OV_EREAD);

  }

  /* the initial header memory is referenced by vf after; don't free it */
  _prefetch_all_headers(vf,dataoffset);
  return 0;//(ov_raw_seek(vf,0));
}

static int _ov_open1(void *f,Ogg_File *vf,char *initial,
		     long ibytes, ov_callbacks callbacks){
  int offsettest=(f?callbacks.seek_func(f,0,SEEK_CUR):-1);
  int ret;

  memset(vf,0,sizeof(*vf));
  vf->datasource=f;
  vf->callbacks = callbacks;

  /* init the framing state */
  ogg_sync_init(&vf->oy);

  /* perhaps some data was previously read into a buffer for testing
     against other stream types.  Allow initialization from this
     previously read data (as we may be reading from a non-seekable
     stream) */
  if(initial){
    char *buffer=ogg_sync_buffer(&vf->oy,ibytes);
    memcpy(buffer,initial,ibytes);
    ogg_sync_wrote(&vf->oy,ibytes);
  }

  /* can we seek? Stevens suggests the seek test was portable */
  if(offsettest!=-1)vf->seekable=1;

  /* No seeking yet; Set up a 'single' (current) logical bitstream
     entry for partial open */
  vf->links=1;
  ogg_stream_init(&vf->os,-1); /* fill in the serialno later */

  /* Try to fetch the headers, maintaining all the storage */
  if((ret=_fetch_headers(vf,&vf->current_serialno,NULL))<0){
    vf->datasource=NULL;
    ov_clear(vf);
  }else if(vf->ready_state < PARTOPEN)
    vf->ready_state=PARTOPEN;
  return(ret);
}

static int _ov_open2(Ogg_File *vf){
  if(vf->ready_state < OPENED)
    vf->ready_state=OPENED;
  if(vf->seekable){
    int ret=_open_seekable2(vf);
    if(ret){
      vf->datasource=NULL;
      ov_clear(vf);
    }
    return(ret);
  }
  return 0;
}


/* clear out the Ogg_File struct */
int ov_clear(Ogg_File *vf){
  if(vf){
    ogg_stream_clear(&vf->os);
    
    if(vf->dataoffsets)_ogg_free(vf->dataoffsets);
    if(vf->serialnos)_ogg_free(vf->serialnos);
    if(vf->offsets)_ogg_free(vf->offsets);
    ogg_sync_clear(&vf->oy);
    if(vf->datasource)(vf->callbacks.close_func)(vf->datasource);
    memset(vf,0,sizeof(*vf));
  }
#ifdef DEBUG_LEAKS
  _VDBG_dump();
#endif
  return(0);
}

int ov_open_callbacks(void *f,Ogg_File *vf,char *initial,long ibytes,
    ov_callbacks callbacks){
  int ret=_ov_open1(f,vf,initial,ibytes,callbacks);
  if(ret)return ret;
  return _ov_open2(vf);
}

/* Guess */
long ov_serialnumber(Ogg_File *vf,int i){
  if(i>=vf->links)return(ov_serialnumber(vf,vf->links-1));
  if(!vf->seekable && i>=0)return(ov_serialnumber(vf,-1));
  if(i<0){
    return(vf->current_serialno);
  }else{
    return(vf->serialnos[i]);
  }
}

/* returns: total raw (compressed) length of content if i==-1
            raw (compressed) length of that logical bitstream for i==0 to n
	    OV_EINVAL if the stream is not seekable (we can't know the length)
	    or if stream is only partially open
*/
ogg_int64_t ov_raw_total(Ogg_File *vf,int i){
  if(vf->ready_state<OPENED)return(OV_EINVAL);
  if(!vf->seekable || i>=vf->links)return(OV_EINVAL);
  if(i<0){
    ogg_int64_t acc=0;
    int i;
    for(i=0;i<vf->links;i++)
      acc+=ov_raw_total(vf,i);
    return(acc);
  }else{
    return(vf->offsets[i+1]-vf->offsets[i]);
  }
}

struct ogg_callback_struct
{
	service_ptr_t<file> m_reader;
	abort_callback & m_abort;
	ogg_callback_struct(const service_ptr_t<file> & p_reader,abort_callback & p_abort) : m_reader(p_reader), m_abort(p_abort) {}
};

static size_t callback_fread(void *ptr, size_t p_size, size_t p_nmemb, ogg_callback_struct * r)
{
	t_io_result io_status;
	unsigned io_bytes_done;	
	io_status = r->m_reader->read(ptr,p_size*p_nmemb,io_bytes_done,r->m_abort);
	if (io_result_failed(io_status)) return 0;
	else return io_bytes_done / p_size;
}

static size_t callback_write(const void *ptr, size_t p_size, size_t p_nmemb, ogg_callback_struct * r)
{
	t_io_result io_status;
	unsigned io_bytes_done;	
	io_status = r->m_reader->write(ptr,p_size*p_nmemb,io_bytes_done,r->m_abort);
	if (io_result_failed(io_status)) return 0;
	else return io_bytes_done / p_size;
}

static int callback_fseek(ogg_callback_struct *r, t_int64 offset, int whence)
{
	if (!r->m_reader->can_seek()) return -1;
	return io_result_succeeded(r->m_reader->seek2(offset,whence,r->m_abort)) ? 0 : 1;
}

static int callback_fclose(ogg_callback_struct *r) {return 0;}

static t_int64 callback_ftell(ogg_callback_struct *r)
{
	t_io_result status;
	t_filesize ret;
	status = r->m_reader->get_position(ret,r->m_abort);
	if (io_result_failed(status)) return -1;
	else if (ret == filesize_invalid) return -1;
	else return (t_int64) ret;
}


static void* callbacks[4]=
{
	callback_fread,callback_fseek,callback_fclose,callback_ftell
};

static void bad_stream_whine(const char * fn)
{
	if (fn && *fn)
	{
		string8 temp;
		temp = "Ogg stream is corrupted : ";
		temp += file_path_display(fn);
		console::info(temp);
	}
	else
	{
		console::info("Ogg stream is corrupted.");			
	}
}

namespace ogg_helper
{
#if 0
	int get_tracks(const char* filename,track_indexer::callback * out,reader * r)
	{
		if (stricmp(string_extension_8(filename),"OGG") && stricmp(string_extension_8(filename),"SPX") ) return 0;
		bool own_reader = 0;
		if (!r)
		{
			r = file::g_open_precache(filename);
			if (!r) return 0;
			own_reader = 1;
		}

		int links = query_link_count(filename,r);

		if (links==0)
		{
			if (own_reader) r->reader_release();
			return 0;
		}
		
		int n;
		for(n=0;n<links;n++)
			out->on_entry(make_playable_location(filename,n));

		return 1;
	}
#endif
	t_io_result query_link_count(const char * filename,const service_ptr_t<file> & r,unsigned & p_count,abort_callback & p_abort)
	{
		char hdr[4];
		t_io_result status;

		if (io_result_failed(status = r->seek(0,p_abort))) return status;
		if (io_result_failed(status = r->read_object(hdr,4,p_abort))) return status;
		if (io_result_failed(status = r->seek(0,p_abort))) return status;

		if (strncmp(hdr,"OggS",4))
		{
			if (filename) bad_stream_whine(filename);
		}
		
		if (io_result_failed(status = r->seek(0,p_abort))) return status;
		Ogg_File l_vf;
		ogg_callback_struct callbackstruct(r,p_abort);
		memset(&l_vf,0,sizeof(l_vf));
		if (ov_open_callbacks(&callbackstruct,&l_vf,0,0,*(ov_callbacks*)callbacks))
			return io_result_error_data;

		p_count = l_vf.links;

		ov_clear(&l_vf);

		return io_result_success;
	}


	t_io_result query_chained_stream_offset(const service_ptr_t<file> & r,const playable_location & p_location,t_filesize & out_beginning,t_filesize & out_end,abort_callback & p_abort)
	{
		char hdr[4];
		t_io_result status;
		if (io_result_failed(status = r->seek(0,p_abort))) return status;
		if (io_result_failed(status = r->read_object(hdr,4,p_abort))) return status;
		if (io_result_failed(status = r->seek(0,p_abort))) return status;
		if (strncmp(hdr,"OggS",4) != 0) bad_stream_whine(p_location.get_path());
		Ogg_File l_vf;
		memset(&l_vf,0,sizeof(l_vf));
		ogg_callback_struct callbackstruct(r,p_abort);
		if (ov_open_callbacks(&callbackstruct,&l_vf,0,0,*(ov_callbacks*)callbacks))
			return io_result_error_data;
		bool success = false;
		int number = p_location.get_subsong();
		if (number>=0 && number<l_vf.links)
		{
			success = true;
			out_beginning = l_vf.offsets[number];
			out_end = l_vf.offsets[number+1];
		}

		ov_clear(&l_vf);
		return success ? io_result_success : io_result_error_data;;
	}
}