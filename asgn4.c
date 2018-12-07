//headers
#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <stdint.h>

//////
//define structs
//////

struct super{
  int32_t magic;
  int32_t N;
  int32_t k;
  int32_t block_size;
  int32_t root_start;
};

struct fat{
  int32_t *phat;
};

struct dirEntry{
  char *name;
  int64_t ctime;
  int64_t mtime;
  int64_t atime;
  int32_t length;
  int32_t startBlock;
  int32_t flag;
};

//////
//initialize structs
//////

void initsup(struct super * newsuper, int blockbytes){
   newsuper->magic = 111111;
   newsuper->N = 65536/blockbytes;
   newsuper->k =newsuper->N % (blockbytes/4) +1;
   newsuper->block_size = blockbytes;
   newsuper->root_start = newsuper->k + 1;
}

void initfat(struct fat * fatty, int k, int n){
  fatty->phat = malloc(n * sizeof( int32_t));
  for(int i = 0; i<n; i++){
    if(i < k+1)
      fatty->phat[i] = -1;
    else
      fatty->phat[i] = 0; 
  }
}

void initDE(struct dirEntry * de, char *newname, int32_t startB, int32_t type){
  de->name = malloc(24 * sizeof(char));
  for(int i = 0; i<24; i++){
    de->name[i] = newname[i];
    if(newname[i] == '\0')
      break;
  }
  de->ctime = 420;
  de->mtime = 420;
  de->atime = 420;
  de->length = 0;
  de->startBlock = startB;
  de->flag = type;
}

//////
//getFunctions(from Disc,Table,super);
//////

struct super*  getSuper(){
  struct super *block = (struct super*)malloc(sizeof(struct super));
  int disc = open("dimage", O_RDONLY);
  read(disc, block, 64);
  close(disc);
  return block;
}

struct fat*  getFat(){
  struct super *sb = getSuper();
  struct fat *table = (struct fat*)malloc(sizeof(struct fat));
  int disc = open("dimage", O_RDONLY);
  lseek(disc,sb->block_size,SEEK_SET);
  read(disc, table, sb->k * sb->block_size);
  close(disc);
  return table;
}

int getFirstOpen(){
  struct super *sb = getSuper();
  struct fat *table = getFat();
  for(int i = 0; i<sb->N; i++){
    if(table->phat[i] == 0)
      return i;
  }
  return -ENOENT;
}

//////
//update functions
//////




static int phat_open(const char *path, struct fuse_file_info *fi){
  
  return 0;
}

static int phat_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
  if(strcmp(path,"/") != 0)
    return -ENOENT;

  filler(buf,".",NULL,0);
  filler(buf,"..",NULL,0);
  return 0;
}

/////
//fuse override runctions
/////

static int phat_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi){
  return size;
}

static int phat_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
  return 0;
}

static int phat_mkdir(const char *path, mode_t mode)
{
  return 0;
}

static int phat_getattr(const char *path, struct stat *stbuf)
{
  int res = 0;

  memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
	stbuf->st_mode = S_IFDIR | 0755;
	stbuf->st_nlink = 2;
    } else
	res = -ENOENT;

  return res;
}

static struct fuse_operations phat_oper = {
        .getattr        = phat_getattr,
        .readdir	= phat_readdir,
	.mkdir          = phat_mkdir,
        .open           = phat_open,
        .read           = phat_read,
        .write          = phat_write,
};

int main(int argc, char *argv[])
{
        int disc = open("dimage", O_CREAT | O_RDWR);
	struct super *superblock = (struct super*)malloc(sizeof(struct super));
       	struct fat *table = (struct fat*)malloc(sizeof(struct fat));
	
	struct dirEntry *de = (struct dirEntry*)malloc(sizeof(struct dirEntry));
	initDE(de,"dir1",4,1);

	initsup(superblock, 512);
	printf(" superblock k = %d, superblock n = %d \n",superblock->k,superblock->N);
	initfat(table,superblock->k,superblock->N);
        for(int i = 0; i<5; i++){
	  printf(" fat value %d \n",table->phat[i]);
	} 
	write(disc, superblock, superblock->block_size);
	write(disc, table, superblock->block_size);
	close(disc);
	struct super *block2 = getSuper();
        struct fat *table2 = getFat();
	printf(" superblock2 rootstart = %d, superblock2 n = %d \n",block2->magic, block2->N);
	for(int i = 0; i<5; i++){
          printf(" fat value %d \n",table2->phat[i]);
        }
	umask(0);
        return fuse_main(argc, argv, &phat_oper, NULL);
}
