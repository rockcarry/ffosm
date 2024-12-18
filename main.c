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
#define FFOSM_PAGE_SIZE   10

typedef struct {
    char *buf;
    int   len;
} CBDATA;

typedef struct {
    char admin [64];
    char passwd[64];
} MAINAPP;

static char* get_page_param(char *path, char *post, char *key, char *val, int len)
{
    if (*post) parse_params(post, key, val, len);
    if (!*val) parse_params(path, key, val, len);
    return val;
}

static int query_inventory_cb(void *cbctx, int argc, char **argv, char **colname)
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
    if (data->len > 0) {
        ret = snprintf(data->buf, data->len,
            "<td>"
            "<a href='%s?action=%s&id=I%010d'>入库</a>&nbsp;"
            "<a href='%s?action=%s&id=I%010d'>借出</a>"
            "</td>",
            FFOSM_TRANS_PAGE, "putin", atoi(argv[0]),
            FFOSM_TRANS_PAGE, "lend" , atoi(argv[0]));
        data->buf += ret;
        data->len -= ret;
    }
    if (data->len > 0) {
        ret = snprintf(data->buf, data->len, "</tr>\n");
        data->buf += ret;
        data->len -= ret;
    }
    return 0;
}

static int query_borrowed_cb(void *cbctx, int argc, char **argv, char **colname)
{
    CBDATA *data = cbctx;
    int ret = snprintf(data->buf, data->len, "<tr>");
    data->buf += ret;
    data->len -= ret;
    for (int i = 0; i < argc && data->len > 0; i++) {
        if (i == 2) {
            ret = snprintf(data->buf, data->len, "<td>I%010d</td>", atoi(argv[i]));
        } else {
            ret = snprintf(data->buf, data->len, "<td>%s</td>", argv[i]);
        }
        data->buf += ret;
        data->len -= ret;
    }
    if (data->len > 0) {
        ret = snprintf(data->buf, data->len,
            "<td>"
            "<a href='%s?user=%s&action=%s&id=I%010d&quantity=%s'>归还</a>&nbsp;"
            "<a href='%s?user=%s&action=%s&id=I%010d&quantity=%s'>报废</a>"
            "</td>",
            FFOSM_TRANS_PAGE, argv[0], "return", atoi(argv[2]), argv[3],
            FFOSM_TRANS_PAGE, argv[0], "scrap" , atoi(argv[2]), argv[3]);
        data->buf += ret;
        data->len -= ret;
    }
    if (data->len > 0) {
        ret = snprintf(data->buf, data->len, "</tr>\n");
        data->buf += ret;
        data->len -= ret;
    }
    return 0;
}

static int query_transaction_cb(void *cbctx, int argc, char **argv, char **colname)
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
            static char *OP_STR_TAB[] = { "-", "入库", "借出", "归还", "报废" };
            ret = snprintf(data->buf, data->len, "<td>%s</td>", OP_STR_TAB[type]);
        } else if (i == 3) {
            ret = snprintf(data->buf, data->len, "<td>I%010d</td>", atoi(argv[i]));
        } else if (i == 5) {
            ret = snprintf(data->buf, data->len, "<td>%d</td>", abs(atoi(argv[i])));
        } else {
            ret = snprintf(data->buf, data->len, "<td>%s</td>", argv[i]);
        }
        data->buf += ret;
        data->len -= ret;
    }
    if (data->len > 0) {
        ret = snprintf(data->buf, data->len, "</tr>\n");
        data->buf += ret;
        data->len -= ret;
    }
    return 0;
}

