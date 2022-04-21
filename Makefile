SRC_DIR=./src/
OUTPUT=main
##################################
##     Compiler and linker      ##
##################################

LIBS_INCLUDE=
SANITIZER_FLAG=-static-libsan -fsanitize-address-use-after-scope -fsanitize=undefined -fsanitize=address
CFLAGS=-g -fPIE -fPIC $(LIBS_INCLUDE) -I./include -O0 -std=c17 -Wall -xc -fblocks $(SANITIZER_FLAG) -I$(SRC_DIR) -D_POSIX_C_SOURCE=200809L
LFLAGS=-g -fPIE -fPIC -rdynamic -lBlocksRuntime -lpthread $(SANITIZER_FLAG) -L./libs/

C_COMPILER=clang
LINKER=clang

##################################
SRCS=$(shell find $(SRC_DIR) -regex .+[.]c)
OBJS=$(SRCS:.c=.o)

.PHONY: link
.SUFFIXES: .c .o
link: $(OBJS)
	@echo Linking '$(OUTPUT)'
	@$(LINKER) $(OBJS) $(LFLAGS) -o $(OUTPUT)

.c.o:
	@echo 'Compiling "$<"'
	@$(C_COMPILER) $(CFLAGS) -c -o $@ $<

clean:
	rm $(OBJS) $(OUTPUT) 







