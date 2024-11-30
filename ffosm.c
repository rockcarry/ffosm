#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include "ffosm.h"
#include "utils.h"

void* ffosm_init(char *file)
{
    char *sql1 = "create table tab_inventory ("
                 "id          integer  primary key autoincrement,"
                 "item_name   char(64) not null,"
                 "total_putin integer  default 0,"
                 "total_lend  integer  default 0,"
                 "total_scrap integer  default 0,"
                 "total_stock integer  default 0,"
                 "price       real     default 0,"
                 "remarks     text"
                 ");";
    char *sql2 = "create table tab_transaction ("
                 "id       integer  primary key autoincrement,"
                 "type     integer  default 0,"
                 "user     char(32) not null,"
                 "item_id  integer  not null,"
                 "quantity integer  default 0,"
                 "datetime datetime default 0,"
                 "remarks  text"
                 ");";
    int      exists = file_exists(file);
    sqlite3 *db     = NULL;
    char    *err    = NULL;
    int      rc     = sqlite3_open(file, &db);
    if (rc) {
        printf("failed to open database !\n");
        goto failed;
    }
    if (exists <= 0) {
        rc = sqlite3_exec(db, sql1, NULL, NULL, &err);
        if (rc) {
            printf("failed to create tab_inventory !\n");
            printf("err: %s\n", err);
            goto failed;
        }

        rc = sqlite3_exec(db, sql2, NULL, NULL, &err);
        if (rc) {
            printf("failed to create tab_transaction !\n");
            printf("err: %s\n", err);
            goto failed;
        }
    }
    return db;

failed:
    if (db) sqlite3_close(db);
    return NULL;
}

void ffosm_exit(void *ctx)
{
    if (!ctx) return;
    sqlite3 *db = ctx;
    sqlite3_close(db);
}

int ffosm_new_item(void *ctx, char *name, float price, char *remarks)
{
    if (!ctx || !name || !name[0]) return -1;
    sqlite3 *db  = ctx;
    char    *err = NULL;
    char     sql[256];
    int      rc;
    snprintf(sql, sizeof(sql), "insert into tab_inventory (item_name, price, remarks) VALUES ('%s', %f, '%s');",
        name, price, remarks ? remarks : "");
    rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if (rc) {
        printf("failed to new item !\n");
        printf("sql: %s\n", sql);
        printf("err: %s\n", err);
        return -1;
    }
    return 0;
}

int ffosm_del_item(void *ctx, int id)
{
    if (!ctx || id <= 0) return -1;
    sqlite3 *db  = ctx;
    char    *err = NULL;
    char     sql[256];
    int      rc;
    snprintf(sql, sizeof(sql), "begin; delete from tab_inventory where id = %d; delete from tab_transaction where item_id = %d; commit;", id, id);
    rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if (rc) {
        printf("failed to del item !\n");
        printf("sql: %s\n", sql);
        printf("err: %s\n", err);
        return -1;
    }
    return 0;
}

