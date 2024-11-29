#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <libffhttpd/ffhttpd.h>
#include "ffosm.h"
#include "utils.h"

static char *s_header1 =
"HTTP/1.1 200 OK\r\n"
"Server: ffhttpd/1.0.0\r\n"
"Accept-Ranges: bytes\r\n"
"Content-Type: text/html\r\n"
"Content-Length: %d\r\n\r\n"
"%s";

#define FFOSM_MAIN_PAGE  "/ffosm.cgi"
#define FFOSM_TRANS_PAGE "/transact.cgi"
#define FFOSM_DATABASE   "ffosm.db"

typedef struct {
    char *buf;
    int   len;
} CBDATA;

typedef struct {
    char admin [64];
    char passwd[64];
} MAINAPP;

static int query_stock_cb(void *cbctx, int argc, char **argv, char **colname)
{
    CBDATA *data = cbctx;
    int ret = snprintf(data->buf, data->len, "<tr>");
    data->buf += ret;
    data->len -= ret;
    for (int i = 0; i < argc && data->len > 0; i++) {
        ret = snprintf(data->buf, data->len, "<td>%s</td>", argv[i]);
        data->buf += ret;
        data->len -= ret;
    }

    ret = snprintf(data->buf, data->len,
        "<td>"
        "<a href='%s?act=putin&id=%s'>入库</a>&nbsp;"
        "<a href='%s?act=borrow&id=%s'>领用</a>&nbsp;"
        "<a href='%s?act=delete&id=%s'>删除</a>"
        "</td>",
        FFOSM_TRANS_PAGE, argv[0],
        FFOSM_TRANS_PAGE, argv[0],
        FFOSM_TRANS_PAGE, argv[0]);
    data->buf += ret;
    data->len -= ret;

    if (data->len > 0) {
        ret = snprintf(data->buf, data->len, "</tr>\n");
        data->buf += ret;
        data->len -= ret;
    }
    return 0;
}

static int query_record_cb(void *cbctx, int argc, char **argv, char **colname)
{
    CBDATA *data = cbctx;
    int ret =snprintf(data->buf, data->len, "<tr>");
    data->buf += ret;
    data->len -= ret;
    for (int i = 0; i < argc && data->len > 0; i++) {
        if (i == 1) {
            int type = atoi(argv[1]);
            if (type < OP_PUTIN || type > OP_SCRAP) type = 0;
            static char *OP_STR_TAB[] = { "-", "入库", "领用", "归还", "报废" };
            ret = snprintf(data->buf, data->len, "<td>%s</td>", OP_STR_TAB[type]);
        } else {
            ret = snprintf(data->buf, data->len, "<td>%s</td>", argv[i]);
        }
        data->buf += ret;
        data->len -= ret;
    }
    ret = snprintf(data->buf, data->len, "<td>"
        "<a href='%s?act=return&id=%s'>归还</a>&nbsp;"
        "<a href='%s?act=scrap&id=%s'>报废</a>"
        "</td>",
        FFOSM_TRANS_PAGE, argv[0],
        FFOSM_TRANS_PAGE, argv[0]);
    data->buf += ret;
    data->len -= ret;
    if (data->len > 0) {
        ret = snprintf(data->buf, data->len, "</tr>\n");
        data->buf += ret;
        data->len -= ret;
    }
    return 0;
}

static int stock_table(char *buf, int len, char *name, char *remarks)
{
    void *osm = ffosm_init(FFOSM_DATABASE);
    CBDATA cbdata = { buf, len };
    ffosm_query_stock(osm, query_stock_cb, &cbdata, name, remarks);
    ffosm_exit(osm);
    return 0;
}

static int record_table(char *buf, int len, int type, char *user, int item_id, char *remarks)
{
    void *osm = ffosm_init(FFOSM_DATABASE);
    CBDATA cbdata = { buf, len };
    ffosm_query_record(osm, query_record_cb, &cbdata, type, user, item_id, remarks);
    ffosm_exit(osm);
    return 0;
}

