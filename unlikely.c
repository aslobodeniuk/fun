#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

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

#ifdef WITH_G_LIKELY
#define ZZ_UNLIKELY G_UNLIKELY
#define ZZ_LIKELY G_LIKELY
#else
#define ZZ_UNLIKELY(x) x
#define ZZ_LIKELY(x) x
#endif

static int
run_search (const char * where, const char * what)
{
  int where_i, what_i, ans = 0;
  for (where_i = 0;; where_i++) {
    int wi = where_i;

    /* We step file by-char and compare */
    for (what_i = 0;; what_i++, wi++) {

      /* If we've arrived to the end of the word, and didn't break before
       * - match is found. */
      if (ZZ_UNLIKELY (what[what_i] == 0)) {
        ans++;
        /* no need to scan this part again */
        where_i = wi;
        break;
      }

      /* If "where" is over */
      if (ZZ_UNLIKELY (where[wi] == 0))
        return ans;

      /* If symbol doesn't match - reset whole search */
      if (ZZ_LIKELY (what[what_i] != where[wi]))
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
