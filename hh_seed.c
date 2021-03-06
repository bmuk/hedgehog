/* This file is part of Hedgehog LISP.
 * Copyright (C) 2005 Kenneth Oksanen.
 * See file LICENSE.LGPL for pertinent licensing conditions.
 *
 * Author: Kenneth Oksanen <cessu@iki.fi>
 */

/* This file precomputes the seed value and the SHA256 of the shared
   secret. */


#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "hh_crypto.h"


#define DIGEST_SIZE  32
#define BUF_SIZE  1024
static unsigned char digest[DIGEST_SIZE], buf[BUF_SIZE + DIGEST_SIZE];
static unsigned int buf_ptr;

static void digest_init(void)
{
  memset(digest, 0, DIGEST_SIZE);
  memset(buf, 0, BUF_SIZE + DIGEST_SIZE);
  buf_ptr = 0;
}

static void digest_char(unsigned char ch)
{
  if (buf_ptr == BUF_SIZE) {
    hh_sha256(buf, BUF_SIZE + DIGEST_SIZE, digest);
    memset(buf, 0, BUF_SIZE);
    memcpy(buf + BUF_SIZE, digest, DIGEST_SIZE);
    buf_ptr = 0;
  }
  buf[buf_ptr++] = ch;
}

static void digest_finish(void)
{
  hh_sha256(buf, BUF_SIZE + DIGEST_SIZE, digest);
}


int main(void)
{
#define LINE_LEN  1024
  char *s, line[LINE_LEN];
  int i;

  printf("/* This file is automatically generated by `hh_seed.c' from\n\
   the contents of the `hh_insn.def' file and the shared secret. */\n\n");

  digest_init();
  for (s = HH_SHARED_SECRET; *s != '\0'; s++)
    digest_char(*s);
  digest_finish();
  
  printf("unsigned char hh_shared_secret[32] = {");
  for (i = 0; i < 32; i++) {
    if (i % 8 == 0)
      printf("\n ");
    printf(" 0x%02X,", digest[i]);
  }
  printf("\n};\n\n");

  /* The shared secret used in communication does not include the
     cookie, because that could cause logistic hassles when
     communicating (but not updating byte codes) with clients on
     several different targets.  Nor does the insn cookie include the
     shared secret, because the shared secret is only 32-bits long and
     not sufficiently strong agaist a determined attack. */

  digest_init();
  while (!feof(stdin)) {
    fgets(line, LINE_LEN, stdin);
    if (strncmp(line, "INSN", 4) == 0
	|| strncmp(line, "EXT_INSN", 8) == 0
	|| strncmp(line, "IMM", 3) == 0)
      for (s = line; *s != '\0' && *s != ',' && *s != 0x0A; s++)
	digest_char(*s);
  }
  digest_finish();
  printf("#define HH_INSN_COOKIE  0x%02X%02X%02X%02X\n", 
	 digest[0], digest[1], digest[2], digest[3]);

  return 0;
}
