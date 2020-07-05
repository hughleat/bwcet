CXXFLAGS=`llvm-config --cxxflags`
LLVMLIBS=`llvm-config --libs`
LDFLAGS=`llvm-config --ldflags`
LOADABLEOPTS=-Wl,-flat_namespace -Wl,-undefined,suppress

build/BWCET: src/BWCET.cpp
	clang -O3 -g0 $(CXXFLAGS) $(LLVMLIBS) $(LDFLAGS) $(LOADABLEOPTS) src/BWCET.cpp -o build/BWCET

build/foo-O0.ll: test/foo.c
	clang -emit-llvm -S -O0 test/foo.c -o build/foo-O0.ll
build/foo-O3.ll: test/foo.c
	clang -emit-llvm -S -O3 test/foo.c -o build/foo-O3.ll
build/large-O0.ll: test/large.c
	clang -emit-llvm -S -O0 test/large.c -o build/large-O0.ll
build/large-O3.ll: test/large.c
	clang -emit-llvm -S -O3 test/large.c -o build/large-O3.ll

bitcode: build/foo-O0.ll build/foo-O3.ll build/large-O0.ll

tests: bitcode build/BWCET
	build/BWCET --help
	build/BWCET --help-list-hidden
	build/BWCET build/foo-O0.ll build/foo-O3.ll build/large-O0.ll
	build/BWCET build/foo-O0.ll build/foo-O3.ll build/large-O0.ll -o build/tmpout
	build/BWCET build/foo-O0.ll build/foo-O3.ll build/large-O0.ll -f JSON
	build/BWCET build/foo-O0.ll build/foo-O3.ll build/large-O0.ll -f CSV
	build/BWCET build/foo-O0.ll build/foo-O3.ll build/large-O0.ll -k throughput
	build/BWCET build/foo-O0.ll build/foo-O3.ll build/large-O0.ll -k latency
	build/BWCET build/foo-O0.ll build/foo-O3.ll build/large-O0.ll -k code-size

test-large-O3: build/large-O3.ll build/BWCET
	build/BWCET build/large-O3.ll -k code-size

clean:
	rm -f build/*
