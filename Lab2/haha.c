#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <math.h>
#include<string.h>


typedef struct {
        uint32_t magic;
        int32_t  off_str;
        int32_t  off_dat;
        uint32_t n_files;
} __attribute((packed)) pako_header_t;

typedef struct{
        int32_t off_name;
        int32_t file_size;
        int32_t off_content;
        uint64_t checksum;
} __attribute((packed)) pako_content;

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


int main(int argc, char *argv[]){
    // open file
    int fd = open(argv[1], O_RDONLY);

    // header
    pako_header_t header;
    read(fd, &header, sizeof(pako_header_t));
    printf("FILE_NUMS: %d\n", header.n_files);
    // read the structure of pako
    int nums = header.n_files;
    pako_content pako[nums];
    for(int i=0; i<nums; i++){
        read(fd, &pako[i],sizeof(pako_content));
        printf("%d: %x %x %x %lx\n",i+1,pako[i].off_name ,htonl(pako[i].file_size),pako[i].off_content,pako[i].checksum);
    }

    // deal with the length of filename
    int filename_len[nums];
    int cum = nums*(sizeof(pako_content))+(sizeof(pako_header_t)); //cumulative pos after header
    for(int i=0; i<nums-1; i++){
        filename_len[i]= pako[i+1].off_name - pako[i].off_name;
        // printf("%d\n",filename_len[i]);
    }
    filename_len[nums-1] = header.off_dat - cum;

    // the filename of pako
    //int off_cur = nums*(sizeof(pako_content))+(sizeof(pako_header_t)); // cumulative pos after header
    char** filenames = calloc(nums, sizeof(char*));
    for(int i = 0; i < nums; i++){
        filenames[i] = calloc(nums, filename_len[i]);
        read(fd, filenames[i], filename_len[i]);
        printf("%d: %s\n",i+1, filenames[i]);  // ?
        cum += filename_len[i];
    }



    // int32_t segment[nums][pako[i].file_size/8+1];
    // char** filecontent = calloc(nums, sizeof(char*));
    uint64_t** segment = calloc(nums,sizeof(uint64_t*));
    int32_t content_len[nums];
    for(int i = 0; i < nums; i++){
        content_len[i] = htonl(pako[i].file_size); // file size turn into big endian
        segment[i] = calloc(nums, content_len[i]); //
        printf("%d: %d\n",i+1, content_len[i]);
    }

    for(int i = 0; i < nums; i++){
        // printf("%d\n",i+1);
        float tmp =(float)content_len[i]/8.0;
        int x, tmp1 = tmp;
        if(tmp > (float) tmp1){  x = tmp + 1;}
        else{   x = tmp;}

        lseek(fd,header.off_dat + pako[i].off_content, SEEK_SET);
        uint64_t checksum = 0;
        for(int j = 0; j < x ; j++){
            read(fd, &segment[i][j], 8);
            checksum = checksum ^ segment[i][j];
            // printf("%lx\n", segment[i][j]);
        }
        // printf("%lx\n", checksum);
        // printf("%lx\n", pako[i].checksum);
        // FILE * fp;
        // check the checksum
        if(checksum == endian_invert_64(pako[i].checksum)){
            char str[200] = "";
            char slash[2]="/";
            strcat(str, argv[2]);
            strcat(str, slash);
            strcat(str, filenames[i]);
            printf("%s\n", str);
            // strcat(argv[2], filenames[i]);
            int fdw = open(str, O_WRONLY | O_CREAT | O_TRUNC);
            for( int j = 0; j < x ; j++){
                if(j == (x - 1)){
                    write(fdw, &segment[i][j], content_len[i]-(8*(j)));
                    break;
                }
                write(fdw, &segment[i][j], 8);
                // printf("%d\n", segment[i][j]);
            }


        }
    }


    return 0;
}