static int callback_query_stock(void *data, int argc, char **argv, char **colname)
{
    for (int i = 0; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");
    return 0;
}

int ffosm_query_inventory(void *ctx, FFOSM_QUERY_CB callback, void *cbctx, char *name, char *remarks)
{
    if (!ctx) return -1;
    sqlite3 *db  = ctx;
    char    *err = NULL;
    char     sql[512];
    char    *str = sql;
    int      len = sizeof(sql);
    int      ret, rc;
    callback = callback ? callback : callback_query_stock;
    ret = snprintf(str, len, "select id, item_name, total_putin, total_lend, total_scrap, total_stock, price, remarks from tab_inventory where true");
    str += ret, len -= ret;
    if (name && strcmp(name, "") != 0 && strcmp(name, "*") != 0) {
        ret = snprintf(str, len, " and item_name like '%%%s%%'", name);
        str += ret, len -= ret;
    }
    if (remarks && strcmp(remarks, "") != 0 && strcmp(remarks, "*") != 0) {
        ret = snprintf(str, len, " and remarks like '%%%s%%'", remarks);
        str += ret, len -= ret;
    }
    ret = snprintf(str, len, " order by id desc;");
    str += ret, len -= ret;
//  printf("sql: %s\n", sql);
    printf("inventory list:\n");
    rc = sqlite3_exec(db, sql, callback, cbctx, &err);
    if (rc) {
        printf("failed to query inventory !\n");
        printf("sql: %s\n", sql);
        printf("err: %s\n", err);
        return -1;
    }
    return 0;
}

int ffosm_query_borrowed(void *ctx, FFOSM_QUERY_CB callback, void *cbctx, char *user, char *name)
{
    if (!ctx) return -1;
    sqlite3 *db  = ctx;
    char    *err = NULL;
    char     sql[512];
    char    *str = sql;
    int      len = sizeof(sql);
    int      ret, rc;
    callback = callback ? callback : callback_query_stock;
    ret = snprintf(str, len, "select distinct user, item_name, item_id, sum(quantity) from tab_transaction join tab_inventory on tab_transaction.item_id = tab_inventory.id where type > 1");
    str += ret, len -= ret;
    if (user && strcmp(user, "") != 0 && strcmp(user, "*") != 0) {
        ret = snprintf(str, len, " and user like '%%%s%%'", user);
        str += ret, len -= ret;
    }
    if (name && strcmp(name, "") != 0 && strcmp(name, "*") != 0) {
        ret = snprintf(str, len, " and item_name like '%%%s%%'", name);
        str += ret, len -= ret;
    }
    ret = snprintf(str, len, " group by user, item_id having sum(quantity) > 0;");
    str += ret, len -= ret;
//  printf("sql: %s\n", sql);
    printf("transaction list:\n");
    rc = sqlite3_exec(db, sql, callback, cbctx, &err);
    if (rc) {
        printf("failed to query borrowed !\n");
        printf("sql: %s\n", sql);
        printf("err: %s\n", err);
        return -1;
    }
    return 0;
}

int ffosm_query_transaction(void *ctx, FFOSM_QUERY_CB callback, void *cbctx, int type, char *user, char *name, char *remarks)
{
    if (!ctx) return -1;
    sqlite3 *db  = ctx;
    char    *err = NULL;
    char     sql[512];
    char    *str = sql;
    int      len = sizeof(sql);
    int      ret, rc;
    callback = callback ? callback : callback_query_stock;
    ret = snprintf(str, len, "select distinct tab_transaction.id, type, user, item_id, item_name, quantity, datetime(datetime, 'unixepoch', 'localtime'), tab_transaction.remarks from tab_transaction join tab_inventory ON tab_transaction.item_id = tab_inventory.id where true");
    str += ret, len -= ret;
    if (type > 0) {
        ret = snprintf(str, len, " and type = %d", type);
        str += ret, len -= ret;
    }
    if (user && strcmp(user, "") != 0 && strcmp(user, "*") != 0) {
        ret = snprintf(str, len, " and user = '%s'", user);
        str += ret, len -= ret;
    }
    if (name && strcmp(name, "") != 0 && strcmp(name, "*") != 0) {
        ret = snprintf(str, len, " and item_id in (select id from tab_inventory where item_name like '%%%s%%')", name);
        str += ret, len -= ret;
    }
    if (remarks && strcmp(remarks, "") != 0 && strcmp(remarks, "*") != 0) {
        ret = snprintf(str, len, " and tab_transaction.remarks like '%%%s%%'", remarks);
        str += ret, len -= ret;
    }
    ret = snprintf(str, len, " order by tab_transaction.id desc;");
    str += ret, len -= ret;
//  printf("sql: %s\n", sql);
    printf("transaction list:\n");
    rc = sqlite3_exec(db, sql, callback, cbctx, &err);
    if (rc) {
        printf("failed to query transaction !\n");
        printf("sql: %s\n", sql);
        printf("err: %s\n", err);
        return -1;
    }
    return 0;
}

static int putin_or_lend(void *ctx, int op, char *user, int itemid, int quantity, char *remarks)
{
    if (!ctx || !user || !*user || itemid <= 0 || quantity <= 0) return -1;
    sqlite3 *db  = ctx;
    char    *err = NULL;
    char     sql[512];
    int      rc;
    snprintf(sql, sizeof(sql),
        "begin;\n"
        "insert into tab_transaction (type, user, item_id, quantity, datetime, remarks)"
        "  select %d, '%s', tab_inventory.id, %d, strftime('%%s','now'), '%s'\n"
        "    from tab_inventory where id = %d and total_stock >= %d;\n"
        "update tab_inventory set total_putin = total_putin + %d, total_lend = total_lend + %d, total_stock = total_stock %c %d"
        "  where id = %d and total_stock >= %d;\n"
        "commit;\n",
        op, user, quantity, remarks, itemid, op == OP_PUTIN ? 0 : quantity,
        op == OP_PUTIN ? quantity : 0,
        op == OP_LEND  ? quantity : 0,
        op == OP_PUTIN ? '+' : '-', quantity,
        itemid, op == OP_PUTIN ? 0 : quantity);
    rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if (rc) {
        printf("failed to putin_or_lend !\n");
        printf("sql: %s\n", sql);
        printf("err: %s\n", err);
        return -1;
    }
    return 0;
}

int ffosm_putin(void *ctx, char *user, int itemid, int quantity, char *remarks)
{
    return putin_or_lend(ctx, OP_PUTIN , user, itemid, quantity, remarks);
}

int ffosm_lend(void *ctx, char *user, int itemid, int quantity, char *remarks)
{
    return putin_or_lend(ctx, OP_LEND, user, itemid, quantity, remarks);
}

static int callback_query_quantity(void *data, int argc, char **argv, char **colname)
{
    int *quantity = data;
    if (quantity) *quantity = atoi(argv[0]);
    return 0;
}

static int return_or_scrap(void *ctx, int op, char *user, int itemid, int quantity, char *remarks)
{
    if (!ctx || !user || !*user || itemid <= 0 || quantity <= 0) return -1;
    sqlite3 *db  = ctx;
    char    *err = NULL;
    char     sql[512];
    int      borrow = 0;
    int      rc;

    snprintf(sql, sizeof(sql), "select sum(quantity) from tab_transaction where user = '%s' and item_id = %d and type > 1;", user, itemid);
//  printf("sql: %s\n", sql);
    rc = sqlite3_exec(db, sql, callback_query_quantity, &borrow, &err);
    if (rc) {
        printf("failed to query borrow quantity !\n");
        printf("sql: %s\n", sql);
        printf("err: %s\n", err);
        return -1;
    }
    if (borrow < quantity) {
        printf("borrow quantity %d is less than return or scrap quantity %d !\n", borrow, quantity);
        return -1;
    }

    snprintf(sql, sizeof(sql),
        "begin;\n"
        "insert into tab_transaction (type, user, item_id, quantity, datetime, remarks) values (%d, '%s', %d, %d, strftime('%%s','now'), '%s');\n"
        "update tab_inventory set total_lend = total_lend - %d, total_scrap = total_scrap + %d, total_stock = total_stock + %d where id = %d;\n"
        "commit;\n",
        op, user, itemid, -quantity, remarks, quantity,
        op == OP_SCRAP  ? quantity : 0,
        op == OP_RETURN ? quantity : 0,
        itemid);
    rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if (rc) {
        printf("failed to return_or_scrap !\n");
        printf("sql: %s\n", sql);
        printf("err: %s\n", err);
        return -1;
    }
    return 0;
}

int ffosm_return(void *ctx, char *user, int itemid, int quantity, char *remarks)
{
    return return_or_scrap(ctx, OP_RETURN, user, itemid, quantity, remarks);
}

int ffosm_scrap(void *ctx, char *user, int itemid, int quantity, char *remarks)
{
    return return_or_scrap(ctx, OP_SCRAP, user, itemid, quantity, remarks);
}

#ifdef _TEST_
int main(int argc, char *argv[])
{
    char *file = "ffosm.db";
    void *osm  = NULL;
    int   ret;
    if (argc > 1) file = argv[1];

    osm = ffosm_init(file);
    while (1) {
        char cmd[256];
        scanf("%255s", cmd);
        if (strcmp(cmd, "new") == 0) {
            char name[65] = ""; float price = 0; char remarks[256] = "";
            scanf("%64s %f %255s", name, &price, remarks);
            ret = ffosm_new_item(osm, name, price, remarks);
            printf("new item, ret: %d\n", ret);
        } else if (strcmp(cmd, "del") == 0) {
            int id = -1;
            scanf("%d", &id);
            ret = ffosm_del_item(osm, id);
            printf("del item, ret: %d\n", ret);
        } else if (strcmp(cmd, "query_stock" ) == 0) {
            char name[65] = "", remarks[256] = "";
            scanf("%64s %255s", name, remarks);
            ret = ffosm_query_inventory(osm, NULL, NULL, name, remarks);
            printf("query stock, ret: %d\n", ret);
        } else if (strcmp(cmd, "query_record" ) == 0) {
            int type = -1; char user[33] = "", name[65] = "", remarks[256] = "";
            scanf("%d %32s %64s %255s", &type, user, name, remarks);
            ret = ffosm_query_transaction(osm, NULL, NULL, type, user, name, remarks);
            printf("query record, ret: %d\n", ret);
        } else if (strcmp(cmd, "putin") == 0) {
            char user[33] = "", remarks[256] = ""; int itemid = -1, quantity = -1;
            scanf("%32s %d %d %255s", user, &itemid, &quantity, remarks);
            ret = ffosm_putin(osm, user, itemid, quantity, remarks);
            printf("putin, ret: %d\n", ret);
        } else if (strcmp(cmd, "lend") == 0) {
            char user[33] = "", remarks[256] = ""; int itemid = -1, quantity = -1;
            scanf("%32s %d %d %255s", user, &itemid, &quantity, remarks);
            ret = ffosm_lend(osm, user, itemid, quantity, remarks);
            printf("lend, ret: %d\n", ret);
        } else if (strcmp(cmd, "return") == 0) {
            char user[33] = "", remarks[256] = ""; int itemid = -1, quantity = -1;
            scanf("%32s %d %d %255s", user, &itemid, &quantity, remarks);
            ret = ffosm_return(osm, user, itemid, quantity, remarks);
            printf("return, ret: %d\n", ret);
        } else if (strcmp(cmd, "scrap") == 0) {
            char user[33] = "", remarks[256] = ""; int itemid = -1, quantity = -1;
            scanf("%32s %d %d %255s", user, &itemid, &quantity, remarks);
            ret = ffosm_scrap(osm, user, itemid, quantity, remarks);
            printf("scrap, ret: %d\n", ret);
        } else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            break;
        }
    }
    ffosm_exit(osm);
    return 0;
}
#endif
