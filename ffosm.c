#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include "ffosm.h"
#include "utils.h"

void* ffosm_init(char *file)
{
    char *sql1 = "create table StockTab ("
                 "Id       integer  primary key autoincrement,"
                 "ItemName char(65) not null,"
                 "Quantity integer  default 0,"
                 "Price    real     default 0.0,"
                 "Remarks  text"
                 ");";
    char *sql2 = "create table RecordTab ("
                 "Id       integer  primary key autoincrement,"
                 "Type     integer  default 0,"
                 "User     char(33) not null,"
                 "ItemId   integer  not null,"
                 "Quantity integer  default 0,"
                 "Datetime datetime default 0,"
                 "Remarks  text"
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
            printf("failed to create table StockTab !\n");
            printf("err: %s\n", err);
            goto failed;
        }

        rc = sqlite3_exec(db, sql2, NULL, NULL, &err);
        if (rc) {
            printf("failed to create table TransactionTab !\n");
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

int ffosm_new_item(void *ctx, char *name, int quantity, float price, char *remarks)
{
    if (!ctx || !name || !name[0]) return -1;
    sqlite3 *db  = ctx;
    char    *err = NULL;
    char     sql[256];
    int      rc;
    snprintf(sql, sizeof(sql), "INSERT INTO StockTab (ItemName, Quantity, Price, Remarks) VALUES ('%s', %d, %f, '%s');",
        name, quantity, price, remarks ? remarks : "");
    rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if (rc) {
        printf("failed to new item !\n");
        printf("sql: %s\n", sql);
        printf("err: %s\n", err);
        return -1;
    }
    return 0;
}

int ffosm_mod_item(void *ctx, int id, int quantity)
{
    if (!ctx || id < 0 || quantity < 0) return -1;
    sqlite3 *db  = ctx;
    char    *err = NULL;
    char     sql[256];
    int      rc;
    snprintf(sql, sizeof(sql), "UPDATE StockTab SET Quantity = %d WHERE Id = %d;", quantity, id);
    rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if (rc) {
        printf("failed to mod item !\n");
        printf("sql: %s\n", sql);
        printf("err: %s\n", err);
        return -1;
    }
    return 0;
}

int ffosm_del_item(void *ctx, int id)
{
    if (!ctx) return -1;
    sqlite3 *db  = ctx;
    char    *err = NULL;
    char     sql[256];
    int      rc;
    snprintf(sql, sizeof(sql), "BEGIN; DELETE FROM StockTab WHERE Id = %d; DELETE FROM RecordTab WHERE ItemId = %d; COMMIT;", id, id);
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
    int  i;
    for (i = 0; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");
    return 0;
}

int ffosm_query_stock(void *ctx, FFOSM_QUERY_CB callback, void *cbctx, char *name, char *remarks)
{
    if (!ctx) return -1;
    sqlite3 *db  = ctx;
    char    *err = NULL;
    char     sql[512];
    char    *str = sql;
    int      len = sizeof(sql);
    int      ret, rc;
    callback = callback ? callback : callback_query_stock;
    ret = snprintf(str, len, "SELECT Id, ItemName, Quantity, Price, Remarks FROM StockTab WHERE TRUE");
    str += ret, len -= ret;
    if (name && strcmp(name, "") != 0 && strcmp(name, "*") != 0) {
        ret = snprintf(str, len, " AND ItemName LIKE '%%%s%%'", name);
        str += ret, len -= ret;
    }
    if (remarks && strcmp(remarks, "") != 0 && strcmp(remarks, "*") != 0) {
        ret = snprintf(str, len, " AND Remarks LIKE '%%%s%%'", remarks);
        str += ret, len -= ret;
    }
    ret = snprintf(str, len, " ORDER BY Id DESC;");
    str += ret, len -= ret;
    printf("sql: %s\n", sql);
    printf("stock list:\n");
    rc = sqlite3_exec(db, sql, callback, cbctx, &err);
    if (rc) {
        printf("failed to query stock !\n");
        printf("sql: %s\n", sql);
        printf("err: %s\n", err);
        return -1;
    }
    return 0;
}

int ffosm_query_record(void *ctx, FFOSM_QUERY_CB callback, void *cbctx, int type, char *user, int item_id, char *remarks)
{
    if (!ctx) return -1;
    sqlite3 *db  = ctx;
    char    *err = NULL;
    char     sql[512];
    char    *str = sql;
    int      len = sizeof(sql);
    int      ret, rc;
    callback = callback ? callback : callback_query_stock;
    ret = snprintf(str, len, "SELECT DISTINCT RecordTab.Id, Type, User, ItemId, ItemName, RecordTab.Quantity, datetime(Datetime, 'unixepoch', 'localtime'), RecordTab.Remarks FROM RecordTab JOIN StockTab ON RecordTab.ItemId = StockTab.Id WHERE TRUE");
    str += ret, len -= ret;
    if (type > 0) {
        ret = snprintf(str, len, " AND Type = %d", type);
        str += ret, len -= ret;
    }
    if (user && strcmp(user, "") != 0 && strcmp(user, "*") != 0) {
        ret = snprintf(str, len, " AND User = '%s'", user);
        str += ret, len -= ret;
    }
    if (item_id > 0) {
        ret = snprintf(str, len, " AND ItemId = %d", item_id);
        str += ret, len -= ret;
    }
    if (remarks && strcmp(remarks, "") != 0 && strcmp(remarks, "*") != 0) {
        ret = snprintf(str, len, " AND RecordTab.Remarks LIKE '%%%s%%'", remarks);
        str += ret, len -= ret;
    }
    ret = snprintf(str, len, " ORDER BY RecordTab.Id DESC;");
    str += ret, len -= ret;
    printf("sql: %s\n", sql);
    printf("record list:\n");
    rc = sqlite3_exec(db, sql, callback, cbctx, &err);
    if (rc) {
        printf("failed to query record !\n");
        printf("sql: %s\n", sql);
        printf("err: %s\n", err);
        return -1;
    }
    return 0;
}

static int putin_or_borrow(void *ctx, int op, char *user, int itemid, int quantity, char *remarks)
{
    if (!ctx || !user || !*user || itemid <= 0 || quantity <= 0) return -1;
    sqlite3 *db  = ctx;
    char    *err = NULL;
    char     sql[512];
    int      rc;
    snprintf(sql, sizeof(sql),
        "BEGIN;\n"
        "INSERT INTO RecordTab (Type, User, ItemId, Quantity, Datetime, remarks) SELECT %d, '%s', StockTab.Id, %d, strftime('%%s','now'), '%s'\n"
        "  From StockTab WHERE Id = %d AND Quantity >= %d;\n"
        "UPDATE StockTab SET Quantity = Quantity %c %d WHERE Id = %d AND Quantity >= %d;\n"
        "COMMIT;\n",
        op, user, quantity, remarks, itemid, op == OP_PUTIN ? 0 : quantity,
        op == OP_PUTIN ? '+' : '-', quantity, itemid, op == OP_PUTIN ? 0 : quantity);
    rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if (rc) {
        printf("failed to putin_or_borrow !\n");
        printf("sql: %s\n", sql);
        printf("err: %s\n", err);
        return -1;
    }
    return 0;
}

int ffosm_putin(void *ctx, char *user, int itemid, int quantity, char *remarks)
{
    return putin_or_borrow(ctx, OP_PUTIN , user, itemid, quantity, remarks);
}

int ffosm_borrow(void *ctx, char *user, int itemid, int quantity, char *remarks)
{
    return putin_or_borrow(ctx, OP_BORROW, user, itemid, quantity, remarks);
}

static int return_or_scrap(void *ctx, int op, int operid, int quantity, char *remarks)
{
    if (!ctx || operid <= 0 || quantity <= 0) return -1;
    sqlite3 *db  = ctx;
    char    *err = NULL;
    char     sql[512];
    int      rc;
    snprintf(sql, sizeof(sql),
        "BEGIN;\n"
        "INSERT INTO RecordTab (Type, User, ItemId, Quantity, Datetime, Remarks) SELECT %d, User, ItemId, %d, strftime('%%s','now'), '%s' FROM RecordTab\n"
        "  WHERE Type = %d AND Id = %d AND Quantity >= %d;\n"
        "UPDATE StockTab SET Quantity = Quantity + %d WHERE Id = (SELECT ItemId FROM RecordTab WHERE Id = %d);\n"
        "COMMIT;\n",
        op, quantity, remarks, OP_BORROW, operid, quantity, op == OP_RETURN ? quantity : 0, operid);
    rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if (rc) {
        printf("failed to return_or_scrap !\n");
        printf("sql: %s\n", sql);
        printf("err: %s\n", err);
        return -1;
    }
    return 0;
}

int ffosm_return(void *ctx, int operid, int quantity, char *remarks)
{
    return return_or_scrap(ctx, OP_RETURN, operid, quantity, remarks);
}

int ffosm_scrap(void *ctx, int operid, int quantity, char *remarks)
{
    return return_or_scrap(ctx, OP_SCRAP , operid, quantity, remarks);
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
            char name[65] = ""; int quantity = 0; float price = 0; char remarks[256] = "";
            scanf("%64s %d %f %255s", name, &quantity, &price, remarks);
            ret = ffosm_new_item(osm, name, quantity, price, remarks);
            printf("new item, ret: %d\n", ret);
        } else if (strcmp(cmd, "mod") == 0) {
            int id = -1, quantity = -1;
            scanf("%d %d", &id, &quantity);
            ret = ffosm_mod_item(osm, id, quantity);
            printf("mod item, ret: %d\n", ret);
        } else if (strcmp(cmd, "del") == 0) {
            int id = -1;
            scanf("%d", &id);
            ret = ffosm_del_item(osm, id);
            printf("del item, ret: %d\n", ret);
        } else if (strcmp(cmd, "query_stock" ) == 0) {
            char name[65] = "", remarks[256] = "";
            scanf("%64s %255s", name, remarks);
            ret = ffosm_query_stock(osm, NULL, NULL, name, remarks);
            printf("query stock, ret: %d\n", ret);
        } else if (strcmp(cmd, "query_record" ) == 0) {
            int type = -1, itemid = -1; char name[65] = "", remarks[256] = "";
            scanf("%d %64s %d %255s", &type, name, &itemid, remarks);
            ret = ffosm_query_record(osm, NULL, NULL, type, name, itemid, remarks);
            printf("query record, ret: %d\n", ret);
        } else if (strcmp(cmd, "putin") == 0) {
            char user[65] = "", remarks[256] = ""; int itemid = -1, quantity = -1;
            scanf("%64s %d %d %255s", user, &itemid, &quantity, remarks);
            ret = ffosm_putin(osm, user, itemid, quantity, remarks);
            printf("putin, ret: %d\n", ret);
        } else if (strcmp(cmd, "borrow") == 0) {
            char user[65] = "", remarks[256] = ""; int itemid = -1, quantity = -1;
            scanf("%64s %d %d %255s", user, &itemid, &quantity, remarks);
            ret = ffosm_borrow(osm, user, itemid, quantity, remarks);
            printf("borrow, ret: %d\n", ret);
        } else if (strcmp(cmd, "return") == 0) {
            char remarks[256] = ""; int operid = -1, quantity = -1;
            scanf("%d %d %255s", &operid, &quantity, remarks);
            ret = ffosm_return(osm, operid, quantity, remarks);
            printf("return, ret: %d\n", ret);
        } else if (strcmp(cmd, "scrap") == 0) {
            char remarks[256] = ""; int operid = -1, quantity = -1;
            scanf("%d %d %255s", &operid, &quantity, remarks);
            ret = ffosm_scrap(osm, operid, quantity, remarks);
            printf("scrap, ret: %d\n", ret);
        } else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            break;
        }
    }
    ffosm_exit(osm);
    return 0;
}
#endif
