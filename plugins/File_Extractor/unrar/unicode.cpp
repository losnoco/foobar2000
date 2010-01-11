#include "rar.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#if defined(_EMX) && !defined(_DJGPP)
#include "unios2.cpp"
#endif

bool CharToWide(const char *Src,wchar_t *Dest,int DestSize)
{
  bool RetCode=true;
#ifdef _WIN32
  if (MultiByteToWideChar(CP_ACP,0,Src,-1,Dest,DestSize)==0)
    RetCode=false;
#else
#ifdef _APPLE
  UtfToWide(Src,Dest,DestSize);
#else
#ifdef MBFUNCTIONS

  size_t ResultingSize=mbstowcs(Dest,Src,DestSize);
  if (ResultingSize==(size_t)-1)
    RetCode=false;
  if (ResultingSize==0 && *Src!=0)
    RetCode=false;

  if ((!RetCode || *Dest==0 && *Src!=0) && DestSize>NM && strlen(Src)<NM)
  {
    /* Workaround for strange Linux Unicode functions bug.
       Some of wcstombs and mbstowcs implementations in some situations
       (we are yet to find out what it depends on) can return an empty
       string and success code if buffer size value is too large.
    */
    return(CharToWide(Src,Dest,NM));
  }
#else
  if (UnicodeEnabled())
  {
#if defined(_EMX) && !defined(_DJGPP)
    int len=Min(strlen(Src)+1,DestSize-1);
    if (uni_toucs((char*)Src,len,(UniChar*)Dest,(size_t*)&DestSize)==-1 ||
        DestSize>len)
      DestSize=0;
    RetCode=false;
#endif
  }
  else
    for (int I=0;I<DestSize;I++)
    {
      Dest[I]=(wchar_t)Src[I];
      if (Src[I]==0)
        break;
    }
#endif
#endif
#endif
  return(RetCode);
}

void WideToUtf(const wchar_t *Src,char *Dest,int DestSize)
{
  DestSize--;
  while (*Src!=0 && --DestSize>=0)
  {
    uint c=*(Src++);
	if (c >= 0xD800 && c <= 0xDBFF &&
		*Src >= 0xDC00 && *Src <= 0xDFFF)
	{
		c = (( c & 0x3FF ) << 10) |
			(*Src & 0x3FF);
		Src++;
	}
    if (c<0x80 && --DestSize>=0)
      *(Dest++)=c;
    else
      if (c<0x800 && (DestSize-=2)>=0)
      {
        *(Dest++)=(0xc0|(c>>6));
        *(Dest++)=(0x80|(c&0x3f));
      }
      else
        if (c<0x10000 && (DestSize-=3)>=0)
        {
          *(Dest++)=(0xe0|(c>>12));
          *(Dest++)=(0x80|((c>>6)&0x3f));
          *(Dest++)=(0x80|(c&0x3f));
        }
        else
          if (c < 0x200000 && (DestSize-=4)>=0)
          {
            *(Dest++)=(0xf0|(c>>18));
            *(Dest++)=(0x80|((c>>12)&0x3f));
            *(Dest++)=(0x80|((c>>6)&0x3f));
            *(Dest++)=(0x80|(c&0x3f));
          }
          else
            if (c < 0x4000000 && (DestSize-=5)>=0)
            {
              *(Dest++)=(0xf8|(c>>24));
              *(Dest++)=(0x80|((c>>18)&0x3f));
              *(Dest++)=(0x80|((c>>12)&0x3f));
              *(Dest++)=(0x80|((c>>6)&0x3f));
              *(Dest++)=(0x80|(c&0x3f));
            }
            else
              if (c <= 0x7fffffff && (DestSize-=6)>=0)
              {
                *(Dest++)=(0xFC|(c>>30));
                *(Dest++)=(0x80|((c>>24)&0x3f));
                *(Dest++)=(0x80|((c>>18)&0x3f));
                *(Dest++)=(0x80|((c>>12)&0x3f));
                *(Dest++)=(0x80|((c>>6)&0x3f));
                *(Dest++)=(0x80|(c&0x3f));
              }
  }
  *Dest=0;
}
