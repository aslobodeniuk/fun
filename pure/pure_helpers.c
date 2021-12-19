#include "pure.h"

gboolean
end_of_string (char c)
{
  return c == 0;
}

gboolean
char_match (char a, char b)
{
  return a == b;
}
