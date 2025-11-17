#!/bin/bash
# Test to capture what euporie receives

echo "Starting euporie console and capturing output..."

# Send test code and capture output
{
    sleep 3
    echo "PROC PRINT DATA=sashelp.class; RUN;"
    sleep 5
    echo "exit"
} | euporie console --kernel=xeus-sas 2>&1 | tee /tmp/euporie_output.log

echo ""
echo "Output saved to /tmp/euporie_output.log"
echo "Checking for HTML..."
grep -c "<!DOCTYPE html>" /tmp/euporie_output.log || echo "No HTML doctype found"
grep -c "</html>" /tmp/euporie_output.log || echo "No HTML end tag found"

# Try to extract just the HTML
sed -n '/<!DOCTYPE html>/,/<\/html>/p' /tmp/euporie_output.log > /tmp/euporie_extracted.html
echo "Extracted HTML size: $(wc -c < /tmp/euporie_extracted.html) bytes"
echo "Expected HTML size: $(wc -c < /tmp/sas_html_test.html) bytes"
