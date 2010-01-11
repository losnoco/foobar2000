#ifndef __FESTAH_FESTALONH
#define __FESTAH_FESTALONH

typedef struct {
           int disabled;
           int soundq;
           int lowpass;
} FCEUS;

typedef struct {
        char *GameName, *Artist, *Copyright, *Ripper;

        char **SongNames;
        int32 *SongLengths;
        int32 *SongFades;

		int PlaylistSize;
		uint8 *Playlist;

        int TotalSongs;
        int StartingSong;
	int CurrentSong;
        int TotalChannels;
        int VideoSystem;

	int OutChannels;
        FCEUS FSettings;

	void *nsf;
	void *hes;
} FESTALON;

uint32 uppow2(uint32 n);
char *FESTA_FixString(char *);

void *FESTA_malloc(uint32 align, uint32 total);
void FESTA_free(void *m);

#endif
