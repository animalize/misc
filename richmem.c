#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "lib/zstd.h"
#include "common.h"

//const char* PATH = "E:\\txt.tar";
const char* PATH = "txt.tar";


int compress(char *input_buffer, size_t input_buffer_size, size_t output_buffer_size) {
    ZSTD_CCtx *cctx;
    ZSTD_inBuffer in;
    ZSTD_outBuffer out;
    char *output_buffer;
    
    in.src = input_buffer;
    in.size = input_buffer_size;
    in.pos = 0;
    
    output_buffer = malloc(output_buffer_size);
    out.dst = output_buffer;
    out.size = output_buffer_size;
    out.pos = 0;
    
    // create context
    cctx = ZSTD_createCCtx();
    
    // compress
    int t1 = clock();
    ZSTD_compressStream2(cctx, &out, &in, ZSTD_e_end);
    int t2 = clock();
    
    // free
    free(output_buffer);
    ZSTD_freeCCtx(cctx);
    
    printf("output size: %ld, time: %d ms\n", out.pos, t2-t1);
    return 0;
}


int main(int argc, const char** argv)
{
    char *input_buffer;
    size_t input_buffer_size;

    // read file
    input_buffer = mallocAndLoadFile_orDie(PATH, &input_buffer_size);
    printf("read %ld bytes.\n", input_buffer_size);

    printf("ZSTD_compressBound() value: %ld\n", ZSTD_compressBound(input_buffer_size));
    
    // ZSTD_compressBound() - 1
    printf("\nZSTD_compressBound() - 1\n");
    compress(input_buffer, input_buffer_size, ZSTD_compressBound(input_buffer_size)-1);
    
    // ZSTD_compressBound()
    printf("\nZSTD_compressBound()\n");
    compress(input_buffer, input_buffer_size, ZSTD_compressBound(input_buffer_size));
}
