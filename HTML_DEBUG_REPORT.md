# HTML Table Rendering Issues - Debug Report

## Problem Description

User reported HTML table rendering issues when displaying PROC PRINT output:
1. **Header Misalignment**: Table headers (Obs, Name, Sex, Age, Height, Weight) not aligned with columns
2. **Black Boxes at Row 18**: Numerical data in row 18 displays as black boxes

## HTML Structure Analysis

### Reference File: `/tmp/sas_html_test.html`
- **Size**: 35,205 bytes
- **Lines**: 1,286
- **Encoding**: ASCII text
- **Format**: Valid HTML5 document

### HTML Structure Verification

✅ **DOCTYPE**: `<!DOCTYPE html>` present
✅ **HTML Tags**: Properly opened and closed
✅ **CSS**: Complete HTMLBlue style (1107 lines, lines 8-1107)
✅ **Table Structure**:
   - `<table class="table">` with proper attributes
   - `<colgroup>`: TWO colgroups (1 col for Obs, 5 cols for data)
   - `<thead>`: 6 header columns
   - `<tbody>`: 19 data rows (complete)
✅ **Row 18 Data**: Complete and valid
   ```html
   <tr>
   <th class="r rowheader" scope="row">18</th>
   <td class="data">Thomas</td>
   <td class="data">M</td>
   <td class="r data">11</td>
   <td class="r data">57.5</td>
   <td class="r data">85.0</td>
   </tr>
   ```

### CSS Classes Used

