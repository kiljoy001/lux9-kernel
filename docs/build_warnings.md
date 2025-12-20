CC kernel/9front-pc64/vmdetect.c
In file included from kernel/9front-pc64/vmdetect.c:8:
kernel/include/portlib.h:43:9: error: redeclaration of enumerator ‘UTFmax’
   43 |         UTFmax          = 4,            /* maximum bytes per rune */
      |         ^~~~~~
In file included from kernel/9front-pc64/vmdetect.c:3:
kernel/include/lib.h:40:9: note: previous definition of ‘UTFmax’ with type ‘enum <anonymous>’
   40 |         UTFmax          = 4,            /* maximum bytes per rune */
      |         ^~~~~~
kernel/include/portlib.h:44:9: error: redeclaration of enumerator ‘Runesync’
   44 |         Runesync        = 0x80,         /* cannot represent part of a UTF sequence */
      |         ^~~~~~~~
kernel/include/lib.h:41:9: note: previous definition of ‘Runesync’ with type ‘enum <anonymous>’
   41 |         Runesync        = 0x80,         /* cannot represent part of a UTF sequence */
      |         ^~~~~~~~
kernel/include/portlib.h:45:9: error: redeclaration of enumerator ‘Runeself’
   45 |         Runeself        = 0x80,         /* rune and UTF sequences are the same (<) */
      |         ^~~~~~~~
kernel/include/lib.h:42:9: note: previous definition of ‘Runeself’ with type ‘enum <anonymous>’
   42 |         Runeself        = 0x80,         /* rune and UTF sequences are the same (<) */
      |         ^~~~~~~~
kernel/include/portlib.h:46:9: error: redeclaration of enumerator ‘Runeerror’
   46 |         Runeerror       = 0xFFFD,       /* decoding error in UTF */
      |         ^~~~~~~~~
kernel/include/lib.h:43:9: note: previous definition of ‘Runeerror’ with type ‘enum <anonymous>’
   43 |         Runeerror       = 0xFFFD,       /* decoding error in UTF */
      |         ^~~~~~~~~
kernel/include/portlib.h:47:9: error: redeclaration of enumerator ‘Runemax’
   47 |         Runemax         = 0x10FFFF,     /* 21 bit rune */
      |         ^~~~~~~
kernel/include/lib.h:44:9: note: previous definition of ‘Runemax’ with type ‘enum <anonymous>’
   44 |         Runemax         = 0x10FFFF,     /* 21 bit rune */
      |         ^~~~~~~
kernel/include/portlib.h:76:8: error: redefinition of ‘struct Fmt’
   76 | struct Fmt{
      |        ^~~
kernel/include/lib.h:73:8: note: originally defined here
   73 | struct Fmt{
      |        ^~~
kernel/include/portlib.h:218:8: error: redefinition of ‘struct Qid’
  218 | struct Qid
      |        ^~~
kernel/include/lib.h:215:8: note: originally defined here
  215 | struct Qid
      |        ^~~
kernel/include/portlib.h:225:8: error: redefinition of ‘struct Dir’
  225 | struct Dir {
      |        ^~~
kernel/include/lib.h:222:8: note: originally defined here
  222 | struct Dir {
      |        ^~~
kernel/include/portlib.h:241:8: error: redefinition of ‘struct OWaitmsg’
  241 | struct OWaitmsg
      |        ^~~~~~~~
kernel/include/lib.h:238:8: note: originally defined here
  238 | struct OWaitmsg
      |        ^~~~~~~~
kernel/include/portlib.h:248:8: error: redefinition of ‘struct Waitmsg’
  248 | struct Waitmsg
      |        ^~~~~~~
kernel/include/lib.h:245:8: note: originally defined here
  245 | struct Waitmsg
      |        ^~~~~~~
make: *** [GNUmakefile:109: kernel/9front-pc64/vmdetect.o] Error 1
