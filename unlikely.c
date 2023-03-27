/* Benchmarking test for G_LIKELY / G_UNLIKELY
 *
 * Copyright (C) 2023 Alexander Slobodeniuk <aslobodeniuk@fluendo.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <glib.h>

static void
usage (void)
{
  g_message ("usage:\nmygrep <filename> <string to search>\n");
}

#ifdef WITH_G_LIKELY
#define UNLIKELY G_UNLIKELY
#define LIKELY G_LIKELY
#else
#define UNLIKELY(x) x
#define LIKELY(x) x
#endif

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
      if (UNLIKELY (what[what_i] == 0)) {
        ans++;
        /* no need to scan this part again */
        where_i = wi;
        break;
      }

      /* If "where" is over */
      if (UNLIKELY (where[wi] == 0))
        return ans;

      /* If symbol doesn't match - reset whole search */
      if (LIKELY (what[what_i] != where[wi]))
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
  gsize length;
  gint64 measurement_start, measurement = 0;
  int ans = -1, nr, num_runs = 100;

  if (argc < 2) {
    g_warning ("need file name\n");
    goto bad_arguments;
  }

  if (argc < 3) {
    g_warning ("need search string\n");
    goto bad_arguments;
  }

  if (!g_file_get_contents (argv[1], &file, &length, NULL)) {
    g_warning ("couldn't read file");
    return 1;
  }

  g_message ("File size = %" G_GSIZE_FORMAT " bytes, checking %d times", length,
      num_runs);

  for (nr = 0; nr < num_runs; nr++) {
    int prev_ans = -1;
    if (ans != -1)
      prev_ans = ans;

    g_message ("check number %d", nr);
    measurement_start = g_get_monotonic_time ();
    ans = run_search (file, argv[2]);
    measurement += g_get_monotonic_time () - measurement_start;

    if (prev_ans != -1 && ans != prev_ans) {
      g_error ("Sanity check failed\n");
    }
  }

  g_message ("---------------------------------------------------------------");
  g_message ("Total time elapsed: %" G_GINT64_FORMAT " us\n", measurement);
  g_message ("matches found: %d\n", ans);

  g_free (file);
  return 0;

bad_arguments:
  usage ();
  return 1;
}
