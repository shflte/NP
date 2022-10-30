#include <stdint.h>
#include <sys/types.h>    
#include <sys/stat.h>    
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>

typedef struct {
    uint32_t magic;
    int32_t  off_str;
    int32_t  off_dat;
    uint32_t n_files;
} __attribute((packed)) pako_header_t;

typedef struct {
    uint32_t filename_offset;
    uint32_t filesize;
    uint32_t content_offset;
    uint64_t checksum;
} __attribute((packed)) pako_file_e_t;

u_int64_t endian_invert_64(u_int64_t x) {
    return (((x) >> 56) 
    | (((x) & 0x00FF000000000000) >> 40) 
    | (((x) & 0x0000FF0000000000) >> 24) 
    | (((x) & 0x000000FF00000000) >> 8) 
    | (((x) & 0x00000000FF000000) << 8) 
    | (((x) & 0x0000000000FF0000) << 24) 
    | (((x) & 0x000000000000FF00) << 40) 
    | ((x) << 56));
}

int main(int argc, char *argv[]) {
    pako_header_t header;

    int fd = open(argv[1], O_RDONLY);

    // read header
    read(fd, &header, sizeof(pako_header_t));

    // read file e 
    pako_file_e_t* file_e_arr = calloc(header.n_files, sizeof(pako_file_e_t));
    for (int i = 0; i < header.n_files; i++) {
        read(fd, &file_e_arr[i], sizeof(pako_file_e_t));
        file_e_arr[i].filesize = ntohl(file_e_arr[i].filesize);
    }

    // calculate file name length for each file
    int* file_name_len = calloc(header.n_files, sizeof(int));
    for (int i = 0; i < header.n_files - 1; i++) {
        file_name_len[i] = file_e_arr[i + 1].filename_offset - file_e_arr[i].filename_offset;
    }
    file_name_len[header.n_files - 1] = header.off_dat - header.off_str - file_e_arr[header.n_files - 1].filename_offset;

    // read file names
    // int max_filename_size = header.off_dat - header.off_str;
    char** file_name_arr = calloc(header.n_files, sizeof(char*));
    for (int i = 0; i < header.n_files; i++) {
        // file_name_arr[i] = calloc(header.n_files, file_name_len[i]);
        // read(fd, &file_name_arr[i], file_name_len[i]);
        file_name_arr[i] = calloc(header.n_files, 20);
        read(fd, file_name_arr[i], file_name_len[i]);
    }

    // unpack
    int num_of_segments;
    uint64_t** segment_arr = calloc(header.n_files, sizeof(uint64_t*));
    uint64_t calculated_checksum;
    for (int i = 0; i < header.n_files; i++) {
        num_of_segments = ((file_e_arr[i].filesize % 8) == 0) ? (file_e_arr[i].filesize / 8) : (file_e_arr[i].filesize / 8 + 1);
        segment_arr[i] = calloc(num_of_segments, sizeof(uint64_t));
        calculated_checksum = 0;
        lseek(fd, header.off_dat + file_e_arr[i].content_offset, SEEK_SET);
        for (int j = 0; j < num_of_segments; j++) {
            read(fd, &segment_arr[i][j], sizeof(uint64_t));
            calculated_checksum = calculated_checksum ^ endian_invert_64(segment_arr[i][j]);
        }
    
        if (calculated_checksum == file_e_arr[i].checksum) {
            char temp[20000];
            strcpy(temp, argv[2]);
            char slash[] = "/";
            strcat(temp, slash);
            strcat(temp, file_name_arr[i]);
            printf("path: %s\n", temp);
            int fd_write = open(temp, O_WRONLY | O_CREAT | O_TRUNC);
            for (int j = 0; j < num_of_segments; j++) {
                int size_to_write;
                if (j == num_of_segments - 1) {
                    size_to_write = file_e_arr[i].filesize - j * 8;
                }
                else {
                    size_to_write = sizeof(u_int64_t);
                }
                write(fd_write, &segment_arr[i][j], size_to_write);
            }
        } 
        else {
            printf("%d checksum mismatch", i);
        }
    }

    printf("Number of files packed: %d.\n", header.n_files);

    for (int i = 0; i < header.n_files; i++) {
        printf("File name: %s; file size: %d bytes; file data offset: %x \n", file_name_arr[i], file_e_arr[i].filesize, file_e_arr[i].content_offset);
    }

    return 0;
}