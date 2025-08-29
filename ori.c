#define TB_IMPL

#include "termbox2.h"

#include <stdio.h>
#include <string.h>

#include <dirent.h>
#include <libgen.h>

#define MAX 528


/* = TODO =
 * do termbox magic, UI
 * add adding topics and subtopics
 * cleanup whatever is going on here 
 */

void create(char *name, FILE *fp, char *dirpth) 
{ 
	fprintf(fp, "\"%s\"\t0\n", name); 
	char tdir[MAX]; 
	snprintf(tdir, sizeof(tdir), "%s/%s", dirpth, name); 
	
	if(mkdir(tdir, 0755) != 0) 
	{
		if(errno == EEXIST)
			fprintf(stderr, "Directory already exists\n");
		else
			fprintf(stderr, "Directory creation failed\n"); 
	}
}

int
main(int argc, char *argv[])
{
  if(argc < 2) 
  {
  	printf("Usage: %s <index file>\n", argv[0]);
        return -1;
  }
  
  FILE *fp = fopen(argv[1], "a+");

  if(!fp)
  {
	fprintf(stderr, "File open fail\n");
	return -1;
  }
  
  char filpth[MAX];
  strcpy(filpth, argv[1]);

  char *dirpth = dirname(filpth);
  
  char name[MAX];
  int n;

  char line[MAX];

  while(fgets(line, sizeof(line), fp)) 
  {
	if(sscanf(line, " \"%[^\"]\"\t%d", name, &n) == 2)
	{
		for(int i = 1; i <= n; i++)
		{
			char entry[MAX];
			snprintf(entry, sizeof(entry), "%s/%s/%d.txt", dirpth, name, i);

			FILE *efp = fopen(entry, "r");

			if(!efp)
				printf("placeholder error\n");

			/* to even test if worky :3 temporary */
			else
			{
				printf("\tEntry %d (%s):\n", i, entry);
				char buf[MAX];

				while(fgets(buf, sizeof(buf), efp))
					printf("%s", buf);

				
				fclose(efp);
			}
		
			create("hihi", fp, dirpth);
		}
  	}
   }

  fclose(fp);
  return 0;
}
