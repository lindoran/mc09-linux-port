/*
 * Safely copy a string 
 */

#include <string.h>

char *safestrcpy(char dest[],size_t dsz,char const *src)
{
  size_t ssz = strlen(src) + 1; /* Include NUL byte */
  size_t len = dsz < ssz ? dsz : ssz;
  memcpy(dest,src,len);
  dest[len - 1] = '\0'; /* set NUL byte at end just to be sure */
  return dest;
}
