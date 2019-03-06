/*This file implements a simple fat12 formatting utility.*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <linux/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "fat12.h"

int fid; /* global variable set by the open() function */

int fd_write(int, byte *);
int fd_read(int, byte *);
void set_fat_entry(byte *, int, unsigned short);
void empty_buffer(byte *);
void error_handler(byte *);
boot_record_t set_boot_sector(byte *);
void set_fat_tables(byte *);
int set_root_directory_sector(void);
void format_data_area(byte *, boot_record_t);

/*
 * writes the buffer (which is equivalent in size to one sector) to the disk image
 * in the proper sector number. lseek should be able to move beyond the file's boundaries
 * in case the user wants to write at an index beyond the file's actual size and expand it.
 *
 * Paramters:
 * sector_number: the sector number in the image to write to.
 * buffer: the data buffer.
 *
 * Returns:
 * the length of the sector in case of success, -1 if any syscall errors.
 */
int fd_write(int sector_number, byte *buffer){
	int dest, len;
	dest = lseek(fid, sector_number * DEFAULT_SECTOR_SIZE, SEEK_SET);
	if (dest != sector_number * DEFAULT_SECTOR_SIZE){
		 perror("call to \'lseek\' failed.\n");
		 return -1;
	}
	len = write(fid, buffer, DEFAULT_SECTOR_SIZE);
	if (len != DEFAULT_SECTOR_SIZE){
		 perror("call to \'write\' failed.\n");
		 return -1;
	}
	return len;
}

/*markerd
 * reads the to buffer (which is equivalent in size to one sector) from the disk image.
 *
 * Paramters:
 * sector_number: the sector number in the image to read.
 * buffer: the data buffer.
 *
 * Returns:
 * the length of the sector in case of success, -1 if any syscall errors.
 */
int fd_read(int sector_number, byte *buffer){
	int dest, len;
	dest = lseek(fid, sector_number * DEFAULT_SECTOR_SIZE, SEEK_SET);
	if (dest != sector_number * DEFAULT_SECTOR_SIZE){
		 perror("call to \'lseek\' failed.\n");
		 return -1;
	}
	len = read(fid, buffer, DEFAULT_SECTOR_SIZE);
	if (len != DEFAULT_SECTOR_SIZE){
		 perror("call to \'write\' failed.\n");
		 return -1;
	}
	return len;
}

/*Big Endian*/
/*
void set_fat_entry(byte * buffer, int FATindex, unsigned short value){
	int offset = ((FATindex * 3) / 2);
	if (FATindex % 2 == 0){
		buffer[offset] = (byte)((value & 0x0ff0) >> 4);
		buffer[offset + 1] = (byte)((value & 0x000f) << 4) + buffer[offset + 1];
	}
	else {
		buffer[offset] = (byte)((value & 0x0f00) >> 8) + buffer[offset];
		buffer[offset + 1] = (byte)(value & 0x00ff);
	}
}
*/

/*
 * given a byte array (buffer), an index and a 12 bit value, this functoin stores
 * the value at the index in the array in a little endian manner: the higher 4 bits will
 * be stored in the higher index: if the index is even, the higher 4 bits will be stored
 * in the lower 4 bits of the next index, otherwise, the next index will hold the higher
 * 8 bits.
 * assumes the user will not provide an out of bound index (no error checking).
 *
 * Parameters:
 * buffer: the byte array where the value will be stored (can be the data area if the disk).
 * FATindex: the index of the 12 bit value in the data area.
 * value: a 12 bit value stored in unsigned short sized 2 bytes, wide enough to hold
 * the value, the higher 4 bits, if any, will be ignored.
 */
void set_fat_entry(byte * buffer, int FATindex, unsigned short value){
	int offset = ((FATindex * 3) / 2);
	if (FATindex % 2 == 0){
		buffer[offset] = (byte)(value & 0x0ff);
		buffer[offset + 1] = (byte)((value & 0xf00) >> 8) + (buffer[offset + 1] & 0xf0);
	}
	else {
		buffer[offset] = (byte)((value & 0x00f) << 4) + (buffer[offset] & 0x0f);
		buffer[offset + 1] = (byte)((value & 0xff0) >> 4);
	}
}

/*
 * zeroes all data in the supplied byte array.
 *
 * Parameters:
 * buffer: the byte array to clear.
 */
void empty_buffer(byte * buffer){
	int i =0;
	while(i < DEFAULT_SECTOR_SIZE)
		buffer[i++] = 0;
}

/*
 * makes sure resources are properly released and exits with error code 1:
 *
 * Parameters:
 * buffer: the byte array to release.
 */
void error_handler(byte * buffer){
	free(buffer);
	close(fid);
	exit(1);
}

/*
 * creates the boot sector structure and initializes the proper values, sets
 * the bootjump values from the table in page 9 of the file fat_paper.pdf, then
 * writes the structure to the buffer which is then written to the boot sector
 * of the image file.
 *
 * Parameters:
 * buffer: a byte array which contains the boot sector.
 *
 * Returns:
 * the boot sector structure.
 */
boot_record_t set_boot_sector(byte * buffer){
	boot_record_t boot_record ={"", "FAT12",DEFAULT_SECTOR_SIZE, 1, 1, 2, 224, 2880, 0xf0, 9, 18, 2, 0, 0};
	boot_record.bootjmp[0] = 0xeb;
	boot_record.bootjmp[2] = 0x90;

	memcpy(buffer, &boot_record, sizeof(boot_record));

	if (fd_write(0, buffer) == -1) error_handler(buffer);
	return boot_record;
}

/*
 * creates and sets the 2 fat tables: the tables should be empty, and only the
 * first two 12bit values of each are set to 0xff0 and 0xfff, though they hold
 * no special meaning in FAT12 case (this is for demonstration purposes only).
 *
 * Parameters:
 * buffer: a byte array to hold the sectors to be stored.
 */
