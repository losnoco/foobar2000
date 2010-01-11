/* platform depend data */
typedef struct {
#ifdef _WIN32
	void *hwnd;
#else
#endif
} SOUNDDEVICEPDI;

typedef struct {
	unsigned bit;
	unsigned ch;
	unsigned freq;
	/* callback */
	void *lpargs;
	void (*Write)(void *lpargs, void *lpbuf, unsigned len);
	void (*Term)(void *lpargs);
	SOUNDDEVICEPDI *ppdi;		/* platform depend data */
} SOUNDDEVICEINITDATA;

typedef struct SOUNDDEVICE_TAG {
	void *lpSystemData;
	SOUNDDEVICEINITDATA sdid;
	void (*Term)(struct SOUNDDEVICE_TAG *psd);
	unsigned (*IsPause)(struct SOUNDDEVICE_TAG *psd);
	void (*Pause)(struct SOUNDDEVICE_TAG *psd, unsigned isPause);
} SOUNDDEVICE;

SOUNDDEVICE *CreateSoundDevice(SOUNDDEVICEINITDATA *psdid);
