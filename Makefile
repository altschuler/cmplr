LLVM_CONF = /usr/lib/llvm-3.2/bin/llvm-config

FILES = $(wildcard src/*.cpp)

dbuild:
	clang++ -std=c++0x -g -rdynamic -I lib $(FILES)  \
		`$(LLVM_CONF) --cppflags --libs core jit native` \
		`$(LLVM_CONF) --ldflags` \
		-o bin/wtf
noopti:
	clang++ -Wall -std=c++0x -g -rdynamic -I lib $(FILES)  \
		`$(LLVM_CONF) --cppflags --libs core jit native` \
		`$(LLVM_CONF) --ldflags` \
		 -O0 -o bin/wtf
