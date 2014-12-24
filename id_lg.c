/* 
 * id_lg.c 24/06/2012
 * ID Looking Glass -- Merubah format file looking glass lg.mohonmaaf.com
 *                     ke file berformat maxmind.com
 *
 * Copyright (c) 2012 Lulus Wijayakto <l.wijayakto@yahoo.com>
 *
 * Can be freely distributed and used under the terms of the GNU GPLv2.
 *
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


struct looking_glass {
	u_int32_t network;
	u_int32_t broadcast;
	u_int8_t prefix;
	u_int8_t flag;
};

#define BUF_MAX 128
#define OVERLAPS 1
#define NOT_OVERLAPS 2
#define IP_OK 3

u_int32_t prefix2mask(int prefix)
{
	return htonl(~((1 << (32 - prefix)) - 1));
}

u_int32_t calc_broadcast(u_int32_t addr, int prefix)
{
	return (addr & prefix2mask(prefix)) | ~prefix2mask(prefix);
}

void scan_ip_menyambung(struct looking_glass *lg, int count)
{
	u_int32_t net;
	u_int32_t i, p, y;
		
	for (i = 0; i < count; i++) {
		if (lg[i].flag == OVERLAPS)
			continue;
			
		net = lg[i].network;
		p = i +1;
		
		/* nge-loop terus jika masih menemukan ip yang bersambung */
		while (1) {
			if ((net << 8) == 0)
				y = (256 * 256 * 256);
			else if ((net << 16) == 0)
				y = (256 * 256);
			else if ((net << 24) == 0)
				y = 256;
				
			if ((net + y) == lg[p].network) {
				net = lg[p].network;
				p++;
			} else
				break;
		}
		
		lg[i].broadcast = lg[p -1].broadcast;
		lg[i].flag = IP_OK;
		i = p -1;
	}
}

void scan_ip_overlaps(struct looking_glass *lg, int count)
{
	u_int32_t net, brd;
	u_int32_t i, r;
	
	for (i = 0; i < count; i++) {
		if (lg[i].flag == OVERLAPS)
			continue;
		
		net = lg[i].network;
		brd = lg[i].broadcast;
		
		for (r = i+1; r < count; r++) {
			if ((net < lg[r].broadcast) && (brd > lg[r].network))
				lg[r].flag = OVERLAPS;
		}
	}
}

static int qsort_cmp(const void *m1, const void *m2)
{
	struct looking_glass *lg1 = (struct looking_glass *) m1;
	struct looking_glass *lg2 = (struct looking_glass *) m2;
	int i = 0;
	
	if (lg1->prefix > lg2->prefix)
		i = 1;
	else if (lg1->prefix < lg2->prefix)
		i = -1;
	
	return i;
}

int load_into_memory(struct looking_glass **lg, const char *file)
{
	FILE *fp = NULL;
	int count = 0, p, q;
	char buf[BUF_MAX];
	char ip[20];
	struct looking_glass *lg_tmp = NULL;
	struct in_addr addr;
	
	memset(buf, 0, BUF_MAX);
	memset(ip, 0, 20);
	memset(&addr, 0, sizeof(struct in_addr));
	
	fp = fopen(file, "r");
	if (!fp) {
		printf("tidak ada file\n");
		return -1;
	}
	
	while (fgets(buf, BUF_MAX, fp)) {
		if (buf[0] == ' ')
			continue;
		
		q = 0;
		
		/* dapatkan ip network */
		for (p = 0; p < 5; p++) {
			if (isdigit(buf[p])) {
				q = p; /* simpan lokasi ip start */
				
				/* scan ip network */
				while (1) {
					if ((buf[p] == ' ') || (buf[p] == '\n')) { /* ketemu ip network */
						count++;
						lg_tmp = realloc(*lg, sizeof(struct looking_glass) * (count +1));
						
						memset(ip, '\0', 20);
						memcpy(ip, buf + q, p -q);
						
						p = strlen(ip);
						
						if (ip[p -1] == '\n')
							ip[p -1] = '\0';

						/* cek jika ada prefix */
						if (strchr(ip, '/')) { /* ada prefix */
							q = strcspn(ip, "/");
							/* ambil prefix */
							memset(buf, '\0', BUF_MAX);
							memcpy(buf, ip +(q +1), p - (q +1));
							/* ambil ip */
							memset(ip +q, '\0', p - (q +1));
							
							q = atoi(buf); /* q sekarang adalah prefix */
						} else { /* tidak ada prefix */
							/* cek netmask (cek octet jika == 0) */
							int ff[4] = {0};
							q = 64; /* q sekarang adalah prefix */
							
							sscanf(ip, "%d.%d.%d.%d", &ff[0], &ff[1], &ff[2], &ff[3]);
							
							if ((ff[1] == 0) && (ff[2] == 0) && (ff[3] == 0))
								q >>= 3; // 8
							else if ((ff[1] > 0) && (ff[2] == 0) && (ff[3] == 0))
								q >>= 2; // 16
							else if ((ff[2] > 0) && (ff[3] == 0)) {
								q >>= 1; // 32
								q -= 8; // 24
							}
						}
						
						/* prefix */
						lg_tmp[count -1].prefix = q;
						
						/* network */
						inet_aton(ip, &addr);
						lg_tmp[count -1].network = ntohl(addr.s_addr);
						
						/* broadcast */
						lg_tmp[count -1].broadcast = ntohl(calc_broadcast(addr.s_addr, q));
						
						/* flags (inisialisasi); */
						lg_tmp[count -1].flag = NOT_OVERLAPS;
						
						*lg = lg_tmp;
						break;
					} else
						p++;
				}
			}
			
			if (q > 0)
				break;
		}
	}
	
	fclose(fp);
	return count;
}