- `.table`: Main table styling
- `.header`: Header cell styling (blue background #edf2f9)
- `.rowheader`: Row number styling
- `.data`: Data cell styling (white background #ffffff)
- `.r`: Right-alignment class

## Potential Root Causes

### 1. HTML Transmission Issues

**Symptoms**: If HTML is truncated, modified, or escaped during transmission from sas_session → xinterpreter → Jupyter → Euporie

**Debug Steps Added**:
- `sas_session.cpp` now logs:
  - Raw HTML output length and boundaries
  - HTML extraction positions
  - First/last 400 chars of raw and extracted HTML
  - Writes extracted HTML to `/tmp/xeus_sas_extracted_html_debug.html`

- `xinterpreter.cpp` now logs:
  - HTML length before sending
  - First/last 300 chars
  - Document structure checks (DOCTYPE, html tags, table tags)
  - Row count verification
  - JSON encoding verification

**Test Command**:
```bash
# Rebuild
cd /home/eh2889/projects/xeus-sas
cmake --build build

# Run with debug logging (stderr will show all debug info)
jupyter console --kernel=xsas 2>&1 | tee /tmp/xeus_sas_debug.log
```

### 2. MIME Type Issues

**Symptoms**: HTML rendered as plain text instead of rich HTML

**Check**:
```cpp
// In xinterpreter.cpp line 111
html_data["text/html"] = result.html_output;  // MUST be "text/html" not "text/plain"
```

**Verify**: Debug logs will show `JSON has text/html key: 1`

### 3. JSON Escaping Issues

**Symptoms**: HTML might be double-escaped in JSON (e.g., `&lt;` instead of `<`)

**Check**: Debug logs will compare original HTML vs JSON-encoded HTML

### 4. CSS Loading/Parsing Issues in Euporie

**Symptoms**: Euporie might not support all CSS features used by SAS HTMLBlue style

**Test with minimal HTML**:
```bash
# Created minimal test file
cat /tmp/test_minimal_table.html
```

**Potential Issues**:
- Complex CSS selectors might not work
- Font families might not be available
- Border styling might not render correctly

### 5. Terminal Width/Size Constraints

**Symptoms**: If terminal is too narrow, HTML rendering might wrap or truncate

**Check**: Ensure terminal is wide enough (at least 100 columns)

### 6. Character Encoding in Euporie

**Symptoms**: If Euporie expects UTF-8 but receives ASCII with special chars

**Check**: HTML is pure ASCII (verified), so this is unlikely

## Debug Workflow

### Step 1: Capture Debug Logs
```bash
cd /home/eh2889/projects/xeus-sas
cmake --build build
jupyter console --kernel=xsas 2>&1 | tee /tmp/xeus_sas_debug.log
```

Then run:
```python
proc print data=sashelp.class;
run;
```

### Step 2: Analyze Debug Output

Look for:
```
=== SAS_SESSION: RAW HTML OUTPUT DEBUG ===
Raw HTML output length: [should be ~35000]
First 400 chars: [should start with <!DOCTYPE html>]
Last 400 chars: [should end with </html>]
```

```
=== XINTERPRETER: HTML DEBUG INFO ===
HTML Length: [should match sas_session length]
Starts with <!DOCTYPE: 1
Ends with </html>: 1
Number of </tr> tags: [should be 20 - 1 header + 19 data rows]
```

### Step 3: Compare Files

```bash
# Compare extracted HTML with reference
diff /tmp/sas_html_test.html /tmp/xeus_sas_extracted_html_debug.html
```

If they differ, the issue is in **HTML capture/extraction**.
If they match, the issue is in **Jupyter/Euporie rendering**.

### Step 4: Test with Simplified HTML

If full HTML works but rendered HTML fails, test with:
```bash
# Open minimal test in browser
open /tmp/test_minimal_table.html

# Or use Python to display
python3 << 'EOF'
from IPython.display import HTML, display
with open('/tmp/test_minimal_table.html') as f:
    display(HTML(f.read()))
EOF
```

## Known Euporie Limitations

Based on terminal HTML rendering limitations:

1. **Complex CSS**: Some CSS3 features might not render
2. **Font families**: Limited font support in terminals
3. **Colors**: Terminal color palette limitations
4. **Table borders**: Border-collapse might not work perfectly
5. **Fixed positioning**: Absolute/fixed positioning not supported

## Recommended Fixes

### If Issue is HTML Extraction:
```cpp
// Ensure complete HTML capture in sas_session.cpp
// Verify html_end calculation includes </html>
html_end += 7;  // Include "</html>" - ALREADY DONE
```

### If Issue is JSON Encoding:
```cpp
// Verify no double-escaping
html_data["text/html"] = result.html_output;  // Direct assignment, no escaping
```

### If Issue is CSS Complexity:
Try simpler ODS style:
```cpp
// In sas_session.cpp line 224
ods html5 (id=xeus_sas_internal) file=stdout style=Printer;  // Simpler style
// OR
ods html (id=xeus_sas_internal) file=stdout;  // Basic HTML
```

### If Issue is Terminal Rendering:
Use HTML output destination instead:
```cpp
// Generate separate HTML file
ods html5 file="/tmp/sas_output.html" style=HTMLBlue;
// Then embed iframe or link to file
```

## Next Steps

1. **Rebuild with debug logging**: ✅ DONE
2. **Run test case and capture logs**: ⏳ NEEDS USER
3. **Analyze debug output**: ⏳ DEPENDS ON LOGS
4. **Compare extracted vs reference HTML**: ⏳ DEPENDS ON LOGS
5. **Identify specific failure point**: ⏳ DEPENDS ON ANALYSIS
6. **Implement targeted fix**: ⏳ DEPENDS ON ROOT CAUSE

## Test Case

```sas
PROC PRINT DATA=sashelp.class;
RUN;
```

**Expected Output**:
- 19 rows of data
- 6 columns (Obs, Name, Sex, Age, Height, Weight)
- Properly aligned headers
- All numerical data visible (no black boxes)
- HTMLBlue styling applied

**Current Issue**:
- Headers misaligned
- Row 18 shows black boxes in numerical columns

## Files Modified

1. `/home/eh2889/projects/xeus-sas/src/sas_session.cpp`
   - Added comprehensive debug logging at lines 358-427
   - Writes extracted HTML to `/tmp/xeus_sas_extracted_html_debug.html`

2. `/home/eh2889/projects/xeus-sas/src/xinterpreter.cpp`
   - Added comprehensive debug logging at lines 77-130
   - Logs HTML structure and JSON encoding

## Debug Log Locations

- **sas_session logs**: stderr (console)
- **xinterpreter logs**: stderr (console)
- **Extracted HTML**: `/tmp/xeus_sas_extracted_html_debug.html`
- **Reference HTML**: `/tmp/sas_html_test.html`
- **Test minimal HTML**: `/tmp/test_minimal_table.html`
