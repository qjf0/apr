/* apr @alpha 0.0.2
 * Maintainer: qjf0 <https://github.com/qjf0>
 *                  <qianjunfan0@outlook.com>
 */
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

struct ent {
        char *filename;
        char *set;
        char *id;
        char *title;
        char *comment;
        int date;
        int wday;
};

/* Global Runtime */
struct {
        char *dpath;
        char *ftype;

        struct ent *ents;
        int entc;
        int ecap;
} rt;

/* A filter that filters out files with the matching extension */
int filter(const struct dirent *entry);

/* Used to extract comments */
char *readcmt(FILE *fp);

/* Get the day of the week based on the date */
int getwday(int d);

/* Parse a file, extract set/id/date, title and detailed comment */
struct ent *parse(const char *fpath);

/* Removes leading and trailing whitespace from string s (in-place) */
char *trim(char *str);

/* Load all files in the specified directory */
void load(void);

/* Exit the program and free all memory */
void quit(void);

/* Just print the usage instructions */
void help(void);

/* Compare function for qsort() */
int qcompare(const void *a, const void *b);

/* Print out all the items */
void list_print(void);

/* Pattern matching structure */
struct mode {
        char *name;
        void (*func)(void);
};

#define MODC sizeof(modes)/sizeof(modes[0])
struct mode modes[] = {
        {"--list",  list_print}
};

int main(int argc, char **argv)
{
        /* Parse command line arguments */
        if (argc == 2 && !strcmp(argv[1], "--help")) {
                help();
                return EXIT_SUCCESS;
        } else if (argc != 4) {
                printf("use 'apr --help' to view all available commands\n");
                return EXIT_FAILURE;
        }

        rt.dpath = argv[2];
        rt.ftype = argv[3];

        /* Load files and sort in descending order */
        load();
        qsort(rt.ents, rt.entc, sizeof(struct ent), qcompare);

        /* Match the output mode */
        for (int i = 0; i < MODC; ++i) {
                if (!strcmp(argv[1], modes[i].name) && modes[i].func)
                        modes[i].func();
        }

        /* Quit and free all memory */
        quit();
        return EXIT_SUCCESS;
}

void add(struct ent *e)
{
       if (rt.entc >= rt.ecap) {
               rt.ecap = rt.ecap ? rt.ecap * 2 : 4;
               rt.ents = realloc(rt.ents, rt.ecap * sizeof(struct ent));
       }

       rt.ents[rt.entc++] = (struct ent) {
               .filename = strdup(e->filename),
               .set = strdup(e->set ? e->set : ""),
               .id = strdup(e->id ? e->id : ""),
               .title = strdup(e->title ? e->title : ""),
               .comment = strdup(e->comment ? e->comment : ""),
               .date = e->date,
               .wday = e->wday
       };
}

void quit(void)
{
        for (int i = 0; i < rt.entc; ++i) {
                free(rt.ents[i].filename);
                free(rt.ents[i].set);
                free(rt.ents[i].id);
                free(rt.ents[i].title);
                free(rt.ents[i].comment);
        }
        free(rt.ents);
        rt.ents = NULL;
        rt.entc = rt.ecap = 0;
}

void load(void)
{
        struct dirent **list;
        int n = scandir(rt.dpath, &list, filter, alphasort);

        if (n <= 0)
                return;

        for (int i = 0; i < n; ++i) {
                char path[1024];
                snprintf(path, sizeof(path), "%s/%s",
                                rt.dpath, list[i]->d_name);
                struct ent *e = parse(path);

                if (e)
                        add(e);

                free(list[i]);
        }

        free(list);
}

int filter(const struct dirent *entry)
{
        const char *dot = strrchr(entry->d_name, '.');
        if (dot && !strcmp(dot, rt.ftype))
                return 1;
        return 0;
}

char *trim(char *s)
{
        while (isspace(*s))
                ++s;

        if (!(*s))
                return s;

        char *e = s + strlen(s) - 1;
        while (e > s && isspace(*e))
                --e;

        e[1] = '\0';
        return s;
}

void help(void)
{
        printf("usage: apr <operation> [...]\n");
        printf("operations:\n");
        printf("       pacman --help\n");
        printf("       pacman --list [path] [ext]          List all items.\n");
}

