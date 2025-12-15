#ifndef _SETJMP_H_
#define _SETJMP_H_

typedef long jmp_buf[16];
int setjmp(jmp_buf);
void longjmp(jmp_buf, int);

#endif
