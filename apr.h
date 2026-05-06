/* Contains all type definitions and function declarations. */
#ifndef APR_H
#define APR_H

#include <sys/types.h>
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

/* parsed file entry */
struct ent {
        char *file;
        char *set;
        char *id;
        char *title;
        char **cmts;
        int cmtc;
        int date;               /* YYYYMMDD (e.g. 20260508) */
        int wday;
};

/* heatmap cel */
struct mapcel {
        int date;
        int cnt;
};

/* output mode */
struct out_mode {
        char *name;
        void (*fn)(void);
};

/* parse mode */
struct parse_mode {
        char *type;
        struct ent *(*fn)(const char *);
};

/* global runtime state */
struct runtime {
        char *dpath;
        char *ftype;

        struct ent *ents;
        int entc;
        int ecap;

        struct mapcel hmap[30][7];
};

extern struct runtime rt;
extern const char *mons[12];
extern const char *weeks[7];

/*
 * auxiliary helpers
 * implemented in utils.c
 */
int filter(const struct dirent *d);
int cmp(const void *a, const void *b);
char *trim(char *s);
int get_today(void);
char *ntod(int date);
int getwday(int d);
char *readcmt(FILE *fp);

/*
 * metadata parser logic
 * implemented in parser.c
 */
struct ent *parser_c(const char *path);

/*
 * view and rendering logic
 * implemented in output.c
 */
void out_dbg(void);                     /* debug output mode */
void out_list(void);                    /* list view mode */
void out_unimap(void);                  /* unicode heatmap mode */
void genmap(void);                      /* fill heatmap matrix */

/*
 * core workflow and lifecycle
 * implemented in main.c
 */
void add(struct ent *e);
void load(void);
void quit(void);
void help(void);

#endif