static int gen_page_bar(char *buf, int len, int type, int pagenum, int pageidx)
{
    if (pagenum < 2) return -1;
    int pagemin, pagemax, ret, i;
    pageidx = pageidx > 1 ? pageidx : 1;
    pageidx = pageidx <= pagenum ? pageidx : pagenum;
    pagemin = pageidx - 10 / 2;
    pagemin = pagemin > 1 ? pagemin : 1;
    pagemax = pagemin + 10 / 2;
    pagemax = pagemax <= pagenum ? pagemax : pagenum;
    if (len > 0) {
        ret = snprintf(buf, len, "<tr><td colspan=\"%d\">\n", 100);
        buf += ret, len -= ret;
    }
    if (len > 0 && pageidx - 1 >= 1) {
        ret = snprintf(buf, len, "<a href=\"javascript:void(0);\" onclick=\"change_page(%d,%d)\" style=\"text-decoration:none;\">&lt;</a>&nbsp;\n", type, pageidx - 1);
        buf += ret, len -= ret;
    }
    for (i = pagemin; i <= pagemax && len > 0; i++) {
        if (i != pageidx) {
            ret = snprintf(buf, len, "<a href=\"javascript:void(0);\" onclick=\"change_page(%d,%d)\">%d</a>&nbsp;\n", type, i, i);
        } else {
            ret = snprintf(buf, len, "%d&nbsp;\n", i);
        }
        buf += ret, len -= ret;
    }
    if (len > 0 && pageidx + 1 <= pagenum) {
        ret = snprintf(buf, len, "<a href=\"javascript:void(0);\" onclick=\"change_page(%d,%d)\" style=\"text-decoration:none;\">&gt;</a>&nbsp;\n", type, pageidx + 1);
        buf += ret, len -= ret;
    }
    if (len > 0) {
        ret = snprintf(buf, len, "%s", "</td></tr>\n");
        buf += ret, len -= ret;
    }
    return 0;
}

static int inventory_table(char *buf, int len, int page, char *name, char *remarks)
{
    void *osm = ffosm_init(FFOSM_DATABASE);
    CBDATA cbdata = { buf, len };
    int total = ffosm_query_inventory(osm, NULL, NULL, -1, 0, name, remarks);
    ffosm_query_inventory(osm, query_inventory_cb, &cbdata, FFOSM_PAGE_SIZE, page, name, remarks);
    ffosm_exit(osm);
    gen_page_bar(cbdata.buf, cbdata.len, 0, (total + FFOSM_PAGE_SIZE - 1) / FFOSM_PAGE_SIZE, page);
    return 0;
}

static int borrowed_table(char *buf, int len, int page, char *user, char *name)
{
    void *osm = ffosm_init(FFOSM_DATABASE);
    CBDATA cbdata = { buf, len };
    int total = ffosm_query_borrowed(osm, NULL, NULL, -1, 0, user, name);
    ffosm_query_borrowed(osm, query_borrowed_cb, &cbdata, FFOSM_PAGE_SIZE, page, user, name);
    ffosm_exit(osm);
    gen_page_bar(cbdata.buf, cbdata.len, 1, (total + FFOSM_PAGE_SIZE - 1) / FFOSM_PAGE_SIZE, page);
    return 0;
}

static int transaction_table(char *buf, int len, int page, int type, char *user, char *name, char *remarks)
{
    void *osm = ffosm_init(FFOSM_DATABASE);
    CBDATA cbdata = { buf, len };
    int total = ffosm_query_transaction(osm, NULL, NULL, -1, 0, type, user, name, remarks);
    ffosm_query_transaction(osm, query_transaction_cb, &cbdata, FFOSM_PAGE_SIZE, page, type, user, name, remarks);
    ffosm_exit(osm);
    gen_page_bar(cbdata.buf, cbdata.len, 2, (total + FFOSM_PAGE_SIZE - 1) / FFOSM_PAGE_SIZE, page);
    return 0;
}

