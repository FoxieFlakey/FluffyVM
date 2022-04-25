#ifndef header_1650096891_binary_h
#define header_1650096891_binary_h

// Provide conversion
// for little or big integer to native endian

#include <stdint.h>

static inline uint16_t binary_u16_little(void* _data) {
  uint8_t* data = (uint8_t*) _data;
  return data[0] | (data[1] << 8);
}

static inline uint16_t binary_u16_big(void* _data) {
  uint8_t* data = (uint8_t*) _data;
  return data[1] | (data[0] << 8);
}

static inline uint32_t binary_u32_little(void* _data) {
  uint8_t* data = (uint8_t*) _data;
  return data[0]         | (data[1] << 8) |
         (data[2] << 16) | (data[3] << 24);
}

static inline uint32_t binary_u32_big(void* _data) {
  uint8_t* data = (uint8_t*) _data;
  return data[3]         | (data[2] << 8) |
         (data[1] << 16) | (data[0] << 24);
}

#endif

