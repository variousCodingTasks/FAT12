// Desc: fat-12 file system structs
// By  : OS course, Open University

#ifndef __FAT_12__
#define __FAT_12__

#define DEFAULT_SECTOR_SIZE                 512

#define ONE_K                               1024
#define ONE_MEG                             (1024 * 1024)
#define ONE_GIG                             (1024 * 1024 * 1024)

#define FAT_TABLE_1_START                   1
#define FAT_TABLE_2_START                   10
#define FAT_TABLE_2_END                     18
#define EMPTY_FOLDER_MARK                   0xe5
#define ROOT_DIRECTORY_START                19
#define ROOT_DIRECTORY_END                  32
#define DATA_AREA_START                     33

/*
 * FAT structures
 */
typedef struct fat_file_date {
    uint32_t    day:5;
    uint32_t    month:4;
    uint32_t    year:7;
} __attribute__ ((packed)) fat_file_date_t;

typedef struct fat_file_time {
    uint32_t    sec:5;
    uint32_t    min:6;
    uint32_t    hour:5;
} __attribute__ ((packed)) fat_file_time_t;

/*
 * File info
 */
typedef struct fat_file {
    uint8_t     name[8];           /*  0 file name */
    uint8_t     ext[3];            /*  8 file extension */
    uint8_t     attr;              /* 11 attribute byte */
    uint8_t     winnt_flags;       /* 12 case of short filename */
    uint8_t     create_time_secs;  /* 13 creation time, milliseconds (?) */
    uint16_t    create_time;       /* 14 creation time */
    uint16_t    create_data;       /* 16 creation date */
    uint16_t    last_access;       /* 18 last access date */
    uint16_t    h_first_cluster;   /* 20 start cluster, Hi */
    fat_file_time_t lm_time;       /* 22 time stamp */
    fat_file_date_t lm_date;       /* 24 date stamp */
    uint16_t    l_first_cluster;   /* 26 starting cluster number */
    uint32_t    size;              /* 28 size of the file */
} __attribute__ ((packed)) fat_dirent_t;


/*
 * Boot record.
 */
typedef struct {
    uint8_t     bootjmp[3];  /* 0  Jump to boot code */
    uint8_t     oem_id[8];   /* 3  OEM name & version */
    uint16_t    sector_size; /* 11 Bytes per sector hopefully 512 */
    uint8_t     sectors_per_cluster;    /* 13 Cluster size in sectors */
    uint16_t    reserved_sector_count;  /* 14 Number of reserved (boot) sectors */
    uint8_t     number_of_fats;         /* 16 Number of FAT tables hopefully 2 */
    uint16_t    number_of_dirents;      /* 17 Number of directory slots */

    /*
     * If 0, look in sector_count_large
     */
    uint16_t    sector_count;           /* 19 Total sectors on disk */
    uint8_t     media_type;             /* 21 Media descriptor=first byte of FAT */

    /*
     * Set for FAT12/16 only.
     *
     * The number of blocks occupied by one copy of the File Allocation Table.
     */
    uint16_t    fat_size_sectors;       /* 22 Sectors in FAT */
    uint16_t    sectors_per_track;      /* 24 Sectors/track */
    uint16_t    nheads;                 /* 26 Heads */
    uint32_t    sectors_hidden;         /* 28 number of hidden sectors */
    uint32_t    sector_count_large;     /* 32 big total sectors */

} __attribute__ ((packed)) boot_record_t;

typedef unsigned char byte;

#endif
