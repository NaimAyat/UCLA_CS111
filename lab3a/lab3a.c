//NAME: Naim Ayat
//EMAIL: naimayat@ucla.edu
//ID: 000000000
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "ext2_fs.h"

// Globals
int mbOff = 0, files, tbl;
__u32 blockLen;

// Bit masking function
int mask(int8_t x, int y) {
	return (x & (1 << y));
}

// Block printing function
void printBlocks(int inode, unsigned int paBlock, __u32 chBlock, int layerA) {
	fprintf(stdout, "INDIRECT");
	fprintf(stdout, ",%u", inode);
	fprintf(stdout, ",%d", layerA);
	fprintf(stdout, ",%u", mbOff);
	fprintf(stdout, ",%u", paBlock);
	fprintf(stdout, ",%u", chBlock);
	fprintf(stdout, "\n");
	return;
}

// Prototypes
void blockHandler(int inode, unsigned int paBlock,
	int layerA, int layerB);

void blockHandler(int inode, unsigned int paBlock, int layerA, int layerB) {
	if (layerA > 0) {
		void* blockBuf = malloc(blockLen);
		if ((unsigned)blockLen > 
			pread(files, blockBuf, blockLen, blockLen*paBlock)) {
			fprintf(stderr, "ERROR: Failed to read from indirection table. \n");
			exit(1);
		}
		__u32 *iTbl = (__u32 *)blockBuf;
		int i;
		for (i = 0; i < tbl; i++) {
			if (iTbl[i] == 0) {
				switch (layerA) {
				case(1):
					mbOff++;
					break;
				case(2):
					mbOff += tbl;
					break;
				case (3):
					mbOff += (tbl*tbl);
					break;
				default:
					(void)mbOff;
				}
				continue;
			}
			int prevBlock = mbOff;
			printBlocks(inode, paBlock, iTbl[i], layerA);
			blockHandler(inode, iTbl[i], layerA - 1, layerB);
			int chBlock = mbOff;
			mbOff = prevBlock;
			mbOff = chBlock;
		}
	}
	else
		mbOff++;
	return;
}

