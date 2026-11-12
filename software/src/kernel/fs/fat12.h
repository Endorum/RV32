#pragma once


#include "stdint.h"




#define FAT_ATTR_DIR    0x10
#define FAT_ATTR_VOLUME 0x08
#define FAT_ATTR_LFN    0x0F


#define FAT_END     0xFF8
#define DIR_END     0x00
#define DIR_DELETED 0xE5


struct fat12_volume {
    uint8_t     sectors_per_cluster;
    uint16_t    root_entries;
    
    // to get byte address x_start * SECTOR_SIZE (512)
    uint32_t    fat_start;      // first sector of fat
    uint32_t    root_start;     // first sector of root directory
    uint32_t    root_sectors;   // how many sector the root dir has
    uint32_t    data_start;     // first sector of Cluster 2
    
    uint8_t*    fat;            // the FAT in RAM
};

struct fat12_entry{
    char        name[8];
    char        ext[3];
    uint8_t     attr;
    uint8_t     reserved;
    uint8_t     create_time_tenth;
    uint16_t    create_time;
    uint16_t    create_date;
    uint16_t    access_data;
    uint16_t    first_cluster_hi;
    uint16_t    write_time;
    uint16_t    write_date;
    uint16_t    first_cluster;
    uint32_t    size;
}__attribute__((packed));

_Static_assert(sizeof(struct fat12_entry) == 32, "FAT12 Entry Layout not right" );

struct fat12_bpb{
    uint8_t     jump[3];
    char        oem[8];
    uint16_t    bytes_per_sector; 
    uint8_t     sectors_per_cluster; 
    uint16_t    reserved_sectors; 
    uint8_t     number_of_fats; 
    uint16_t    number_root_entries; 
    uint16_t    number_of_sectors;
    uint8_t     med_desc_type;
    uint16_t    sectors_per_fat;
    uint16_t    sectors_per_track;
    uint16_t    number_of_heads;
    uint32_t    number_hidden_sectors;
    uint32_t    large_sector_count;
}__attribute__((packed));

_Static_assert(sizeof(struct fat12_bpb) == 36, "BPB Layout not right" );

struct fat12_ebpb{
    uint8_t     drive_number;
    uint8_t     reserved;
    uint8_t     signatur; // 0x28 or 0x29
    uint32_t    serial_number;
    char        label_string[11];
    char        sys_id_string[8];
    uint8_t     boot_code[448];
    uint16_t    boot_signatur; // 0xAA55
}__attribute__((packed));

_Static_assert(sizeof(struct fat12_ebpb) == 476, "EBPB Layout not right" );
    


void print_bpb(struct fat12_bpb bpb);
void print_ebpb(struct fat12_ebpb ebpb);
void read_boot_sector(struct fat12_bpb* bpb, struct fat12_ebpb* ebpb);
    
void fat12_mount();

void fat_name_to_str(struct fat12_entry* e, char* out);
void fat_str_to_name(const char* in, char* out);
uint32_t fat12_next_cluster(uint32_t cluster);

int dir_sector(uint16_t dir_cluster, uint32_t idx, uint32_t* sector);
int fat12_read_file(struct fat12_entry* e, uint8_t* out, uint32_t max);
int fat12_find(uint16_t dir_cluster, const char* name, struct fat12_entry* out);
void fat12_read_dir(uint16_t dir_cluster);