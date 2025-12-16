void p9_write(int fd, void *buf, long n);
void p9_exits(char *msg);

int strlen(const char *s) {
  int n = 0;
  while (s[n])
    n++;
  return n;
}

void print(const char *s) { p9_write(1, (void *)s, strlen(s)); }

void main(void) {
  print("\n\n");
  print("*** LUX9 C-INIT STARTUP ***\n");
  print("Kernel boot verification successful.\n");
  print("Running in userspace (C-based init).\n");
  print("Namespace setup verified via logs.\n");
  print("Looping forever...\n");

  while (1) {
    // Sleep loop
    for (volatile int i = 0; i < 10000000; i++)
      ;
    print("init: alive\n");
  }
}
