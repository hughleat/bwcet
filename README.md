# bwcet
A really dumb best/worst case execution time estimation tool.

The tool uses LLVM's cost model to give estimates of best and worst case execution time for very, very simple cases. 
You pass it a list of bitcode files and it will compute b/wcet for every function in each module.

The huge, enormous caveat is that it only works for DAG like code which doesn't call other functions! That, right there, should probably put you off. (Actually, it will try to give the best case estimates for non DAGs, but will assume infinite for the worst.)
Additionally, the estimate works by just asking LLVM for the cost of each instruction and summing the best and worst case paths through the CFG. This ignores all scheduling, cache effects, etc.
These are the reasons it is a 'dumb' tool. I needed it for a particular task.

## Building
You need llvm installed (v10).

#### __Step 1:__ Tell `cmake` where LLVM is:
    export LLVM_DIR=<path to llvm-10 dir>
#### __Step 2:__ Call `make`
    make
This will create a build dir. In there you should find the tool in the `bin` directory, called `bwcet`.
#### __Step 3:__ (Optional) Run the tests
Quick tests:

    make tests

Test on a big function compiled with -O3 (it takes LLVM ages to do this, so it is separated from the rest of the tests).

    make test-large-O3

Some help for Windows machines is at [the bottom of this page](#non-mac-machines).

## Use
The command line options are:

    OVERVIEW: Estimates the best and worst case runtime for each function the input IR file

    USAGE: BWCET [options] <Modules to analyse>

    OPTIONS:

    Generic Options:

      --help                     - Display available options (--help-hidden for more)
      --help-list                - Display list of available options (--help-list-hidden for more)
      --version                  - Display the version of this program

    bwcet options:

      --cost-kind=<value>        - Target cost kind
        =throughput              -   Reciprocal throughput (default)
        =latency                 -   Instruction latency
        =code-size               -   Code size
      -k                         - Alias for --cost-kind
      --format=<value>           - Choose output format
        =TXT                     -   Human readable format (default)
        =JSON                    -   JSON format
        =CSV                     -   CSV format
      -f                         - Alias for --format
      --output=<output filename> - Specify output filename (default to std out)
      -o                         - Alias for --output

## Output
Sample output:

    build/BWCET build/foo-O0.ll build/foo-O3.ll build/large-O0.ll -k latency
    Module: build/foo-O0.ll
      Function: foo min=12 max=12
      Function: Config1Setting157574 min=19 max=118
      Function: loopy min=20 max=inf
    Module: build/foo-O3.ll
      Function: foo min=2 max=2
      Function: Config1Setting157574 min=9 max=24
      Function: loopy min=3 max=inf
    Module: build/large-O0.ll
      Function: aLargeFunction min=30 max=952263

## Non Mac Machines
I have only tested this on my Macbook, running MacOS Catalina. YMMV.

There might be some issues on Windows machines (I haven't used one for a long time). I copied most of my build from [`llvm-tutor`](https://github.com/banach-space/llvm-tutor). In his `HelloWorld` directory, there is a [file](https://github.com/banach-space/llvm-tutor/blob/master/HelloWorld/CMakeLists.txt_for_windows) which might have useful info. Also his [README](https://github.com/banach-space/llvm-tutor/blob/master/README.md) might be useful.

My friend, Chad, got this working on his Windows machine. Here are the steps he took, verbatim.

1. mkdir Foo
2. cd Foo
3. git clone --config core.autocrlf=false https://github.com/llvm/llvm-project.git
4. cd llvm-project
5. mkdir build
6. cd build
7. cmake -G “Visual Studio 16 2019” ..\llvm
8. cmake --build .
    - NOTE this builds the DEBUG version…
9. mkdir Foo\llvm-project\BwCet
10. git clone --config core.autocrlf=false https://github.com/hughleat/bwcet.git

11. Change this line in Foo\llvm-project\BwCet\CMakeLists.txt
    - set(LT_LLVM_INSTALL_DIR "" CACHE PATH "LLVM installation directory")
    - TO
    - set(LT_LLVM_INSTALL_DIR "Foo/llvm-project/build")
    - NOTE: change ‘\’ to ‘/’ in the path!!
12. cd into Foo\llvm-project\BwCet
13. In Foo\llvm-project\BwCet\cmakelists.txt change
    - find_package(LLVM 10.0.0 REQUIRED CONFIG)
    - TO
    - find_package(LLVM 11.0.0 REQUIRED CONFIG)
14. Comment out these lines
    - \#if (${SUPPORTS_FVISIBILITY_INLINES_HIDDEN_FLAG} EQUALS "1")
    - \#  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden")
    - \#endif()
15. cmake -G “Visual Studio 16 2019” ..\llvm
16. cmake --build .
17. The binary will be in foo\llvm-project\bwcet\bin\debug\bwcet.exe

