/* Contains all utilities */
#include "apr.h"

const char *mons[12] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

const char *weeks[7] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

int filter(const struct dirent *d)
{
        const char *dot = strrchr(d->d_name, '.');
        if (dot && !strcmp(dot, rt.ftype))
                return 1;
        return 0;
}

int cmp(const void *a, const void *b)
{
        const struct ent *ea = a;
        const struct ent *eb = b;
        return eb->date - ea->date;
}

char *trim(char *s)
{
        char *e;

        while (isspace(*s))
                s++;
        if (!*s)
                return s;
        e = s + strlen(s) - 1;
        while (e > s && isspace(*e))
                e--;
        e[1] = '\0';
        return s;
}

int get_today(void)
{
        time_t t = time(NULL);
        struct tm *m = localtime(&t);
        return (m->tm_year + 1900) * 10000 + (m->tm_mon + 1) * 100 + m->tm_mday;
}

char *ntod(int date)
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

int getwday(int d)
{
        struct tm t = {0};

        t.tm_year = (d / 10000) - 1900;
        t.tm_mon = ((d / 100) % 100) - 1;
        t.tm_mday = d % 100;
        t.tm_isdst = -1;
        mktime(&t);
        return t.tm_wday;
}

char *readcmt(FILE *fp)
{
        char buf[4096];
        char *cmt = NULL;
        size_t len = 0;
        int in = 0;

        while (fgets(buf, sizeof(buf), fp)) {
                char *p = buf;
                while (*p) {
                        if (!in) {
                                if (p[0] == '/' && p[1] == '*') {
                                        in = 1;
                                        p += 2;
                                } else if (!isspace((unsigned char)*p)) {
                                        free(cmt);
                                        return NULL;
                                } else {
                                        p++;
                                }
                        } else {
                                if (p[0] == '*' && p[1] == '/') {
                                        goto done;
                                } else {
                                        cmt = realloc(cmt, len + 2);
                                        cmt[len++] = *p++;
                                        cmt[len] = '\0';
                                }
                        }
                }
        }
done:
        if (!cmt)
                cmt = strdup("");
        return cmt;
}
