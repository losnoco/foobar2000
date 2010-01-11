#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED


int  head_check(unsigned long head,int check_layer);
int  decode_header(struct frame *fr,unsigned long newhead);//newhead = needs correct byte order
void print_header(struct frame *fr);
void print_header_compact(struct frame *fr);
int set_pointer( PMPSTR mp, long backstep);
unsigned get_frame_size_dword(unsigned long newhead);

#endif
