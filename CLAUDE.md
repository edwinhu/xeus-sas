# CLAUDE.md

This file provides guidance to Claude Code and other AI agents when working with code in this repository.

## Project Overview

**xeus-sas** is a Jupyter kernel for SAS based on the xeus framework. It provides:
- Interactive SAS execution in Jupyter environments
- Persistent SAS sessions with proper state management
- Rich HTML output for tables and plots
- Terminal-based client support (euporie console)

## Repository Structure

```
xeus-sas/
├── src/           # C++ source files
├── include/       # Header files
├── test/          # Formal unit tests
├── cmake/         # CMake modules
├── share/         # Jupyter kernel specifications
├── .claude/       # Internal documentation (gitignored)
└── README.md      # Public-facing documentation
```

## Documentation Guidelines

### Where to Write Documentation

1. **Internal Development Notes** → `.claude/`
   - Agent-to-agent communications
   - Debugging reports and analysis
   - Test results and verification logs
   - Implementation summaries and technical details
   - Development checklists and planning documents
   - Files like: `*_AGENT_*.md`, `*_DEBUG_*.md`, `*_FIX_*.md`, `*_SUMMARY.md`

2. **Test Files** → `.claude/testing/`
   - Test scripts (`test_*.sh`, `test_*.py`)
   - Test SAS programs (`test_*.sas`)
   - Test utilities and helpers
   - Manual test instructions

3. **Public Documentation** → Root directory
   - Only `README.md` and `CONTRIBUTING.md` should be in root
   - All other documentation goes in `.claude/`

### Documentation Best Practices

- **Always use `.claude/` for internal work** - The `.claude/` folder is gitignored to keep the repository clean
- **Descriptive filenames** - Use clear names like `HTML_RENDERING_FIX.md` not `notes.md`
- **Markdown format** - Use proper markdown formatting for readability
- **Include timestamps** - Add dates to document when work was done
- **Link to source code** - Reference specific files and line numbers when relevant

### Example File Organization

```
.claude/
├── HTML_RENDERING_FIX.md           # Fix documentation
├── TABULATE_IMPLEMENTATION.md      # Feature implementation notes
├── DEBUGGING_SESSION_2024_01.md    # Debugging notes
└── testing/
    ├── test_tables.sas             # Test programs
    ├── test_plots.sh               # Test scripts
    └── verify_output.py            # Verification utilities
```

## Code Modification Guidelines

### Key Files

1. **`src/sas_session.cpp`** - Core SAS session management
   - Handles persistent session via fork/exec
   - Captures stdout/stderr with unbuffered I/O
   - Post-processes HTML output for terminal clients
   - Line ~184: Unbuffered I/O setup (CRITICAL)
   - Line ~435-810: HTML post-processing pipeline

2. **`src/xinterpreter.cpp`** - Jupyter kernel interpreter
   - Implements xeus protocol
   - Manages execution requests
   - Handles display_data messages

3. **`include/xeus-sas/`** - Header files
   - Class declarations
   - Public API

### HTML Post-Processing Pipeline

The kernel applies several transformations to SAS-generated HTML for terminal compatibility:

1. **Colgroup merging** - Combines multiple `<colgroup>` elements
2. **Style stripping** - Removes inline CSS
3. **thead conversion** - Moves `<thead>` content to `<tbody>`
4. **Caption cleanup** - Removes empty caption elements
5. **Table flattening** - Removes rowspan/colspan for PROC TABULATE

**Important**: This pipeline is critical for euporie console rendering. Test any changes with:
```bash
euporie console --kernel=xeus-sas
```

### Testing Requirements

Before committing code changes:

1. **Build successfully**: `cmake --build build/`
2. **Test simple tables**: PROC PRINT with sashelp.class
3. **Test complex tables**: PROC TABULATE with multiple dimensions
4. **Test plots**: PROC SGPLOT with various chart types
5. **Test in euporie**: Verify rendering in terminal client

### Common Pitfalls

1. **Buffered I/O** - Always use unbuffered mode (`setvbuf(..., _IONBF, 0)`) when using `read()` on FILE* streams
2. **HTML truncation** - Ensure complete HTML capture with proper polling timeouts
3. **Terminal compatibility** - Terminal clients don't support complex CSS or rowspan/colspan
4. **Session persistence** - Verify SAS process stays alive between executions

## Development Workflow

### Making Changes

1. **Document your approach** in `.claude/` first (e.g., `.claude/NEW_FEATURE_PLAN.md`)
2. **Implement changes** in source files
3. **Test thoroughly** with various SAS procedures
4. **Document results** in `.claude/` (e.g., `.claude/NEW_FEATURE_RESULTS.md`)
5. **Update README.md** only if adding user-facing features
6. **Commit** with clear message referencing `.claude/` docs

### Debugging

1. **Check stderr logs**: `/tmp/xeus_sas_kernel_stderr.log` (configured in xsas-wrapper)
2. **Examine HTML output**: `/tmp/xeus_sas_extracted_html_debug.html`
3. **Compare with LISTING**: Generate both HTML and plain text output for comparison
4. **Test with simple cases first**: Use sashelp datasets for reproducibility

### Environment

- **SAS**: SAS Foundation 9.4 (`/data/sas/SASFoundation/9.4/bin/sas_u8`)
- **Build**: CMake + C++17
- **Testing**: euporie console, Jupyter Lab, JupyterHub
- **Platform**: Linux (primary), macOS (secondary)

## Key Concepts

### Persistent Sessions

Unlike sas_kernel which spawns a new SAS process per execution, xeus-sas maintains:
- Single fork/exec at kernel startup
- stdin/stdout pipes for bidirectional communication
- Persistent workspace across executions
- Proper cleanup on kernel shutdown

### ODS HTML5 Output

SAS output is captured via:
```sas
ods html5 body=_webout;
/* user code */
ods html5 close;
```

The kernel then post-processes this HTML for terminal compatibility.

### Terminal Rendering

Terminal clients like euporie have limitations:
- No complex CSS support
- No rowspan/colspan support
- Limited color palette
- Text-based layout

Our HTML post-processing addresses these constraints.

## Questions or Issues?

When investigating issues:

1. **Check `.claude/` first** - Previous debugging sessions may have answers
2. **Search existing documentation** - `rg "keyword" .claude/`
3. **Document your findings** - Add new `.claude/NEW_FINDING.md` file
4. **Reference line numbers** - Use `file:line` format for clarity

## License

This project is licensed under BSD-3-Clause. See LICENSE file for details.
