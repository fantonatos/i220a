#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
doCopy(const char *inName, FILE *in, const char *outName, FILE *out)
{
  int c;
  while ((c = fgetc(in)) != EOF  && ferror(in) == 0) {
    if (fputc(c, out) == EOF) {
      fprintf(stderr, "error writing to %s: %s\n", outName, strerror(errno));
      exit(1);
    }
  }
}

/** Output number of chars in file specified by single command-line
 *  argument.
 */
int
main(int argc, const char *argv[])
{
  if (argc != 3) {
    fprintf(stderr, "usage: %s SRC_NAME DEST_NAME\n", argv[0]);
    exit(1);
  }
  const char *srcName = argv[1];
  const char *destName = argv[2];
  FILE *in = fopen(srcName, "r");
  if (!in) {
    fprintf(stderr, "cannot read %s: %s\n", srcName, strerror(errno));
    exit(1);
  }
  FILE *out = fopen(destName, "a");
  if (!out) {
    fprintf(stderr, "cannot write %s: %s\n", destName, strerror(errno));
    exit(1);
  }
  doCopy(srcName, in, destName, out);
  if (fclose(in) != 0) {
    fprintf(stderr, "cannot close %s: %s\n", srcName, strerror(errno));
    exit(1);
  }
  if (fclose(out) != 0) {
    fprintf(stderr, "cannot close %s: %s\n", destName, strerror(errno));
    exit(1);
  }
  return 0;
}
