#ifndef __FFOSM_H__
#define __FFOSM_H__

#include <time.h>

enum { OP_PUTIN = 1, OP_BORROW, OP_RETURN, OP_SCRAP };

typedef int (*FFOSM_QUERY_CB)(void *data, int argc, char **argv, char **colname);

void* ffosm_init(char *db );
void  ffosm_exit(void *ctx);

int ffosm_new_item(void *ctx, char *name, int quantity, float price, char *remarks);
int ffosm_mod_item(void *ctx, int id, int quantity);
int ffosm_del_item(void *ctx, int id);

int ffosm_query_stock (void *ctx, FFOSM_QUERY_CB callback, void *cbctx, char *name, char *remarks);
int ffosm_query_record(void *ctx, FFOSM_QUERY_CB callback, void *cbctx, int type, char *user, int item_id, char *remarks);

int ffosm_putin (void *ctx, char *user, int itemid, int quantity, char *remarks);
int ffosm_borrow(void *ctx, char *user, int itemid, int quantity, char *remarks);
int ffosm_return(void *ctx, int operid, int quantity, char *remarks);
int ffosm_scrap (void *ctx, int operid, int quantity, char *remarks);

#endif

