#include <stdlib.h>
#include <stdio.h>
#include <glib.h>

void *g_malloc (size_t size)
{
  printf ("hiiiii %d\n", (int)size);

  return malloc (size);
}

void g_free (gpointer p) {

  printf ("byyye %p\n", p);

  free (p);
}

int main (void) {

  g_free (g_malloc (123));

  /* uses g_malloc, but this one is not replaced */
  g_memdup ("abc", 3);
  
  return 0;
}
