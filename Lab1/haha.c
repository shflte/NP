#include <stdint.h>
#include <sys/types.h>    
#include <sys/stat.h>    
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

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

int main() {
    pako_header_t header;

    int fd = open("example.pak", O_RDONLY);
    int bytes = read(fd, &header, 16);

    return 0;
}