/*
 * proceso_filesystem.c
 *
 *  Created on: 04/09/2011
 *      Author: utn_so
 */
#include <stdio.h>
#include <stdint.h>
#include "fuse_operations.h"
#include "pfs_comm.h"

int32_t leer_bootsector();

int main(int argc, char *argv[])
{
	//SANTIAGO
	leer_bootsector();
 //return fuse_main(argc, argv, &fuselage_oper,NULL);
}


int fuselage_readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi)
{
		(void) offset;
	    (void) fi;


	    filler(buf, ".", NULL, 0);
	    filler(buf, "..", NULL, 0);
	    filler(buf, "HOLA MUNDO", NULL, 0);

	    return 0;
}

int32_t leer_bootsector()
{
	char* msg = malloc(4);
	msg[0] =  0x0;
	msg[1] =  0x1;
    msg[2] =  0x0;
    msg[3] =  0x1;
	pfs_send(msg);
}
