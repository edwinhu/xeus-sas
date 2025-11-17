# xeus-sas Quick Start Guide

This guide will help you get xeus-sas up and running quickly.

## Prerequisites

### 1. Install Dependencies

#### On macOS (with Homebrew)
```bash
# Install conda/mamba for xeus dependencies
brew install mambaforge

# Activate conda
conda activate base

# Install xeus dependencies
mamba install xeus xeus-zmq xtl nlohmann_json cmake -c conda-forge

# Install GTest (optional, for tests)
mamba install gtest -c conda-forge
```

#### On Ubuntu/Debian
```bash
# Install conda
wget https://github.com/conda-forge/miniforge/releases/latest/download/Mambaforge-Linux-x86_64.sh
bash Mambaforge-Linux-x86_64.sh

# Install xeus dependencies
mamba install xeus xeus-zmq xtl nlohmann_json cmake -c conda-forge

# Install GTest (optional)
sudo apt-get install libgtest-dev
```

### 2. Verify SAS Installation

```bash
# Check if SAS is in PATH
which sas

# If not found, set SAS_PATH
export SAS_PATH=/path/to/your/sas/executable

# Common SAS locations:
# Linux: /usr/local/SASHome/SASFoundation/9.4/bin/sas
# macOS: /Applications/SASHome/SASFoundation/9.4/bin/sas
```

## Build and Install

### 1. Clone and Build

```bash
# Clone the repository
git clone https://github.com/jupyter-xeus/xeus-sas.git
cd xeus-sas

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. \
    -DCMAKE_INSTALL_PREFIX=$CONDA_PREFIX \
    -DCMAKE_PREFIX_PATH=$CONDA_PREFIX \
    -DBUILD_TESTS=ON

# Build
make -j4

# Run tests (optional)
ctest --output-on-failure

# Install
make install
```

### 2. Verify Installation

```bash
# Check that xsas binary is installed
which xsas

# Check Jupyter kernel spec
jupyter kernelspec list

# Should see 'xeus-sas' in the list
```

## First Notebook

### 1. Launch Jupyter

```bash
jupyter notebook
# or
jupyter lab
```

### 2. Create a New Notebook

1. Click "New" → "SAS"
2. Try this simple example:

```sas
/* Create a sample dataset */
DATA work.test;
    INPUT name $ age;
    DATALINES;
Alice 25
Bob 30
Charlie 35
;
RUN;

/* Print the dataset */
PROC PRINT DATA=work.test;
RUN;
```

### 3. Test Code Completion

1. Type `PROC ME` and press Tab
2. Should see `MEANS` in suggestions
3. Complete to `PROC MEANS` and continue:

```sas
PROC MEANS DATA=work.test;
    VAR age;
RUN;
```

### 4. Test Inline Help

1. Place cursor on `MEANS`
2. Press Shift+Tab
3. Should see procedure documentation

## Troubleshooting

### SAS Not Found

**Problem**: "SAS executable not found" error

**Solution**:
```bash
# Set SAS_PATH environment variable
export SAS_PATH=/path/to/sas

# Add to your shell rc file for persistence
echo 'export SAS_PATH=/path/to/sas' >> ~/.bashrc  # or ~/.zshrc
```

### Build Errors

**Problem**: Cannot find xeus/xeus-zmq

**Solution**:
```bash
# Ensure conda environment is activated
conda activate base

# Reinstall dependencies
mamba install xeus xeus-zmq xtl nlohmann_json -c conda-forge --force-reinstall

# Rebuild with correct prefix
cmake .. -DCMAKE_PREFIX_PATH=$CONDA_PREFIX
```

### Kernel Not Found in Jupyter

**Problem**: SAS kernel doesn't appear in Jupyter

**Solution**:
```bash
# Check installation
jupyter kernelspec list

# Manually install kernel spec if needed
jupyter kernelspec install \
    $CONDA_PREFIX/share/jupyter/kernels/xeus-sas \
    --user
```

### Execution Hangs

**Problem**: Code execution hangs indefinitely

**Solution**:
- Check that SAS is working: `$SAS_PATH -nodms -noterminal -sysin /dev/null`
- Check SAS licensing (batch mode may require specific license)
- Try a simpler command: `DATA _null_; RUN;`

## Example Notebooks

See `docs/examples/` for more comprehensive examples:

- `basic_operations.ipynb`: DATA steps and basic PROCs
- `data_steps.ipynb`: Advanced DATA step programming
- `graphics.ipynb`: ODS graphics examples
- `macro_programming.ipynb`: SAS macro language

## Development Workflow

### Make Changes

```bash
# Edit source files in src/ or include/xeus-sas/

# Rebuild
cd build
make -j4

# Test changes
ctest --output-on-failure

# Reinstall
make install

# Restart Jupyter kernel to pick up changes
```

### Debug Mode

```bash
# Build with debug symbols
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j4

# Run with debugger
lldb $CONDA_PREFIX/bin/xsas
# or
gdb $CONDA_PREFIX/bin/xsas
```

### Add New Completions

Edit `src/completion.cpp` and add to the appropriate vector:

```cpp
static const std::vector<std::string> procedures = {
    "MEANS", "FREQ", "REG",
    "MYNEWPROC",  // Add here
    // ...
};
```

### Add New Help

Edit `src/inspection.cpp` and add a new case:

```cpp
else if (upper_proc == "MYNEWPROC")
{
    help << "# PROC MYNEWPROC\n\n";
    help << "Description here...\n";
    // ...
}
```

## Next Steps

1. Read [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) for architecture details
2. Review [CONTRIBUTING.md](CONTRIBUTING.md) for contribution guidelines
3. Check [PHASE1_SUMMARY.md](PHASE1_SUMMARY.md) for current status
4. Explore the codebase in `include/xeus-sas/` and `src/`

## Getting Help

- **GitHub Issues**: Report bugs or request features
- **GitHub Discussions**: Ask questions, share ideas
- **xeus Gitter**: Chat with xeus community
- **SAS Documentation**: https://documentation.sas.com/

## Resources

- [Jupyter kernel documentation](https://jupyter-client.readthedocs.io/en/stable/kernels.html)
- [xeus framework](https://github.com/jupyter-xeus/xeus)
- [xeus-stata](https://github.com/jupyter-xeus/xeus-stata) - Similar kernel
- [sas_kernel](https://github.com/sassoftware/sas_kernel) - Python alternative

Enjoy using xeus-sas!
