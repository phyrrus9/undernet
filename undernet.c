#include <netdb.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/dns.h>
#include <time.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

struct unetent { char name[64], ip[13]; };
struct lnode
{
	struct unetent data;
	struct lnode *next;
};

struct lnode * list_insert(struct lnode *list, char *name, char *ip);
int list_size(struct lnode *list);
struct unetent * list_get(struct lnode *list, int p);
struct lnode * list_getp(struct lnode *list, int p);
struct lnode * insert_from_file(struct lnode *list, FILE *f, int *code);
int (*original_bufferevent_socket_connect_hostname)(void *bev, void *evdns_base, int family, const char *hostname, int port) = NULL;
struct lnode *node_list = NULL;
void update_list(int);

int list_includes(struct lnode *list, struct unetent *dat)
{
	int i;
	struct unetent *tmp = NULL;
	if (list == NULL) return -1;
	for (i = 0; i < list_size(list); i++)
	{
		tmp = list_get(list, i);
		if (strcmp(tmp->name, dat->name) == 0)
			return i;
	}
	return -1;
}

struct lnode * list_insert(struct lnode *list, char *name, char *ip)
{
	struct lnode *ptr = malloc(sizeof(struct lnode)), *tmp = NULL;
	int lnum = -1;
	strcpy(ptr->data.name, name);
	strcpy(ptr->data.ip, ip);
	ptr->next = NULL;
	if (list == NULL) return ptr;
	if ((lnum = list_includes(list, &ptr->data)) != -1)
	{
		tmp = list_getp(list, lnum);
		strcpy(tmp->data.name, ptr->data.name);
		strcpy(tmp->data.ip, ptr->data.ip);
	}
	else
	{
		for (tmp = list; tmp->next != NULL; tmp = tmp->next);
		tmp->next = ptr;
	}
	return list;
}

int list_size(struct lnode *list)
{
	int i;
	struct lnode *tmp = list;
	for (i = 0; tmp != NULL; i++, tmp = tmp->next);
	return i;
}

struct unetent * list_get(struct lnode *list, int p)
{
	int i;
	struct lnode *tmp = list;
	for (i = 0; i < p; i++)
		tmp = tmp->next;
	return &tmp->data;
}

struct lnode * list_getp(struct lnode *list, int p)
{
	int i;
	struct lnode *tmp = list;
	if (p < 0) return NULL;
	for (i = 0; i < p; i++)
		tmp = tmp->next;
	return tmp;
}

struct lnode * insert_from_file(struct lnode *list, FILE *f, int *code)
{
	struct unetent tmp;
	if (code == NULL)
		code = malloc(sizeof(int));
	*code = fread(&tmp, sizeof(struct unetent), 1, f);
	if (*code > 0)
		return list_insert(list, tmp.name, tmp.ip);
	return list;
}

struct lnode * list_move_front(struct lnode *list, int nn)
{
	struct lnode * p = list_getp(list, nn - 1);
	struct lnode * c = list_getp(list, nn + 1);
	struct lnode * n = list_getp(list, nn + 0);
	if (nn == 0) return list;
	p->next = c;
	n->next = list;
	list = n;
	return list;
}

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t written = fwrite(ptr, size, nmemb, stream);
	return written;
}

void update_list(int sig)
{
	CURL *curl;
	FILE *fp;
	CURLcode res;
	char *url = "http://97.85.72.250/out.dat";
	char *outfilename = "/tmp/undernet.dat";
	int readcode = 1;
	printf("\t[unet] Refreshing node cache...\n");
	curl = curl_easy_init();
	if (curl)
	{
		fp = fopen(outfilename,"wb");
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		fclose(fp);
		fp = fopen(outfilename, "rb");
		while (readcode > 0)
			node_list = insert_from_file(node_list, fp, &readcode);
		unlink(outfilename);
	}
	else
	{
		printf("\t[update_list] ERROR, could not refresh node cache\n");
	}
	alarm(60);
}

const char *lookup_ip_from_nodelist(const char *host)
{
	int i;
	const int end = list_size(node_list);
	struct unetent *tmp = NULL;
	for (i = 0; i < end; i++)
	{
		tmp = list_get(node_list, i);
		if (strcmp(tmp->name, host) == 0)
		{
			node_list = list_move_front(node_list, i);
			return (const char *)tmp->ip;
		}
	}
	return (const char *)host;
}

char isunet(const char *name)
{
	const char *endurit = strstr(name, "/");
	const char *enduri = endurit ? endurit : name + strlen(name);
	return (strstr(name, ".unet") == enduri - strlen(".unet"));
}

const char *undernet_translate_URI(const char *name)
{
	if (isunet(name)) return lookup_ip_from_nodelist(name);
	return name;
}

int bufferevent_socket_connect_hostname(struct bufferevent *bev, struct evdns_base *evdns_base, int family, const char *hostname, int port)
{
	signal(SIGALRM, update_list);
	if (original_bufferevent_socket_connect_hostname == NULL)
		original_bufferevent_socket_connect_hostname = dlsym(RTLD_NEXT, "bufferevent_socket_connect_hostname");
	if (node_list == NULL)
		update_list(0);
	return original_bufferevent_socket_connect_hostname(bev, evdns_base, family, undernet_translate_URI(hostname), port);
}
