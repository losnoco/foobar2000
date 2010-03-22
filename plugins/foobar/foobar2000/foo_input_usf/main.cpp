#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include "usf.h"

int InitalizeApplication ( void )
{

	return 1;
}

void Cpu::StopEmulation(void)
{
	//_asm int 3
	//printf("Arrivederci!\n\n");
	//Release_Memory();
}

void DisplayError (char * Message, ...)
{
	/*char Msg[1000];
	va_list ap;

	va_start( ap, Message );
	vsprintf( Msg, Message, ap );
	va_end( ap );

	printf("Error: %s\n", Msg);
	*/
}

void UsfSleep(int32_t time)
{
	Sleep(time);
}

void * GetAddress(int32_t num, ...)
{
	va_list args;
	void * addr;

	va_start(args, num);
	addr = va_arg(args, void *);
	va_end(args);

	return addr;
}

