/*
 * Copyright (c) 2016 Simon Schmidt
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgement in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 */
#include "rng.h"
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*  ----------------------- Salsa20 by Alex Weber -----------------------------*/
/* https://github.com/alexwebr/salsa20 */
/*
The person or persons who have associated work with this document (the
"Dedicator" or "Certifier") hereby either (a) certifies that, to the best of
his knowledge, the work of authorship identified is in the public domain of
the country from which the work is published, or (b) hereby dedicates whatever
copyright the dedicators holds in the work of authorship identified below (the
"Work") to the public domain. A certifier, moreover, dedicates any copyright
interest he may have in the associated work, and for these purposes, is
described as a "dedicator" below.

A certifier has taken reasonable steps to verify the copyright status of this
work. Certifier recognizes that his good faith efforts may not shield him from
liability if in fact the work certified is not in the public domain.

Dedicator makes this dedication for the benefit of the public at large and to
the detriment of the Dedicator's heirs and successors. Dedicator intends this
dedication to be an overt act of relinquishment in perpetuity of all present
and future rights under copyright law, whether vested or contingent, in the
Work. Dedicator understands that such relinquishment of all rights includes
the relinquishment of all rights to enforce (by lawsuit or otherwise) those
copyrights in the Work.

Dedicator recognizes that, once placed in the public domain, the Work may be
freely reproduced, distributed, transmitted, used, modified, built upon, or
otherwise exploited by anyone for any purpose, commercial or non-commercial,
and in any way, including by methods that have not yet been invented or
conceived.
*/

// Implements DJB's definition of '<<<'
static uint32_t rotl(uint32_t value, int shift)
{
  return (value << shift) | (value >> (32 - shift));
}

static void s20_quarterround(uint32_t *y0, uint32_t *y1, uint32_t *y2, uint32_t *y3)
{
  *y1 = *y1 ^ rotl(*y0 + *y3, 7);
  *y2 = *y2 ^ rotl(*y1 + *y0, 9);
  *y3 = *y3 ^ rotl(*y2 + *y1, 13);
  *y0 = *y0 ^ rotl(*y3 + *y2, 18);
}

static void s20_rowround(uint32_t y[static 16])
{
  s20_quarterround(&y[0], &y[1], &y[2], &y[3]);
  s20_quarterround(&y[5], &y[6], &y[7], &y[4]);
  s20_quarterround(&y[10], &y[11], &y[8], &y[9]);
  s20_quarterround(&y[15], &y[12], &y[13], &y[14]);
}

static void s20_columnround(uint32_t x[static 16])
{
  s20_quarterround(&x[0], &x[4], &x[8], &x[12]);
  s20_quarterround(&x[5], &x[9], &x[13], &x[1]);
  s20_quarterround(&x[10], &x[14], &x[2], &x[6]);
  s20_quarterround(&x[15], &x[3], &x[7], &x[11]);
}

static void s20_doubleround(uint32_t x[static 16])
{
  s20_columnround(x);
  s20_rowround(x);
}

// Creates a little-endian word from 4 bytes pointed to by b
static uint32_t s20_littleendian(uint8_t *b)
{
  return b[0] +
         (b[1] << 8) +
         (b[2] << 16) +
         (b[3] << 24);
}

// Moves the little-endian word into the 4 bytes pointed to by b
static void s20_rev_littleendian(uint8_t *b, uint32_t w)
{
  b[0] = w;
  b[1] = w >> 8;
  b[2] = w >> 16;
  b[3] = w >> 24;
}

// The core function of Salsa20
static void s20_hash(uint8_t seq[static 64])
{
  int i;
  uint32_t x[16];
  uint32_t z[16];

  // Create two copies of the state in little-endian format
  // First copy is hashed together
  // Second copy is added to first, word-by-word
  for (i = 0; i < 16; ++i)
    x[i] = z[i] = s20_littleendian(seq + (4 * i));

  for (i = 0; i < 10; ++i)
    s20_doubleround(z);

  for (i = 0; i < 16; ++i) {
    z[i] += x[i];
    s20_rev_littleendian(seq + (4 * i), z[i]);
  }
}

/*  ----------------------------------------------------------------------------*/

#define DiffIt(pre,post) (pre[0]post) - (pre[1]post)
#define AsPtr(expr,typ) *((typ*)(expr))

struct RNG{
	uint8_t buffer[64];
};

static void update(TRNG* s){
	struct timespec times[2];
	clock_gettime(CLOCK_REALTIME,&(times[1]));
	s20_hash(s->buffer);
	clock_gettime(CLOCK_REALTIME,&(times[0]));
	AsPtr(s->buffer,long) ^= DiffIt(times,.tv_nsec);
	AsPtr(&(s->buffer)[8],time_t) ^= DiffIt(times,.tv_sec);
	s20_hash(s->buffer);
}

TRNG* rng_new(){
	TRNG* rng = malloc(sizeof(TRNG));
	return rng;
}
void rng_random(TRNG* rng,void* buf,uintz i){
	update(rng);
	while(i>64){
		memcpy(buf,rng->buffer,64);
		i-=64;
		buf+=64;
		s20_hash(rng->buffer);
	}
	memcpy(buf,rng->buffer,i);
}
void rng_destroy(TRNG* rng){
	free(rng);
}

