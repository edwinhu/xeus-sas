#!/bin/bash
# Test output cleaning with euporie

echo "Testing output cleaning with euporie console..."
echo "=============================================="
echo ""

# Create a test SAS file
cat > /tmp/test_output.sas << 'EOF'
%put hello world;
EOF

# Run euporie with the test
timeout 10s euporie console --kernel-name=xeus-sas --execute-file=/tmp/test_output.sas --exit-on-complete 2>&1 | tee /tmp/euporie_output.txt

echo ""
echo "=============================================="
echo "Checking output for issues..."
echo ""

# Check for execution markers
if grep -q "XEUS_SAS_END" /tmp/euporie_output.txt; then
    echo "✗ FAILURE: Execution marker found in output!"
    exit 1
fi

# Check for input echo (numbered lines)
if grep -E "^[0-9]+\s+%put" /tmp/euporie_output.txt; then
    echo "✗ FAILURE: Input echo (numbered line) found in output!"
    exit 1
fi

# Check that we see the expected output
if grep -q "hello world" /tmp/euporie_output.txt; then
    echo "✓ SUCCESS: Output is clean!"
    echo "  - No execution markers"
    echo "  - No input echo"
    echo "  - Only actual SAS output visible"
    exit 0
else
    echo "✗ FAILURE: Expected output 'hello world' not found!"
    exit 1
fi
