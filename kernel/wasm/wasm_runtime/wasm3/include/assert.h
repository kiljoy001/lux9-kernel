#ifndef _ASSERT_H_
#define _ASSERT_H_

void panic(char *, ...);

#define assert(x) if(!(x)) panic("assertion failed: %s", #x)

#endif
