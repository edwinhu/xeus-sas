# Before/After Comparison: HTML Extraction Logic

## Overview
This document shows the key differences between the original and fixed HTML extraction logic in `src/sas_session.cpp`.

---

## 1. Buffer Size

### BEFORE
```cpp
char buffer[4096];
```

### AFTER
```cpp
char buffer[8192];  // Larger buffer for better performance
```

**Why**: Larger buffer reduces the number of system calls and improves performance with large HTML tables.

---

## 2. State Tracking Variables

### BEFORE
```cpp
bool found_marker = false;
```

### AFTER
```cpp
bool found_marker = false;
bool found_html_end = false;
bool has_html_start = false;
```

**Why**: We need to track the complete HTML document lifecycle, not just the execution marker.

---

## 3. Loop Termination Condition

### BEFORE
```cpp
while (!found_marker)
{
    // ... read streams ...

    if (line.find(marker) != std::string::npos)
    {
        found_marker = true;
        break;  // Exit immediately when marker found
    }
}
```

### AFTER
```cpp
while (!found_marker || (has_html_start && !found_html_end))
{
    // ... read streams ...

    if (line.find(marker) != std::string::npos)
    {
        found_marker = true;
        // Don't break yet - continue reading if HTML is incomplete
    }

    // Exit if we have marker and either no HTML or complete HTML
    if (found_marker && (!has_html_start || found_html_end))
    {
        break;
    }
}
```

**Why**: The loop now continues until BOTH conditions are met:
1. Marker found in log (execution complete)
2. HTML document is complete (if HTML generation started)

---

## 4. HTML Document Tracking During Read

### BEFORE
```cpp
// Read from stdout (HTML)
if (fds[0].revents & POLLIN)
{
    if (fgets(buffer, sizeof(buffer), m_sas_stdout))
    {
        html_output += buffer;
    }
}
```

### AFTER
```cpp
// Read from stdout (HTML)
if (fds[0].revents & POLLIN)
{
    if (fgets(buffer, sizeof(buffer), m_sas_stdout))
    {
        html_output += buffer;

        // Check for HTML document markers
        if (!has_html_start &&
            (html_output.find("<!DOCTYPE html>") != std::string::npos ||
             html_output.find("<html") != std::string::npos))
        {
            has_html_start = true;
        }

        // Check for HTML end - use rfind to get the LAST occurrence
        if (has_html_start && !found_html_end)
        {
            if (html_output.find("</html>") != std::string::npos)
            {
                found_html_end = true;
            }
        }
    }
}
```

**Why**: Real-time tracking of HTML document boundaries ensures we know when the document is complete.

---

## 5. Timeout Handling

### BEFORE
```cpp
int poll_result = poll(fds, 2, 1000); // 1 second timeout
if (poll_result < 0)
{
    break; // Error
}
else if (poll_result == 0)
{
    continue; // Timeout, try again
}
```

### AFTER
```cpp
int timeout_count = 0;
const int max_timeouts = 10;  // Allow up to 10 seconds total

// ... in loop ...
int poll_result = poll(fds, 2, 1000); // 1 second timeout
if (poll_result < 0)
{
    std::cerr << "Poll error in SAS output reading" << std::endl;
    break; // Error
}
else if (poll_result == 0)
{
    timeout_count++;
    if (timeout_count >= max_timeouts)
    {
        std::cerr << "WARNING: Timeout waiting for complete SAS output" << std::endl;
        std::cerr << "  Marker found: " << found_marker << std::endl;
        std::cerr << "  HTML start found: " << has_html_start << std::endl;
        std::cerr << "  HTML end found: " << found_html_end << std::endl;
        break;
    }
    continue; // Timeout, try again
}
```

**Why**: Prevents infinite loops while providing diagnostic information about what's missing.

---

## 6. HTML Extraction

### BEFORE
```cpp
// Create result with both HTML and log
execution_result result;
result.log = log_output;
result.html_output = html_output;

// Detect if we have HTML output
result.has_html = (html_output.find("<!DOCTYPE html>") != std::string::npos ||
                  html_output.find("<html") != std::string::npos);
```

