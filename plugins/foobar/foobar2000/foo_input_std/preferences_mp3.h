extern cfg_int cfg_mp3_notagaction,cfg_mp3_id3v1action,cfg_mp3_forcetag,cfg_mp3_forcetagtype;

enum
{
	mp3_tagtype_id3v1 = 0,
	mp3_tagtype_apev2,
	mp3_tagtype_apev2_id3v1,
	mp3_tagtype_id3v2,
	mp3_tagtype_id3v2_id3v1
};
