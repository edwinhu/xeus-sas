# xeus-sas Implementation Plan

## Overview

xeus-sas is a proposed native C++ implementation of a Jupyter kernel for SAS, built on the [xeus](https://github.com/jupyter-xeus/xeus) framework. This project aims to provide native performance and tight integration with the Jupyter ecosystem, similar to xeus-stata but adapted for SAS statistical software.

## Background Research

### Existing Solutions

1. **sas_kernel** (Python-based)
   - Uses MetaKernel framework
   - Relies on SASPy for SAS communication
   - Supports multiple connection methods (IOM, STDIO, HTTP)
   - Handles log and listing output intelligently
   - Licensed under Apache 2.0

2. **xeus-stata** (C++-based)
   - Native C++ implementation using xeus framework
   - Direct process communication with Stata
   - Components: xinterpreter, stata_session, stata_parser, completion, inspection
   - Uses CMake build system
   - Licensed under BSD-3-Clause

### Design Decision: Native C++ vs Python

**Recommendation: Native C++ Implementation**

Rationale:
- sas_kernel already provides an excellent Python-based solution
- A native C++ kernel would offer:
  - Better performance and lower latency
  - Reduced memory footprint
  - Tighter integration with xeus ecosystem
  - Potential for better widget support via xwidgets
  - Different licensing options (BSD vs GPL)

## Architecture

### Core Components

1. **xinterpreter** (`xinterpreter.hpp/cpp`)
   - Implements the Jupyter kernel protocol via xeus
   - Inherits from `xeus::xinterpreter`
   - Methods to implement:
     - `execute_request_impl()` - Execute SAS code
     - `complete_request_impl()` - Code completion
     - `inspect_request_impl()` - Code inspection/help
     - `is_complete_request_impl()` - Check if code is complete
     - `kernel_info_request_impl()` - Kernel metadata
     - `shutdown_request_impl()` - Clean shutdown

2. **sas_session** (`sas_session.hpp/cpp`)
   - Manages SAS process lifecycle
   - Handles communication with SAS
   - Returns `execution_result` structure:
     ```cpp
     struct execution_result {
         std::string log;           // SAS log output
         std::string listing;       // LST/ODS output
         bool is_error;             // Error flag
         int error_code;            // SAS error code
         std::string error_message; // Error details
         std::vector<std::string> graph_files; // Generated graphics
     };
     ```
   - Methods:
     - `execute(code)` - Run SAS code
     - `get_version()` - Get SAS version
     - `is_ready()` - Check session status
     - `shutdown()` - Terminate session
     - `interrupt()` - Interrupt execution
     - `get_macro(name)` - Get macro variable value
     - `set_macro(name, value)` - Set macro variable

3. **sas_parser** (`sas_parser.hpp/cpp`)
   - Parses SAS log and listing output
   - Detects errors, warnings, and notes
   - Extracts generated graphics paths
   - Determines which output to display (log vs listing)
   - Methods:
     - `parse_output(raw_output)` - Parse SAS output
     - `extract_errors(log)` - Find ERROR messages
     - `extract_graphics(log)` - Find ODS graphics
     - `colorize_log(log)` - Add ANSI color codes

4. **completion_engine** (`completion.hpp/cpp`)
   - Provides intelligent code completion
   - Completion types:
     - SAS procedures (PROC MEANS, PROC REG, etc.)
     - DATA step keywords
     - Global statements
     - Macro language elements
     - Variable names (from active datasets)
     - Dataset names (from libraries)
   - Data source: JSON files with SAS syntax dictionary

5. **inspection_engine** (`inspection.hpp/cpp`)
   - Provides inline help and documentation
   - Shows procedure syntax
   - Displays dataset information (PROC CONTENTS)
   - Shows macro definitions

## SAS Communication Strategy

### Approach 1: Batch Mode (Recommended for Initial Implementation)

Use SAS in batch/non-interactive mode similar to how xeus-stata handles Stata:

```cpp
// Pseudocode
std::string sas_path = find_sas_executable();
// Run: sas -nodms -noterminal -stdio
// Communicate via stdin/stdout
```

**Advantages:**
- Simpler to implement
- Cross-platform (works on Linux, Windows, macOS)
- No dependency on SASPy

**Challenges:**
- May require specific SAS licensing (batch mode)
- Need to handle different SAS installations (Base, Enterprise, Viya)
- Platform-specific executable names and paths

### Approach 2: IOM Integration (Future Enhancement)

For more advanced use cases, integrate with SAS IOM (Integration Object Model):

**Advantages:**
- Can connect to remote SAS servers
- Better for enterprise environments
- Leverages existing SAS infrastructure

**Challenges:**
- More complex implementation
- Requires IOM bridge libraries
- Platform-specific code

## Implementation Phases

### Phase 1: Foundation (Weeks 1-3)

**Goals:**
- Set up project structure
- Implement basic SAS session management
- Get code execution working

**Tasks:**
1. Create CMake build configuration
   - Find xeus, xeus-zmq, xtl, nlohmann_json
   - Set up C++17 standard
   - Configure installation paths

2. Implement basic `sas_session` class
   - Detect SAS installation paths
   - Spawn SAS process in batch mode
   - Send code via stdin
   - Read output from stdout/stderr

3. Implement basic `xinterpreter`
   - Handle execute requests
   - Return raw output to Jupyter

4. Create kernel.json specification
   - Define kernel metadata
   - Set up argv for kernel launch

5. Test basic execution
   - Simple DATA steps
   - PROC PRINT
   - Verify log and listing output

### Phase 2: Output Parsing (Weeks 4-5)

**Goals:**
- Intelligent output handling
- Error detection and display

**Tasks:**
1. Implement `sas_parser` class
   - Parse log for ERROR/WARNING/NOTE patterns
   - Separate log from listing output
   - Detect ODS graphics output

2. Implement output selection logic
   - Show listing if no errors
   - Show log if errors occurred
   - Show both if needed

3. Add ANSI colorization
   - Color ERRORs in red
   - Color WARNINGs in yellow
   - Color NOTEs in blue

4. Handle ODS graphics
   - Detect PNG/SVG output
   - Embed in notebook using Jupyter display protocol

### Phase 3: Code Intelligence (Weeks 6-7)

**Goals:**
- Code completion
- Inline help

**Tasks:**
1. Create SAS syntax dictionaries
   - Procedures list (JSON)
   - Global statements
   - DATA step keywords
   - Functions
   - Format/informat names

2. Implement `completion_engine`
   - Token-based completion
   - Context-aware suggestions
   - Variable name completion (from PROC CONTENTS)

3. Implement `inspection_engine`
   - Show procedure syntax
   - Display PROC CONTENTS output
   - Show macro definitions

### Phase 4: Advanced Features (Weeks 8-10)

**Goals:**
- Session management
- Macro support
- Configuration

**Tasks:**
1. Environment variable support
   - `SAS_PATH` - SAS executable location
   - `SAS_OPTIONS` - Additional SAS options
   - `SASLOCALCFG` - SAS configuration file

2. Macro variable access
   - `get_macro()` / `set_macro()` methods
   - Integration with Jupyter user_expressions

3. Interrupt handling
   - SIGINT to interrupt SAS execution
   - Graceful cleanup

4. Session restart
   - Handle SAS crashes
   - Clear WORK library on restart

### Phase 5: Testing & Documentation (Weeks 11-12)

**Goals:**
- Comprehensive testing
- User documentation

**Tasks:**
1. Unit tests (Google Test)
   - Parser tests
   - Session management tests
   - Completion engine tests

2. Integration tests
   - End-to-end notebook execution
   - Error handling scenarios
   - Graphics generation

3. Documentation
   - Installation guide
   - Usage examples
   - API documentation (Doxygen)
   - Troubleshooting guide

4. Example notebooks
   - Basic SAS operations
   - DATA steps and PROCs
   - Graphics (ODS)
   - Macro programming

## Technical Specifications

### Dependencies

**Required:**
- xeus >= 5.0
- xeus-zmq >= 3.0
- xtl >= 0.7
- nlohmann_json >= 3.11
- CMake >= 3.8
- C++17 compatible compiler
- SAS 9.4+ or SAS Viya

**Optional:**
- Google Test (for unit tests)
- Doxygen (for documentation)

### Platform Support

**Initial targets:**
- Linux (Ubuntu, RHEL, CentOS)
- macOS (Intel and Apple Silicon)

**Future:**
- Windows (requires different SAS invocation)

### SAS Requirements

**Minimum:**
- SAS 9.4 (released July 2013 or later)
- SAS/STAT (recommended)
- SAS/GRAPH or ODS Graphics enabled

**Licensing:**
- Must support batch/non-interactive mode
- May require additional licensing for non-interactive execution

## Project Structure

```
xeus-sas/
├── CMakeLists.txt              # Main build configuration
├── LICENSE                     # BSD-3-Clause
├── README.md                   # Project overview
├── CONTRIBUTING.md             # Contribution guidelines
├── IMPLEMENTATION_PLAN.md      # This document
│
├── cmake/                      # CMake modules
│   └── FindSAS.cmake          # Locate SAS installation
│
├── include/xeus-sas/          # Public headers
│   ├── xeus_sas_config.hpp    # Build configuration
│   ├── xinterpreter.hpp       # Kernel interpreter
│   ├── sas_session.hpp        # SAS session management
│   ├── sas_parser.hpp         # Output parser
│   ├── completion.hpp         # Code completion
│   └── inspection.hpp         # Code inspection
│
├── src/                       # Implementation
│   ├── main.cpp              # Entry point
│   ├── xinterpreter.cpp      # Interpreter implementation
│   ├── sas_session.cpp       # Session implementation
│   ├── sas_parser.cpp        # Parser implementation
│   ├── completion.cpp        # Completion implementation
│   └── inspection.cpp        # Inspection implementation
│
├── share/jupyter/kernels/xeus-sas/
│   ├── kernel.json.in        # Kernel specification
│   ├── logo-32x32.png        # Kernel icon (small)
│   └── logo-64x64.png        # Kernel icon (large)
│
├── data/                      # Data files
│   ├── sas_procedures.json   # SAS procedures list
│   ├── sas_keywords.json     # SAS keywords
│   └── sas_functions.json    # SAS functions
│
├── test/                      # Tests
│   ├── CMakeLists.txt
│   ├── test_parser.cpp
│   ├── test_session.cpp
│   └── test_completion.cpp
│
├── docs/                      # Documentation
│   ├── source/
│   │   ├── conf.py
│   │   ├── index.rst
│   │   ├── installation.rst
│   │   └── usage.rst
│   └── examples/             # Example notebooks
│       ├── basic_operations.ipynb
│       ├── data_steps.ipynb
│       ├── graphics.ipynb
│       └── macro_programming.ipynb
│
└── .github/                   # GitHub configuration
    └── workflows/
        ├── build.yml         # CI/CD pipeline
        └── test.yml          # Test suite
```

## Key Differences from xeus-stata

1. **SAS Output Model**
   - SAS has two distinct output streams (log and listing)
   - Need intelligent logic to determine which to display
   - Must handle ODS (Output Delivery System) graphics

2. **SAS Invocation**
   - Different command-line options than Stata
   - Platform-specific executable names:
     - Linux/Unix: `sas`
     - macOS: `sas` or `/Applications/SASHome/SASFoundation/9.4/bin/sas_u8`
     - Windows: `sas.exe` or `sas.com`

3. **Language Features**
   - SAS has macro language (separate from base SAS)
   - Need to support `%let`, `%put`, etc.
   - Global statements (LIBNAME, FILENAME, OPTIONS)

4. **Graphics Handling**
   - ODS graphics engine (PNG, SVG, PDF)
   - May need temporary directory for graphics
   - Graphics detection from log

## Risks and Mitigation

### Risk 1: SAS Licensing Requirements

**Risk:** SAS may require specific licensing for batch/non-interactive mode.

**Mitigation:**
- Document licensing requirements clearly
- Provide installation verification steps
- Support multiple connection methods (batch, IOM)

### Risk 2: Platform-Specific Behavior

**Risk:** SAS behaves differently on Windows vs Unix systems.

**Mitigation:**
- Start with Linux/macOS (simpler stdio model)
- Add Windows support in Phase 4
- Abstract platform differences in `sas_session` class

### Risk 3: Output Parsing Complexity

**Risk:** SAS log format is complex and may vary by version.

**Mitigation:**
- Use robust regex patterns
- Test with multiple SAS versions (9.4, Viya)
- Provide fallback to raw output if parsing fails

### Risk 4: Performance with Large Datasets

**Risk:** Copying large output through pipes may be slow.

**Mitigation:**
- Use ODS to limit output size
- Consider output caching
- Implement pagination for large results

## Success Criteria

### Minimum Viable Product (MVP)

- [ ] Execute basic SAS code (DATA steps, PROCs)
- [ ] Display log on errors
- [ ] Display listing on success
- [ ] Basic error detection
- [ ] Works on Linux with SAS 9.4+

### Full Release (v1.0)

- [ ] Intelligent output selection (log vs listing)
- [ ] ODS graphics display
- [ ] Code completion (procedures, keywords)
- [ ] Code inspection (help)
- [ ] Cross-platform (Linux, macOS)
- [ ] Comprehensive tests (>80% coverage)
- [ ] Documentation and examples
- [ ] CI/CD pipeline

### Future Enhancements (v2.0+)

- [ ] IOM connection support (remote SAS)
- [ ] SAS Viya integration
- [ ] Macro debugging
- [ ] xwidgets integration
- [ ] Windows support
- [ ] Conda packaging

## Alternative Approaches Considered

### 1. Extend sas_kernel

**Pros:**
- Builds on existing, working code
- Large user base
- Well-tested

**Cons:**
- Already exists and works well
- Limited value-add
- Python performance limitations

**Decision:** Rejected - sas_kernel is already excellent

### 2. Use SASPy from C++

**Pros:**
- Leverage existing SASPy functionality
- Multiple connection methods

**Cons:**
- Would require embedding Python in C++
- Defeats purpose of native implementation
- Complex dependency management

**Decision:** Rejected - defeats purpose of native kernel

### 3. Pure IOM Implementation

**Pros:**
- Enterprise-ready
- Remote execution

**Cons:**
- Complex to implement
- Requires proprietary libraries
- Not accessible to all users

**Decision:** Deferred to Phase 4 / v2.0

## Timeline Estimate

| Phase | Duration | Target Completion |
|-------|----------|-------------------|
| Phase 1: Foundation | 3 weeks | Week 3 |
| Phase 2: Output Parsing | 2 weeks | Week 5 |
| Phase 3: Code Intelligence | 2 weeks | Week 7 |
| Phase 4: Advanced Features | 3 weeks | Week 10 |
| Phase 5: Testing & Documentation | 2 weeks | Week 12 |
| **Total** | **12 weeks** | **~3 months** |

## Resources Required

### Development
- 1 C++ developer with Jupyter/xeus experience
- Access to SAS installations (9.4 and Viya)
- Linux development environment
- macOS testing environment

### Reference Materials
- SAS 9.4 documentation
- xeus documentation and examples
- xeus-stata source code (reference)
- sas_kernel source code (reference)
- Jupyter kernel protocol specification

## Next Steps

1. **Immediate (Week 1)**
   - Set up development environment
   - Create CMakeLists.txt
   - Implement FindSAS.cmake module
   - Create basic project structure

2. **Short-term (Weeks 2-4)**
   - Implement sas_session with basic execution
   - Test with simple SAS programs
   - Implement basic xinterpreter

3. **Medium-term (Weeks 5-8)**
   - Complete output parsing
   - Add code completion
   - Implement graphics handling

4. **Long-term (Weeks 9-12)**
   - Comprehensive testing
   - Documentation
   - Release preparation

## References

- [xeus Framework](https://github.com/jupyter-xeus/xeus)
- [xeus-stata](https://github.com/jupyter-xeus/xeus-stata) - Reference implementation
- [sas_kernel](https://github.com/sassoftware/sas_kernel) - Python alternative
- [SASPy](https://github.com/sassoftware/saspy) - SAS Python interface
- [Jupyter Kernel Protocol](https://jupyter-client.readthedocs.io/en/stable/kernels.html)
- [SAS 9.4 Documentation](https://documentation.sas.com/)

## Conclusion

xeus-sas represents an opportunity to create a high-performance, native Jupyter kernel for SAS that complements the existing Python-based sas_kernel. By following the proven architecture of xeus-stata and adapting it to SAS's unique requirements (dual output streams, ODS graphics, macro language), we can deliver a valuable tool for the SAS user community.

The 12-week timeline is ambitious but achievable with focused effort. The phased approach allows for early testing and iteration, reducing risk of major issues late in development.

The native C++ implementation will offer performance benefits and tighter integration with the xeus ecosystem, while the BSD license provides flexibility for users and potential contributors.
