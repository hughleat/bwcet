//==============================================================================
// Estimate best and worst case execution time of LLVM code.
//
// Hugh Leather hughleat@gmail.com 2020-06-30
//==============================================================================

#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Pass.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/CFG.h"
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <queue>
#include <cassert>

using namespace llvm;
using namespace std;

//------------------------------------------------------------------------------
// Command line options
//------------------------------------------------------------------------------
cl::OptionCategory bwcetCategory{"bwcet options"};

cl::list<std::string> inputFiles(
    cl::Positional, 
    cl::desc{"<Modules to analyse>"},
    cl::value_desc{"bitcode filename"},
    cl::OneOrMore,
    cl::cat{bwcetCategory}
);

cl::opt<string> outputFilename(
    "output", 
    cl::desc("Specify output filename (default to std out)"), 
    cl::value_desc("output filename"), 
    cl::init("-"),
    cl::cat{bwcetCategory}
);
cl::alias outputFilenameA(
    "o", 
    cl::desc("Alias for --output"), 
    cl::aliasopt(outputFilename),
    cl::cat{bwcetCategory}
);

enum OutputFormat {
    TXT, JSON, CSV
};
cl::opt<OutputFormat> outputFormat(
    "format",
    cl::desc("Choose output format"),
    cl::values(
        clEnumVal(TXT, "Human readable format (default)"),
        clEnumVal(JSON, "JSON format"),
        clEnumVal(CSV, "CSV format")
    ),
    cl::init(TXT),
    cl::cat{bwcetCategory}
);
cl::alias outputFormatA(
    "f", 
    cl::desc("Alias for --format"), 
    cl::aliasopt(outputFormat),
    cl::cat{bwcetCategory}
);

cl::opt<TargetTransformInfo::TargetCostKind> costKind(
    "cost-kind", 
    cl::desc("Target cost kind"),
    cl::init(TargetTransformInfo::TCK_RecipThroughput),
    cl::values(
        clEnumValN(TargetTransformInfo::TCK_RecipThroughput, "throughput", "Reciprocal throughput (default)"),
        clEnumValN(TargetTransformInfo::TCK_Latency, "latency", "Instruction latency"),
        clEnumValN(TargetTransformInfo::TCK_CodeSize, "code-size", "Code size")
    ),
    cl::cat{bwcetCategory}
);
cl::alias costKindA(
    "k", 
    cl::desc("Alias for --cost-kind"), 
    cl::aliasopt(costKind),
    cl::cat{bwcetCategory}
);

//------------------------------------------------------------------------------
// A pair of min max. 
//------------------------------------------------------------------------------
struct MinMax {
    size_t min;
    size_t max;
    MinMax(): min(SIZE_MAX), max(0) {}
    MinMax(size_t v): min(v), max(v) {}
    MinMax(size_t min, size_t max): min(min), max(max) {}
    // If elements of other are better, adopt them.
    // Return true if improved (i.e. min reduced or max increased)
    bool merge(const MinMax& other) {
        bool better = false;
        if(other.min < min) {
            min = other.min;
            better = true;
        }
        if(other.max > max) {
            max = other.max;
            better = true;
        }
        return true;
    }
};
// Add two MinMax. 
MinMax operator+(const MinMax& a, const MinMax& b) {
    return MinMax(a.min + b.min, a.max + b.max);
}
// Print MinMax
ostream& operator<<(ostream &os, const MinMax& mm) {
    os << "{min:" << mm.min << ",max:" << mm.max << "}";
    return os;
}

//------------------------------------------------------------------------------
// Get min and max cost of functions and basic blocks
//------------------------------------------------------------------------------
TargetIRAnalysis tira;
unique_ptr<TargetTransformInfoWrapperPass> ttiwp((TargetTransformInfoWrapperPass*)createTargetTransformInfoWrapperPass(tira));

MinMax getCost(const BasicBlock& bb, const TargetTransformInfo& tti) {
    int cost = 0;
    for(const auto& insn: bb) {
        cost += tti.getInstructionCost(&insn, costKind);
    }
    return cost;
}
// Get the cost of a function. The function better have basic blocks and the CFG had better be a DAG.
// If not a DAG, will never finish.
// Algorithm is simple dataflow like.
MinMax getCost(const Function& f) {
    auto& tti = ttiwp->getTTI(f);
    
    // Precompute BB costs.
    const auto& bbs = f.getBasicBlockList();
    unordered_map<const BasicBlock*, MinMax> bbCost;
    for(const auto& bb: bbs) bbCost[&bb] = getCost(bb, tti);

    // Costs into each BB.
    unordered_map<const BasicBlock*, MinMax> in;
    const auto& entry = f.getEntryBlock();
    in[&entry] = MinMax(0);
    
    // Push costs around the DAG (BTW, this better be a DAG!)
    MinMax best;
    // Simple worklist from the entry.
    unordered_map<const BasicBlock*, bool> isQueued;
    queue<const BasicBlock*> q;
    q.push(&entry);
    isQueued[&entry] = true;

    cout << "Starting " << bbs.size() << "\n";
    int pcount = 1000000;
    while(!q.empty()) {
        if(pcount-- == 0) {
            cout << "q.size " << q.size() << " in.size " << in.size() << endl;
            pcount = 1000000;
        }

        const auto* bb = q.front();
        assert(isQueued[bb]);
        isQueued[bb] = false;
        q.pop();
        
        // Xfer is adding cost
        MinMax out = in[bb] + bbCost[bb];
        best.merge(out);
        
        // If this is better than in for any successor, update and add to queue
        for(const auto* succ: successors(bb)) {
            if(in[succ].merge(out)) {
                if(!isQueued[succ]) {
                    q.push(succ);
                    isQueued[succ] = true;
                }
            }
        }
    }
    return best;
}