static int main_page(char *buf, int len, char *path, char *post)
{
    char  buf_inventory_table  [64 * 1024];
    char  buf_borrowed_table   [64 * 1024];
    char  buf_transaction_table[64 * 1024];
    char *page = file_read("ffosm.p", NULL);
    if (!page) return snprintf(buf, len, "<html></html>");

    char qs_page[12] = "", qs_name[64] = "", qs_remarks[256] = "";
    get_page_param(path, post, "qs_page"   , qs_page   , sizeof(qs_page   ));
    get_page_param(path, post, "qs_name"   , qs_name   , sizeof(qs_name   ));
    get_page_param(path, post, "qs_remarks", qs_remarks, sizeof(qs_remarks));
    if (!*qs_page) *qs_page = '1';
    printf("qs_page: %s, qs_name: %s, qs_remarks: %s\n", qs_page, qs_name, qs_remarks);

    char qb_page[12] = "", qb_user[32] = "", qb_name[64] = "";
    get_page_param(path, post, "qb_page", qb_page, sizeof(qb_page));
    get_page_param(path, post, "qb_user", qb_user, sizeof(qb_user));
    get_page_param(path, post, "qb_name", qb_name, sizeof(qb_name));
    if (!*qb_page) *qb_page = '1';
    printf("qb_page: %s, qb_user: %s, qb_name: %s\n", qb_page, qb_user, qb_name);

    char qt_page[12] = "", qt_type[12] = "", qt_user[64] = "", qt_name[64] = "", qt_remarks[256] = "";
    get_page_param(path, post, "qt_page"   , qt_page   , sizeof(qt_page   ));
    get_page_param(path, post, "qt_type"   , qt_type   , sizeof(qt_type   ));
    get_page_param(path, post, "qt_user"   , qt_user   , sizeof(qt_user   ));
    get_page_param(path, post, "qt_name"   , qt_name   , sizeof(qt_name   ));
    get_page_param(path, post, "qt_remarks", qt_remarks, sizeof(qt_remarks));
    if (!*qt_page) *qt_page = '1';
    printf("qt_page: %s, qt_type: %s, qt_user: %s, qt_name: %s, qt_remarks: %s\n", qt_page, qt_type, qt_user, qt_name, qt_remarks);

    inventory_table  (buf_inventory_table  , sizeof(buf_inventory_table  ), atoi(qs_page), qs_name, qs_remarks);
    borrowed_table   (buf_borrowed_table   , sizeof(buf_borrowed_table   ), atoi(qb_page), qb_user, qb_name);
    transaction_table(buf_transaction_table, sizeof(buf_transaction_table), atoi(qt_page), atoi(qt_type), qt_user, qt_name, qt_remarks);
    int ret = snprintf(buf, len, page, qt_type, qs_page, qs_name, qs_remarks, buf_inventory_table, qb_page, qb_user, qb_name, buf_borrowed_table, qt_page, qt_user, qt_name, qt_remarks, buf_transaction_table);

    free(page);
    return ret;
}