/*
int maxmind_start(const char *file, struct looking_glass *lg, int count)
{
	FILE *in = NULL;
	FILE *out = NULL;
	char buf[BUF_MAX];
	char cc[8];
	
	in = fopen(file, "r");
	if (!in)
		return -1;
		
	out = fopen("lg__id", "w");
	
	while (fgets(buf, 526, out)) {
		sscanf(buf, "%*s,%*s,%*s,%*s,%s", cc);
		
		if (strcmp(cc, "\"ID\""))
			fputs(out, buf);
	}
	
	fclose(out);
	fclose(in);
	
	in = fopen("lg__id", "a+");
	
	for (i = 0; i < count; i++) {
		if (lg[i].flag == NOT_OVERLAPS)
			fprintf(k, "\"\",\"\",\"%u\",\"%u\",\"ID\",\"Indonesia\"\n",
					lg[i].network, lg[i].broadcast);
	}
	
	fclose(in);
}
*/

int looking_glass_start(const char *looking_glass_file, const char *maxmind)
{
	struct looking_glass *lg = NULL;
	int count, i;
	struct in_addr addr;
	
	lg = malloc(1 * sizeof(struct looking_glass));
	
	memset(lg, 0, sizeof(struct looking_glass));
	memset(&addr, 0, sizeof(struct in_addr));
	
	/* masukan data dari file ke memory "lg" */
	count = load_into_memory(&lg, looking_glass_file);
	if (count == -1)
		return -1;
	
	/* sorting berdasarkan prefix terkecil ke yang terbesar */
	qsort(lg, count, sizeof (struct looking_glass), qsort_cmp);
	
	/* scan jika ip overlaps atau tidak */
	scan_ip_overlaps(lg, count);
	
	/* scan jika ip sebelumnya menyambung dengan ip berikutnya */
	scan_ip_menyambung(lg, count);
	
	/* print to file */

	//if (!maxmind)
	//	maxmind_start(maxmind, lg, count);
	//else {
		FILE *fp = NULL;
		
		fp = fopen("lg__id", "w");
		
		for (i = 1; i < count; i++) {
			if (lg[i].flag == IP_OK) {
				addr.s_addr = htonl(lg[i].network);
				fprintf(fp, "\"%s\",", inet_ntoa(addr));
				
				addr.s_addr = htonl(lg[i].broadcast);
				fprintf(fp, "\"%s\",", inet_ntoa(addr));
				
				fprintf(fp, "\"%u\",\"%u\",\"ID\",\"Indonesia\"\n",
						lg[i].network, lg[i].broadcast);
			}
		}
		
		fclose(fp);
	//}
	
	free(lg);
	return 0;
}


int main(int argc, char **argv)
{
	if (argc == 1)
		return -1;

	looking_glass_start(argv[1], NULL);
	
	return 0;
}
