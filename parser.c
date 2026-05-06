/* Contains all parsers */
#include "apr.h"

struct ent *parser_c(const char *path)
{
        FILE *fp;
        char *blk, *ln;
        struct ent *e;
        const char *sl;
        int st = 0;
        char sb[256], ib[256], db[256];
        char mon[16];
        int y, m, d;

        fp = fopen(path, "r");
        if (!fp)
                return NULL;
        blk = readcmt(fp);
        fclose(fp);
        if (!blk || !*blk) {
                free(blk);
                return NULL;
        }

        e = calloc(1, sizeof(struct ent));
        sl = strrchr(path, '/');
        e->file = strdup(sl ? sl + 1 : path);

        ln = strtok(blk, "\n");
        while (ln) {
                char *tr = trim(ln);
                if (*tr == '*') {
                        tr++;
                        if (isspace((unsigned char)*tr))
                                tr++;
                }

                if (st == 0) {
                        if (sscanf(tr, "%255s / %255s / %255[^\n]",
                                   sb, ib, db) == 3) {
                                e->set = strdup(sb);
                                e->id = strdup(ib);
                                if (sscanf(db, "%15s %d, %d", mon, &d, &y) == 3) {
                                        for (m = 1; m <= 12; m++) {
                                                if (!strcmp(mon, mons[m - 1]))
                                                        break;
                                        }
                                        e->date = y * 10000 + m * 100 + d;
                                }
                        }
                        st = 1;
                } else if (st == 1) {
                        if (*tr) {
                                e->title = strdup(tr);
                                st = 2;
                        }
                } else {
                        e->cmts = realloc(e->cmts, sizeof(char *) * (e->cmtc + 1));
                        e->cmts[e->cmtc++] = strdup(tr);
                }
                ln = strtok(NULL, "\n");
        }

        e->wday = getwday(e->date);
        free(blk);
        return e;
}
