#include <glib.h>

#ifdef WITH_PURE
#define MY_PURE G_GNUC_PURE
#else
#define MY_PURE 
#endif

gboolean end_of_string (char c) MY_PURE;

gboolean char_match (char a, char b) MY_PURE;