static int transact_page(char *buf, int len, char *path, char *post, char *adminpasswd)
{
    char  action[32] = "", name[64] = "", price[32] = "", user[64] = "", quantity[12] = "", remarks[256] = "", id[12] = "", submit[12] = "", formpasswd[64] = "", *page = NULL;
    int   ret = 0;
    get_page_param(path, post, "action"  , action    , sizeof(action    ));
    get_page_param(path, post, "name"    , name      , sizeof(name      ));
    get_page_param(path, post, "price"   , price     , sizeof(price     ));
    get_page_param(path, post, "user"    , user      , sizeof(user      ));
    get_page_param(path, post, "quantity", quantity  , sizeof(quantity  ));
    get_page_param(path, post, "remarks" , remarks   , sizeof(remarks   ));
    get_page_param(path, post, "id"      , id        , sizeof(id        ));
    get_page_param(path, post, "submit"  , submit    , sizeof(submit    ));
    get_page_param(path, post, "passwd"  , formpasswd, sizeof(formpasswd));
    printf("action: %s, name: %s, price: %s, user: %s, quantity: %s, remarks: %s, id: %s, submit: %s, passwd: %s\n", action, name, price, user, quantity, remarks, id, submit, formpasswd);

    if (strcmp(action, "create") == 0) {
        void *osm = ffosm_init(FFOSM_DATABASE);
        ret = ffosm_new_item(osm, name, atof(price), remarks);
        ffosm_exit(osm);
        if (ret == 0) return -1; // redirect to main page
    } else if (strcmp(action, "delete") == 0 && strcmp(adminpasswd, formpasswd) == 0) {
        void *osm = ffosm_init(FFOSM_DATABASE);
        ret = ffosm_del_item(osm, atoi(id + 1));
        ffosm_exit(osm);
        if (ret == 0) return -1; // redirect to main page
    } else if (strcmp(action, "putin") == 0) {
        if (strcmp(submit, "1") == 0) {
            void *osm = ffosm_init(FFOSM_DATABASE);
            int   ret = ffosm_putin(osm, user, atoi(id + 1), atoi(quantity), remarks);
            ffosm_exit(osm);
            if (ret == 0) return -1; // redirect to main page
        } else {
            page = file_read("transact.p", NULL);
            if (page) ret = snprintf(buf, len, page, "物品入库", "物品编码：", id, user, quantity, action, id);
        }
    } else if (strcmp(action, "lend") == 0) {
        if (strcmp(submit, "1") == 0) {
            void *osm = ffosm_init(FFOSM_DATABASE);
            int   ret = ffosm_lend(osm, user, atoi(id + 1), atoi(quantity), remarks);
            ffosm_exit(osm);
            if (ret == 0) return -1; // redirect to main page
        } else {
            page = file_read("transact.p", NULL);
            if (page) ret = snprintf(buf, len, page, "物品借出", "物品编码：", id, user, quantity, action, id);
        }
    } else if (strcmp(action, "return") == 0) {
        if (strcmp(submit, "1") == 0) {
            void *osm = ffosm_init(FFOSM_DATABASE);
            int   ret = ffosm_return(osm, user, atoi(id + 1), atoi(quantity), remarks);
            ffosm_exit(osm);
            if (ret == 0) return -1; // redirect to main page
        } else {
            page = file_read("transact.p", NULL);
            if (page) ret = snprintf(buf, len, page, "物品归还", "交易流水：", id, user, quantity, action, id);
        }
    } else if (strcmp(action, "scrap") == 0) {
        if (strcmp(submit, "1") == 0) {
            void *osm = ffosm_init(FFOSM_DATABASE);
            int   ret = ffosm_scrap(osm, user, atoi(id + 1), atoi(quantity), remarks);
            ffosm_exit(osm);
            if (ret == 0) return -1; // redirect to main page
        } else {
            page = file_read("transact.p", NULL);
            if (page) ret = snprintf(buf, len, page, "物品报废", "交易流水：", id, user, quantity, action, id);
        }
    }

    if (ret <= 0) ret = snprintf(buf, len, "<html><head><meta charset=\"utf-8\"></head>:-( &nbsp; 操作失败！&nbsp;<a href=\"/ffosm.cgi\">[返回主页面]</a></html>");
    free(page);
    return ret;
}

static int my_ffhttpd_cb(void *cbctx, int id, int cmd, char *path, char *postdata, int postlen, char *respbuf, int resplen)
{
    MAINAPP *app = cbctx;
    char buf[64 * 1024];
    int  len;
    if (strcmp(path, "/index.html") == 0 || strncmp(path, FFOSM_MAIN_PAGE, sizeof(FFOSM_MAIN_PAGE) - 1) == 0) {
        switch (cmd) {
        case TYPE_REQUEST_GET:
        case TYPE_REQUEST_POST:
            ffhttpd_url_decode(postdata);
            len = main_page(buf, sizeof(buf), path, postdata);
            snprintf(respbuf, resplen, s_header1, len, buf);
            return 0;
        }
    } else if (strncmp(path, FFOSM_TRANS_PAGE, sizeof(FFOSM_TRANS_PAGE) - 1) == 0) {
        switch (cmd) {
        case TYPE_REQUEST_GET:
        case TYPE_REQUEST_POST:
            ffhttpd_url_decode(postdata);
            len = transact_page(buf, sizeof(buf), path, postdata, app->passwd);
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
        if (strstr(argv[i], "--admin=" ) == argv[i]) admin  = argv[i] + sizeof("--admin=" ) - 1;
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
    if (1 && strlen(passwd) > 0) {
        ffhttpd_set(server, FFHTTPD_PARAM_DIGESTAUTH , (void*)FFOSM_TRANS_PAGE);
        ffhttpd_set(server, FFHTTPD_PARAM_PASSWORDCB , (void*)my_userpasswd_callback);
    }
    while (!s_exit) sleep(1);
    ffhttpd_exit(server);
    return 0;
}