char *readcmt(FILE *fp)
{
        char line[4096];
        char *comment = NULL;
        size_t len = 0;
        int in = 0;
        while (fgets(line, sizeof(line), fp)) {
                char *p = line;
                while (*p) {
                        if (!in) {
                                if (p[0] == '/' && p[1] == '*') {
                                        in = 1;
                                        p += 2;
                                } else if (!isspace((unsigned char)*p)) {
                                        free(comment);
                                        return NULL;
                                } else {
                                        p++;
                                }
                        } else {
                                if (p[0] == '*' && p[1] == '/') {
                                        in = 0;
                                        p += 2;
                                        goto done;
                                } else {
                                        comment = realloc(comment, len + 2);
                                        comment[len++] = *p;
                                        comment[len] = '\0';
                                        p++;
                                }
                        }
                }
        }
done:
        if (!comment)
                comment = strdup("");
        return comment;
}

int getwday(int d)
{
        int year = d / 10000;
        int month = (d / 100) % 100;
        int day = d % 100;

        struct tm t = {0};
        t.tm_year = year - 1900;
        t.tm_mon = month - 1;
        t.tm_mday = day;
        t.tm_isdst = -1;

        mktime(&t);
        return t.tm_wday;
}

const char *mons[] = {
        "Jan","Feb","Mar","Apr",
        "May","Jun","Jul","Aug",
        "Sep","Oct","Nov","Dec"
};

struct ent *parse(const char *fpath)
{
        FILE *fp = fopen(fpath, "r");

        if (!fp)
                return NULL;

        char *block = readcmt(fp);
        fclose(fp);

        if (!block || !*block) {
                free(block);
                return NULL;
        }

        struct ent *e = calloc(1, sizeof(struct ent));
        const char *slash = strrchr(fpath, '/');
        e->filename = strdup(slash ? slash + 1 : fpath);
        char *line = strtok(block, "\n");
        int state = 0;

        while (line) {
                char *trimmed = trim(line);

                if (*trimmed == '*') {
                        trimmed++;
                        if (isspace((unsigned char)*trimmed))
                                trimmed++;
                }

                if (*trimmed == '\0') {
                        if (state >= 2) {
                                if (!e->comment)
                                        e->comment = strdup("");
                                char *tmp;
                                asprintf(&tmp, "%s%s", e->comment,
                                         (strlen(e->comment) ? "\n" : ""));
                                free(e->comment);
                                e->comment = tmp;
                        }
                        line = strtok(NULL, "\n");
                        continue;
                }

                if (state == 0) {
                        char setb[256], idb[256], dateb[256];
                        if (sscanf(trimmed, "%255s / %255s / %255[^\n]",
                                   setb, idb, dateb) == 3) {
                                e->set = strdup(setb);
                                e->id = strdup(idb);
                                int y, m, d;
                                char mon[16];
                                if (sscanf(dateb, "%15s %d, %d", mon,
                                           &d, &y) == 3) {

                                        for (m = 1; m <= 12; m++)
                                                if (strcmp(mon,
                                                   mons[m-1]) == 0)
                                                        break;
                                        e->date = y * 10000 + m * 100 + d;
                                }
                        }
                        state = 1;
                } else if (state == 1) {
                        e->title = strdup(trimmed);
                        state = 2;
                } else {
                        if (!e->comment)
                                e->comment = strdup("");
                        char *tmp;
                        asprintf(&tmp, "%s%s%s", e->comment,
                                 (strlen(e->comment) ? "\n" : ""),
                                 trimmed);
                        free(e->comment);
                        e->comment = tmp;
                }

                line = strtok(NULL, "\n");
        }

        e->wday = getwday(e->date);

        free(block);
        return e;
}

int qcompare(const void *a, const void *b)
{
        const struct ent *ea = (const struct ent *)a;
        const struct ent *eb = (const struct ent *)b;
        return eb->date - ea->date;
}

void debug_print(void)
{
        for (int i = 0; i < rt.entc; ++i) {
                struct ent *e = &rt.ents[i];
                printf("[%d] %s\n", i + 1, e->filename);
                printf("Set:     %s\n", e->set ? e->set : "NONE");
                printf("ID:      %s\n", e->id ? e->id : "NONE");
                printf("Title:   %s\n", e->title ? e->title : "NONE");
                printf("Date:    %d\n", e->date);
                printf("Wday:    %d\n", e->wday);
                if (e->comment && e->comment[0])
                        printf("Comment:\n%s\n", e->comment);
        }
}

void list_print(void)
{
        int latest = rt.ents[0].date + 1;
        for (int i = 0; i < rt.entc; ++i) {
                struct ent *e = &rt.ents[i];
                if (e->date < latest) {
                        latest = e->date;
                        printf("%d:\n", e->date);
                }

                printf("\t%s %s - %s\n", e->set, e->id, e->title);
        }
}
