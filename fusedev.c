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
#define _FILE_OFFSET_BITS 64

#include <fuse/cuse_lowlevel.h>
#include <fuse/fuse_opt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "rng.h"

// 256 KB
#define MAXSIZ 262144

struct RANDOM{
	TRNG* rng;
	void* buf;
	size_t siz;
};

static void rndev_grow(struct RANDOM* s,size_t siz){
	if(siz>MAXSIZ)siz=MAXSIZ;
	if(s->siz>=siz)return;
	void* n = realloc(s->buf,siz);
	if(!n)return;
	s->buf = n;
	s->siz = siz;
}

static void rndev_read (fuse_req_t req, size_t size, off_t off, struct fuse_file_info *fi) {
	struct RANDOM* s = (struct RANDOM*)(fi->fh);
	rndev_grow(s,size);
	if(size>s->siz)size=s->siz;
	rng_random(s->rng,s->buf,size);
	fuse_reply_buf(req, s->buf, size);
}
static void rndev_write (fuse_req_t req, const char *buf, size_t size, off_t off, struct fuse_file_info *fi) {
	/* Fake Write / Ignore Input */
	fuse_reply_write(req, size);
}
static void rndev_open(fuse_req_t req, struct fuse_file_info *fi) {
	struct RANDOM* s = malloc(sizeof(struct RANDOM));
	if(!s){
		fuse_reply_err(req, ENOMEM);
		return;
	}
	s->rng = rng_new();
	if(!(s->rng)){
		fuse_reply_err(req, ENOMEM);
		free(s);
		return;
	}
	s->buf = NULL;
	s->siz = 0;
	rndev_grow(s,getpagesize());
	fi->fh = (uint64_t)s;
	fuse_reply_open(req, fi);
}
static void rndev_release (fuse_req_t req, struct fuse_file_info *fi) {
	struct RANDOM* s = (struct RANDOM*)(fi->fh);
	rng_destroy(s->rng);
	free(s->buf);
	free(s);
	fuse_reply_err(req, 0);
}

static const struct cuse_lowlevel_ops cllop = {
	.read	 = rndev_read,
	.write	 = rndev_write,
	.open	 = rndev_open,
	.release = rndev_release,
};

const struct cuse_lowlevel_ops* GET_RNDEV_OPS() { return &cllop; }

