
/* apr @alpha 0.1.0
 * https://github.com/qjf0/apr
 *
 * Maintainer: qjf0 <https://github.com/qjf0>
 *                  <qianjunfan0@outlook.com>
 *                  <qianjunfan0@gmail.com>
 */

#include "apr.h"

struct runtime rt = {0};

#define OUTC (sizeof(outs) / sizeof(outs[0]))
static struct out_mode outs[] = {
        {"--debug",  out_dbg},
        {"--list",   out_list},
        {"--unimap", out_unimap}
};

#define PMODC (sizeof(pmod) / sizeof(pmod[0]))
static struct parse_mode pmod[] = {
        {".c", parser_c},
};

int main(int argc, char **argv)
{
        int i;

        if (argc == 2 && !strcmp(argv[1], "--help")) {
                help();
                return EXIT_SUCCESS;
        } else if (argc != 4) {
                help();
                return EXIT_FAILURE;
        }

        rt.dpath = argv[2];
        rt.ftype = argv[3];

        load();
        qsort(rt.ents, rt.entc, sizeof(struct ent), cmp);

        for (i = 0; i < OUTC; ++i) {
                if (!strcmp(argv[1], outs[i].name)) {
                        outs[i].fn();
                        break;
                }
        }

        quit();
        return EXIT_SUCCESS;
}

void help(void)
{
        printf("usage: apr <operation> [...]\n");
        printf("operations:\n");
        printf("       apr --help                       to view all available commands\n\n");
        printf("       apr --list [path] [ext]          list all items on your terminal\n\n");
        printf("       apr --unimap [path] [ext]        print out a unicode‑based heatmap\n");
        printf("                                        on your terminal\n");
}

void add(struct ent *e)
{
        if (rt.entc >= rt.ecap) {
                rt.ecap = rt.ecap ? rt.ecap * 2 : 4;
                rt.ents = realloc(rt.ents, rt.ecap * sizeof(struct ent));
        }

        rt.ents[rt.entc++] = *e;
        free(e);
}

void load(void)
{
        struct dirent **ls;
        int n, i;
        struct ent *(*fn)(const char *) = NULL;

        n = scandir(rt.dpath, &ls, filter, alphasort);
        if (n <= 0)
                return;

        for (i = 0; i < PMODC; ++i) {
                if (!strcmp(pmod[i].type, rt.ftype)) {
                        fn = pmod[i].fn;
                        break;
                }
        }

        if (!fn) {
                printf("apr err: no parser for %s\n", rt.ftype);
                for (i = 0; i < n; i++)
                        free(ls[i]);
                free(ls);
                return;
        }

        for (i = 0; i < n; ++i) {
                char path[1024];
                struct ent *e;

                snprintf(path, sizeof(path), "%s/%s", rt.dpath, ls[i]->d_name);
                e = fn(path);
                if (e)
                        add(e);
                free(ls[i]);
        }
        free(ls);
}

void quit(void)
{
        int i, j;

        for (i = 0; i < rt.entc; ++i) {
                free(rt.ents[i].file);
                free(rt.ents[i].set);
                free(rt.ents[i].id);
                free(rt.ents[i].title);

                for (j = 0; j < rt.ents[i].cmtc; j++)
                        free(rt.ents[i].cmts[j]);
                free(rt.ents[i].cmts);
        }
        free(rt.ents);
}
