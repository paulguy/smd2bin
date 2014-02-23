/* based on smd-rom.txt <http://emu-docs.org/Genesis/File%20Formats/smd_rom.txt> */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#define SMD_BLOCK_SIZE (16384)
#define SMD_HEADER_SIZE (512)
#define SMD_MAGIC_ADDR (8)

typedef struct {
	long size;
	uint8_t reported_blocks;
	long calculated_blocks;
	uint8_t unknown;
	uint8_t notfirst;
	uint16_t magic;
} SMDHeader;

int main(int argc, char **argv) {
	SMDHeader header;
	uint8_t smd_block[SMD_BLOCK_SIZE];
	uint8_t bin_block[SMD_BLOCK_SIZE];
	FILE *infile, *outfile;
	char *filename;
	int namelen;
	int i, j;

	if(argc < 2) {
		fprintf(stderr, "USAGE: smd2bin <filename>\n");
		exit(EXIT_FAILURE);
	}

	namelen = snprintf(NULL, 0, "%s.bin", argv[1]) + 1;
	filename = malloc(namelen);
	if(filename == NULL) {
		fprintf(stderr, "Couldn't allocate memory.\n");
		exit(EXIT_FAILURE);
	}
	snprintf(filename, namelen, "%s.bin", argv[1]);

	infile = fopen(argv[1], "rb");
	if(infile == NULL) {
		fprintf(stderr, "Couldn't open %s for reading (%s).\n", argv[1], strerror(errno));
		exit(EXIT_FAILURE);
	}

	if(fseek(infile, 0, SEEK_END) == -1) {
		perror("fseek");
		exit(EXIT_FAILURE);
	}

	header.size = ftell(infile);
	header.calculated_blocks = (header.size - SMD_HEADER_SIZE) / SMD_BLOCK_SIZE;
	rewind(infile);

	if(fread(&(header.reported_blocks), 1, 1, infile) == -1) {
		perror("fread");
		exit(EXIT_FAILURE);
	}
	if(fread(&(header.unknown), 1, 1, infile) == -1) {
		perror("fread");
		exit(EXIT_FAILURE);
	}
	if(fread(&(header.notfirst), 1, 1, infile) == -1) {
		perror("fread");
		exit(EXIT_FAILURE);
	}

	if(fseek(infile, SMD_MAGIC_ADDR, SEEK_SET) == -1) {
		perror("fseek");
		exit(EXIT_FAILURE);
	}
	if(fread(&(header.magic), 2, 1, infile) == -1) {
		perror("fread");
		exit(EXIT_FAILURE);
	}

	fprintf(stderr, " In File: %s\n"
	                "Out File: %s\n"
	                "Size: %li\n"
	                "Reported Blocks: %hhu (ignored)\n"
	                "Calculated Blocks: %li\n"
	                "Unknown Field: %hhu\n",
	        argv[1], filename, header.size, header.reported_blocks,
	        header.calculated_blocks, header.unknown);
	if(header.notfirst) {
		fprintf(stderr, "Not the first file in this set.\n");
	} else {
		fprintf(stderr, "First file in this set or single file set.\n");
	}
	fprintf(stderr, "Magic number: 0x%hX\n\n", header.magic);
	if(header.reported_blocks != header.calculated_blocks)
		fprintf(stderr, "WARNING: Reported blocks doesn't match calculated blocks.  Using calculated blocks.\n");
	if(header.calculated_blocks * SMD_BLOCK_SIZE + SMD_HEADER_SIZE != header.size) {
		fprintf(stderr, "WARNING: %li bytes junk at end.\n",
		        header.size - (header.calculated_blocks * SMD_BLOCK_SIZE +
		        SMD_HEADER_SIZE));
	}
	if(header.magic != 0xBBAA) {
		fprintf(stderr, "ERROR: Magic number is incorrect.\n");
		exit(EXIT_FAILURE);
	}

	outfile = fopen(filename, "wb");
	if(outfile == NULL) {
		fprintf(stderr, "Couldn't open %s for writing (%s).\n", filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if(fseek(infile, SMD_HEADER_SIZE, SEEK_SET) == -1) {
		perror("fseek");
		exit(EXIT_FAILURE);
	}

	fprintf(stderr, "\n");
	for(i = 0; i < (int)(header.calculated_blocks); i++) {
		if(fread(smd_block, SMD_BLOCK_SIZE, 1, infile) == -1) {
			perror("fread");
			exit(EXIT_FAILURE);
		}

		for(j = 0; j < SMD_BLOCK_SIZE / 2; j++)
			bin_block[j * 2 + 1] = smd_block[j];
		for(j = 0; j < SMD_BLOCK_SIZE / 2; j++)
			bin_block[j * 2] = smd_block[SMD_BLOCK_SIZE / 2 + j];

		if(fwrite(bin_block, SMD_BLOCK_SIZE, 1, outfile) == -1) {
			perror("fwrite");
			exit(EXIT_FAILURE);
		}
		fprintf(stderr, "\r%i/%li", i + 1, header.calculated_blocks);
	}

	fprintf(stderr, "\nDone.\n");
	fclose(outfile);
	fclose(infile);

	exit(EXIT_SUCCESS);
}
