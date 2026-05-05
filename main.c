/* apr @alpha 0.0.5
 * https://github.com/qjf0/apr
 *
 * Maintainer: qjf0 <https://github.com/qjf0>
 *                  <qianjunfan0@outlook.com>
 *                  <qianjunfan0@gmail.com>
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
        char **comments;
        int cmtc;
        int date;
        int wday;
};

struct map_item {
        int date;
        int cnt;
};

/* Global Runtime */
struct {
        char *dpath;
        char *ftype;

        struct ent *ents;
        int entc;
        int ecap;

        struct map_item hmap[30][7];
} rt;

const char *mons[] = {
        "Jan","Feb","Mar","Apr",
        "May","Jun","Jul","Aug",
        "Sep","Oct","Nov","Dec"
};

/* A filter that filters out files with the matching extension */
int filter(const struct dirent *entry);

/* Used to extract comments */
char *readcmt(FILE *fp);

/* Get the day of the week based on the date */
int getwday(int d);

/* Transfer number (like 20260405) to a string (like Apr 5, 2026) */
char* ntod(int date);

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

/* Get the current date in YYYY/MM/DD format */
int get_today(void);

/* Fill rt.hmap with counts and dates for the last 30 weeks */
void gen_hmap(void);

/* List out all the items */
void list_print(void);

/* Render Unicode heatmap (░, ▒, ▓, █) to terminal */
void chmap_print(void);

/* Just for debugging */
void debug_print(void);

/* Pattern matching structure */
struct mode {
        char *name;
        void (*func)(void);
};

#define MODC sizeof(modes)/sizeof(modes[0])
struct mode modes[] = {
        {"--debug", debug_print},
        {"--list",  list_print},
        {"--chmap", chmap_print}
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

	struct ent *target = &rt.ents[rt.entc++];
	target->filename = strdup(e->filename);
	target->set = strdup(e->set ? e->set : "");
	target->id = strdup(e->id ? e->id : "");
	target->title = strdup(e->title ? e->title : "");
	target->date = e->date;
	target->wday = e->wday;
	target->cmtc = e->cmtc;

	target->comments = malloc(sizeof(char *) * e->cmtc);
	for (int i = 0; i < e->cmtc; i++)
		target->comments[i] = strdup(e->comments[i]);
}

void quit(void)
{
	for (int i = 0; i < rt.entc; ++i) {
		free(rt.ents[i].filename);
		free(rt.ents[i].set);
		free(rt.ents[i].id);
		free(rt.ents[i].title);
		for (int j = 0; j < rt.ents[i].cmtc; j++)
			free(rt.ents[i].comments[j]);
		free(rt.ents[i].comments);
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
        printf("       apr --help                       To view all available commands\n\n");
        printf("       apr --list [path] [ext]          List all items on your terminal\n\n");
        printf("       apr --chmap [path] [ext]         Print out a text‑based heatmap using\n");
        printf("                                        Unicode characters on your terminal\n");
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

int qcompare(const void *a, const void *b)
{
        const struct ent *ea = (const struct ent *)a;
        const struct ent *eb = (const struct ent *)b;
        return eb->date - ea->date;
}

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

		if (state == 0) {
			char setb[256], idb[256], dateb[256];
			if (sscanf(trimmed, "%255s / %255s / %255[^\n]",
				   setb, idb, dateb) == 3) {
				e->set = strdup(setb);
				e->id = strdup(idb);
				int y, m, d;
				char mon[16];
				if (sscanf(dateb, "%15s %d, %d", mon, &d, &y) == 3) {
					for (m = 1; m <= 12; m++)
						if (!strcmp(mon, mons[m-1]))
							break;
					e->date = y * 10000 + m * 100 + d;
				}
			}
			state = 1;
		} else if (state == 1) {
			if (*trimmed != '\0') {
				e->title = strdup(trimmed);
				state = 2;
			}
		} else {
			e->comments = realloc(e->comments,
					      sizeof(char *) * (e->cmtc + 1));
			e->comments[e->cmtc++] = strdup(trimmed);
		}
		line = strtok(NULL, "\n");
	}

	e->wday = getwday(e->date);
	free(block);
	return e;
}

