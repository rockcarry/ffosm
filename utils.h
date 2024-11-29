#ifndef _FFOSM_UTILS_H_
#define _FFOSM_UTILS_H_

int   file_exists(char *file);
char* file_read(char *file, int *size);
char* parse_params(char *str, char *key, char *val, int len);

#endif
