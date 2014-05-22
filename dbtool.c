#include <stdio.h>
#include <string.h>

struct unetent
{
	char name[64];
	char ip[13];
};

long exists(char *fname, char *name)
{
	FILE *f = fopen(fname, "rb");
	struct unetent ent;
	long e;
	while (fread(&ent, sizeof(struct unetent), 1, f) > 0)
	{
		e = ftell(f);
		if (strcmp(ent.name, name) == 0)
		{
			fclose(f);
			return e;
		}
	}
	fclose(f);
	return 0;
}

void update(char *fname, long e, struct unetent *ent)
{
	FILE *f = fopen(fname, "r+");
	fseek(f, e, SEEK_SET);
	fseek(f, -sizeof(struct unetent), SEEK_CUR);
	fwrite(ent, sizeof(struct unetent), 1, f);
	fflush(f);
	fclose(f);
}

int main(int argc, char * * argv)
{
	int i;
	FILE *f = fopen(argv[1], "a+");
	struct unetent ent;
	long e;
	for (i = 2; i < argc; i += 2)
	{
		strcpy(ent.name, argv[i]);
		strcpy(ent.ip, argv[i + 1]);
		if (exists(argv[1], ent.name) > 0)
			update(argv[1], e, &ent);
		else
			fwrite(&ent, sizeof(struct unetent), 1, f);
	}
	fflush(f);
	fclose(f);
	f = fopen(argv[1], "rb");
	printf("%-20s %s\n", "Domain", "IP Addr");
	printf("----------------------------------\n");
	while (fread(&ent, sizeof(struct unetent), 1, f) > 0)
		printf("%-20s %s\n", ent.name, ent.ip);
	fclose(f);
}
