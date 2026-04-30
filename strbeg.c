/*
 * Test if string s begins with prefix p.
 */

#include <stdbool.h>

bool strbeg(char const *restrict s,char const *restrict p)
{
  while(*p)
    if (*s++ != *p++)
      return false;
  return true;
}
