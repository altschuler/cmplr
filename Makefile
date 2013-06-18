LLVM_CONF = /usr/lib/llvm-3.2/bin/llvm-config

FILES = $(wildcard *.cpp)
HEADERS = $(wildcard *.h)

dbuild:
	clang++ -g $(HEADERS) $(FILES) `$(LLVM_CONF) --cppflags --libs core jit native` `$(LLVM_CONF) --ldflags` -o main
