/*
 * fuse_operations.h
 *
 *  Created on: 05/09/2011
 *      Author: utn_so
 */

#ifndef FUSE_OPERATIONS_H_
#define FUSE_OPERATIONS_H_



#include <fuse.h>

static int fuselage_getattr(const char *, struct stat *);
static int fuselage_open(const char *, struct fuse_file_info *);

static int fuselage_read(const char *, char *, size_t, off_t, struct fuse_file_info *);
static int fuselage_flush(const char *, struct fuse_file_info *);
static int fuselage_rename(const char *, const char *);
static int fuselage_truncate(const char *, off_t);
static int fuselage_write(const char *, const char *, size_t, off_t, struct fuse_file_info *);
static int fuselage_create(const char *, mode_t, struct fuse_file_info *);
static int fuselage_mkdir(const char *, mode_t);
static int fuselage_rmdir(const char *);


/*


static int fuselage_rmdir(const char *);
static int fuselage_mkdir(const char *, mode_t);

*/
int fuselage_readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi);

struct fuse_operations fuselage_oper = {
	.readdir	= fuselage_readdir,
  .getattr	= fuselage_getattr,


   .open	= fuselage_open,

   .read	= fuselage_read,
   .flush = fuselage_flush,
   .rename = fuselage_rename,
   .truncate = fuselage_truncate,
   .write = fuselage_write,
   .create = fuselage_create,
   .mkdir = fuselage_mkdir,
   .rmdir = fuselage_rmdir,
   /*

   .create = fuselage_create,
   .truncate = fuselage_truncate,
   .rmdir = fuselage_rmdir,


   */
};

#endif /* FUSE_OPERATIONS_H_ */
