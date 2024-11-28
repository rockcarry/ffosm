#ifndef _FFHTTPD_AVSTREAM_H_
#define _FFHTTPD_AVSTREAM_H_

#include <stdint.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef void (*PFN_AVSTREAM_CB)(void *cbctx, int arg);

void* avstream_init (char *name, int bufsize, int aac, uint8_t *aacinfo, int h265, int frate, PFN_AVSTREAM_CB callback, void *cbctx);
void  avstream_exit (void *ctx);
void  avstream_audio(void *ctx, uint8_t *buf, int len, int key, uint32_t pts);
void  avstream_video(void *ctx, uint8_t *buf, int len, int key, uint32_t pts);

int avstream_ffhttpd_cb(void *cbctx, int id, int cmd, char *path, char *postdata, int postlen, char *respbuf, int resplen);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif

