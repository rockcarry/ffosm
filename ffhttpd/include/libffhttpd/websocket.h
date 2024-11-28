#ifndef _WEB_SOCKET_H_
#define _WEB_SOCKET_H_

#include <stdint.h>

enum {
    WEBSOCKET_TX_SIZE,
    WEBSOCKET_TX_DATA,
    WEBSOCKET_RX_FRAME,
};

typedef void (*PFN_WEBSOCKET_CB)(void *cbctx, int type, uint8_t *buf, int len);

void* websocket_init(PFN_WEBSOCKET_CB callback, void *cbctx);
void  websocket_recv(void *ctx, uint8_t *buf, int len);
void  websocket_send(void *ctx, uint8_t *buf, int len, uint8_t opcode);
void  websocket_exit(void *ctx);
void  websocket_accept_key(char *request_key, char *accept_key, int len);

#endif
