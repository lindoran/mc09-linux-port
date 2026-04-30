/*
 * Implementation of MC_fgets(), a function to read a line
 * from the input file without placing a newline character
 * at the end.
 */

#include <stdio.h>
#include <string.h>

char *MC_fgets(char *dest,size_t size,FILE *fp)
{
  char *ptr = dest;
  
  while(--size)
  {
    int chr = fgetc(fp);
    
    if(chr == EOF)
    {
      if(ptr == dest)
        dest = NULL;
      break;
    }
    
    if(chr == '\n')
      break;
      
    if(chr == '\r')
      continue;
      
    *ptr++ = chr;
  }
  
  *ptr = '\0';
  return dest;
}