void set_fat_tables(byte * buffer){
	int i;
	for(i = FAT_TABLE_1_START + 1; i <= FAT_TABLE_2_END; i++){
		if (i == FAT_TABLE_2_START) continue;
		if (fd_write(i, buffer) == -1) error_handler(buffer);
	}

	set_fat_entry(buffer, 0, 0xff0);
	set_fat_entry(buffer, 1, 0xfff);
	if(fd_write(FAT_TABLE_1_START, buffer) == -1 || fd_write(FAT_TABLE_2_START, buffer) == -1) error_handler(buffer);
}

/*
 * the root directory shall contain empty folders/files, marked with the special character
 * 0xe5 as the first in the name field, though it is pointless in this case since the rest
 * of the directory will be also zeroes (first character equal to 0 would have been enough),
 * and the drive will not be restorable anyway (otherwise I would have copied these sectors
 * from the original image and just set the first character of the name field to 0xe5).
 * calloc is used to allocate the memmory used for the sector to make sure all values are
 * initilized to zeroes and NULL. the sector is written then to the image file at the proper
 * indexes (19-32).
 *
 * Returns:
 * 0 for success, -1 if any system call fails.
 */
int set_root_directory_sector(void){
	int i;
	fat_dirent_t * root_directory_sector = (fat_dirent_t *)calloc(DEFAULT_SECTOR_SIZE / sizeof(fat_dirent_t), sizeof(fat_dirent_t));
	if(!root_directory_sector){
		perror("call to \'calloc\' failed.\n");
		return -1;
	}
	for(i = 0; i < DEFAULT_SECTOR_SIZE / sizeof(fat_dirent_t); i++)
		root_directory_sector[i].name[0] = EMPTY_FOLDER_MARK;
	for(i = ROOT_DIRECTORY_START; i <= ROOT_DIRECTORY_END; i++)
		if (fd_write(i, (byte *)(root_directory_sector)) == -1){
			free(root_directory_sector);
			return -1;
		}
	free(root_directory_sector);
	return 0;
}

/*
 * writes a sector which contains zeroes to all the sectors of the image file
 * in the data area.
 *
 * Parameters:
 * buffer: byte array which contains the zero values to be written to each sector.
 * boot_record: the boot recortd which contains the file system info.
 */
void format_data_area(byte * buffer, boot_record_t boot_record){
	int i;
	empty_buffer(buffer);
	for(i = DATA_AREA_START; i < boot_record.sector_count; i++){
		if (fd_write(i, buffer) == -1) error_handler(buffer);
	}
}

int main(int argc, char *argv[]){
	byte * buffer;
	boot_record_t boot_record;

	if (argc != 2) {
		printf("Usage: %s <floppy_image>\n", argv[0]);
		exit(1);
	}

	if ( (fid = open (argv[1], O_RDWR|O_CREAT, 0644))  < 0 ){
		perror("Error: unable to create new image file.\n");
		return 1;
	}

	buffer = (byte *)calloc(DEFAULT_SECTOR_SIZE, sizeof(byte));
	if (!buffer){
		perror("could not allocate buffer.\n");
		close(fid);
		return 1;
	}

	/* See fat12.pdf for layout details

	* Step 1. Create floppy.img with the school solution. Read the values
	* from the boot sector of the floppy.img and initialize boot sector
	* with those values. If you read bootsector of the floppy.img correctly,
	* the values will be:

	* sector_size: 512 chars
	* sectors_per_cluster: 1 sectors
	* reserved_sector_count: 1
	* number_of_fats: 2
	* number_of_dirents: 224
	* sector_count: 2880 sectors
	* media_type: 0xf0
	* fat_size_sectors: 9
	* sectors_per_track: 18
	* nheads: 2
	* sectors_hidden: 0
	* sector_count_large: 0 sectors
	*/

	boot_record = set_boot_sector(buffer);

	/* Step 2. Set FAT1/FAT2 table entires to 0x0000 (unused cluster)
       according to the fat12.pdf. */

	empty_buffer(buffer);
	set_fat_tables(buffer);

	/* Step 3. Set direntries as free (0xe5) according to the fat12.pdf.
     * While zeroing dentries will also work, we prefer a solution
     * that mark them free. In that case it will be possible to perform
     * unformat operation. However, school solution simply zeros dentries.
	 */

	 if (set_root_directory_sector() == -1) error_handler(buffer);

	/* Step 4. Handle data block (e.g you can zero them or just leave
	 * untouched. What are the pros/cons?)
	 * I chose to set them to zeroes:
	 * pros: better for security reasons.
	 * cons: slower on old machines, no recovery of data is possible, might
	 * contribute to higher rates of wear.
	 */

	 format_data_area(buffer, boot_record);

	/* For steps 2-3, you can also read the sectors from the image that was
	   generated by the school solution if not sure what should be the values.*/

	if (lseek(fid, 0, SEEK_SET)){
		perror("call to \'lseek\' failed.\n");
		error_handler(buffer);
	}
	if (read(fid, &boot_record, sizeof(boot_record)) < 0) {
		perror("could not read the Boot Record.\n");
		error_handler(buffer);
	}

	printf("sector_size: %d\n", boot_record.sector_size);
	printf("sectors_per_cluster: %d\n", boot_record.sectors_per_cluster);
	printf("reserved_sector_count: %d\n", boot_record.reserved_sector_count);
	printf("number_of_fats: %d\n", boot_record.number_of_fats);
	printf("number_of_dirents: %d\n", boot_record.number_of_dirents);
	printf("sector_count: %d\n", boot_record.sector_count);

	free(buffer);
	close(fid);
	return 0;
}
