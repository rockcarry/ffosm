#include <stdlib.h>
#include <stdio.h>
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
        fread(buf, 1, s, fp);
        buf[s] = '\0';
    }
    if (size) *size = s;
    return buf;
}
