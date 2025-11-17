# ODS HTML5 Rich Output Implementation for xeus-sas

## Summary

Successfully implemented ODS HTML5 rich output for xeus-sas kernel, following the approach used by sas_kernel. The kernel now generates and displays HTML tables instead of plain text output.

## Implementation Details

### 1. Updated `execution_result` Structure

**File**: `include/xeus-sas/sas_session.hpp`

Added two new fields:
- `std::string html_output;` - Stores HTML5 output from ODS
- `bool has_html;` - Flag to indicate HTML vs TEXT mode

### 2. Separate stdout and stderr Streams

**File**: `src/sas_session.cpp`

- Added `FILE* m_sas_stderr;` member variable to the impl class
- Modified `initialize_session()` to create three pipes: stdin, stdout, stderr
- stdout receives HTML output from ODS
- stderr receives SAS log messages
- Updated `shutdown()` to properly close all three streams

### 3. Wrap User Code with ODS HTML5 Commands

**File**: `src/sas_session.cpp` in `execute()` method

User code is now wrapped with:
```sas
ods listing close;
ods html5 (id=xeus_sas_internal) file=stdout options(bitmap_mode='inline') device=svg style=HTMLBlue;
ods graphics on / outputfmt=png;

[USER CODE HERE]

ods html5 (id=xeus_sas_internal) close;
ods listing;
```

This approach:
- Closes default text listing
- Opens HTML5 output to stdout
- Enables inline graphics as base64-encoded images
- Executes user code
- Closes HTML5 and restores listing

### 4. Read stdout and stderr Separately

**File**: `src/sas_session.cpp` in `execute()` method

Implemented non-blocking I/O using poll() to read both streams:
- Added `#include <fcntl.h>` and `#include <poll.h>`
- Set both file descriptors to non-blocking mode
- Used `poll()` to monitor both streams simultaneously
- Read HTML from stdout
- Read log from stderr
- Marker (`XEUS_SAS_END_X`) is sent to stderr to detect end of execution
- Restored blocking mode after reading

This prevents deadlock that would occur if we tried to read streams sequentially.

### 5. HTML Detection and Parsing

**File**: `src/sas_session.cpp` in `execute()` method

Inline HTML detection:
```cpp
result.has_html = (html_output.find("<!DOCTYPE html>") != std::string::npos ||
                  html_output.find("<html") != std::string::npos);
```

Also performs error detection and graph file extraction from the log stream.

### 6. Send HTML via display_data()

**File**: `src/xinterpreter.cpp` in `execute_request_impl()`

Modified execution result handling:
- Check if `result.has_html` is true
- If HTML is available, use `display_data()` with MIME type `text/html`
- Include plain text log as fallback in `text/plain`
- Falls back to plain text output if no HTML is detected

Example:
```cpp
if (result.has_html && !result.html_output.empty())
{
    nl::json html_data;
    html_data["text/html"] = result.html_output;
    html_data["text/plain"] = result.log;  // fallback
    display_data(html_data, nl::json::object(), nl::json::object());
}
```

## Key Design Decisions

1. **Marker Placement**: Marker is placed in stderr (log stream), not stdout (HTML stream), to cleanly separate HTML content from log messages.

2. **Non-blocking I/O**: Used `poll()` with non-blocking file descriptors to read both streams in parallel, avoiding potential deadlocks.

3. **HTML Detection**: Simple string search for DOCTYPE or html tags to determine if output contains valid HTML.

4. **Error Handling**: Even with errors, attempt to display HTML if available. Log always contains error information.

5. **Fallback**: If no HTML is detected (e.g., for non-PROC code), system falls back to existing plain text behavior.

## Build and Installation

1. Build the kernel:
   ```bash
   cd build
   pixi run bash -c "make -j4"
   ```

2. Install the binary:
   ```bash
   cp build/xsas ~/.local/bin/xsas
   ```

3. Verify kernel is available:
   ```bash
   jupyter kernelspec list | grep xeus-sas
   ```

## Testing

To test the HTML output:

1. Start a Jupyter notebook or console with xeus-sas kernel
2. Execute a PROC that generates output:
   ```sas
   PROC PRINT DATA=sashelp.class;
   RUN;
   ```

3. Expected behavior:
   - HTML table with formatted output
   - Styled with SAS HTMLBlue theme
   - Inline graphics if any
   - No plain text listing

## Modified Files

- `include/xeus-sas/sas_session.hpp` - Added HTML fields to execution_result
- `src/sas_session.cpp` - Main implementation: stderr stream, ODS wrapping, dual-stream reading
- `src/xinterpreter.cpp` - Display HTML via display_data()

## Compatibility

- Platform: Linux (uses POSIX poll, fcntl, pipes)
- Windows: Falls back to batch mode (not updated for HTML yet)
- Existing behavior: Preserved for non-PROC code and error cases

## Future Enhancements

1. Add Windows support for persistent session with HTML output
2. Optimize HTML output (strip unnecessary whitespace, minify)
3. Add configuration option to toggle HTML vs plain text
4. Support additional ODS styles (beyond HTMLBlue)
5. Better handling of very large HTML output
6. Add unit tests for HTML detection and parsing
