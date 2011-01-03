#include <stdio.h>
#include <string.h>

int open_error(const char *filename, const char *mode)
{
 printf("Failure opening %s for %s\n");
 return 1;
}

int main(int argc, char **argv)
{
 int c;
 int section_count;
 /*int vsize;*/
 FILE *in, *out;
 unsigned char section_header[40];

 if (argc < 2 || argc > 3)
 {
  printf("Fixes MS Win32 object files to be compatible with the incorrect\n");
  printf(" implementation in MinGW32.\n");
  printf("Usage: objfix infile [outfile]\n");
  return 1;
 }

 in = fopen(argv[1], (argc == 2 ? "rb+" : "rb"));
 if (!in) return open_error(argv[1], (argc == 2 ? "read" : "update"));

 if (argc == 3)
 {
  out = fopen(argv[2], "wb");
  if (!out) return open_error(argv[1], (argc == 2 ? "read" : "update"));
 }
 else
 {
  out = NULL;
 }

 if (out)
 {
  fputc(fgetc(in), out);
  fputc(fgetc(in), out);

  fputc(section_count = fgetc(in), out);
  fputc(c = fgetc(in), out);
  section_count += c << 8;

  for (c = 4; c < 0x14; c++)
  {
   fputc(fgetc(in), out);
  }

  /*vsize = 0;*/

  for (c = 0; c < section_count; c++)
  {
   fread(section_header, 1, 40, in);

   if (*(long*)(section_header + 8))
   {
    memcpy(section_header + 16, section_header + 8, 4);
    *(long*)(section_header + 8) = 0;
   }
   if (section_header[39] & 0x20)
   {
    section_header[39] &= 0x7f;
   }
   /*memcpy(section_header + 8, &vsize, 4);
   vsize += *(long *)(section_header + 16);*/

   fwrite(section_header, 1, 40, out);
  }

  while ((c = fgetc(in)) != EOF)
  {
   fputc(c, out);
  }
 }
 else
 {
  fgetc(in);
  fgetc(in);

  section_count = fgetc(in);
  section_count += fgetc(in) << 8;

  fseek(in, 0x14, SEEK_SET);

  /*vsize = 0;*/

  for (c = 0; c < section_count; c++)
  {
   fread(section_header, 1, 40, in);
 
   fseek(in, -40, SEEK_CUR);

   if (*(long*)(section_header + 8))
   {
    memcpy(section_header + 16, section_header + 8, 4);
    *(long*)(section_header + 8) = 0;
   }
   if (section_header[39] & 0x20)
   {
    section_header[39] &= 0x7f;
   }
   *(long*)(section_header + 8) = 0;
   /*memcpy(section_header + 8, &vsize, 4);
   vsize += *(long *)(section_header + 16);*/

   fwrite(section_header, 1, 40, in);

   fseek(in, 0, SEEK_CUR);
  }
 }

 fclose(in);
 if (out) fclose(out);

 return 0;
}
