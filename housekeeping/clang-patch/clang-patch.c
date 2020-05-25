#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]) {
  char **oa = argv;
  for (char **ia = argv; *ia; ++ia) {
    if (strcmp(*ia, "-fcoalesce-templates") != 0) {
      *oa = *ia;
      ++oa;
    }
  }
  *oa = NULL;
  execv("/usr/bin/clang++", argv);
}
