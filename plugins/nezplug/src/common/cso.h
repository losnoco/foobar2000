#ifndef CSO_H__
#define CSO_H__
/* non-interlock critical section object */
#if 0
int sample()
{
	static CRITICAL_SECTION_OBJECT(var);
	if (CRITICAL_SECTION_ENTER(var))
	{
		/* +--------------------+ */
		/* |  critical section  | */
		/* +--------------------+ */
		CRITICAL_SECTION_LEAVE(var);
	}
}
int sample2()
{
	static CRITICAL_SECTION_OBJECT(var);
	while (!CRITICAL_SECTION_ENTER(var)) sleep();
	/* +--------------------+ */
	/* |  critical section  | */
	/* +--------------------+ */
	CRITICAL_SECTION_LEAVE(var);
}
#endif

/* critical section object (WIN32) */
unsigned CriticalSectionEnter(void *a);
void CriticalSectionLeave(void *a);
void CriticalSectionInitialize(void *a);
#define CRITICAL_SECTION_OBJECT(a)	long a = 1
#define CRITICAL_SECTION_MEMBER(a)	long a
#define CRITICAL_SECTION_ENTER(a)	CriticalSectionEnter(&a)
#define CRITICAL_SECTION_LEAVE(a)	CriticalSectionLeave(&a)
#define CRITICAL_SECTION_INIT(a)	CriticalSectionInitialize(&a);

#endif /* CSO_H__ */
