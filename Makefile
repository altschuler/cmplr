LLVM_CONF = /usr/lib/llvm-3.2/bin/llvm-config

dbuild:
	clang++ -g main.cpp `$(LLVM_CONF) --cppflags --libs core jit native` `$(LLVM_CONF) --ldflags` -o main
