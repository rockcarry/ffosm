#ifndef _FFHTTPD_AVTCP_H_
#define _FFHTTPD_AVTCP_H_

#include <stdint.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C"{
#endif

typedef void (*PFN_AVTCP_CB)(void *cbctx, int arg);

// type1: 0 - server, 1 - client, type2: 0 - ts, 1 - flv
void* avtcp_init (int type1, int type2, char *ip, int port, int bufsize, int aac, uint8_t *aacinfo, int h265, int frate, PFN_AVTCP_CB callback, void *cbctx);
void  avtcp_exit (void *ctx);
void  avtcp_audio(void *ctx, uint8_t *buf, int len, int key, uint32_t ts);
void  avtcp_video(void *ctx, uint8_t *buf, int len, int key, uint32_t ts);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
