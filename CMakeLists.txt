cmake_minimum_required(VERSION 3.20.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -g -I/usr/local/include/boost/include -O3 -std=\"c++20\" -Wall -mmacosx-version-min=13.3")
project(SimpleProject)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set your project compile flags.
# E.g. if using the C++ header files
# you will need to enable C++11 support
# for your compiler.

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Now build our tools
add_executable(iron main.cpp)

# Find the libraries that correspond to the LLVM components
# that we wish to use
# support core irreader
llvm_map_components_to_libnames(llvm_libs WindowsManifest
    XRay
    LibDriver
    DlltoolDriver
    Coverage
    LineEditor
    XCoreDisassembler
    XCoreCodeGen
    XCoreDesc
    XCoreInfo
    X86TargetMCA
    X86Disassembler
    X86AsmParser
    X86CodeGen
    X86Desc
    X86Info
    WebAssemblyDisassembler
    WebAssemblyAsmParser
    WebAssemblyCodeGen
    WebAssemblyUtils
    WebAssemblyDesc
    WebAssemblyInfo
    VEDisassembler
    VEAsmParser
    VECodeGen
    VEDesc
    VEInfo
    SystemZDisassembler
    SystemZAsmParser
    SystemZCodeGen
    SystemZDesc
    SystemZInfo
    SparcDisassembler
    SparcAsmParser
    SparcCodeGen
    SparcDesc
    SparcInfo
    RISCVTargetMCA
    RISCVDisassembler
    RISCVAsmParser
    RISCVCodeGen
    RISCVDesc
    RISCVInfo
    PowerPCDisassembler
    PowerPCAsmParser
    PowerPCCodeGen
    PowerPCDesc
    PowerPCInfo
    NVPTXCodeGen
    NVPTXDesc
    NVPTXInfo
    MSP430Disassembler
    MSP430AsmParser
    MSP430CodeGen
    MSP430Desc
    MSP430Info
    MipsDisassembler
    MipsAsmParser
    MipsCodeGen
    MipsDesc
    MipsInfo
    LoongArchDisassembler
    LoongArchAsmParser
    LoongArchCodeGen
    LoongArchDesc
    LoongArchInfo
    LanaiDisassembler
    LanaiCodeGen
    LanaiAsmParser
    LanaiDesc
    LanaiInfo
    HexagonDisassembler
    HexagonCodeGen
    HexagonAsmParser
    HexagonDesc
    HexagonInfo
    BPFDisassembler
    BPFAsmParser
    BPFCodeGen
    BPFDesc
    BPFInfo
    AVRDisassembler
    AVRAsmParser
    AVRCodeGen
    AVRDesc
    AVRInfo
    ARMDisassembler
    ARMAsmParser
    ARMCodeGen
    ARMDesc
    ARMUtils
    ARMInfo
    AMDGPUTargetMCA
    AMDGPUDisassembler
    AMDGPUAsmParser
    AMDGPUCodeGen
    AMDGPUDesc
    AMDGPUUtils
    AMDGPUInfo
    AArch64Disassembler
    AArch64AsmParser
    AArch64CodeGen
    AArch64Desc
    AArch64Utils
    AArch64Info
    OrcJIT
    WindowsDriver
    MCJIT
    JITLink
    Interpreter
    ExecutionEngine
    RuntimeDyld
    OrcTargetProcess
    OrcShared
    DWP
    DebugInfoLogicalView
    DebugInfoGSYM
    Option
    ObjectYAML
    ObjCopy
    MCA
    MCDisassembler
    LTO
    Passes
    CFGuard
    Coroutines
    ipo
    Vectorize
    Linker
    Instrumentation
    FrontendOpenMP
    FrontendOpenACC
    FrontendHLSL
    Extensions
    DWARFLinkerParallel
    DWARFLinker
    GlobalISel
    MIRParser
    AsmPrinter
    SelectionDAG
    CodeGen
    Target
    ObjCARCOpts
    CodeGenTypes
    IRPrinter
    InterfaceStub
    FileCheck
    FuzzMutate
    ScalarOpts
    InstCombine
    AggressiveInstCombine
    TransformUtils
    BitWriter
    Analysis
    ProfileData
    Symbolize
    DebugInfoBTF
    DebugInfoPDB
    DebugInfoMSF
    DebugInfoDWARF
    Object
    TextAPI
    MCParser
    IRReader
    AsmParser
    MC
    DebugInfoCodeView
    BitReader
    FuzzerCLI
    Core
    Remarks
    BitstreamReader
    BinaryFormat
    TargetParser
    TableGen
    Support
    Demangle)

# Link against LLVM libraries
target_link_libraries(iron ${llvm_libs})