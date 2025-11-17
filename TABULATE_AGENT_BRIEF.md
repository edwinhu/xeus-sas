# PROC TABULATE HTML Rendering Fix

## Problem
PROC TABULATE generates complex HTML tables with `rowspan` and `colspan` attributes that render incorrectly in euporie console (terminal-based Jupyter client). The table structure appears misaligned and floating.

## Goal
Transform complex PROC TABULATE HTML tables into simpler structures that euporie can render correctly, while preserving the data and layout.

## Test Files Available
- `/tmp/tabulate_listing.txt` - Plain text output (shows what structure SHOULD look like)
- `/tmp/tabulate_html.html` - HTML output (current broken rendering)
- `/tmp/tabulate_complex_listing.txt` - Complex table plain text
- `/tmp/tabulate_complex_html.html` - Complex table HTML
- `test_tabulate_outputs.sas` - Code to regenerate test files

## Current Approach
The kernel already does HTML post-processing in `src/sas_session.cpp` around line 435-605:
- Merges colgroups
- Strips inline styles
- Converts thead to tbody
- Removes empty captions

## What Needs to be Done
1. Detect tables with `rowspan` and `colspan` attributes
2. Transform the complex table structure into a simpler one by:
   - Flattening multi-level headers into regular rows
   - Removing rowspan/colspan and duplicating cells as needed
   - Preserving the visual layout that the plain text output shows

3. OR alternatively: Convert PROC TABULATE tables to plain text representation

## Reference Structure
Plain text shows the correct layout:
```
|                               |   Height   |
|                               |------------|
|                               |    Sum     |
|-------------------------------+------------|
|Sex                            |            |
|-------------------------------|            |
|F                              |      545.30|
|-------------------------------+------------|
|M                              |      639.10|
```

HTML has complex structure with rowspan:
```html
<th rowspan="2">...</th>
<th>Height</th>
<th>Sum</th>
<td rowspan="2">545.30</td>
```

## Success Criteria
PROC TABULATE tables should render in euporie with:
- All headers visible and aligned
- All data cells visible and aligned
- No floating or misaligned elements
- Structure matching the plain text layout

## Code Location
Add HTML transformation logic in `src/sas_session.cpp` after line 555 (after thead conversion, before caption removal).

## Constraints
- Must preserve all data (no information loss)
- Should work for arbitrary PROC TABULATE complexity
- Plain text fallback is acceptable if HTML transformation is too complex
