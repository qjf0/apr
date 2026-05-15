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

/* crc32 lookup table is omitted for brevity; using bitwise for zero-dep */
static uint32_t png_crc32(uint8_t *buf, int len)
{
	uint32_t crc = 0xffffffff;
	int i, j;

	for (i = 0; i < len; i++) {
		crc ^= buf[i];
		for (j = 0; j < 8; j++)
			crc = (crc >> 1) ^ (0xedb88320 & (-(crc & 1)));
	}
	return ~crc;
}

/* write 32-bit big-endian integer */
static void wp32(uint8_t *p, uint32_t v)
{
	p[0] = (v >> 24) & 0xff;
	p[1] = (v >> 16) & 0xff;
	p[2] = (v >> 8) & 0xff;
	p[3] = v & 0xff;
}

/* generates heatmap in .png */
void export_heatmap_png(const char *filename)
{
	int cell_sz = 12, margin = 3, pad = 10;
	int cols = 30, rows = 7;
	int w = cols * (cell_sz + margin) + pad * 2;
	int h = rows * (cell_sz + margin) + pad * 2;
	int stride = w * 4 + 1;
	int raw_sz = h * stride;
	uint8_t *raw;
	uint8_t colors[4][3] = {
		{235, 237, 240}, /* empty */
		{155, 233, 168}, /* low */
		{64, 196, 99},   /* mid */
		{33, 110, 57}    /* high */
	};
	int d, i, dy, dx, td;
	FILE *f;

	raw = calloc(1, raw_sz);
	if (!raw)
		return;

	td = get_today();

	/* render cells to raw rgba buffer */
	for (d = 0; d < 7; d++) {
		for (i = 0; i < 30; i++) {
			int n = rt.hmap[i][d].cnt;
			int dt = rt.hmap[i][d].date;
			int c_idx, xs, ys;

			if (dt > td)
				continue;

			c_idx = !n ? 0 : (n > 8 ? 3 : (n > 3 ? 2 : 1));
			xs = pad + i * (cell_sz + margin);
			ys = pad + d * (cell_sz + margin);

			for (dy = 0; dy < cell_sz; dy++) {
				for (dx = 0; dx < cell_sz; dx++) {
					int px = ((ys + dy) * stride) + 1 +
						 ((xs + dx) * 4);
					raw[px]     = colors[c_idx][0];
					raw[px + 1] = colors[c_idx][1];
					raw[px + 2] = colors[c_idx][2];
					raw[px + 3] = 255; /* opaque alpha */
				}
			}
		}
	}

	f = fopen(filename, "wb");
	if (!f) {
		free(raw);
		return;
	}

	/* png signature */
	fwrite("\x89PNG\r\n\x1a\n", 1, 8, f);

	/* ihdr chunk */
	uint8_t ihdr[25];
	wp32(ihdr, 13);
	memcpy(ihdr + 4, "IHDR", 4);
	wp32(ihdr + 8, w);
	wp32(ihdr + 12, h);
	ihdr[16] = 8; /* bit depth */
	ihdr[17] = 6; /* rgba */
	ihdr[18] = 0; /* deflate */
	ihdr[19] = 0; /* adaptive filter */
	ihdr[20] = 0; /* no interlace */
	wp32(ihdr + 21, png_crc32(ihdr + 4, 17));
	fwrite(ihdr, 1, 25, f);

	/* idat chunk - zlib uncompressed store mode */
	uint32_t s1 = 1, s2 = 0;
	for (i = 0; i < raw_sz; i++) {
		s1 = (s1 + raw[i]) % 65521;
		s2 = (s2 + s1) % 65521;
	}

	uint32_t idat_body = raw_sz + 6 + 5 * ((raw_sz + 65534) / 65535);
	uint8_t *idat = malloc(idat_body + 12);
	if (!idat) {
		fclose(f);
		free(raw);
		return;
	}

	wp32(idat, idat_body);
	memcpy(idat + 4, "IDAT", 4);
	int p = 8;
	idat[p++] = 0x08; idat[p++] = 0x1d; /* zlib cmf/flg */
	for (i = 0; i < raw_sz; i += 65535) {
		int len = (raw_sz - i > 65535) ? 65535 : (raw_sz - i);
		idat[p++] = (i + len == raw_sz) ? 0x01 : 0x00;
		idat[p++] = len & 0xff;
		idat[p++] = (len >> 8) & 0xff;
		idat[p++] = (~len) & 0xff;
		idat[p++] = ((~len) >> 8) & 0xff;
		memcpy(idat + p, raw + i, len);
		p += len;
	}
	wp32(idat + p, (s2 << 16) | s1);
	p += 4;
	wp32(idat + p, png_crc32(idat + 4, p - 4));
	fwrite(idat, 1, p + 4, f);

	/* iend chunk */
	fwrite("\x00\x00\x00\x00IEND\xae\x42\x60\x82", 1, 12, f);

	fclose(f);
	free(raw);
	free(idat);
}
