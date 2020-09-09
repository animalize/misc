#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include <zstd.h>
#include <zdict.h>

// modify PATH to zstd/lib/compress folder
const char* PATH = "E:\\dev\\zstd\\lib\\compress\\";


// input buffer
#define INPUT_BUFFER_SIZE  (30*1024*1024)
static char input_buffer[INPUT_BUFFER_SIZE];
static size_t input_pos = 0;

// output buffer
#define OUTPUT_BUFFER_SIZE (30*1024*1024)
static char output_buffer[OUTPUT_BUFFER_SIZE];

// for ZDICT_trainFromBuffer()
#define CHUNKS_NUMBER 1000
static size_t content_sizes[CHUNKS_NUMBER];
static int content_count = 0;

// dict_buffer buffer
#define DICT_SIZE 100*1024
static char dict_buffer[DICT_SIZE];

// read a file's content
int read_file(const char *path) {
    FILE *fp = NULL;
    size_t read_size;
    
    fp = fopen(path, "rb");
    if (fp == NULL) {
        printf("Can't read file: %s\n", path);
        return -1;
    }
    
    read_size = fread(&input_buffer[0]+input_pos,
                      1,
                      INPUT_BUFFER_SIZE-input_pos,
                      fp);

    if (read_size > 0) {
        input_pos += read_size;
        content_sizes[content_count] = read_size;
        content_count += 1;
        //printf("read: %s\nsize: %d\n", path, read_size);
    }
    
    fclose(fp);
    return 0;
}

// traverse a dir
int traverse_dir(const char* path) {
    DIR *d = NULL;
    struct dirent *dp = NULL;
    struct stat st;    
    char p[256] = {0};
        
    // dir path
    if(stat(path, &st) < 0 || !S_ISDIR(st.st_mode)) {
        printf("invalid path: %s\n", path);
        return -1;
    }

    // open dir
    if(!(d = opendir(path))) {
        printf("opendir[%s] error: %m\n", path);
        return -1;
    }

    // traverse
    while((dp = readdir(d)) != NULL) {
        if((!strncmp(dp->d_name, ".", 1)) || (!strncmp(dp->d_name, "..", 2)))
            continue;

        snprintf(p, sizeof(p) - 1, "%s/%s", path, dp->d_name);
        stat(p, &st);
        if(!S_ISDIR(st.st_mode)) {
            char fn[256];
            sprintf(fn, "%s%s", PATH, dp->d_name);
            if (read_file(fn) < 0) {
                return -1;
            }
        }
    }
    closedir(d);

    printf("Read %d files, total read size: %d\n", content_count, input_pos);
    return 0;
}

int train_dict() {
    size_t zstd_ret = ZDICT_trainFromBuffer(&dict_buffer, DICT_SIZE,
                                            &input_buffer,
                                            content_sizes, content_count);
    if (ZDICT_isError(zstd_ret)) {
        printf("Can't train dictionary: %s\n", ZDICT_getErrorName(zstd_ret));
        return -1;
    }
    return 0;
}

// one shot compress, ZSTD_e_end
int one_compress(int compression_level) {
    ZSTD_CCtx *cctx;
    ZSTD_inBuffer in;
    ZSTD_outBuffer out;
    size_t zstd_ret;
    ZSTD_CDict *cdict;
    
    in.src = &input_buffer;
    in.size = input_pos;
    in.pos = 0;
    
    out.dst = &output_buffer;
    out.size = OUTPUT_BUFFER_SIZE;
    out.pos = 0;
    
    // create context
    cctx = ZSTD_createCCtx();
    
    // load dict
    cdict = ZSTD_createCDict(dict_buffer, DICT_SIZE, compression_level);
    ZSTD_CCtx_refCDict(cctx, cdict);
    
    // compress
    ZSTD_compressStream2(cctx, &out, &in, ZSTD_e_end);
    
    // free context
    ZSTD_freeCCtx(cctx);
    
    printf("One shot compress, output size: %d\n", out.pos);
    return 0;
}

// stream compress
int stream_compress(int compression_level) {
    ZSTD_CCtx *cctx;
    ZSTD_inBuffer in;
    ZSTD_outBuffer out;
    size_t zstd_ret;
    ZSTD_CDict *cdict;
      
    out.dst = &output_buffer;
    out.size = OUTPUT_BUFFER_SIZE;
    out.pos = 0;
    
    // create context
    cctx = ZSTD_createCCtx();
    
    // load dict
    cdict = ZSTD_createCDict(dict_buffer, DICT_SIZE, compression_level);
    ZSTD_CCtx_refCDict(cctx, cdict);

    // stream compress, file by file
    in.src = &input_buffer;
    for (int i = 0; i < content_count; i++) {
        in.size = content_sizes[i];
        in.pos = 0;
        
        ZSTD_compressStream2(cctx, &out, &in, ZSTD_e_continue);
        
        in.src += content_sizes[i];
    }
    // end the frame
    in.size = 0;
    in.pos = 0;
    ZSTD_compressStream2(cctx, &out, &in, ZSTD_e_end);
    
    // free context
    ZSTD_freeCCtx(cctx);
    
    printf("Stream compress, output size: %d\n", out.pos);
    return 0;
}

int main(int argc, const char** argv)
{
    // traverse a dir, read files' content
    if (traverse_dir(PATH) < 0) {
        return -1;
    }
    
    // train a dictionary
    if (train_dict() < 0) {
        return -1;
    }

    for (int level = 0; level <= 22; level++) {
        printf("\ncompressionLevel %d:\n", level);

        // one shot compress, ZSTD_e_end
        one_compress(level);

        // stream compress
        stream_compress(level);
    }
}
