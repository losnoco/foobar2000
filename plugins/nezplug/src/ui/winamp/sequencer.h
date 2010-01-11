/*
	KUMAamp project(âºèÃ)
	Copyright (C) Mamiya 2000.
	---------------------------------------------------------------------------
	generic sequencer structure
	---------------------------------------------------------------------------
	$Id: sequencer.h,v 0.7.7 2000/05/07 Mamiya Exp $
*/

#ifdef __cplusplus
extern "C" {
#endif
typedef struct SEQUENCER_TAG {
	void (*Play)(struct SEQUENCER_TAG *this__);
#if !N_VERSION
	void (*Mix)(struct SEQUENCER_TAG *this__, void *buf, int numsamp);
	int (*GetRate)(struct SEQUENCER_TAG *this__);
	int (*GetChannel)(struct SEQUENCER_TAG *this__);
#else
	void (*Pause)(struct SEQUENCER_TAG *this__, int ispause);
	void (*SetVolume)(struct SEQUENCER_TAG *this__, int volume);
	void (*SetPan)(struct SEQUENCER_TAG *this__, int panpot);
#endif
	void (*Stop)(struct SEQUENCER_TAG *this__);
	void (*Term)(struct SEQUENCER_TAG *this__);
	void (*SetPosition)(struct SEQUENCER_TAG *this__, int time_in_ms);
	int (*GetPosition)(struct SEQUENCER_TAG *this__);
	int (*GetPriority)(struct SEQUENCER_TAG *this__);
	int (*GetLoopCount)(struct SEQUENCER_TAG *this__);
	int (*IsPlaying)(struct SEQUENCER_TAG *this__);
	void *work;
} SEQUENCER;

#ifdef WIN32
void InitSequencer(HINSTANCE hDllInstance);
void QuitSequencer(HINSTANCE hDllInstance);
void AboutSequencer(HWND hwndParent);
void ConfigSequencer(HWND hwndParent);
int infoBox(char *fn, HWND hwndParent);
#else
void InitSequencer(void);
void QuitSequencer(void);
void AboutSequencer(void);
void ConfigSequencer(void);
int infoBox(char *fn);
#endif

int getFileInfo(char *fn, char *title, int *length_in_ms);
int isOurFile(char *fn);
SEQUENCER *loadFile(char *fn);
char *StartWinamp(void);

#ifdef __cplusplus
}
#endif
