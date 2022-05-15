#include "bootloader.h"

static const char tmp[] =
"{\"constants\":[{\"data\":\"Hello World!\",\"type\":\"string\"},{\"data\":\"print\",\"type\":\"string\"},{\"data\":\"Pogger fox this working\",\"type\":\"string\"}],\"mainPrototype\":{\"prototypes\":[{\"prototypes\":[{\"prototypes\":[],\"instructions\":[{\"high\":83886080,\"low\":0},{\"high\":100663296,\"low\":0},{\"high\":167772160,\"low\":0}]}],\"instructions\":[{\"high\":0,\"low\":0},{\"high\":16777216,\"low\":0},{\"high\":50331648,\"low\":0}]}],\"instructions\":[{\"high\":117440513,\"low\":65536},{\"high\":117440514,\"low\":0},{\"high\":33554433,\"low\":4294836225},{\"high\":83886082,\"low\":0},{\"high\":67108865,\"low\":0},{\"high\":150994944,\"low\":131072},{\"high\":134217728,\"low\":0}]}}"
;

const char* data_bootloader = tmp;
static size_t len = sizeof(tmp);

size_t data_bootloader_get_len() {
  return len;
}

