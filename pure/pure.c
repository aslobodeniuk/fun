/* 
   Benchmarking G_GNUC_PURE macro
   ---------------------------------
   
   1. Compile pure_helpers.o

   gcc -O3 $(pkg-config --cflags --libs glib-2.0) -c pure_helpers.c -o pure_helpers.o

   2. Compile pure.c with enabled pure function attribute 

   gcc -DWITH_PURE -O3 $(pkg-config --cflags --libs glib-2.0) -c pure.c -o pure.o

   3. Compile pure.c without pure function attribute

   gcc -O3 $(pkg-config --cflags --libs glib-2.0) -c pure.c -o no_pure.o

   4. Build 2 executables to compare

   gcc pure.o pure_helpers.o $(pkg-config --libs glib-2.0) -o mygrep_with_pure
   gcc no_pure.o pure_helpers.o $(pkg-config --libs glib-2.0) -o mygrep_without_pure

   5. Generate some big text file to grep
   --------------------------------------

   GST_DEBUG=9 gst-inspect-1.0 2>log.txt

   6. Launch both mygreps and check the time

   $ ./mygrep_without_pure log.txt gst
     Time to search elapsed: 40136163 us
     matches found: 805307

   $ ./mygrep_with_pure log.txt gst
     Time to search elapsed: 37842701 us
     matches found: 805307
 */


#include <stdio.h>
#include <stdlib.h>
#include "pure.h"

static char *
read_whole_file (const char *fname)
{
  FILE *f;
  long fsize;
  char *string;

  f = fopen (fname, "rb");
  if (!f) {
    printf ("File %s not found\n", fname);
  }

  fseek (f, 0, SEEK_END);
  fsize = ftell (f);
  fseek (f, 0, SEEK_SET);

  string = malloc (fsize + 1);
  fread (string, 1, fsize, f);
  fclose (f);

  string[fsize] = 0;

  return string;
}

static void
usage (void)
{
  printf ("usage:\nmygrep <filename> <string to search>\n");
}

static int
run_search (const char *where, const char *what)
{
  int where_i, what_i, ans = 0;
  for (where_i = 0;; where_i++) {
    int wi = where_i;

    /* We step file by-char and compare */
    for (what_i = 0;; what_i++, wi++) {

      /* If we've arrived to the end of the word, and didn't break before
       * - match is found. */
      if (end_of_string (what[what_i])) {
        ans++;
        /* no need to scan this part again */
        where_i = wi;
        break;
      }

      /* If "where" is over */
      if (end_of_string (where[wi]))
        return ans;

      /* If symbol doesn't match - reset whole search */
      if (!char_match (what[what_i], where[wi]))
        break;

      /* And we arrive here if symbol did a match */
    }
  }

  return ans;
}

int
main (int argc, char **argv)
{
  char *file;
  gint64 measurement_start, measurement = 0;
  int ans = -1, nr, num_runs = 100;

  if (argc < 2) {
    printf ("need file name\n");
    goto bad_arguments;
  }

  if (argc < 3) {
    printf ("need search string\n");
    goto bad_arguments;
  }

  if (!(file = read_whole_file (argv[1])))
    return 1;

  for (nr = 0; nr < num_runs; nr++) {
    int prev_ans = -1;
    if (ans != -1)
      prev_ans = ans;

    measurement_start = g_get_monotonic_time ();
    ans = run_search (file, argv[2]);
    measurement += g_get_monotonic_time () - measurement_start;

    if (prev_ans != -1 && ans != prev_ans) {
      printf ("Sanity check failed\n");
      g_abort ();
    }
  }

  printf ("Time to search elapsed: %" G_GINT64_FORMAT " us\n", measurement);

  printf ("matches found: %d\n", ans);

  free (file);
  return 0;

bad_arguments:
  usage ();
  return 1;
}
