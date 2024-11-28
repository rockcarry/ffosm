#ifndef _AVMUXER_H_
#define _AVMUXER_H_

#include <stdint.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

enum {
    AVMUXER_CMD_CHECK_BUF,
    AVMUXER_CMD_WRITE_BUF,
    AVMUXER_CMD_WRITE_HDR,
};

typedef void (*PFN_AVMUXER_CB)(void *cbctx, int cmd, uint8_t *buf, int len);

typedef struct {
    void (*audio )(void *ctx, uint8_t *buf, int len, int key, uint32_t pts);
    void (*video )(void *ctx, uint8_t *buf, int len, int key, uint32_t pts);
    void (*header)(void *ctx, char *type);
    void (*exit  )(void *ctx);
} AVMUXER;

AVMUXER* tsmuxer_init (int aac, uint8_t *aacinfo, int h265, int frate, PFN_AVMUXER_CB callback, void *cbctx);
AVMUXER* flvmuxer_init(int aac, uint8_t *aacinfo, int h265, int frate, PFN_AVMUXER_CB callback, void *cbctx);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
