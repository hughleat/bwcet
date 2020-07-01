# bwcet
A really dumb best/worst case execution time estimation tool.

The tool uses LLVM's cost model to give estimates of best and worst case execution time for very, very simple cases. 
You pass it a list of bitcode files and it will compute b/wcet for every function in each module.

The huge, enormous caveat is that it only works for DAG like code which doesn't call other functions! That, right there, should probably put you off. I needed it for a particular task.

## Building
You need llvm installed (v10).

Just call `make`.

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

    build/BWCET build/foo-O0.ll build/foo-O3.ll -k latency
    Module: build/foo-O0.ll
      Function: foo - {min:12,max:12}
      Function: Config1Setting157574 - {min:12,max:118}
      Function: loopy - CFG not a DAG
    Module: build/foo-O3.ll
      Function: foo - {min:2,max:2}
      Function: Config1Setting157574 - {min:8,max:24}
      Function: loopy - CFG not a DAG