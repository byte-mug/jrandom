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
#include "starter.h"

#define vzero(x) memset(&x, 0, sizeof(x));

const struct cuse_lowlevel_ops* GET_RNDEV_OPS();

int runfuse(int argc, const char* const* argv)
{
	struct fuse_args fua = FUSE_ARGS_INIT(argc, (char**) argv);
	struct cuse_info info;
	const char *iav[] = { "DEVNAME=jrandom",NULL };
	vzero(info);
	info.dev_info_argc = 1;
	info.dev_info_argv = iav;
	info.flags = CUSE_UNRESTRICTED_IOCTL;
	return cuse_lowlevel_main(fua.argc, fua.argv, &info, GET_RNDEV_OPS(), NULL);
}


