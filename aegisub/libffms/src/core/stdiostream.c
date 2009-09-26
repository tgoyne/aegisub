//  Copyright (c) 2007-2009 Fredrik Mellbin
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#include "stdiostream.h"
#include "ffmscompat.h"
#include <errno.h>

/* StdIoStream methods */

/* read count bytes into buffer starting at file position pos
 * return the number of bytes read, -1 on error or 0 on EOF
 */
int StdIoRead(StdIoStream *st, ulonglong pos, void *buffer, int count) {
  size_t  rd;
  if (fseeko(st->fp, pos, SEEK_SET)) {
    st->error = errno;
    return -1;
  }
  rd = fread(buffer, 1, count, st->fp);
  if (rd == 0) {
    if (feof(st->fp))
      return 0;
    st->error = errno;
    return -1;
  }
  return rd;
}

/* scan for a signature sig(big-endian) starting at file position pos
 * return position of the first byte of signature or -1 if error/not found
 */
longlong StdIoScan(StdIoStream *st, ulonglong start, unsigned signature) {
  int	      c;
  unsigned    cmp = 0;
  FILE	      *fp = st->fp;

  if (fseeko(fp, start, SEEK_SET))
    return -1;

  while ((c = getc(fp)) != EOF) {
    cmp = ((cmp << 8) | c) & 0xffffffff;
    if (cmp == signature)
      return ftello(fp) - 4;
  }

  return -1;
}

/* return cache size, this is used to limit readahead */
unsigned StdIoGetCacheSize(StdIoStream *st) {
  return CACHESIZE;
}

/* return last error message */
const char *StdIoGetLastError(StdIoStream *st) {
  return strerror(st->error);
}

/* memory allocation, this is done via stdlib */
void *StdIoMalloc(StdIoStream *st, size_t size) {
  return malloc(size);
}

void *StdIoRealloc(StdIoStream *st, void *mem, size_t size) {
  return realloc(mem,size);
}

void StdIoFree(StdIoStream *st, void *mem) {
  free(mem);
}

/* progress report handler for lengthy operations
 * returns 0 to abort operation, nonzero to continue
 */
int StdIoProgress(StdIoStream *st, ulonglong cur, ulonglong max) {
  return 1;
}

longlong StdIoGetFileSize(StdIoStream *st) {
	longlong epos = 0;
	longlong cpos = ftello(st->fp);
	fseeko(st->fp, 0, SEEK_END);
	epos = ftello(st->fp);
	fseeko(st->fp, cpos, SEEK_SET);
	return epos;
}

void InitStdIoStream(StdIoStream *st) {
	memset(st,0,sizeof(StdIoStream));
	st->base.read = (int (*)(InputStream *,ulonglong,void *,int))StdIoRead;
	st->base.scan = (longlong (*)(InputStream *,ulonglong,unsigned int))StdIoScan;
	st->base.getcachesize =  (unsigned int (*)(InputStream *))StdIoGetCacheSize;
	st->base.geterror = (const char *(*)(InputStream *))StdIoGetLastError;
	st->base.memalloc = (void *(*)(InputStream *,size_t))StdIoMalloc;
	st->base.memrealloc = (void *(*)(InputStream *,void *,size_t))StdIoRealloc;
	st->base.memfree = (void (*)(InputStream *,void *))StdIoFree;
	st->base.progress = (int (*)(InputStream *,ulonglong,ulonglong))StdIoProgress;
	st->base.getfilesize = (longlong (*)(InputStream *))StdIoGetFileSize;
}
