SRCS=$(shell find . -regex .+[.]proto)
C_SRCS=$(SRCS:.proto=.pb-c.c)

.PHONY: all
.SUFFIXES: .proto .pb-c.c
all: $(C_SRCS)
	

clean:
	@rm $(C_SRCS) $(C_SRCS:.c=.h)

.proto.pb-c.c:
	@echo Generating $@
	@protoc-c --c_out=. $<
