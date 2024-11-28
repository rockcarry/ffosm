#ifndef __FFHTTPC_H__
#define __FFHTTPC_H__

#include <stdint.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

enum { // return value of ffhttpc_read
    FFHTTPC_EOF = -2,
};

void* ffhttpc_init(int bufsize);
void  ffhttpc_exit(void *ctx);

void  ffhttpc_request(void *ctx, char *url, char *postdata, int postsize, int64_t offset, int reconnect);
int   ffhttpc_read   (void *ctx, char *buf, int len, int64_t *total, int64_t *progress, int wait);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
