#include "stdafx.h"

enum
{
	MAX_FIELD_SIZE = 16*1024*1024 //treat bigger fields as errors
};

static const char * rg_fields[]=
{
	"replaygain_track_gain","replaygain_track_peak","replaygain_album_gain","replaygain_album_peak",
};

static const char * other_info_fields[]=
{
	"mp3gain_minmax","mp3gain_album_minmax","mp3gain_undo"
};


#define APE_TAG_FIELD_TITLE             "Title"
#define APE_TAG_FIELD_SUBTITLE          "Subtitle"
#define APE_TAG_FIELD_ARTIST            "Artist"
#define APE_TAG_FIELD_ALBUM             "Album"
#define APE_TAG_FIELD_DEBUTALBUM        "Debut Album"
#define APE_TAG_FIELD_PUBLISHER         "Publisher"
#define APE_TAG_FIELD_CONDUCTOR         "Conductor"
#define APE_TAG_FIELD_COMPOSER          "Composer"
#define APE_TAG_FIELD_COMMENT           "Comment"
#define APE_TAG_FIELD_YEAR              "Date"
#define APE_TAG_FIELD_RECORDDATE        "Record Date"
#define APE_TAG_FIELD_RECORDLOCATION    "Record Location"
#define APE_TAG_FIELD_TRACK             "Tracknumber"
#define APE_TAG_FIELD_GENRE             "Genre"
#define APE_TAG_FIELD_COVER_ART_FRONT   "Cover Art (front)"
#define APE_TAG_FIELD_NOTES             "Notes"
#define APE_TAG_FIELD_LYRICS            "Lyrics"
#define APE_TAG_FIELD_COPYRIGHT         "Copyright"
#define APE_TAG_FIELD_PUBLICATIONRIGHT  "Publicationright"
#define APE_TAG_FIELD_FILE              "File"
#define APE_TAG_FIELD_MEDIA             "Media"
#define APE_TAG_FIELD_EANUPC            "EAN/UPC"
#define APE_TAG_FIELD_ISRC              "ISRC"
#define APE_TAG_FIELD_RELATED_URL       "Related"
#define APE_TAG_FIELD_ABSTRACT_URL      "Abstract"
#define APE_TAG_FIELD_LANGUAGE          "Language"
#define APE_TAG_FIELD_BIBLIOGRAPHY_URL  "Bibliography"
#define APE_TAG_FIELD_BUY_URL           "Buy URL"
#define APE_TAG_FIELD_ARTIST_URL        "Artist URL"
#define APE_TAG_FIELD_PUBLISHER_URL     "Publisher URL"
#define APE_TAG_FIELD_FILE_URL          "File URL"
#define APE_TAG_FIELD_COPYRIGHT_URL     "Copyright URL"
#define APE_TAG_FIELD_INDEX             "Index"
#define APE_TAG_FIELD_INTROPLAY         "Introplay"
#define APE_TAG_FIELD_MJ_METADATA       "Media Jukebox Metadata"
#define APE_TAG_FIELD_DUMMY             "Dummy"

static const char *standard_ape2_field[] = {
    APE_TAG_FIELD_TITLE,
    APE_TAG_FIELD_SUBTITLE,
    APE_TAG_FIELD_ARTIST,
    APE_TAG_FIELD_ALBUM,
    APE_TAG_FIELD_DEBUTALBUM,
    APE_TAG_FIELD_PUBLISHER,
    APE_TAG_FIELD_CONDUCTOR,
    APE_TAG_FIELD_COMPOSER,
    APE_TAG_FIELD_COMMENT,
    "Year",     // APE_TAG_FIELD_YEAR
    APE_TAG_FIELD_RECORDDATE,
    APE_TAG_FIELD_RECORDLOCATION,
    "Track",    // APE_TAG_FIELD_TRACK
    APE_TAG_FIELD_GENRE,
    APE_TAG_FIELD_COVER_ART_FRONT,
    APE_TAG_FIELD_NOTES,
    APE_TAG_FIELD_LYRICS,
    APE_TAG_FIELD_COPYRIGHT,
    APE_TAG_FIELD_PUBLICATIONRIGHT,
    APE_TAG_FIELD_FILE,
    APE_TAG_FIELD_MEDIA,
    APE_TAG_FIELD_EANUPC,
    APE_TAG_FIELD_ISRC,
    APE_TAG_FIELD_RELATED_URL,
    APE_TAG_FIELD_ABSTRACT_URL,
    APE_TAG_FIELD_LANGUAGE,
    APE_TAG_FIELD_BIBLIOGRAPHY_URL,
    APE_TAG_FIELD_BUY_URL,
    APE_TAG_FIELD_ARTIST_URL,
    APE_TAG_FIELD_PUBLISHER_URL,
    APE_TAG_FIELD_FILE_URL,
    APE_TAG_FIELD_COPYRIGHT_URL,
    APE_TAG_FIELD_INDEX,
    APE_TAG_FIELD_INTROPLAY,
    APE_TAG_FIELD_MJ_METADATA,
    APE_TAG_FIELD_DUMMY,
};

