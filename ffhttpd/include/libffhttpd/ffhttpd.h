#ifndef _FFHTTPD_H_
#define _FFHTTPD_H_

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define FFHTTPD_VERSION       "v2.3.1" // please also change version in configure.ac
#define FFHTTPD_MAX_CONN_NUM  8

enum {
    TYPE_REQUEST_HEAD,
    TYPE_REQUEST_GET ,
    TYPE_REQUEST_PART,
    TYPE_REQUEST_POST,
    TYPE_RESPONSE_HEAD,
    TYPE_RESPONSE_DATA,
    TYPE_RESPONSE_DONE,
    TYPE_WEBSOCKET_RX,
    TYPE_WEBSOCKET_TX,
};

// id       : client id
// path     : request path
// postdata : data of post request
// postlen  : length of post data
// respbuf  : response buffer
// resplen  : response buffer size
// return   :
// for TYPE_REQUEST_HEAD, TYPE_REQUEST_GET, TYPE_REQUEST_POST and TYPE_REQUEST_PART, the return is the total request data size.
//     if return -1 ffhttpd will handle by default (to read from file)
// for TYPE_RESPONSE_HEAD and TYPE_RESPONSE_DATA, the return is the actual reponsed data size.
//     if return -1 ffhttpd will handle by default (to read from file)
// for other types, ignore the return value, suggest to return 0.
typedef int (*PFN_FFHTTPD_CB)(void *cbctx, int id, int cmd, char *path, char *postdata, int postlen, char *respbuf, int resplen);

// if user exists, return user type and fill passwd buffer, otherwise return -1
typedef int (*PFN_FFHTTPD_USERPASSWD_CB)(void *cbctx, char *username, char *passwd, int size);

void* ffhttpd_init(char *ip, int port, char *dir, int tls, char *certs[3], PFN_FFHTTPD_CB callback, void *cbctx);
void  ffhttpd_exit(void *ctx);
void  ffhttpd_url_decode(char *str);
void  ffhttpd_set_pathmap(void *ctx, char *src[], char *dst[]);

long ffhttpd_set (void *ctx, char *name, void *param);
long ffhttpd_get (void *ctx, char *name, void *param);
void ffhttpd_dump(void *ctx, char *str, int len, int page);

#define FFHTTPD_PARAM_DIGESTAUTH  "s_digestauth"  // set/get, "" - means disable authentication, "/xxx" - match the path prefix, enable authentication for match path
#define FFHTTPD_PARAM_PASSWORDCB  "i_passwordcb"  // set, user password callback for digest authentication
#define FFHTTPD_PARAM_RESPBUFSIZE "i_respbufsize" // set/get, response buffer size

// howto enable & implement ffhttpd server digest authentication ? see example code:
#if 0
static int my_userpasswd_callback(void *cbctx, char *username, char *passwd, int size)
{
    // get user password from database
    // todo..
    strncpy(passwd, "passwd", size);
    return 0;
}

static void setup_ffhttpd_digest_authentication(void *ffhttpd)
{
    ffhttpd_set(ffhttpd, FFHTTPD_PARAM_DIGESTAUTH, (void*)"/ipcapi.cgi");
    ffhttpd_set(ffhttpd, FFHTTPD_PARAM_PASSWORDCB, (void*)my_userpasswd_callback);
}
#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