// Main function
int main(int argc, char** argv) {
	if (argc <= 1) {
		fprintf(stderr, "ERROR: Invalid image file specified. \n");
		exit(1);
	}

	char *FH = argv[1];
	files = open(FH, O_RDONLY);
	if (files <= -1) {
		fprintf(stderr, "ERROR: Failed to open specified image. \n");
		exit(1);
	}

	// Super block buffer
	void* sBuf = malloc(sizeof(struct ext2_super_block));

	if ((unsigned) sizeof(struct ext2_super_block) > 
		pread(files, sBuf, sizeof(struct ext2_super_block), 1024)) {
		fprintf(stderr, "ERROR: Failed to read super block. \n");
		exit(1);
	}

	// Super block
	struct ext2_super_block *sBlock = (struct ext2_super_block *) sBuf;

	blockLen = EXT2_MIN_BLOCK_SIZE << sBlock->s_log_block_size;

	fprintf(stdout, "%s", "SUPERBLOCK");
	fprintf(stdout, ",%u", sBlock->s_blocks_count);
	fprintf(stdout, ",%u", sBlock->s_inodes_count);
	fprintf(stdout, ",%u", EXT2_MIN_BLOCK_SIZE << sBlock->s_log_block_size);
	fprintf(stdout, ",%u", sBlock->s_inode_size);
	fprintf(stdout, ",%u", sBlock->s_blocks_per_group);
	fprintf(stdout, ",%u", sBlock->s_inodes_per_group);
	fprintf(stdout, ",%u\n", sBlock->s_first_ino);

	// Group table buffer
	void *gtBuf = malloc(sizeof(struct ext2_group_desc));

	if ((unsigned) sizeof(struct ext2_group_desc) > 
		pread(files, gtBuf, sizeof(struct ext2_group_desc), blockLen * 2)) {
		fprintf(stderr, "ERROR: Failed to read group descriptor table. \n");
		exit(1);
	}

	// Group table
	struct ext2_group_desc *gt = (struct ext2_group_desc *) gtBuf;

	fprintf(stdout, "%s", "GROUP");
	fprintf(stdout, ",%u", 0);
	fprintf(stdout, ",%u", sBlock->s_blocks_count);
	fprintf(stdout, ",%u", sBlock->s_inodes_count);
	fprintf(stdout, ",%u", gt->bg_free_blocks_count);
	fprintf(stdout, ",%u", gt->bg_free_inodes_count);
	fprintf(stdout, ",%u", gt->bg_block_bitmap);
	fprintf(stdout, ",%u", gt->bg_inode_bitmap);
	fprintf(stdout, ",%u\n", gt->bg_inode_table);

	__u32 bbVar = (gt->bg_block_bitmap) * blockLen;
	__u32 ibmVar = (gt->bg_inode_bitmap) * blockLen;

	// Block bitmap
	void *bbBuf = malloc(blockLen);
	if ((unsigned)blockLen > 
		pread(files, bbBuf, blockLen, bbVar)) {
		fprintf(stderr, "ERROR: Failed to read block bitmap. \n");
		exit(1);
	}

	int8_t *bb = (int8_t *)bbBuf;

	// inode bitmap
	void *iBuf = malloc(blockLen);
	if ((unsigned)blockLen > pread(files, iBuf, blockLen, ibmVar)) {
		fprintf(stderr, "ERROR: Failed to read inode bitmap.");
		exit(1);
	}
	int8_t *ibm = (int8_t *)iBuf;

	int i, j;
	for (i = 0, j = 0; (unsigned)j < 
		sBlock->s_blocks_per_group; j++) {
		if (mask(bb[j / 8], i) == 0) {
			fprintf(stdout, "BFREE,");
			fprintf(stdout, "%d\n", j + 1);
		}
		i = (i + 1) % 8;
	}
	for (i = 0, j = 0; (unsigned)j < 
		sBlock->s_inodes_per_group; j++) {
		if (mask(ibm[j / 8], i) == 0) {
			fprintf(stdout, "IFREE,");
			fprintf(stdout, "%d\n", j + 1);
		}
		i = (i + 1) % 8;
	}

	unsigned int inodeLen = sBlock->s_inode_size;

	// inode table buffer
	void* iTblBuf = malloc(inodeLen);
	tbl = blockLen / sizeof(__u32);

	unsigned int k;

	for (k = 0; k < sBlock->s_inodes_per_group; k++) {
		if ((unsigned)inodeLen > 
			pread(files, iTblBuf, inodeLen, 
			(blockLen * gt->bg_inode_table) + (k * (inodeLen)))) {
			fprintf(stderr, "ERROR: Failed to read inode table. \n");
			exit(1);
		}

		struct ext2_inode *iTbl = (struct ext2_inode *) iTblBuf;
		struct ext2_inode chInode = *(iTbl);

		if (chInode.i_links_count == 0)
			continue;
		if (chInode.i_mode == 0)
			continue;

		fprintf(stdout, "INODE");
		fprintf(stdout, ",%u", k + 1);

		char opt = '?';
		switch (chInode.i_mode & 0xF000) {
			case (0x4000):
				opt = 'd';
				break;
			case(0x8000):
					opt = 'f';
					break;
			case (0xA000):
					opt = 's';
					break;
			default:
				(void)opt;
		}

		fprintf(stdout, ",%c", opt);
		fprintf(stdout, ",%o", chInode.i_mode & 0x0FFF);
		fprintf(stdout, ",%u", chInode.i_uid);
		fprintf(stdout, ",%u", chInode.i_gid);
		fprintf(stdout, ",%u", chInode.i_links_count);

		// Time conversion
		time_t t1 = chInode.i_mtime;
		struct tm t2;
		static const char arr[] = "%D %T";
		gmtime_r(&t1, &t2);
		char res[32];
		strftime(res, sizeof(res), arr, &t2);
		time_t t3 = chInode.i_atime;
		struct tm t4;
		gmtime_r(&t3, &t4);
		char resu[32];
		strftime(resu, sizeof(resu), arr, &t4);
		fprintf(stdout, ",%s", resu);
		fprintf(stdout, ",%s", res);
		fprintf(stdout, ",%s", resu);

		fprintf(stdout, ",%u", chInode.i_size);
		fprintf(stdout, ",%u", chInode.i_blocks);

		int m;
		for (m = 0; m < EXT2_N_BLOCKS; m++) {
			fprintf(stdout, ",%u", chInode.i_block[m]);
		}
		fprintf(stdout, "\n");

		if (opt == 'd') {
			int n = 0, o = 0;
			for (;;) {
				void* dirBuf = malloc(sizeof(struct ext2_dir_entry));
				if ((unsigned) sizeof(struct ext2_dir_entry) > 
					pread(files, dirBuf, sizeof(struct ext2_dir_entry), 
					(blockLen*chInode.i_block[n]) + o)) {
					fprintf(stderr, "ERROR: Failed to read from directory. \n");
					exit(1);
				}
				struct ext2_dir_entry dirDat = *((struct ext2_dir_entry *) dirBuf);

				if (dirDat.inode == 0)
					break;

				fprintf(stdout, "DIRENT");
				fprintf(stdout, ",%u", k + 1);
				fprintf(stdout, ",%u", (n*blockLen) + o);
				fprintf(stdout, ",%u", dirDat.inode);
				fprintf(stdout, ",%u", dirDat.rec_len);
				fprintf(stdout, ",%u", dirDat.name_len);
				fprintf(stdout, ",'");

				int p;
				for (p = 0; p < dirDat.name_len; p++) {
					fprintf(stdout, "%c", dirDat.name[p]);
				}
				fprintf(stdout, "'\n");
				o += dirDat.rec_len;
				if ((unsigned)o >= blockLen) {
					n++;
					o = 0;
				}
			}
		}
		if (opt == 'f') {
			int q = k + 1;
			mbOff = 12;
			blockHandler(q, chInode.i_block[12], 1, 1);
			blockHandler(q, chInode.i_block[13], 2, 2);
			blockHandler(q, chInode.i_block[14], 3, 3);
		}
		if (opt == 'd') {
			int q = k + 1;
			mbOff = 12;
			blockHandler(q, chInode.i_block[12], 1, 1);
			blockHandler(q, chInode.i_block[13], 2, 2);
			blockHandler(q, chInode.i_block[14], 3, 3);
		}
	}
	exit(0);
}