static const char*  ID3v1GenreList[] = {
    "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk",
    "Grunge", "Hip-Hop", "Jazz", "Metal", "New Age", "Oldies",
    "Other", "Pop", "R&B", "Rap", "Reggae", "Rock",
    "Techno", "Industrial", "Alternative", "Ska", "Death Metal", "Pranks",
    "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk",
    "Fusion", "Trance", "Classical", "Instrumental", "Acid", "House",
    "Game", "Sound Clip", "Gospel", "Noise", "AlternRock", "Bass",
    "Soul", "Punk", "Space", "Meditative", "Instrumental Pop", "Instrumental Rock",
    "Ethnic", "Gothic", "Darkwave", "Techno-Industrial", "Electronic", "Pop-Folk",
    "Eurodance", "Dream", "Southern Rock", "Comedy", "Cult", "Gangsta",
    "Top 40", "Christian Rap", "Pop/Funk", "Jungle", "Native American", "Cabaret",
    "New Wave", "Psychadelic", "Rave", "Showtunes", "Trailer", "Lo-Fi",
    "Tribal", "Acid Punk", "Acid Jazz", "Polka", "Retro", "Musical",
    "Rock & Roll", "Hard Rock", "Folk", "Folk/Rock", "National Folk", "Swing",
    "Fast-Fusion", "Bebob", "Latin", "Revival", "Celtic", "Bluegrass", "Avantgarde",
    "Gothic Rock", "Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock", "Big Band",
    "Chorus", "Easy Listening", "Acoustic", "Humour", "Speech", "Chanson",
    "Opera", "Chamber Music", "Sonata", "Symphony", "Booty Bass", "Primus",
    "Porn Groove", "Satire", "Slow Jam", "Club", "Tango", "Samba",
    "Folklore", "Ballad", "Power Ballad", "Rhythmic Soul", "Freestyle", "Duet",
    "Punk Rock", "Drum Solo", "A capella", "Euro-House", "Dance Hall",
    "Goa", "Drum & Bass", "Club House", "Hardcore", "Terror",
    "Indie", "BritPop", "NegerPunk", "Polsk Punk", "Beat",
    "Christian Gangsta", "Heavy Metal", "Black Metal", "Crossover", "Contemporary C",
    "Christian Rock", "Merengue", "Salsa", "Thrash Metal", "Anime", "JPop",
    "SynthPop",
};

static const struct
{
	const char * name_ape,*name_global;
} ape2_field_translation[]=
{
	{"Track","tracknumber"},
	{"Year","date"},
};


// Get ID3v1 genre name
static bool GenreToString ( char* GenreStr, const int genre )
{
    if ( genre >= 0 && genre < sizeof (ID3v1GenreList) / sizeof (*ID3v1GenreList) ) {
        strcpy ( GenreStr, ID3v1GenreList[genre] );
        return true;
    } else {
        GenreStr[0] = '\0';
        return false;
    }
}

static void memcpy_crop ( void * _dst, const void * _src, size_t len )
{
	char * dst = (char*)_dst;
	const char * src = (const char*)_src;
    size_t i;

    for ( i = 0; i < len; i++ )
        if ( src[i] != '\0' )
            dst[i] = src[i];
        else
            break;

    // dst[i] points behind the string contents
    while ( i > 0 && dst[i-1] == ' ' )
        i--;

    dst[i] = '\0';
}

static unsigned long Read_LE_Uint32_unsigned ( const unsigned char* p )
{
    return ((unsigned long)p[0] <<  0) |
           ((unsigned long)p[1] <<  8) |
           ((unsigned long)p[2] << 16) |
           ((unsigned long)p[3] << 24);
}

static unsigned long Read_LE_Uint32 ( const char* p ) {return Read_LE_Uint32_unsigned((const unsigned char*)p);}

static int Lyrics3GetNumber5 ( const unsigned char* string )
{
	return ( string[0] - '0') * 10000 +
		   ( string[1] - '0') * 1000 +
		   ( string[2] - '0') * 100 +
		   ( string[3] - '0') * 10 +
		   ( string[4] - '0') * 1;
}

static int Lyrics3GetNumber6 ( const unsigned char* string )
{
	return ( string[0] - '0') * 100000 +
		   ( string[1] - '0') * 10000 +
		   ( string[2] - '0') * 1000 +
		   ( string[3] - '0') * 100 +
		   ( string[4] - '0') * 10 +
		   ( string[5] - '0') * 1;
}

static void insert_tag_field ( file_info *info, const char *field, const char *value )
{
    info->meta_set ( field, value );
}

static void text_buffer_from_ascii(char * p_buffer,const char * p_source,unsigned p_count)
{
	unsigned n = 0;
	while(n < p_count && p_source[n])
	{
		char c = p_source[n];
		if ((t_uint8)c >= 0x80) c = '?';
		p_buffer[n] = c;
		n++;
	}
	p_buffer[n] = 0;
}

static void ParseID3v1(const char p_buffer[128],file_info & p_info)
{
	char            value[32];
	text_buffer_from_ascii ( value, p_buffer +  3, 30 );
	if ( value[0] != '\0' ) p_info.meta_set(APE_TAG_FIELD_TITLE, value);
	text_buffer_from_ascii ( value, p_buffer + 33, 30 );
	if ( value[0] != '\0' )	p_info.meta_set(APE_TAG_FIELD_ARTIST, value);
	text_buffer_from_ascii ( value, p_buffer + 63, 30 );
	if ( value[0] != '\0' )	p_info.meta_set(APE_TAG_FIELD_ALBUM, value);
	text_buffer_from_ascii( value, p_buffer + 93,  4 );
	if ( value[0] != '\0' )	p_info.meta_set(APE_TAG_FIELD_YEAR, value);
	text_buffer_from_ascii ( value, p_buffer + 97, 30 );
	if ( value[0] != '\0' )	p_info.meta_set(APE_TAG_FIELD_COMMENT, value);
	if ( p_buffer[125] == 0 && p_buffer[126] != 0 ) {
		sprintf ( value, "%d", p_buffer[126] );
		if ( value[0] != '\0' ) p_info.meta_set(APE_TAG_FIELD_TRACK, value);
	}
	GenreToString ( value, p_buffer[127] );
	if ( value[0] != '\0' ) p_info.meta_set(APE_TAG_FIELD_GENRE, value);

	p_info.info_set("tagtype","id3v1");
}

