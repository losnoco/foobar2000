//IDs for TFM tags
#define G_ID_NULL 0
#define G_ID_ARTIST 1
#define G_ID_ALBUM 2
#define G_ID_DATE 3
#define G_ID_COMMENT 4
#define G_ID_COMPAT 5 //single byte
#define G_ID_TITLE 6

//flags for G_ID_COMPAT
#define G_COMPAT_DEFAULT 0
#define G_COMPAT_OLD 1//Turrican 1
#define G_COMPAT_EMOO 2//emulate othet TFMX players
#define G_COMPAT_NEW 3 //Turrican 3

#define S_ID_NULL 0
#define S_ID_TITLE 1
#define S_ID_MUTEMASK 2 //single byte, not string