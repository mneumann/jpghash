/*
 * JPEG Hash
 *
 * This program reads filenames from stdin (JPEG filename) 
 * and produces a hashsum of each file.
 *
 * It is possible to specify specific segments to be included/excluded
 * from hash generation.
 *
 * Copyright (c) 2009 by Michael Neumann (mneumann@ntecs.de)
 *
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include "sha1.h"

const char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

int want_type[256]; 

#define STATE_0 0
#define STATE_FF 1

void process_jpeg(void *ptr, size_t sz, const char *filename)
{
  SHA1_CTX hashsum;
  SHA1_Init(&hashsum);

  int state = STATE_0;
  int want_data = 1;

  uint8_t *c = ptr;
  for (; sz > 0; --sz, ++c)
  {
    if (state == STATE_0)
    {
      if (*c == 0xFF)
      {
        state = STATE_FF;
      }
      else
      {
        if (want_data) SHA1_Update(&hashsum, c, 1);
      }
    }
    else
    {
      assert(state == STATE_FF);

      if (*c != 0x00)
      {
        // This is a new section
        want_data = want_type[*c];
      }

      if (want_data) SHA1_Update(&hashsum, c-1, 2);
      state = STATE_0;
    }
  }

  if (state == STATE_FF && want_data)
  {
    SHA1_Update(&hashsum, c-1, 1);
  }


  /* 
   * Construct the hexdigest
   */

  char sha[SHA1_DIGEST_STRING_LENGTH];
  uint8_t digest[20];

  SHA1_Finish(&hashsum, digest);

  for (int i = 0; i < 20; ++i)
  {
    sha[2*i] = hex[digest[i] >> 4];
    sha[2*i+1] = hex[digest[i] & 15];
  }

  sha[SHA1_DIGEST_STRING_LENGTH-1] = '\000';

  printf("%s\t%s\n", sha, filename);
}

void process_file(const char* filename)
{
  int fd = open(filename, O_RDONLY);
  if (fd == -1)
  {
    printf("!ERROR opening file\t%s\n", filename);
    return;
  }

  struct stat buf;
  if (stat(filename, &buf) != 0)
  {
    close(fd);
    printf("!ERROR stat file\t%s\n", filename);
    return;
  }

  void *ptr = mmap(NULL, buf.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (ptr == MAP_FAILED)
  {
    close(fd);
    printf("!ERROR mmap file\t%s\n", filename);
    return;
  }

  process_jpeg(ptr, buf.st_size, filename);

  munmap(ptr, buf.st_size);
  close(fd);
}

int main(int argc, char **argv)
{
  static char filename[1024];

  for (int i=0; i < 256; ++i)
    want_type[i] = 1;

  /*
   * Exclude types given on command line
   */
  for (int i = 1; i < argc; ++i)
  {
    int type, end_type;
    char *ptr;
    char *ptr2;

    type = strtol(argv[i], &ptr, 16);
    assert(type >= 0 && type <= 255);

    if (*ptr == '\000')
    {
      end_type = type;
    }
    else
    {
      assert(*ptr == '-');
      end_type = strtol(ptr+1, &ptr2, 16);
      assert(*ptr2 == '\000');
      assert(end_type >= 0 && end_type <= 255);
      assert(end_type >= type);
    }

    for (int t=type; t <= end_type; ++t)
    {
      char h[3];
      h[0] = hex[t >> 4];
      h[1] = hex[t & 15];
      h[2] = '\000';
      fprintf(stderr, "Exclude 0x%s\n", h);
      want_type[t] = 0;
    }
  }

  while (fgets(filename, 1024, stdin)) 
  {
    int len = strlen(filename);
    if (len > 0 && filename[len-1] == '\n')
    {
      filename[len-1] = '\000';
      process_file(filename);
    }
    else
    {
      printf("!ERROR Invalid filename\t%s\n", filename);
    }
  }

  return 0;
}