// Reads ID3v1.0 / ID3v1.1 tag
static t_io_result ReadID3v1Tag ( const service_ptr_t<file> & fp, file_info * info, t_uint64 & tag_offset, abort_callback & p_abort)
{
    char   tmp [128];
	t_io_result status;

	if ( tag_offset < sizeof(tmp) ) return io_result_error_data;
	status = fp->seek (tag_offset - sizeof(tmp), p_abort);
	if (io_result_failed(status)) return status;
	status = fp->read_object (tmp, sizeof (tmp), p_abort );
	if (io_result_failed(status)) return status;

    if ( memcmp (tmp, "TAG", 3) ) return io_result_error_data;


	
	if (info)
	{
		ParseID3v1(tmp,*info);
	}

    tag_offset -= sizeof (tmp);

    return io_result_success;
}

struct Lyrics3TagFooterStruct {
    unsigned char   Length  [6];
    unsigned char   ID      [9];
};

struct Lyrics3TagField {
	unsigned char   ID      [3];
	unsigned char   Length  [5];
};

// Reads Lyrics3 v2.0 tag
static t_io_result ReadLyrics3v2Tag ( const service_ptr_t<file> & fp, file_info * info, t_uint64 & tag_offset, abort_callback & p_abort )
{
	int                                 len;
    size_t                              size;
	struct Lyrics3TagFooterStruct       T;
    struct Lyrics3TagField              F;
    unsigned char                       tmpid3[128];
    char                                tmp[11];
    char                                value[32];
    char*                               tagvalue;
	t_io_result status;

    status = fp->seek (tag_offset - 128, p_abort);
	if (io_result_failed(status)) return status;
	status = fp->read_object (tmpid3, 128, p_abort);
	if (io_result_failed(status)) return status;
    // check for id3-tag
    if ( memcmp (tmpid3, "TAG", 3) ) return io_result_error_data;
    status = fp->seek (tag_offset - 128 - sizeof (T), p_abort);
	if (io_result_failed(status)) return status;
    status = fp->read_object (&T, sizeof (T), p_abort);
	if (io_result_failed(status)) return status;
    // check for lyrics3 v2.00 tag
    if ( memcmp (T.ID, "LYRICS200", sizeof (T.ID)) ) return io_result_error_data;
	len = Lyrics3GetNumber6 (T.Length);
	status = fp->seek ( tag_offset - 128 - (int)sizeof (T) - len, p_abort );
	if (io_result_failed(status)) return status;
    status = fp->read_object (tmp, 11, p_abort);
	if (io_result_failed(status)) return status;

    if ( memcmp (tmp, "LYRICSBEGIN", 11) ) return io_result_error_data;

	len -= 11; // header 'LYRICSBEGIN'

	while ( len > 0 ) {
        status = fp->read_object (&F, 8, p_abort);
		if (io_result_failed(status)) return status;
        len -= 8;
        if ( (size = Lyrics3GetNumber5 (F.Length)) == 0 )
            continue;
        len -= size;
        if ( !(tagvalue = (char *)malloc (size + 1)) ) {
            return io_result_error_data;;
        }
        status = fp->read_object (tagvalue, size, p_abort);
		if (io_result_failed(status)) {
            free (tagvalue);
            return status;
        }
        tagvalue[size] = '\0';
        if ( tagvalue[0] == '\0' ) {
            free (tagvalue);
            continue;
        }

		if (info)
		{
			// Extended Title
			if ( !memcmp (F.ID, "ETT", 3) ) {
				if ( !memcmp (tagvalue, tmpid3 +  3, 30 ) ) {
					insert_tag_field (info, APE_TAG_FIELD_TITLE, string_utf8_from_ansi(tagvalue));
				}
			} else
			// Extended Artist
			if ( !memcmp (F.ID, "EAR", 3) ) {
				if ( !memcmp (tagvalue, tmpid3 + 33, 30 ) ) {
					insert_tag_field (info, APE_TAG_FIELD_ARTIST, string_utf8_from_ansi(tagvalue));
				}
			} else
			// Extended Album
			if ( !memcmp (F.ID, "EAL", 3) ) {
				if ( !memcmp (tagvalue, tmpid3 + 63, 30 ) ) {
					insert_tag_field (info, APE_TAG_FIELD_ALBUM, string_utf8_from_ansi(tagvalue));
				}
			} else
			// Additional information
			if ( !memcmp (F.ID, "INF", 3) ) {
				insert_tag_field (info, APE_TAG_FIELD_COMMENT, string_utf8_from_ansi(tagvalue));
			} else
			// Lyrics
			if ( !memcmp (F.ID, "LYR", 3) ) {
				insert_tag_field (info, APE_TAG_FIELD_LYRICS, string_utf8_from_ansi(tagvalue));
			}
		}

        free (tagvalue);
	}

	if (info)
	{
		memcpy_crop ( value, tmpid3 +  3, 30 );
		if ( value[0] != '\0' ) insert_tag_field (info, APE_TAG_FIELD_TITLE, string_utf8_from_ansi(value));
		memcpy_crop ( value, tmpid3 + 33, 30 );
		if ( value[0] != '\0' ) insert_tag_field (info, APE_TAG_FIELD_ARTIST, string_utf8_from_ansi(value));
		memcpy_crop ( value, tmpid3 + 63, 30 );
		if ( value[0] != '\0' ) insert_tag_field (info, APE_TAG_FIELD_ALBUM, string_utf8_from_ansi(value));
		memcpy_crop ( value, tmpid3 + 93,  4 );
		if ( value[0] != '\0' ) insert_tag_field (info, APE_TAG_FIELD_YEAR, string_utf8_from_ansi(value));
		memcpy_crop ( value, tmpid3 + 97, 30 );
		if ( value[0] != '\0' ) insert_tag_field (info, APE_TAG_FIELD_COMMENT, string_utf8_from_ansi(value));
		if ( tmpid3[125] == 0 && tmpid3[126] != 0 ) {
			sprintf ( value, "%d", tmpid3[126] );
			if ( value[0] != '\0' ) insert_tag_field (info, APE_TAG_FIELD_TRACK, string_utf8_from_ansi(value));
		}
		GenreToString ( value, tmpid3[127] );
		if ( value[0] != '\0' ) insert_tag_field (info, APE_TAG_FIELD_GENRE, string_utf8_from_ansi(value));

		info->info_set("tagtype","lyrics3");
	}

    tag_offset -= 128 + Lyrics3GetNumber6 (T.Length) + sizeof (T);

    return io_result_success;
}

