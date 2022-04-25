#include <Block.h>

#include "stream.h"

stream_writer_t stream_file_writer(FILE* f) {
  return Block_copy(^stream_status_t (const char* buffer, size_t size) {
    if (fwrite(buffer, 1, size, f) == size) {
      return STREAM_OK;
    }
    
    return STREAM_FAILED;
  }); 
}

stream_reader_t stream_file_reader(FILE* f) {
  return Block_copy(^stream_status_t (char* buffer, size_t size, size_t* loadedSize) {
    *loadedSize = fread(buffer, 1, size, f);
    if (*loadedSize > 0)
      return STREAM_OK;
    else if (*loadedSize < size)
      return STREAM_EOF;
    
    return STREAM_FAILED;
  }); 
}
