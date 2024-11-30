#ifndef __FFOSM_H__
#define __FFOSM_H__

#include <time.h>

enum { OP_PUTIN = 1, OP_LEND, OP_RETURN, OP_SCRAP };

typedef int (*FFOSM_QUERY_CB)(void *data, int argc, char **argv, char **colname);

void* ffosm_init(char *db );
void  ffosm_exit(void *ctx);

int ffosm_new_item(void *ctx, char *name, float price, char *remarks);
int ffosm_del_item(void *ctx, int id);

int ffosm_query_inventory  (void *ctx, FFOSM_QUERY_CB callback, void *cbctx, int pagesize, int pageidx, char *name, char *remarks);
int ffosm_query_borrowed   (void *ctx, FFOSM_QUERY_CB callback, void *cbctx, int pagesize, int pageidx, char *user, char *name);
int ffosm_query_transaction(void *ctx, FFOSM_QUERY_CB callback, void *cbctx, int pagesize, int pageidx, int type, char *user, char *name, char *remarks);

int ffosm_putin (void *ctx, char *user, int itemid, int quantity, char *remarks);
int ffosm_lend  (void *ctx, char *user, int itemid, int quantity, char *remarks);
int ffosm_return(void *ctx, char *user, int itemid, int quantity, char *remarks);
int ffosm_scrap (void *ctx, char *user, int itemid, int quantity, char *remarks);

#endif