struct APETagFooterStruct {
    char   ID       [8];
    char   Version  [4];
    char   Length   [4];
    char   TagCount [4];
    char   Flags    [4];
    char   Reserved [8];
};

/*
// Reads APE v1.0 tag
int ReadAPE1Tag ( const service_ptr_t<file> & fp, file_info * info,t_int64 &tag_offset )
{
    unsigned long               vsize;
    unsigned long               isize;
    unsigned long               flags;
    char*                       buff;
    char*                       p;
    char*                       end;
    struct APETagFooterStruct   T;
    unsigned long               TagLen;
    unsigned long               TagCount;

	if ( tag_offset < sizeof (T)) return 0;
	if ( !fp->seek (tag_offset - sizeof(T)) ) return 0;
    if ( fp->read (&T, sizeof(T)) != sizeof (T) ) return 0;
    if ( memcmp (T.ID, "APETAGEX", sizeof (T.ID) ) ) return 0;
    if ( Read_LE_Uint32 (T.Version) != 1000 ) return 0;
    if ( (TagLen = Read_LE_Uint32 (T.Length)) < sizeof T ) return 0;
	if ( tag_offset < (int)TagLen ) return 0;
    if ( !fp->seek (tag_offset - TagLen) ) return 0;
    if ( !(buff = (char *)malloc ( TagLen )) ) return 0;
    if ( fp->read (buff, (int)TagLen - sizeof (T)) != (int)(TagLen - sizeof(T)) ) {
        free (buff);
        return 0;
    }

    TagCount = Read_LE_Uint32 (T.TagCount);
    end = buff + TagLen - sizeof (T);
    for ( p = buff; p < end && TagCount--; ) {
        vsize = Read_LE_Uint32 (p); p += 4;
        flags = Read_LE_Uint32 (p); p += 4;
        isize = strlen (p);

        if ( isize > 0 && vsize > 0 ) {
			info->meta_set ( string8(p, isize), string_utf8_from_ansi(p+isize+1) );
		}
        p += isize + 1 + vsize;
    }

    free (buff);

    tag_offset -= TagLen;

    return 1;
}
*/

// Reads APE v1.0/2.0 tag
static t_io_result ReadAPETag ( const service_ptr_t<file> & fp, file_info * info, t_uint64 & tag_offset, abort_callback & p_abort )
{
	unsigned long               vsize;
	unsigned long               isize;
	unsigned long               flags;
	unsigned long				remaining;
	char*                       buff;
	char*                       p;
	char*                       end;
	struct APETagFooterStruct   T;
	unsigned long               TagLen;
	unsigned long               TagCount;
	unsigned long               Ver;

	t_io_result status;

	if ( tag_offset < sizeof (T) ) return io_result_error_data;
	
	status = fp->seek (tag_offset - sizeof (T), p_abort);
	if (io_result_failed(status)) return status;
	status = fp->read_object (&T, sizeof T, p_abort);
	if (io_result_failed(status)) return status;
	if ( memcmp (T.ID, "APETAGEX", sizeof (T.ID)) ) return io_result_error_data;
	Ver = Read_LE_Uint32 (T.Version);
	if ( (Ver != 1000) && (Ver != 2000) ) return io_result_error_data;
	if ( (TagLen = Read_LE_Uint32 (T.Length)) < sizeof (T) ) return io_result_error_data;

	if (info)
	{
		status = fp->seek (tag_offset - TagLen, p_abort);
		if (io_result_failed(status)) return status;

		if ( !(buff = (char *)malloc (TagLen)) ) return io_result_error_data;

		status = fp->read_object (buff, TagLen - sizeof (T), p_abort);
		if (io_result_failed(status)) {free(buff); return status; }

		bool b_warning = false;

		string8_fastalloc name;
		mem_block_fast_aggressive_t<char> value;

		TagCount = Read_LE_Uint32 (T.TagCount);
		end = buff + TagLen - sizeof (T);
		for ( p = buff; p < end && TagCount--; ) {
			if (end - p < 8) break;
			vsize = Read_LE_Uint32 (p); p += 4;
			flags = Read_LE_Uint32 (p); p += 4;

			remaining = (unsigned long)(end - p);
			
			isize = strlen_max (p,remaining);

			if (isize >= remaining || vsize > MAX_FIELD_SIZE || isize + 1 + vsize > remaining) break;//incorrect data

			
			name.set_string (p, isize);
			value.check_size(vsize+1);
			value.copy(p+isize+1,vsize);
			value[vsize]=0;

			bool is_info = false;
			bool is_rg = false;

			{
				unsigned z;
				for ( z=0; z<tabsize (ape2_field_translation); z++ ) {
					if ( !stricmp_ascii (name, ape2_field_translation[z].name_ape) ) {
						name = ape2_field_translation[z].name_global;
					}
				}
				for ( z=0; z<tabsize (rg_fields); z++ ) {
					if ( !stricmp_ascii (name, rg_fields[z]) ) {
						name.set_string(rg_fields[z]);
						is_rg = true;
						break;
					}
				}
				if (!is_rg)
				{
					for ( z=0; z<tabsize (other_info_fields); z++ ) {
						if ( !stricmp_ascii (name, other_info_fields[z]) ) {
							name.set_string(other_info_fields[z]);
							is_info = true;
							break;
						}
					}

				}
			}

			if ( isize > 0 && vsize > 0 ) {
				if (is_info) {
					if (is_lower_ascii(name) && is_lower_ascii(value))
						info->info_set ( name, value );
					else
						b_warning = true;
				} else if (is_rg) {
					if (is_lower_ascii(name) && is_lower_ascii(value))
						info->info_set_replaygain ( name, value );
					else
						b_warning = true;
									
				} else {
					if ( Ver == 2000 ) {
						if ( !(flags & 1<<1) ) {    // insert UTF-8 string
							if (!is_valid_utf8(name))
								b_warning = true;
							else
							{
								if (!is_valid_utf8(value))
									b_warning = true;
								else
								{
									unsigned meta_index = info->meta_set ( name, value );

									const char *p = value;
									const char *pe = p + vsize;

									for(;;) {
										p += strlen_max(p,pe-p) + 1;
										if (p>=pe) break;
										if (is_valid_utf8(p))
											info->meta_add_value ( meta_index, p );
										else
										{
											b_warning = true;
											break;
										}
									}
								}
							}
						} else {                    // insert binary data
							if (is_lower_ascii(name))
								info->meta_set ( name, string_utf8_from_ansi(value) );
							else
								b_warning = true;
						}
					} else {
						if (is_lower_ascii(name))
							info->meta_set ( name, string_utf8_from_ansi(value) );
						else
							b_warning = true;
					}
				}
			}
			p += isize + 1 + vsize;
		}

		free (buff);

		if (b_warning)
		{
			console::warning("invalid characters in APE tag");
//			console::info_location(info->get_location());
		}

		info->info_set("tagtype",Ver==1000 ? "apev1" : "apev2");
	}

	tag_offset -= TagLen;

	if ( Read_LE_Uint32 (T.Flags) & (1<<31) )   // Tag contains header
		tag_offset -= sizeof (T);

	return io_result_success;
}