void debug_print(void)
{
       	int w, d, n, dt, td = get_today();
	const char *ws[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

	gen_hmap();
	printf("Debug Heatmap:\n\n");
	for (d = 0; d < 7; d++) {
		printf("        %s", ws[d]);
		for (w = 0; w < 30; w++) {
			n = rt.hmap[w][d].cnt;
			dt = rt.hmap[w][d].date;
			if (dt > td)
				printf("  *");
			else
				printf("%3d", n);
		}
		printf("\n");
	}
	printf("\n        Latest: %d, *:Future\n",
	       rt.entc > 0 ? rt.ents[0].date : 0);

	for (int i = 0; i < rt.entc; ++i) {
			struct ent *e = &rt.ents[i];
			printf("[%d] %s\n", i + 1, e->filename);
			printf("Set:     %s\n", e->set ? e->set : "NONE");
			printf("ID:      %s\n", e->id ? e->id : "NONE");
			printf("Title:   %s\n", e->title ? e->title : "NONE");
			printf("Date:    %d\n", e->date);
			if (e->cmtc > 0) {
				printf("Comments:\n");
				for (int j = 0; j < e->cmtc; j++)
					printf("  %s\n", e->comments[j]);
			}
		}
}

void list_print(void)
{
	int latest = -1;
	int i, j;

	for (i = 0; i < rt.entc; ++i) {
		struct ent *e = &rt.ents[i];

		if (e->date != latest) {
			latest = e->date;
			printf("\n\033[1m%s:\033[0m\n", ntod(e->date));
		}

		char desc[128];
		snprintf(desc, sizeof(desc), "%s %s - %s",
			 e->set, e->id, e->title);


		printf("\t%-66s \033[90m[%s]\033[0m\n", desc, e->filename);

		for (j = 0; j < e->cmtc; ++j) {
			if (e->comments[j][0] == '\0')
				continue;
			printf("\t\033[32m%s\033[0m\n", e->comments[j]);
		}

		if (i + 1 < rt.entc && rt.ents[i + 1].date == e->date)
			printf("\n");
	}
}

char* ntod(int date)
{
	static char buf[64];
	int y, m, d;

	y = date / 10000;
	m = (date / 100) % 100;
	d = date % 100;

	if (m < 1 || m > 12)
		return NULL;

	sprintf(buf, "%s %d, %d", mons[m - 1], d, y);

	return buf;
}



int get_today(void)
{
	time_t t = time(NULL);
	struct tm *m = localtime(&t);
	return (m->tm_year + 1900) * 10000 + (m->tm_mon + 1) * 100 + m->tm_mday;
}

void gen_hmap(void)
{
	int i, w, d, today;
	time_t now = time(NULL);
	struct tm *m = localtime(&now);
	struct tm stm = *m;

	memset(rt.hmap, 0, sizeof(rt.hmap));
	stm.tm_mday -= m->tm_wday + (29 * 7);
	stm.tm_hour = 12;
	stm.tm_min = stm.tm_sec = 0;
	stm.tm_isdst = -1;
	time_t st = mktime(&stm);

	for (i = 0; i < rt.entc; i++) {
		struct ent *e = &rt.ents[i];
		struct tm et = {0};
		et.tm_year = (e->date / 10000) - 1900;
		et.tm_mon = ((e->date / 100) % 100) - 1;
		et.tm_mday = e->date % 100;
		et.tm_hour = 12;
		et.tm_isdst = -1;
		int diff = (int)(difftime(mktime(&et), st) / 86400 + 0.5);
		if (diff >= 0 && diff < 210) {
			rt.hmap[diff / 7][diff % 7].cnt++;
			rt.hmap[diff / 7][diff % 7].date = e->date;
		}
	}

	for (i = 0; i < 210; i++) {
		w = i / 7; d = i % 7;
		if (!rt.hmap[w][d].date) {
			struct tm tt = *localtime(&st);
			tt.tm_mday += i;
			mktime(&tt);
			rt.hmap[w][d].date = (tt.tm_year + 1900) * 10000 +
					     (tt.tm_mon + 1) * 100 + tt.tm_mday;
		}
	}
}

void chmap_print(void)
{
	int w, d, n, dt, td = get_today();

	const char *ws[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	const char *gs[] = {" ░", " ▒", " ▓", " █"};

	gen_hmap();

	for (d = 0; d < 7; d++) {
		printf("        %s", ws[d]);
		for (w = 0; w < 30; w++) {
			n = rt.hmap[w][d].cnt;
			dt = rt.hmap[w][d].date;
			if (dt > td)
			        printf(" *");
			else if (!n)
			        printf("%s", gs[0]);
			else
			        printf("%s", gs[n > 8 ? 3 : (n > 3 ? 2 : 1)]);
		}

		printf("\n");
	}

	printf("\n        Latest: %d, ░:0, ▒:(0,3], ▓:(3,8], █:(8,∞), *:Future\n",
	       rt.entc > 0 ? rt.ents[0].date : 0);
	printf("        ---------------------------------------------------------------\n\n");
}
