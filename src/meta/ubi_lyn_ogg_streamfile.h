#ifndef _LYN_OGG_STREAMFILE_H_
#define _LYN_OGG_STREAMFILE_H_
#include "../streamfile.h"


typedef struct {
    off_t start_physical_offset; /* interleaved data start, for this substream */
    size_t interleave_block_size; /* max size that can be read before encountering other substreams */
    size_t stride_size; /* step size between interleave blocks (interleave*channels) */
    size_t total_size; /* final size of the deinterleaved substream */
} lyn_ogg_io_data;


/* Handles deinterleaving of complete files, skipping portions or other substreams. */
static size_t lyn_ogg_io_read(STREAMFILE *streamfile, uint8_t *dest, off_t offset, size_t length, lyn_ogg_io_data* data) {
    size_t total_read = 0;

    while (length > 0) {
        size_t to_read;
        size_t length_available;
        off_t block_num;
        off_t intrablock_offset;
        off_t physical_offset;

        block_num = offset / data->interleave_block_size;
        intrablock_offset = offset % data->interleave_block_size;
        physical_offset = data->start_physical_offset + block_num*data->stride_size + intrablock_offset;
        length_available = data->interleave_block_size - intrablock_offset;

        if (length < length_available) {
            to_read = length;
        }
        else {
            to_read = length_available;
        }

        if (to_read > 0) {
            size_t bytes_read;

            bytes_read = read_streamfile(dest, physical_offset, to_read, streamfile);
            total_read += bytes_read;

            if (bytes_read != to_read) {
                return total_read;
            }

            dest += bytes_read;
            offset += bytes_read;
            length -= bytes_read;
        }
    }

    return total_read;
}

static size_t lyn_ogg_io_size(STREAMFILE *streamfile, lyn_ogg_io_data* data) {
    return data->total_size;
}


static STREAMFILE* setup_lyn_ogg_streamfile(STREAMFILE *streamFile, off_t start_offset, size_t interleave_block_size, size_t stride_size, size_t total_size) {
    STREAMFILE *temp_streamFile = NULL, *new_streamFile = NULL;
    lyn_ogg_io_data io_data = {0};
    size_t io_data_size = sizeof(lyn_ogg_io_data);

    io_data.start_physical_offset = start_offset;
    io_data.interleave_block_size = interleave_block_size;
    io_data.stride_size = stride_size;
    io_data.total_size = total_size;


    /* setup subfile */
    new_streamFile = open_wrap_streamfile(streamFile);
    if (!new_streamFile) goto fail;
    temp_streamFile = new_streamFile;

    new_streamFile = open_io_streamfile(temp_streamFile, &io_data,io_data_size, lyn_ogg_io_read,lyn_ogg_io_size);
    if (!new_streamFile) goto fail;
    temp_streamFile = new_streamFile;

    new_streamFile = open_fakename_streamfile(temp_streamFile, NULL,"ogg");
    if (!new_streamFile) goto fail;
    temp_streamFile = new_streamFile;

    return temp_streamFile;

fail:
    close_streamfile(temp_streamFile);
    return NULL;
}

#endif /* _LYN_OGG_STREAMFILE_H_ */
