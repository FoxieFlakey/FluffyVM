#ifndef _headers_1639571356_MinecraftLauncherCLI_v2_stream
#define _headers_1639571356_MinecraftLauncherCLI_v2_stream

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef enum {
  STREAM_OK,
  STREAM_EOF,
  STREAM_FAILED
} stream_status_t;

typedef stream_status_t (^stream_writer_t)(const char* buffer, size_t size);

// if it read up to 'size' return STREAM_OK otherwise STREAM_EOF
// or STREAM_FAILED if it failed
typedef stream_status_t (^stream_reader_t)(char* buffer, size_t size, size_t* loadedSize);

// Convenience wrapper for writing to file (NOTE fp wont close automaticly)
stream_writer_t stream_file_writer(FILE* fp);
stream_reader_t stream_file_reader(FILE* fp);

#endif

