SRC_DIR=./src/
OUTPUT=main
##################################
##     Compiler and linker      ##
##################################

LIBS_INCLUDE=-I libs/hashmap/include
# Coverage report flags
# -fcoverage-mapping

SANITIZER_FLAG=-static-libsan -fsanitize-address-use-after-scope -fsanitize=undefined -fsanitize=address
CFLAGS=-g -fPIE -fPIC $(LIBS_INCLUDE) -I./include -O0 -std=c2x -Wall -xc -fblocks $(SANITIZER_FLAG) -I$(SRC_DIR) -D_POSIX_C_SOURCE=200809L
LFLAGS=-g -fPIE -fPIC -rdynamic -lBlocksRuntime -lpthread $(SANITIZER_FLAG) -L./libs/ -lfoxgc -lxxhash -lprotobuf-c

C_COMPILER=clang
LINKER=clang

##################################
SRCS=$(shell find $(SRC_DIR) -regex .+[.]c)
OBJS=$(SRCS:.c=.o)

.PHONY: all
.SUFFIXES: .c .o
all: protobuf
	@$(MAKE) link

protobuf:
	@cd src/format && $(MAKE)

link: libfoxgc $(OBJS)
	@echo Linking '$(OUTPUT)'
	@$(LINKER) $(OBJS) $(LFLAGS) -o $(OUTPUT)

libfoxgc:
	@cd libs/FoxGC/ && $(MAKE)
	@cp libs/FoxGC/libfoxgc.so libs/libfoxgc.so

.c.o:
	@echo 'Compiling "$<"'
	@$(C_COMPILER) $(CFLAGS) -c -o $@ $<

clean:
	@cd libs/FoxGC/ && $(MAKE) clean || true
	@cd src/format && $(MAKE) clean || true
	@rm $(OBJS) $(OUTPUT) || true

compile_bytecode:
	@echo Compiling bytecode...
	@cd ./assembler && lua main.lua
	@cp ./assembler/bytecode.json ./bytecode.json

run: link compile_bytecode
	@echo Running...
	@echo -------------
	@LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(shell pwd)/libs/ ./main

gdb: link
	@echo Running with gdb...
	@echo -------------
	@LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(shell pwd)/libs/ gdb ./main