static void Write_LE_Uint32 ( char* p, const unsigned long value )
{
    p[0] = (unsigned char) (value >>  0);
    p[1] = (unsigned char) (value >>  8);
    p[2] = (unsigned char) (value >> 16);
    p[3] = (unsigned char) (value >> 24);
}

static void apev2_append_tag(mem_block_t<char> & tag_data,const char * p_name,const char * p_value,unsigned value_size)
{
	char temp[4];
	Write_LE_Uint32(temp, value_size );
	tag_data.append(temp, 4);
	Write_LE_Uint32(temp, 0);
	tag_data.append(temp, 4);
	tag_data.append(p_name, strlen(p_name)+1);
	tag_data.append(p_value, value_size);
}

static void apev2_append_tag(mem_block_t<char> & tag_data,const char * p_name,const char * p_value)
{
	apev2_append_tag(tag_data,p_name,p_value,strlen(p_value));
}

// Writes APE v2.0 tag
//seek to tag offset before calling
static t_io_result WriteAPE2Tag ( const service_ptr_t<file> & fp, const file_info * info, abort_callback & p_abort )
{
    char                        *buff;
    char                        *p;
    struct APETagFooterStruct   T;
    unsigned int                flags;
    int                         TagCount;
    int                         TagSize;
    unsigned                    i;

	mem_block_t<char> tag_data;
    TagCount = 0;

    TagSize = sizeof(T);                                // calculate size of buffer needed

	bool b_warning = false;

    for ( i = 0; i < info->meta_get_count() ; i++ ) {
		const char * name = info->meta_enum_name(i);

        {
            unsigned z;
            bool changed = false;

            for ( z = 0; z < tabsize(ape2_field_translation); z++ ) {
                if ( !stricmp_utf8(name, ape2_field_translation[z].name_global) ) {
                    name = ape2_field_translation[z].name_ape;
                    changed = true;
                    break;
                }
            }

            if ( !changed ) {
                for ( z = 0; z < sizeof(standard_ape2_field)/sizeof(*standard_ape2_field); z++ ) {
                    if ( !stricmp_utf8(name, standard_ape2_field[z]) ) {
                        name = standard_ape2_field[z];
                        break;
                    }
                }
            }
        }

        mem_block_t<char> value;

		{
			unsigned n, m = info->meta_enum_value_count(i);
			for(n=0;n<m;n++)
			{
				const char * val = info->meta_enum_value(i,n);
				if (!is_valid_utf8(val)) {b_warning = true;break;}
				if (n > 0) value.append("\0", 1);
				value.append(val, strlen(val));
			}
		}

		apev2_append_tag(tag_data,name,value,value.get_size());
		TagCount++;
    }

	for ( i = 0; i < tabsize(other_info_fields); i++ ) {
		const char * name = other_info_fields[i];
		const char *value = info->info_get(name);
		if (value) {
			apev2_append_tag(tag_data,name,string_ascii_from_utf8(value));
			TagCount++;
		}
	}

	{
		replaygain_info rg = info->get_replaygain();
		char rgtemp[replaygain_info::text_buffer_size];
		if (rg.is_album_gain_present())
		{
			rg.format_album_gain(rgtemp);
			apev2_append_tag(tag_data,"replaygain_album_gain",rgtemp);
			TagCount++;
		}
		if (rg.is_album_peak_present())
		{
			rg.format_album_peak(rgtemp);
			apev2_append_tag(tag_data,"replaygain_album_peak",rgtemp);
			TagCount++;
		}
		if (rg.is_track_gain_present())
		{
			rg.format_track_gain(rgtemp);
			apev2_append_tag(tag_data,"replaygain_track_gain",rgtemp);
			TagCount++;
		}
		if (rg.is_track_peak_present())
		{
			rg.format_track_peak(rgtemp);
			apev2_append_tag(tag_data,"replaygain_track_peak",rgtemp);
			TagCount++;
		}
	}



	if (b_warning)
	{
		console::warning("Invalid UTF-8 characters encountered when writing APEv2 tag.");
//		console::info_location(info->get_location());
	}

#if 0 //deprecated, calling code now decides
	if (TagCount == 0) {
		t_io_result status = fp->set_eof(p_abort) ;
		if (io_result_failed(status)) return status;
		return io_result_success;
	}
#endif

	TagSize += tag_data.get_size();

    if ( !(buff = (char*)malloc(TagSize + sizeof (T))) ) return io_result_error_generic;

    p = buff;

    flags  = 1<<31;                                     // contains header
    flags |= 1<<29;                                     // this is the header
    memcpy(T.ID, "APETAGEX", sizeof(T.ID));             // ID String
    Write_LE_Uint32(T.Version, 2000);                   // Version 2.000
    Write_LE_Uint32(T.Length, TagSize);                 // Tag size
    Write_LE_Uint32(T.TagCount, TagCount);              // Number of fields
    Write_LE_Uint32(T.Flags, flags);                    // Flags
    memset(T.Reserved, 0, sizeof(T.Reserved));          // Reserved
    memcpy(p, &T, sizeof(T)); p += sizeof(T);           // insert header

	memcpy(p,tag_data.get_ptr(), tag_data.get_size());
	p += tag_data.get_size();

    flags  = 1<<31;                                     // contains header
    memcpy(T.ID, "APETAGEX", sizeof(T.ID));             // ID String
    Write_LE_Uint32(T.Version, 2000);                   // Version 2.000
    Write_LE_Uint32(T.Length, TagSize);                 // Tag size - header
    Write_LE_Uint32(T.TagCount, TagCount);              // Number of fields
    Write_LE_Uint32(T.Flags, flags);                    // Flags
    memset(T.Reserved, 0, sizeof(T.Reserved));          // Reserved
    memcpy(p, &T, sizeof(T));                           // insert footer

	t_io_result status = fp->write_object (buff, TagSize + sizeof(T), p_abort) ;
	free(buff);
	if (io_result_failed( status ) ) return status;

	status = fp->set_eof(p_abort) ;
	if (io_result_failed( status ) ) return status;

    return io_result_success;
}

