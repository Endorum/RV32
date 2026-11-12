
#include <stddef.h>
#include <stdint.h>

#include "fat12.h"
#include "kmalloc.h"
#include "kstring.h"
#include "kctype.h"

#include "cpu.h"
#include "mem.h"
#include "hd.h"

#include "kstdio.h"




void print_bpb(struct fat12_bpb bpb){
    
    char oem[9];
    
    memcpy(oem, bpb.oem, 8);
    
    oem[8] = '\0';
    
    kprintf("jump:                  %02X%02X%02X\n", bpb.jump[0],bpb.jump[1],bpb.jump[2]);   //  uint8_t
    kprintf("");
    kprintf("oem:                   %s\n",   oem);                       //  char*
    kprintf("bytes_per_sector:      %04X\n", bpb.bytes_per_sector);      //  uint16_t
    kprintf("sectors_per_cluster:   %02X\n", bpb.sectors_per_cluster);   //  uint8_t
    kprintf("reserved_sectors:      %04X\n", bpb.reserved_sectors);      //  uint16_t
    kprintf("number_of_fats:        %02X\n", bpb.number_of_fats);        //  uint8_t
    kprintf("number_root_entries:   %04X\n", bpb.number_root_entries);   //  uint16_t
    kprintf("number_of_sectors:     %04X\n", bpb.number_of_sectors);     //  uint16_t
    kprintf("med_desc_type:         %02X\n", bpb.med_desc_type);         //  uint8_t
    kprintf("sectors_per_fat:       %04X\n", bpb.sectors_per_fat);       //  uint16_t
    kprintf("sectors_per_track:     %04X\n", bpb.sectors_per_track);     //  uint16_t
    kprintf("number_of_heads:       %04X\n", bpb.number_of_heads);       //  uint16_t
    kprintf("number_hidden_sectors: %08X\n", bpb.number_hidden_sectors); //  uint32_t
    kprintf("large_sector_count:    %08X\n", bpb.large_sector_count);    //  uint32_t
}

void print_ebpb(struct fat12_ebpb ebpb){
    kprintf("drive_number:      %02X\n", ebpb.drive_number); //  uint8_t
    kprintf("reserved:          %02X\n", ebpb.reserved); //  uint8_t
    kprintf("signatur:          %02X\n", ebpb.signatur);// 0x28 or 0x29  uint8_t
    kprintf("serial_number:     %02X\n", ebpb.serial_number); //  uint32_t
    
    char label_string[12];
    memcpy(label_string, ebpb.label_string, 11);
    label_string[11] = '\0';
    
    char sys_id_string[9];
    memcpy(sys_id_string, ebpb.sys_id_string, 8);
    sys_id_string[8] = '\0';
    
    kprintf("label_string:      %s\n",   label_string);
    kprintf("sys_id_string:     %s\n",   sys_id_string); //  char
    
    // lets not print the entiere code
    //kprintf("%02X", ebpb.boot_code[448]); //  uint8_t
    
    kprintf("boot_signatur:     %02X\n", ebpb.boot_signatur); // 0xAA55 uint16_t
}

void read_boot_sector(struct fat12_bpb* bpb, struct fat12_ebpb* ebpb){

    kprintf("Reading boot sector...\n");

    // read first sector
    uint8_t buf[SECTOR_SIZE];
    hd_read_sector(0, buf);

    // cast into bpb and ebpb
    *bpb     = *((struct fat12_bpb*)buf);
    *ebpb    = *((struct fat12_ebpb*)(buf + sizeof(struct fat12_bpb)));


    //print both
    kprintf("\nBIOS Parameter Block (BPB):\n");
    print_bpb(*bpb);
    kprintf("\nExtended Boot Record (EBPB):\n");    
    print_ebpb(*ebpb);
    
}

static struct fat12_volume vol; // THE volume
    
void fat12_mount(){
    
    kprintf("mounting Harddisk as FAT12...\n");
    
    struct fat12_bpb bpb;
    struct fat12_ebpb ebpb;
    read_boot_sector(&bpb, &ebpb);
    
    
    vol.sectors_per_cluster = bpb.sectors_per_cluster;
    vol.root_entries        = bpb.number_root_entries;
    
    vol.fat_start       = bpb.reserved_sectors;
    vol.root_start      = vol.fat_start + bpb.number_of_fats * bpb.sectors_per_fat;
    vol.root_sectors    = (bpb.number_root_entries * 32 + bpb.bytes_per_sector - 1) / bpb.bytes_per_sector;
    vol.data_start      = vol.root_start + vol.root_sectors;
    
    // reserve space for the whole fat
    vol.fat = kmalloc(bpb.sectors_per_fat * SECTOR_SIZE);
    
    // preloading entire fat
    for(uint32_t i=0;i<bpb.sectors_per_fat; i++){
        hd_read_sector(vol.fat_start + i, vol.fat + i * SECTOR_SIZE);
    }
    
    kprintf("\n");
    kprintf("VOLUME:\n");
    
    kprintf("sectors_per_cluster:   %02X\n", vol.sectors_per_cluster);
    kprintf("root_entries:          %04X\n", vol.root_entries);
    kprintf("fat_start:             %08X\n", vol.fat_start); 
    kprintf("root_start:            %08X\n", vol.root_start); 
    kprintf("root_sectors:          %08X\n", vol.root_sectors); 
    kprintf("data_start:            %08X\n", vol.data_start); 
    kprintf("fat:                   %08X\n", vol.fat);


    
    
    
}