//------------------------------------------------------------------------------
// Determine if CFG is a DAG.
//------------------------------------------------------------------------------
// Colour for DFS
enum Colour {WHITE, GREY, BLACK};
// DFS
bool isDAG(const BasicBlock* bb, unordered_map<const BasicBlock*, Colour>& colour) {
    switch(colour[bb]) {
        case BLACK: return true;
        case GREY: return false;
        case WHITE: {
            colour[bb] = GREY;
            for(const auto* succ: successors(bb)) {
                if(!isDAG(succ, colour)) return false;
            }
            colour[bb] = BLACK;
            return true;
        }
    }
}
bool isDAG(const Function& f) {
    unordered_map<const BasicBlock*, Colour> colour;
    return isDAG(&f.getEntryBlock(), colour);
}

//------------------------------------------------------------------------------
// Visitor functions, called to process the module
//------------------------------------------------------------------------------
void visit(const Function& f, ostream& os) {
    bool dag = isDAG(f);
    os << boolalpha;
    switch(outputFormat) {
        case TXT: {
            os << "  Function: " << f.getName().str() << " - ";
            if(dag) os << getCost(f) << "\n";
            else os << "CFG not a DAG\n";
            break;
        }
        case JSON: {
            os << "{";
            os <<   "\"function\":\"" << f.getName().str() << "\",";
            os <<   "\"dag\":" << dag;
            if(dag) {
                os << ",";
                MinMax mm = getCost(f);
                os << "\"min\":" << mm.min << ",";
                os << "\"max\":" << mm.max;
            }
            os << "}";
            break;
        }
        case CSV: {
            os << f.getParent()->getName().str() << ",";
            os << f.getName().str() << ",";
            os << dag << ",";
            if(dag) {
                MinMax mm = getCost(f);
                os << mm.min << ",";
                os << mm.max << "\n";
            } else os << ",\n";
            break;
        }
    }
}
void visit(const Module& m, ostream& os) {
    switch(outputFormat) {
        case TXT: {
            os << "Module: " << m.getName().str() << "\n";
            for(const auto& f: m.functions()) visit(f, os);
            break;
        }
        case JSON: {
            os << "{";
            os <<   "\"module\":\"" << m.getName().str() << "\",";
            os <<   "\"functions\":[";
            bool isFirst = true;
            for(const auto& f: m.functions()) {
                if(!isFirst) os << ",";
                else isFirst = false;
                visit(f, os);
            }
            os << "]}";
            break;
        }
        case CSV: {
            for(const auto& f: m.functions()) visit(f, os);
            break;
        }
    }
}
void visit(const string& filename, ostream& os) {
    // Parse the IR file passed on the command line.
    SMDiagnostic err;
    LLVMContext ctx;
    unique_ptr<Module> m = parseIRFile(filename, err, ctx);
    
    if(!m) throw err;
    
    // Run the analysis and print the results
    visit(*m, os);    
}
void visit(const vector<string>& filenames, ostream& os) {
    switch(outputFormat) {
        case TXT: {
            for(const auto& fn: filenames) visit(fn, os);
            break;
        }
        case JSON: {
            os << "[";
            bool isFirst = true;
            for(const auto& fn: filenames) {
                if(!isFirst) os << ",";
                else isFirst = false;
                visit(fn, os);
            }
            os << "]\n";
            break;
        }
        case CSV: {
            os << "Module, Function, DAG, Min, Max\n";
            for(const auto& fn: filenames) visit(fn, os);
            break;
        }
    }
}
//------------------------------------------------------------------------------
// Driver
//------------------------------------------------------------------------------
int main(int argc, char **argv) {
    // Hide all options apart from the ones specific to this tool
    cl::HideUnrelatedOptions(bwcetCategory);

    cl::ParseCommandLineOptions(argc, argv, "Estimates the best and worst case runtime for each function the input IR file\n");

    try {
        // Get the output file
        unique_ptr<ostream> ofs(outputFilename == "-" ? nullptr : new ofstream(outputFilename.c_str()));
        if(ofs && !ofs->good()) {
            throw "Error opening output file: " + outputFilename;
        }
        ostream& os = ofs ? *ofs : cout;

        // Makes sure llvm_shutdown() is called (which cleans up LLVM objects)
        // http://llvm.org/docs/ProgrammersManual.html#ending-execution-with-llvm-shutdown
        llvm_shutdown_obj shutdown_obj;
    
        // Do the work
        visit(inputFiles, os);
    
    } catch(string e) {
        errs() << e;
        return -1;
    } catch(SMDiagnostic e) {
        e.print(argv[0], errs(), false);
        return -1;
    }
    return 0;
}
