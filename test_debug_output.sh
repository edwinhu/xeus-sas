#!/bin/bash
# Test to capture debug output from xeus-sas

echo "Testing xeus-sas with debug logging..."
echo "Output will be saved to /tmp/xeus_debug_test.log"
echo ""

{
    sleep 2
    echo "PROC PRINT DATA=sashelp.class(obs=5); RUN;"
    sleep 8
    echo "exit"
} | jupyter console --kernel=xeus-sas 2>&1 | tee /tmp/xeus_debug_test.log

echo ""
echo "=== Debug Analysis ==="
echo ""
echo "Checking for debug sections in stderr:"
grep -c "SAS_SESSION: RAW HTML OUTPUT DEBUG" /tmp/xeus_debug_test.log && echo "✓ Found sas_session debug output" || echo "✗ No sas_session debug"
grep -c "XINTERPRETER: HTML DEBUG INFO" /tmp/xeus_debug_test.log && echo "✓ Found xinterpreter debug output" || echo "✗ No xinterpreter debug"

echo ""
echo "HTML size comparison:"
if [ -f /tmp/xeus_sas_extracted_html_debug.html ]; then
    echo "Extracted HTML size: $(wc -c < /tmp/xeus_sas_extracted_html_debug.html) bytes"
    echo "Reference HTML size: $(wc -c < /tmp/sas_html_test.html) bytes"
    diff -q /tmp/sas_html_test.html /tmp/xeus_sas_extracted_html_debug.html && echo "✓ Files match!" || echo "✗ Files differ"
else
    echo "✗ Debug HTML file not created"
fi

echo ""
echo "Full log saved to: /tmp/xeus_debug_test.log"
echo "Run: grep 'DEBUG\|===\|Length\|Row count' /tmp/xeus_debug_test.log"