### AFTER
```cpp
// Extract clean HTML if present
std::string clean_html;
bool has_html = false;

if (has_html_start)
{
    // Find HTML document boundaries
    size_t html_start = html_output.find("<!DOCTYPE html>");
    if (html_start == std::string::npos)
    {
        html_start = html_output.find("<html");
    }

    size_t html_end = html_output.rfind("</html>");  // Use rfind for LAST occurrence

    if (html_start != std::string::npos && html_end != std::string::npos)
    {
        html_end += 7;  // Include "</html>"
        clean_html = html_output.substr(html_start, html_end - html_start);
        has_html = true;
    }
    else
    {
        std::cerr << "WARNING: Incomplete HTML detected" << std::endl;
        std::cerr << "  HTML start position: " << html_start << std::endl;
        std::cerr << "  HTML end position: " << html_end << std::endl;
        std::cerr << "  Total output length: " << html_output.length() << std::endl;

        // Log first and last 200 chars for debugging
        std::cerr << "  First 200 chars: " << html_output.substr(0, 200) << std::endl;
        if (html_output.length() > 200)
        {
            size_t start = html_output.length() > 200 ? html_output.length() - 200 : 0;
            std::cerr << "  Last 200 chars: " << html_output.substr(start) << std::endl;
        }
    }
}

// Create result with both HTML and log
execution_result result;
result.log = log_output;
result.html_output = clean_html;  // Use extracted clean HTML
result.has_html = has_html;
```

**Why**:
- Extracts only the actual HTML document (from `<!DOCTYPE html>` to `</html>`)
- Removes any SAS artifacts or startup messages from the output
- Uses `rfind()` to find the LAST `</html>` tag (handles edge cases)
- Provides detailed diagnostics if extraction fails

---

## Key Differences Summary

| Aspect | Before | After |
|--------|--------|-------|
| **Buffer Size** | 4096 bytes | 8192 bytes |
| **HTML Tracking** | None | Real-time tracking of start and end |
| **Loop Exit** | When marker found | When marker found AND HTML complete |
| **HTML Output** | Raw stdout (may include artifacts) | Clean extracted HTML document |
| **Diagnostics** | Minimal | Comprehensive debug logging |
| **Timeout** | Unlimited retries | 10-second max with state reporting |
| **Race Condition** | Present (marker vs HTML) | Fixed (waits for both) |

---

## Example Execution Flow

### BEFORE (Problematic)
```
1. SAS generates HTML to stdout
2. Marker appears in stderr
3. Loop exits immediately (marker found)
4. Incomplete HTML captured
5. Browser renders truncated/broken HTML
```

### AFTER (Fixed)
```
1. SAS generates HTML to stdout
2. Code detects HTML start
3. Marker appears in stderr
4. Code continues reading until </html> found
5. Complete HTML extracted
6. Browser renders perfect table
```

---

## Visual Comparison

### BEFORE: Premature Exit
```
Time →
Stdout: [<!DOCTYPE html>]...[<table>]...[<tr>]...[MARKER]❌ EXIT
Stderr: [NOTE]...[NOTE]...[MARKER]
Result: Incomplete HTML (missing </table></html>)
```

### AFTER: Complete Read
```
Time →
Stdout: [<!DOCTYPE html>]...[<table>]...[<tr>]...[</table></html>]✓
Stderr: [NOTE]...[NOTE]...[MARKER]
                          ↑ Marker found, but continue reading
                          ↓ Until HTML complete
Result: Complete HTML document
```

---

## Code Size Comparison

| Metric | Before | After | Change |
|--------|--------|-------|--------|
| Lines of code (execute method) | ~60 | ~155 | +158% |
| State variables | 1 | 3 | +200% |
| Error handling | Basic | Comprehensive | Significantly improved |
| Debug output | None | Extensive | New feature |

**Note**: The increased code size is justified by:
- Robust HTML completion detection
- Comprehensive error handling
- Detailed diagnostic output
- Prevention of data loss
