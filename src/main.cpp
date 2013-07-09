#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#include <iostream>

#include "Driver.hpp"
#include "BuiltIns.hpp"

static Driver *driver;

int main(int argc, const char *argv[]) {
	InitializeNativeTarget();

	LLVMContext &Context = getGlobalContext();
	Module *module = new Module("WTFJIT", Context);

	string ErrStr;
	ExecutionEngine *execEngine = EngineBuilder(module).setErrorStr( &ErrStr).create();
	if ( !execEngine) {
		fprintf(stderr, "Could not create ExecutionEngine: %s\n", ErrStr.c_str());
		exit(1);
	}

	driver = new Driver(new Codegen(execEngine, module));

	string inputFile(argv[1]);

	driver->Go(inputFile);

	// hax to flush cout
	pline();

	return 0;
}

