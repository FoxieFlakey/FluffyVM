#include "bootloader.h"

static const char tmp[] =
"{\"mainPrototype\":{\"prototypes\":[{\"prototypes\":[{\"prototypes\":[],\"instructions\":[{\"low\":0,\"high\":83886080},{\"low\":0,\"high\":100663296},{\"low\":0,\"high\":167772160}]}],\"instructions\":[{\"low\":0,\"high\":0},{\"low\":0,\"high\":16777216},{\"low\":0,\"high\":50331648}]}],\"instructions\":[{\"low\":65536,\"high\":117440513},{\"low\":4294836225,\"high\":33554433},{\"low\":0,\"high\":117440514},{\"low\":0,\"high\":83886082},{\"low\":131072,\"high\":117440514},{\"low\":0,\"high\":83886082},{\"low\":0,\"high\":67108865},{\"low\":0,\"high\":150994945},{\"low\":0,\"high\":134217728}]},\"constants\":[{\"data\":\"Hello World!\",\"type\":\"string\"},{\"data\":\"print\",\"type\":\"string\"},{\"data\":\"Pogger fox this working\",\"type\":\"string\"}]}"
;

const char* data_bootloader = tmp;
static size_t len = sizeof(tmp);

size_t data_bootloader_get_len() {
  return len;
}

