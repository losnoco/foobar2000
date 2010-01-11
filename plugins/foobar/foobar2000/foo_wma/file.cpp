/*
 * Buffered file io for ffmpeg system
 * Copyright (c) 2001 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include "file.h"

/*
#include <fcntl.h>
#ifndef CONFIG_WIN32
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#else
#include <io.h>
#define open(fname,oflag,pmode) _open(fname,oflag,pmode)
#endif /* CONFIG_WIN32 */

#include "../sdk/foobar2000.h"



/* standard file protocol */

static int file_open(URLContext *h, const char *filename, int flags)
{
    file * r;

    strstart(filename, "foobar:", &filename);

    if (flags & URL_WRONLY) {
		r = filesystem::g_open(filename, reader::MODE_WRITE_NEW);
    } else {
		r = file::g_open(filename, reader::MODE_READ);
    }
    if (!r)
        return -ENOENT;
    h->priv_data = (void *)r;
    return 0;
}

static int file_read(URLContext *h, unsigned char *buf, int size)
{
    reader * r = (reader *)h->priv_data;
    return r->read(buf, size);
}

static int file_write(URLContext *h, unsigned char *buf, int size)
{
	reader * r = (reader *)h->priv_data;
	return r->write(buf, size);
}

/* XXX: use llseek */
static offset_t file_seek(URLContext *h, offset_t pos, int whence)
{
	reader * r = (reader *)h->priv_data;
	if (r->seek2(pos, whence))
		return r->get_position();
	else
		return 0;
}

static int file_close(URLContext *h)
{
	reader * r = (reader *)h->priv_data;
	r->reader_release();
	return 0;
}

URLProtocol fb2k_protocol = {
    "foobar",
    file_open,
    file_read,
    file_write,
    file_seek,
    file_close,
};
