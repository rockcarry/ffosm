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

static char *s_header2 =
"HTTP/1.1 302 Moved Temporarily\r\n"
"Server: ffhttpd/1.0.0\r\n"
"Location: /ffosm.cgi\r\n\r\n";

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
        if (i) ret = snprintf(data->buf, data->len, "<td>%s</td>", argv[i]);
        else   ret = snprintf(data->buf, data->len, "<td>I%010d</td>", atoi(argv[i]));
        data->buf += ret;
        data->len -= ret;
    }

    ret = snprintf(data->buf, data->len,
        "<td>"
        "<a href='%s?action=putin&id=%s'>入库</a>&nbsp;"
        "<a href='%s?action=borrow&id=%s'>领用</a>&nbsp;"
        "<a href='%s?action=delete&id=%s'>删除</a>"
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
    int type = atoi(argv[1]);
    int ret  = snprintf(data->buf, data->len, "<tr>");
    data->buf += ret;
    data->len -= ret;
    for (int i = 0; i < argc && data->len > 0; i++) {
        if (i == 0) {
            ret = snprintf(data->buf, data->len, "<td>T%010d</td>", atoi(argv[i]));
        } else if (i == 1) {
            if (type < OP_PUTIN || type > OP_SCRAP) type = 0;
            static char *OP_STR_TAB[] = { "-", "入库", "领用", "归还", "报废" };
            ret = snprintf(data->buf, data->len, "<td>%s</td>", OP_STR_TAB[type]);
        } else {
            ret = snprintf(data->buf, data->len, "<td>%s</td>", argv[i]);
        }
        data->buf += ret;
        data->len -= ret;
    }
    if (type == OP_BORROW) {
        ret = snprintf(data->buf, data->len, "<td>"
            "<a href='%s?action=return&id=%s&user=%s&quantity=%s'>归还</a>&nbsp;"
            "<a href='%s?action=scrap&id=%s&user=%s&quantity=%s'>报废</a>"
            "</td>",
            FFOSM_TRANS_PAGE, argv[0], argv[2], argv[5],
            FFOSM_TRANS_PAGE, argv[0], argv[2], argv[5]);
    } else {
        ret = snprintf(data->buf, data->len, "<td>-</td>");
    }
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

static int main_page(char *buf, int len, char *formdata)
{
    char  buf_stock_table [32 * 1024];
    char  buf_record_table[32 * 1024];
    int   size = 0, ret;
    char *page = file_read("ffosm.p", &size);
    if (!page) return snprintf(buf, len, "<html></html>");

    char qs_name[65] = "", qs_remarks[256] = "";
    parse_params(formdata, "qs_name"   , qs_name   , sizeof(qs_name   ));
    parse_params(formdata, "qs_remarks", qs_remarks, sizeof(qs_remarks));
    printf("qs_name   : %s\n", qs_name   );
    printf("qs_remarks: %s\n", qs_remarks);

    char qt_type[12] = "", qt_user[65] = "", qt_id[12] = "", qt_remarks[256] = "";
    parse_params(formdata, "qt_type"   , qt_type   , sizeof(qt_type   ));
    parse_params(formdata, "qt_user"   , qt_user   , sizeof(qt_user   ));
    parse_params(formdata, "qt_id"     , qt_id     , sizeof(qt_id     ));
    parse_params(formdata, "qt_remarks", qt_remarks, sizeof(qt_remarks));
    printf("qt_type   : %s\n", qt_type   );
    printf("qt_user   : %s\n", qt_user   );
    printf("qt_id     : %s\n", qt_id     );
    printf("qt_remarks: %s\n", qt_remarks);

    stock_table (buf_stock_table , sizeof(buf_stock_table ), qs_name, qs_remarks);
    record_table(buf_record_table, sizeof(buf_record_table), atoi(qt_type), qt_user, atoi(qt_id), qt_remarks);
    ret = snprintf(buf, len, page, qt_type, qs_name, qs_remarks, buf_stock_table, qt_user, qt_id, qt_remarks, buf_record_table);

    free(page);
    return ret;
}

static int transact_page(char *buf, int len, char *formdata)
{
    char action[32] = "", name[65] = "", price[32] = "", user[65] = "", quantity[12] = "", remarks[256] = "", id[12] = "", submit[12] = "";
    int  size = 0, ret = 0;
    parse_params(formdata, "action"  , action  , sizeof(action  ));
    parse_params(formdata, "name"    , name    , sizeof(name    ));
    parse_params(formdata, "price"   , price   , sizeof(price   ));
    parse_params(formdata, "user"    , user    , sizeof(user    ));
    parse_params(formdata, "quantity", quantity, sizeof(quantity));
    parse_params(formdata, "remarks" , remarks , sizeof(remarks ));
    parse_params(formdata, "id"      , id      , sizeof(id      ));
    parse_params(formdata, "submit"  , submit  , sizeof(submit  ));

    if (strcmp(action, "create") == 0) {
        void *osm = ffosm_init(FFOSM_DATABASE);
        ret = ffosm_new_item(osm, name, 0, atof(price), remarks);
        ffosm_exit(osm);
        if (ret == 0) return -1; // redirect to main page
    } else if (strcmp(action, "delete") == 0) {
        void *osm = ffosm_init(FFOSM_DATABASE);
        ret = ffosm_del_item(osm, atoi(id));
        ffosm_exit(osm);
        if (ret == 0) return -1; // redirect to main page
    } else if (strcmp(action, "putin") == 0) {
        if (strcmp(submit, "1") == 0) {
            void *osm = ffosm_init(FFOSM_DATABASE);
            int   ret = ffosm_putin(osm, user, atoi(id), atoi(quantity), remarks);
            ffosm_exit(osm);
            if (ret == 0) return -1; // redirect to main page
        } else {
            char *page = file_read("transact.p", &size);
            ret = snprintf(buf, len, page, "物品入库", "物品编码：", id, user, quantity, action, id);
            free(page);
        }
    } else if (strcmp(action, "borrow") == 0) {
        if (strcmp(submit, "1") == 0) {
            void *osm = ffosm_init(FFOSM_DATABASE);
            int   ret = ffosm_borrow(osm, user, atoi(id), atoi(quantity), remarks);
            ffosm_exit(osm);
            if (ret == 0) return -1; // redirect to main page
        } else {
            char *page = file_read("transact.p", &size);
            ret = snprintf(buf, len, page, "物品领用", "物品编码：", id, user, quantity, action, id);
            free(page);
        }
    } else if (strcmp(action, "return") == 0) {
        if (strcmp(submit, "1") == 0) {
            void *osm = ffosm_init(FFOSM_DATABASE);
            int   ret = ffosm_return(osm, atoi(id), atoi(quantity), remarks);
            ffosm_exit(osm);
            if (ret == 0) return -1; // redirect to main page
        } else {
            char *page = file_read("transact.p", &size);
            ret = snprintf(buf, len, page, "物品归还", "交易流水：", id, user, quantity, action, id);
            free(page);
        }
    } else if (strcmp(action, "scrap") == 0) {
        if (strcmp(submit, "1") == 0) {
            void *osm = ffosm_init(FFOSM_DATABASE);
            int   ret = ffosm_scrap(osm, atoi(id), atoi(quantity), remarks);
            ffosm_exit(osm);
            if (ret == 0) return -1; // redirect to main page
        } else {
            char *page = file_read("transact.p", &size);
            ret = snprintf(buf, len, page, "物品报废", "交易流水：", id, user, quantity, action, id);
            free(page);
        }
    }

    if (ret <= 0) ret = snprintf(buf, len, "<html>:-( &nbsp; 操作失败！&nbsp;<a href=\"/ffosm.cgi\">[返回主页面]</a></html>");
    return ret;
}

static int my_ffhttpd_cb(void *cbctx, int id, int cmd, char *path, char *postdata, int postlen, char *respbuf, int resplen)
{
    char buf[64 * 1024];
    int  len;
    if (strcmp(path, "/index.html") == 0 || strncmp(path, FFOSM_MAIN_PAGE, sizeof(FFOSM_MAIN_PAGE) - 1) == 0) {
        switch (cmd) {
        case TYPE_REQUEST_GET:
        case TYPE_REQUEST_POST:
            ffhttpd_url_decode(postdata);
            len = main_page(buf, sizeof(buf), path);
            snprintf(respbuf, resplen, s_header1, len, buf);
            return 0;
        }
    } else if (strncmp(path, FFOSM_TRANS_PAGE, sizeof(FFOSM_TRANS_PAGE) - 1) == 0) {
        switch (cmd) {
        case TYPE_REQUEST_GET:
        case TYPE_REQUEST_POST:
            ffhttpd_url_decode(postdata);
            len = transact_page(buf, sizeof(buf), path);
            if (len == -1) {
                strncpy(respbuf, s_header2, resplen - 1);
            } else {
                snprintf(respbuf, resplen, s_header1, len, buf);
            }
            return 0;
        }
    }
    return -1;
}

static int my_userpasswd_callback(void *cbctx, char *username, char *passwd, int size)
{
    MAINAPP *app = cbctx;
    if (strcmp(username, app->admin) == 0) {
        strncpy(passwd, app->passwd, size - 1);
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
            printf("usage: ffhttpd [--ip=server_ip] [--port=server_port] [--dir==root_dir] [--admin=admin] [--passwd=passwd]\n");
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
    if (strlen(passwd) > 0) {
        ffhttpd_set(server, FFHTTPD_PARAM_DIGESTAUTH , (void*)FFOSM_TRANS_PAGE);
        ffhttpd_set(server, FFHTTPD_PARAM_PASSWORDCB , (void*)my_userpasswd_callback);
    }
    while (!s_exit) sleep(1);
    ffhttpd_exit(server);
    return 0;
}