static char GenreToInteger ( const char *GenreStr )
{
    static const char*  ID3v1GenreList[] = {
        "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk",
        "Grunge", "Hip-Hop", "Jazz", "Metal", "New Age", "Oldies",
        "Other", "Pop", "R&B", "Rap", "Reggae", "Rock",
        "Techno", "Industrial", "Alternative", "Ska", "Death Metal", "Pranks",
        "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk",
        "Fusion", "Trance", "Classical", "Instrumental", "Acid", "House",
        "Game", "Sound Clip", "Gospel", "Noise", "AlternRock", "Bass",
        "Soul", "Punk", "Space", "Meditative", "Instrumental Pop", "Instrumental Rock",
        "Ethnic", "Gothic", "Darkwave", "Techno-Industrial", "Electronic", "Pop-Folk",
        "Eurodance", "Dream", "Southern Rock", "Comedy", "Cult", "Gangsta",
        "Top 40", "Christian Rap", "Pop/Funk", "Jungle", "Native American", "Cabaret",
        "New Wave", "Psychadelic", "Rave", "Showtunes", "Trailer", "Lo-Fi",
        "Tribal", "Acid Punk", "Acid Jazz", "Polka", "Retro", "Musical",
        "Rock & Roll", "Hard Rock", "Folk", "Folk/Rock", "National Folk", "Swing",
        "Fast-Fusion", "Bebob", "Latin", "Revival", "Celtic", "Bluegrass", "Avantgarde",
        "Gothic Rock", "Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock", "Big Band",
        "Chorus", "Easy Listening", "Acoustic", "Humour", "Speech", "Chanson",
        "Opera", "Chamber Music", "Sonata", "Symphony", "Booty Bass", "Primus",
        "Porn Groove", "Satire", "Slow Jam", "Club", "Tango", "Samba",
        "Folklore", "Ballad", "Power Ballad", "Rhythmic Soul", "Freestyle", "Duet",
        "Punk Rock", "Drum Solo", "A capella", "Euro-House", "Dance Hall",
        "Goa", "Drum & Bass", "Club House", "Hardcore", "Terror",
        "Indie", "BritPop", "NegerPunk", "Polsk Punk", "Beat",
        "Christian Gangsta", "Heavy Metal", "Black Metal", "Crossover", "Contemporary C",
        "Christian Rock", "Merengue", "Salsa", "Thrash Metal", "Anime", "JPop",
        "SynthPop",
    };

    for ( size_t i = 0; i < sizeof (ID3v1GenreList) / sizeof (*ID3v1GenreList); i++ ) {
        if ( 0 == stricmp_utf8 ( GenreStr, ID3v1GenreList[i] ) )
            return (char)(unsigned char)i;
    }

    return (char)255;
}

