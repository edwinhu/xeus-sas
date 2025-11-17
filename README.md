# xeus-sas

A native C++ Jupyter kernel for SAS, built on the [xeus](https://github.com/jupyter-xeus/xeus) framework.

## Overview

xeus-sas is a Jupyter kernel for SAS that enables interactive SAS programming in Jupyter notebooks and JupyterLab. Built with modern C++ and the xeus framework, it provides:

- **Native Performance**: Direct C++ implementation for low latency and efficient resource usage
- **Intelligent Output**: Automatically displays the most relevant output (log vs listing)
- **Code Completion**: Context-aware suggestions for procedures, functions, and keywords
- **Inline Help**: Documentation and syntax help via Shift+Tab
- **Graphics Support**: Automatic display of ODS graphics (PNG, SVG)
- **Full SAS Support**: DATA steps, PROCs, macro language, and more

## Status

**Phase 1 (Foundation)** - Current Implementation

This is the initial implementation providing:
- Basic SAS code execution in batch mode
- Log and listing output parsing
- Error detection and display
- Code completion for common procedures and keywords
- Basic inspection/help system
- CMake build system

## Requirements

### System Requirements

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.8 or later
- SAS 9.4+ or SAS Viya

### Dependencies

**Required:**
- [xeus](https://github.com/jupyter-xeus/xeus) >= 5.0
- [xeus-zmq](https://github.com/jupyter-xeus/xeus-zmq) >= 3.0
- [xtl](https://github.com/xtensor-stack/xtl) >= 0.7
- [nlohmann_json](https://github.com/nlohmann/json) >= 3.11

**Optional:**
- Google Test (for running tests)
- Doxygen (for generating documentation)

### SAS Requirements

- SAS 9.4 or later
- Batch/non-interactive mode support
- SAS/STAT recommended
- ODS Graphics enabled (for graphics support)

## Installation

### From Source

```bash
# Clone the repository
git clone https://github.com/jupyter-xeus/xeus-sas.git
cd xeus-sas

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local

# Build
make -j4

# Install
sudo make install
```

### Setting SAS Path

If SAS is not detected automatically, set the `SAS_PATH` environment variable:

```bash
export SAS_PATH=/path/to/sas/executable
```

Add this to your `~/.bashrc` or `~/.zshrc` for persistence.

### Installing the Kernel Spec

The kernel specification is installed automatically during `make install`. To verify:

```bash
jupyter kernelspec list
```

You should see `xeus-sas` in the list.

## Usage

### Starting a Notebook

```bash
jupyter notebook
```

Create a new notebook and select "SAS" as the kernel.

### Example Code

```sas
/* Create a sample dataset */
DATA work.cars;
    SET sashelp.cars;
    WHERE origin = 'USA';
RUN;

/* Calculate statistics */
PROC MEANS DATA=work.cars N MEAN STD MIN MAX;
    VAR mpg_city mpg_highway;
    CLASS type;
RUN;

/* Create a plot */
PROC SGPLOT DATA=work.cars;
    SCATTER X=mpg_city Y=mpg_highway / GROUP=type;
RUN;
```

## Features

### Code Execution

The kernel executes SAS code in batch mode and intelligently displays output:

- **Errors**: Shows colorized log with error messages highlighted
- **Successful execution**: Shows listing output (procedure results)
- **Graphics**: Automatically displays ODS graphics inline

### Code Completion

Press `Tab` while typing to get suggestions for:

- SAS procedures (MEANS, FREQ, REG, etc.)
- DATA step keywords (SET, MERGE, BY, etc.)
- Global statements (LIBNAME, OPTIONS, etc.)
- Macro language (%LET, %PUT, etc.)
- SAS functions (MEAN, SUM, SUBSTR, etc.)

### Code Inspection

Press `Shift+Tab` to view help for:

- Procedure syntax and options
- Function signatures
- Dataset information

### Output Handling

The kernel parses SAS output to determine what to display:

1. **If errors occur**: Log is displayed with errors highlighted in red
2. **If successful**: Listing output is displayed
3. **Graphics**: PNG/SVG images are embedded in the notebook

## Architecture

xeus-sas consists of several key components:

- **xinterpreter**: Implements the Jupyter kernel protocol
- **sas_session**: Manages SAS process lifecycle and communication
- **sas_parser**: Parses SAS log and listing output
- **completion_engine**: Provides code completion
- **inspection_engine**: Provides inline help and documentation

See [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) for detailed architecture documentation.

## Development

### Building Tests

```bash
cmake .. -DBUILD_TESTS=ON
make
ctest
```

### Project Structure

```
xeus-sas/
├── cmake/                  # CMake modules
├── include/xeus-sas/      # Public headers
├── src/                   # Implementation files
├── share/jupyter/kernels/ # Kernel specification
├── test/                  # Unit tests
└── docs/                  # Documentation
```

## Roadmap

### Phase 1: Foundation (Complete)
- ✓ Basic SAS execution
- ✓ Output parsing
- ✓ Code completion
- ✓ Inline help

### Phase 2: Output Parsing (Planned)
- Enhanced error detection
- ANSI colorization
- ODS graphics embedding
- Warning handling

### Phase 3: Code Intelligence (Planned)
- Variable name completion
- Dataset name completion
- Enhanced procedure help
- Macro introspection

### Phase 4: Advanced Features (Planned)
- Interactive SAS session (vs batch mode)
- Interrupt handling
- Session restart
- Macro variable access via user_expressions

### Phase 5: Testing & Documentation (Planned)
- Comprehensive test suite
- User documentation
- Example notebooks
- API documentation

## Comparison with sas_kernel

[sas_kernel](https://github.com/sassoftware/sas_kernel) is an excellent Python-based Jupyter kernel for SAS. xeus-sas offers:

**Advantages:**
- Native C++ performance
- Lower memory footprint
- Tighter xeus ecosystem integration
- BSD-3-Clause license (vs Apache 2.0)

**Tradeoffs:**
- Newer/less mature
- Fewer connection methods (currently batch only)
- Different feature set

Both kernels are valid choices depending on your needs.

## Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

BSD-3-Clause License. See [LICENSE](LICENSE) for details.

## Acknowledgments

- Built on the [xeus](https://github.com/jupyter-xeus/xeus) framework
- Inspired by [xeus-stata](https://github.com/jupyter-xeus/xeus-stata)
- Reference implementation: [sas_kernel](https://github.com/sassoftware/sas_kernel)

## Support

- **Issues**: [GitHub Issues](https://github.com/jupyter-xeus/xeus-sas/issues)
- **Documentation**: [Read the Docs](https://xeus-sas.readthedocs.io)
- **SAS Documentation**: [documentation.sas.com](https://documentation.sas.com/)

## Authors

See [AUTHORS.md](AUTHORS.md) for contributor list.