void fat_name_to_str(struct fat12_entry* e, char* out){
    
    int i=0;
    for(;i<8; i++){
        if(e->name[i] == ' ' || !e->name[i]) break;
        out[i] = e->name[i];
    }
    
    if(e->attr & FAT_ATTR_DIR){
        out[i] = '\0';
        return;
    }
    
    out[i] = '.';
    
    int j=0;
    for(;j<3;j++){
        if(e->ext[j] == ' ' || !e->ext[j]) break;
        out[i + j + 1] = e->ext[j];
    }
    
    out[i + j + 1] = '\0';
}

void fat_str_to_name(const char* in, char* out){
    // fill out with spaces first
    for(int i=0;i<11;i++) out[i] = ' ';
        
    if (in[0] == '.' && in[1] == '.' && in[2] == '\0') {
        out[0] = '.';
        out[1] = '.';
        return;
    }

    if(in[0] == '.' && in[1] == '\0'){
        out[0] = '.';
        return;
    }
     
    const char* p = in;
    
    // name
    for(int n=0; *p && *p != '.'; p++){
        if(n < 8) out[n++] = toupper(*p);
    }
    
    // extension
    if(*p == '.'){
        p++;
        for(int e=0; *p && e < 3; p++){
            out[8 + e++] = toupper(*p);
        }
    }
    
}

uint32_t fat12_next_cluster(uint32_t cluster){
    uint32_t off = cluster + cluster / 2;
    uint16_t word = vol.fat[off] | (vol.fat[off + 1] << 8);
    return (cluster & 0x1) ? (word >> 4) : (word & 0xFFF);
}


int fat12_read_file(struct fat12_entry* e, uint8_t* out, uint32_t max){
    

    uint32_t remaining = e->size;
    if(remaining > max) remaining = max;
    
    uint32_t done = 0;
    uint32_t cluster = e->first_cluster;
    uint8_t buf[SECTOR_SIZE];
    
    while(remaining > 0){
        uint32_t first = vol.data_start + (cluster - 2) * vol.sectors_per_cluster;
        
        for(uint32_t s=0; s < vol.sectors_per_cluster && remaining > 0; s++){
            hd_read_sector(first + s, buf);
            
            uint32_t n = remaining < SECTOR_SIZE ? remaining : SECTOR_SIZE;
            memcpy(out + done, buf, n);
            
            done += n;
            remaining -= n;
        }
        
        
        cluster = fat12_next_cluster(cluster);
        if(cluster >= FAT_END) break;
        
    }
    
    
    return done;
}

// returns in *sector the idx-nth sector of the directory
// returns 0 if the directory doesnt have that many sectors
int dir_sector(uint16_t dir_cluster, uint32_t idx, uint32_t* sector){
    
    if(dir_cluster == 0){
        if(idx >= vol.root_sectors) return 0;
        *sector = vol.root_start + idx;
        return 1;
    }
    
    // sub directories: run along chain till the correct cluster
    uint32_t cluster = dir_cluster;
    for(uint32_t skip = idx / vol.sectors_per_cluster; skip > 0; skip--) {
        cluster = fat12_next_cluster(cluster);
        if(cluster >= FAT_END) return 0;
    }
    
    *sector = vol.data_start + (cluster - 2) * vol.sectors_per_cluster
                + idx % vol.sectors_per_cluster;
    
    return 1;
}

int fat12_find(uint16_t dir_cluster, const char* name, struct fat12_entry* out){
    // returns 1 if found, 0 if not
    // kprintf("searching for '%s'...\n", name);
    
    char fat_name[11];
    fat_str_to_name(name, fat_name);
    
    uint8_t buf[SECTOR_SIZE];
    
    uint32_t sec;
    
    for(uint32_t idx = 0; dir_sector(dir_cluster, idx, &sec); idx++){
        hd_read_sector(sec, buf);

        for (int i=0; i<SECTOR_SIZE / 32; i++){
            struct fat12_entry* e = (struct fat12_entry*)(buf + i * 32);

            if(e->name[0] == DIR_END)       return 0;     // completly done not only with this sector
            if(e->name[0] == DIR_DELETED)   continue;
            if(e->attr == FAT_ATTR_LFN)     continue;   // long name entry
            if(e->attr & FAT_ATTR_VOLUME)   continue;   // label

            if(memcmp(e->name, fat_name, 11) == 0) {
                *out = *e;
                return 1;
            }

        }

    } 
    
    
    
    return 0;
}

void fat12_read_dir(uint16_t dir_cluster){
    
    uint8_t buf[SECTOR_SIZE];
    
    uint32_t sec;
    for(uint32_t idx = 0; dir_sector(dir_cluster, idx, &sec); idx++){
        hd_read_sector(sec, buf);

        for (int i=0; i<SECTOR_SIZE / 32; i++){
            struct fat12_entry* e = (struct fat12_entry*)(buf + i * 32);

            if(e->name[0] == DIR_END)       return;     // completly done not only with this sector
            if(e->name[0] == DIR_DELETED)   continue;
            if(e->attr == FAT_ATTR_LFN)     continue;   // long name entry
            if(e->attr & FAT_ATTR_VOLUME)   continue;   // label
                
            if(e->attr & FAT_ATTR_DIR){
                char out[8];
                fat_name_to_str(e, out);
                kprintf("%s\n", out);
            }else{
                char out[13];
                fat_name_to_str(e, out);
                kprintf("%s \t %d Bytes\n", out, e->size);
            }

            

        }

    } 
}