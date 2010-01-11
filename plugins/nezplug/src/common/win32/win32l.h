#ifndef WIN32LEAST_H_
#define WIN32LEAST_H_

#define STRICT
#define WIN32_LEAN_AND_MEAN
/* WIN32 least */
#define NONLS
#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
/* #define NOKERNEL */
#define NOUSER
#define NONLS
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX
/* MMSYSTEM least */
#define MMNODRV
#define MMNOSOUND
#define MMNOWAVE
#define MMNOMIDI
#define MMNOAUX
#define MMNOMIXER
#define MMNOTIMER
#define MMNOJOY
#define MMNOMCI
#define MMNOMMIO
#define MMNOMMSYSTEM
/* COMMCTRL least */
#define NOTOOLBAR
#define NOUPDOWN
#define NOSTATUSBAR
#define NOMENUHELP
#define NOTRACKBAR
#define NODRAGLIST
#define NOPROGRESS
#define NOHOTKEY
#define NOHEADER
#define NOIMAGEAPIS
#define NOLISTVIEW
#define NOTREEVIEW
#define NOTABCONTROL
#define NOANIMATE


void *GetDLLInstance(void);
#endif /* WIN32LEAST_H_ */