static int main_page(char *buf, int len)
{
    char  buf_stock_table [64 * 1024];
    char  buf_record_table[64 * 1024];
    int   size = 0;
    char *page = file_read("ffosm.page", &size);
    if (!page) return snprintf(buf, len, "<html></html>");

    stock_table (buf_stock_table , sizeof(buf_stock_table ), "", "");
    record_table(buf_record_table, sizeof(buf_record_table), -1, "", -1, "");
    int ret = snprintf(buf, len, page, buf_stock_table, buf_record_table);

    free(page);
    return ret;
}

static int my_ffhttpd_cb(void *cbctx, int id, int cmd, char *path, char *postdata, int postlen, char *respbuf, int resplen)
{
    if (strcmp(path, "/index.html") == 0 || strncmp(path, FFOSM_MAIN_PAGE, sizeof(FFOSM_MAIN_PAGE) - 1) == 0) {
        char buf[256 * 1024];
        int  len;
        switch (cmd) {
        case TYPE_REQUEST_GET:
        case TYPE_REQUEST_POST:
            ffhttpd_url_decode(postdata);
            len = main_page(buf, sizeof(buf));
            snprintf(respbuf, resplen, s_header1, len, buf);
            return 0;
        }
    }
    return -1;
}

static int my_userpasswd_callback(void *cbctx, char *username, char *passwd, int size)
{
    // get user password from database
    if (strcmp(username, "apical") == 0) {
        strncpy(passwd, "apicalgood", size);
        return 0;
    }
    return -1;
}

static int s_exit = 0;
static void sig_handler(int sig)
{
    printf("ffosm sig_handler %d\n", sig);
    switch (sig) {
    case SIGPIPE:
        break;
    case SIGINT: case SIGTERM:
        s_exit = 1;
        break;
    }
}

int main(int argc, char *argv[])
{
    char *ip     = "0.0.0.0";
    char *dir    = "www/";
    char *admin  = "apical";
    char *passwd = "apicalgood";
    int   port   = 8080, i;
    MAINAPP app  = {};

    for (i = 1; i < argc; i++) {
        if (strstr(argv[i], "--ip="    ) == argv[i]) ip     = argv[i] + sizeof("--ip=" ) - 1;
        if (strstr(argv[i], "--dir="   ) == argv[i]) dir    = argv[i] + sizeof("--dir=") - 1;
        if (strstr(argv[i], "--port="  ) == argv[i]) port   = atoi(argv[i] + sizeof("--port=") - 1);
        if (strstr(argv[i], "--admin=" ) == argv[i]) admin  = argv[i] + sizeof("--admin=") - 1;
        if (strstr(argv[i], "--passwd=") == argv[i]) passwd = argv[i] + sizeof("--passwd=") - 1;
        if (strstr(argv[i], "--help") == argv[i]) {
            printf("ffhttpd version 1.0.0\n");
            printf("usage: ffhttpd [--ip=server_ip] [--port=server_port] [--dir==root_dir]\n");
            return 0;
        }
    }
    printf("server ip: %s, port: %d, dir: %s, admin: %s, passwd: %s\n", ip, port, dir, admin, passwd);
    strncpy(app.admin , admin , sizeof(app.admin ) - 1);
    strncpy(app.passwd, passwd, sizeof(app.passwd) - 1);

    signal(SIGINT , sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGPIPE, sig_handler);

    void *server = ffhttpd_init(ip, port, dir, 0, NULL, my_ffhttpd_cb, &app);
    ffhttpd_set(server, FFHTTPD_PARAM_RESPBUFSIZE, (void*)(64 * 1024));
    ffhttpd_set(server, FFHTTPD_PARAM_DIGESTAUTH , (void*)FFOSM_TRANS_PAGE);
    ffhttpd_set(server, FFHTTPD_PARAM_PASSWORDCB , (void*)my_userpasswd_callback);
    while (!s_exit) sleep(1);
    ffhttpd_exit(server);
    return 0;
}
