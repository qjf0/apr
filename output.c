/* Contains all output functions */
#include "apr.h"

void genmap(void)
{
        int i, w, d;
        time_t now = time(NULL);
        struct tm *m = localtime(&now);
        struct tm stm = *m;
        time_t st;

        memset(rt.hmap, 0, sizeof(rt.hmap));
        stm.tm_mday -= m->tm_wday + (29 * 7);
        stm.tm_hour = 12;
        stm.tm_min = stm.tm_sec = 0;
        stm.tm_isdst = -1;
        st = mktime(&stm);

        for (i = 0; i < rt.entc; i++) {
                struct ent *e = &rt.ents[i];
                struct tm et = {0};
                int df;

                et.tm_year = (e->date / 10000) - 1900;
                et.tm_mon = ((e->date / 100) % 100) - 1;
                et.tm_mday = e->date % 100;
                et.tm_hour = 12;
                et.tm_isdst = -1;

                df = (int)(difftime(mktime(&et), st) / 86400 + 0.5);
                if (df >= 0 && df < 210) {
                        rt.hmap[df / 7][df % 7].cnt++;
                        rt.hmap[df / 7][df % 7].date = e->date;
                }
        }

        for (i = 0; i < 210; i++) {
                w = i / 7;
                d = i % 7;
                if (!rt.hmap[w][d].date) {
                        struct tm tt = *localtime(&st);
                        tt.tm_mday += i;
                        mktime(&tt);
                        rt.hmap[w][d].date = (tt.tm_year + 1900) * 10000 +
                                             (tt.tm_mon + 1) * 100 + tt.tm_mday;
                }
        }
}

void out_dbg(void)
{
        int w, d, i, j;
        int td = get_today();

        genmap();
        printf("Debug Heatmap:\n\n");
        for (d = 0; d < 7; d++) {
                printf("        %s", weeks[d]);
                for (w = 0; w < 30; w++) {
                        if (rt.hmap[w][d].date > td)
                                printf("  *");
                        else
                                printf("%3d", rt.hmap[w][d].cnt);
                }
                printf("\n");
        }

        printf("\n        Latest: %d\n", rt.entc > 0 ? rt.ents[0].date : 0);

        for (i = 0; i < rt.entc; ++i) {
                struct ent *e = &rt.ents[i];
                printf("[%d] %s\n", i + 1, e->file);
                printf("Set:     %s\n", e->set ? e->set : "NONE");
                printf("ID:      %s\n", e->id ? e->id : "NONE");
                printf("Title:   %s\n", e->title ? e->title : "NONE");
                printf("Date:    %d\n", e->date);
                if (e->cmtc > 0) {
                        printf("Comments:\n");
                        for (j = 0; j < e->cmtc; j++)
                                printf("  %s\n", e->cmts[j]);
                }
        }
}

void out_list(void)
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


		printf("\t%-66s \033[90m[%s]\033[0m\n", desc, e->file);

		for (j = 0; j < e->cmtc; ++j) {
			if (e->cmts[j][0] == '\0')
				continue;
			printf("\t\033[32m%s\033[0m\n", e->cmts[j]);
		}

		if (i + 1 < rt.entc && rt.ents[i + 1].date == e->date)
			printf("\n");
	}
}

void out_unimap(void)
{
	int w, d, n, dt, td = get_today();

	const char *ws[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	const char *gs[] = {" ░", " ▒", " ▓", " █"};

	genmap();

	for (d = 0; d < 7; d++) {
		printf("\t%s", ws[d]);
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

	printf("\t");
	for (int i = 0; i < 63; ++i)
	        printf("-");

	printf("\n\tLatest: %s              ░:0, ▒:(0,3], ▓:(3,8], █:(8,∞)\n\n",
	       ntod(rt.entc > 0 ? rt.ents[0].date : 0));
}