static void utf8_to_ascii_buffer(char * p_buffer,unsigned p_size,const char * p_source)
{
	unsigned n = 0;
	while(n<p_size && p_source[n])
	{
		char c = p_source[n];
		if ((t_uint8)c >= 0x80) c = '?';
		p_buffer[n++] = c;
	}
	while(n<p_size) p_buffer[n++] = 0;
}

static void BuildID3v1( char p_buffer[128], const file_info & p_info)
{
    memset ( p_buffer, 0, sizeof (p_buffer) );
    strncpy( p_buffer , "TAG", 3 );
    if (p_info.meta_get (APE_TAG_FIELD_TITLE,0)) {
        utf8_to_ascii_buffer( p_buffer+ 3, 30, p_info.meta_get (APE_TAG_FIELD_TITLE,0) );
    }
    if (p_info.meta_get (APE_TAG_FIELD_ARTIST,0)) {
        utf8_to_ascii_buffer( p_buffer+33, 30, p_info.meta_get (APE_TAG_FIELD_ARTIST,0) );
    }
    if (p_info.meta_get (APE_TAG_FIELD_ALBUM,0)) {
        utf8_to_ascii_buffer( p_buffer+63, 30, p_info.meta_get (APE_TAG_FIELD_ALBUM,0) );
    }
    if (p_info.meta_get (APE_TAG_FIELD_YEAR,0)) {
        utf8_to_ascii_buffer ( p_buffer+93, 4, p_info.meta_get (APE_TAG_FIELD_YEAR,0) );
    }

    // if track# is used, write ID3v1.1 format
    if ( !p_info.meta_get (APE_TAG_FIELD_TRACK,0) ) {
        if (p_info.meta_get (APE_TAG_FIELD_COMMENT,0)) {
            utf8_to_ascii_buffer ( p_buffer+97, 30, p_info.meta_get (APE_TAG_FIELD_COMMENT,0) );
        }
    } else {
        if (p_info.meta_get (APE_TAG_FIELD_COMMENT,0)) {
            utf8_to_ascii_buffer ( p_buffer+97, 28, p_info.meta_get (APE_TAG_FIELD_COMMENT,0) );
        }
        p_buffer[126] = (char)(unsigned char)atoi (p_info.meta_get (APE_TAG_FIELD_TRACK,0) );
    }

    if ( p_info.meta_get (APE_TAG_FIELD_GENRE,0) ) {
        p_buffer[127] = GenreToInteger ( p_info.meta_get (APE_TAG_FIELD_GENRE,0) );
    } else {
        p_buffer[127] = (char)(unsigned char)255;
    }
}

// Writes ID3v1.1 tag
//seek to tag offset before calling

static t_io_result WriteID3v1Tag ( const service_ptr_t<file> & fp, const file_info * info, abort_callback & p_abort)
{
    char    tmp[128];

	BuildID3v1(tmp,*info);
	t_io_result status;
	status = fp->write_object (tmp, sizeof (tmp), p_abort);
	if (io_result_failed(status)) return status;

    return fp->set_eof(p_abort);
}


static t_io_result find_tag_offset(const service_ptr_t<file> & fp, abort_callback & p_abort,t_uint64 & p_offset)
{
	t_io_result status;
	t_filesize tag_offset;
	t_filesize offs_bk;

	status = fp->get_size(tag_offset,p_abort);
	if (io_result_failed(status)) return status;
	
	status = fp->seek(tag_offset,p_abort);
	if (io_result_failed(status)) return status;

	do {
		offs_bk = tag_offset;
		status = ReadAPETag ( fp, 0, tag_offset, p_abort );
		if (status != io_result_error_data && io_result_failed(status) ) return status;
        status = ReadLyrics3v2Tag ( fp, 0, tag_offset, p_abort);
		if (status != io_result_error_data && io_result_failed(status) ) return status;
		status = ReadID3v1Tag ( fp, 0, tag_offset, p_abort);
		if (status != io_result_error_data && io_result_failed(status) ) return status;
	} while ( offs_bk != tag_offset );

	p_offset = tag_offset;

	return io_result_success;
}

enum tagtype
{
	tag_ape,tag_id3,tag_lyrics,
};

namespace {
	class tag_processor_trailing_impl : public tag_processor_trailing
	{
	public:
/*		enum {
			flag_apev2 = 1,
			flag_id3v1 = 2,
		};*/

		bool is_tag_empty_apev2(const file_info & p_info)
		{
			if (p_info.meta_get_count() > 0) return false;
			if (p_info.get_replaygain().get_value_count() > 0) return false;

			unsigned n, m = tabsize(other_info_fields);
			for(n=0;n<m;n++)
			{
				if (p_info.info_get(other_info_fields[n]) != 0) return false;
			}

			return true;
		}

		bool is_tag_empty_id3v1(const file_info & p_info)
		{
			if ( p_info.meta_get (APE_TAG_FIELD_TITLE,0) != 0 ) return false;
			if ( p_info.meta_get (APE_TAG_FIELD_ARTIST,0) != 0 ) return false;
			if ( p_info.meta_get (APE_TAG_FIELD_ALBUM,0) != 0) return false;
			if ( p_info.meta_get (APE_TAG_FIELD_YEAR,0) != 0 ) return false;
			if ( p_info.meta_get (APE_TAG_FIELD_TRACK,0) != 0 ) return false;
			if ( p_info.meta_get (APE_TAG_FIELD_COMMENT,0) != 0 ) return false;
			if ( p_info.meta_get (APE_TAG_FIELD_GENRE,0) != 0 ) return false;
			return true;
		}


