CXXFLAGS=`llvm-config --cxxflags`
LLVMLIBS=`llvm-config --libs`
LDFLAGS=`llvm-config --ldflags`
LOADABLEOPTS=-Wl,-flat_namespace -Wl,-undefined,suppress

build/BWCET: src/BWCET.cpp
	clang $(CXXFLAGS) $(LLVMLIBS) $(LDFLAGS) $(LOADABLEOPTS) src/BWCET.cpp -o build/BWCET

build/foo-O0.ll: test/foo.c
	clang -emit-llvm -S -O0 test/foo.c -o build/foo-O0.ll

build/foo-O3.ll: test/foo.c
	clang -emit-llvm -S -O3 test/foo.c -o build/foo-O3.ll

bitcode: build/foo-O0.ll build/foo-O3.ll

tests: build/foo-O0.ll build/foo-O3.ll build/BWCET
	build/BWCET --help
	build/BWCET --help-list-hidden
	build/BWCET build/foo-O0.ll build/foo-O3.ll
	build/BWCET build/foo-O0.ll build/foo-O3.ll -o build/tmpout
	build/BWCET build/foo-O0.ll build/foo-O3.ll -f JSON
	build/BWCET build/foo-O0.ll build/foo-O3.ll -f CSV
	build/BWCET build/foo-O0.ll build/foo-O3.ll -k throughput
	build/BWCET build/foo-O0.ll build/foo-O3.ll -k latency
	build/BWCET build/foo-O0.ll build/foo-O3.ll -k code-size

clean:
	rm -f build/*
