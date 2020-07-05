tool: build/bin/bwcet

build/bin/bwcet: src/BWCET.cpp invoke_cmake.sh CMakeLists.txt
	mkdir -p build
	cmake -DLT_LLVM_INSTALL_DIR=${LLVM_DIR} -S . -B build
	(cd build; make)

bitcode: build/test/foo-O0.ll build/test/foo-O3.ll build/test/large-O0.ll

build/test/foo-O0.ll: test/foo.c
	mkdir -p build/test
	clang -emit-llvm -S -O0 test/foo.c -o build/test/foo-O0.ll
build/test/foo-O3.ll: test/foo.c
	mkdir -p build/test
	clang -emit-llvm -S -O3 test/foo.c -o build/test/foo-O3.ll
build/test/large-O0.ll: test/large.c
	mkdir -p build/test
	clang -emit-llvm -S -O0 test/large.c -o build/test/large-O0.ll
build/test/large-O3.ll: test/large.c
	mkdir -p build/test
	clang -emit-llvm -S -O3 test/large.c -o build/test/large-O3.ll

tests: bitcode tool
	build/bin/bwcet --help
	build/bin/bwcet --help-list-hidden
	build/bin/bwcet build/test/foo-O0.ll build/test/foo-O3.ll build/test/large-O0.ll
	build/bin/bwcet build/test/foo-O0.ll build/test/foo-O3.ll build/test/large-O0.ll -o build/test/tmpout
	build/bin/bwcet build/test/foo-O0.ll build/test/foo-O3.ll build/test/large-O0.ll -f JSON
	build/bin/bwcet build/test/foo-O0.ll build/test/foo-O3.ll build/test/large-O0.ll -f CSV
	build/bin/bwcet build/test/foo-O0.ll build/test/foo-O3.ll build/test/large-O0.ll -k throughput
	build/bin/bwcet build/test/foo-O0.ll build/test/foo-O3.ll build/test/large-O0.ll -k latency
	build/bin/bwcet build/test/foo-O0.ll build/test/foo-O3.ll build/test/large-O0.ll -k code-size

test-large-O3: build/test/large-O3.ll tool
	build/BWCET build/test/large-O3.ll -k code-size

clean:
	rm -rf build