		t_io_result read(const service_ptr_t<file> & p_file,file_info & p_info,abort_callback & p_abort)
		{
			t_io_result status;
			t_filesize tag_offset;
			t_filesize offs_bk;
			
			status = p_file->get_size(tag_offset,p_abort);
			if (io_result_failed(status)) return status;

			status = p_file->seek(tag_offset, p_abort);
			if (io_result_failed(status)) return status;
			
			file_info_impl infos[3];
			ptr_list_t<const file_info> infos_done;
			

			int retval = 0;

			do {
				offs_bk = tag_offset;
				
				if (!infos_done.have_item(&infos[0]))
				{
					status = ReadAPETag ( p_file, &infos[0], tag_offset, p_abort) ;
					if (status != io_result_error_data && io_result_failed(status) ) return status;
					if (io_result_succeeded(status)) infos_done.add_item(&infos[0]);
				}
				if (!infos_done.have_item(&infos[1]))
				{
					status = ReadLyrics3v2Tag ( p_file, &infos[1], tag_offset, p_abort );
					if (status != io_result_error_data && io_result_failed(status) ) return status;
					if (io_result_succeeded(status)) infos_done.add_item(&infos[1]);
				}
				if (!infos_done.have_item(&infos[2]))
				{
					status = ReadID3v1Tag ( p_file, &infos[2], tag_offset, p_abort );
					if (status != io_result_error_data && io_result_failed(status) ) return status;
					if (io_result_succeeded(status)) infos_done.add_item(&infos[2]);
				}
			} while ( offs_bk != tag_offset );

			if (infos_done.get_count() == 0) return io_result_error_not_found;

			{
				//keep tagtype order proper....
				unsigned n, t = infos_done.get_count(), m = t/2;
				for(n=0;n<m;n++)
					infos_done.swap_items(n, t - 1 - n);
			}

			p_info.merge(infos_done);

			return io_result_success;
		}
		
		t_io_result write(const service_ptr_t<file> & p_file,const file_info & p_info,unsigned p_flags,abort_callback & p_abort)
		{
			t_io_result status;
			status = remove(p_file,p_abort);
			if (io_result_failed(status)) return status;

			status = p_file->seek2(0,SEEK_END,p_abort);
			if (io_result_failed(status)) return status;

			if (p_flags & flag_apev2)
			{
				status = WriteAPE2Tag ( p_file, &p_info, p_abort );
				if (io_result_failed(status)) return status;
			}
			if (p_flags & flag_id3v1)
			{
				status = WriteID3v1Tag ( p_file, &p_info, p_abort );
				if (io_result_failed(status)) return status;
			}

			return io_result_success;
		}

		t_io_result remove(const service_ptr_t<file> & p_file,abort_callback & p_abort)
		{
			t_uint64 offset;
			t_io_result status = find_tag_offset(p_file,p_abort,offset);
			if (io_result_failed(status)) return status;
			status = p_file->seek(offset,p_abort);
			if (io_result_failed(status)) return status;
			status = p_file->set_eof(p_abort);
			if (io_result_failed(status)) return status;
			status = p_file->seek(0,p_abort);
			if (io_result_failed(status)) return status;
			return io_result_success;
		}
		
		static bool check_id3v1_field(const char * p_field,unsigned p_length)
		{
			unsigned ptr = 0;
			if (p_field[0] == 0) return false;//id3v1 can't store "empty" fields
			while(p_field[ptr])
			{
				if (ptr >= p_length) return false;
				if ((t_uint8)p_field[ptr] >= 0x80) return false;
				ptr++;
			}
			return true;
		}

		bool is_id3v1_sufficient(const file_info & p_info)
		{
			if (p_info.get_replaygain().get_value_count() > 0) return false;
			
			unsigned n; const unsigned m = p_info.meta_get_count();
			for(n=0;n<m;n++)
			{
				if (p_info.meta_enum_value_count(n) != 1) return false;
				const char * name = p_info.meta_enum_name(n),
					* value = p_info.meta_enum_value(n,0);
				if (!stricmp_utf8(name,APE_TAG_FIELD_TITLE))
				{
					if (!check_id3v1_field(value,30)) return false;
				}
				else if (!stricmp_utf8(name,APE_TAG_FIELD_ARTIST))
				{
					if (!check_id3v1_field(value,30)) return false;
				}
				else if (!stricmp_utf8(name,APE_TAG_FIELD_ALBUM))
				{
					if (!check_id3v1_field(value,30)) return false;
				}
				else if (!stricmp_utf8(name,APE_TAG_FIELD_YEAR))
				{
					if (!check_id3v1_field(value,4)) return false;
				}
				else if (!stricmp_utf8(name,APE_TAG_FIELD_COMMENT))
				{
					if (!check_id3v1_field(value,28)) return false;
				}
				else if (!stricmp_utf8(name,APE_TAG_FIELD_TRACK))
				{
					char temp[32];
					unsigned val = atoi(value);
					if (val > 255) return false;
					itoa(val,temp,10);
					if (strcmp(temp,value)) return false;					
				}
				else if (!stricmp_utf8(name,APE_TAG_FIELD_GENRE))
				{
					unsigned n;
					bool found = false;
					for(n=0;n<tabsize(ID3v1GenreList);n++)
					{
						if (!stricmp_utf8(value,ID3v1GenreList[n])) {found = true; break;}
					}
					if (!found) return false;
				}
				else return false;
			}
			return true;
		}


		void truncate_to_id3v1(file_info & p_info)
		{
			char tag[128];
			BuildID3v1(tag,p_info);
			p_info.meta_remove_all();
			p_info.reset_replaygain();
			ParseID3v1(tag,p_info);
		}
	};

	static service_factory_single_t<tag_processor_trailing,tag_processor_trailing_impl> g_tag_processor_trailing_impl_factory;
}