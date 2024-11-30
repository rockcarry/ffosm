#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"

int file_exists(char *file)
{
    FILE *fp   = fopen(file, "rb");
    int   size = -1;
    if (fp) {
        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fclose(fp);
    }
    return size;
}

char* file_read(char *file, int *size)
{
    FILE *fp = fopen(file, "rb");
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    int s = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buf = malloc(s + 1);
    if (buf) {
        int ret = fread(buf, 1, s, fp);
        if (ret != s) printf("fread ret != s !\n");
        buf[s] = '\0';
    }
    if (size) *size = s;
    fclose(fp);
    return buf;
}

char* parse_params(char *str, char *key, char *val, int len)
{
    char *p = (char*)strstr(str, key);
    int   i;

    *val = '\0';
    if (!p) return NULL;
    p += strlen(key);
    if (*p == '\0') return NULL;

    while (*p) {
        if (*p != ':' && *p != '"' && *p != '=' && *p != ' ') break;
        else p++;
    }

    for (i = 0; i < len; i++) {
        if (*p == '\\') {
            p++;
        } else if (*p == '"' || *p == ',' || *p == '&' || *p == '\n' || *p == '\0') {
            break;
        }
        val[i] = *p++;
    }
    val[i < len ? i : len - 1] = '\0';
    return val;
}
