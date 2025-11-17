#!/usr/bin/env python3
"""Test that xeus-sas properly cleans output (no markers, no input echo)."""

import subprocess
import sys
import time
import re

def test_output_cleaning():
    """Test that output is clean - no markers, no input echo."""

    print("Testing xeus-sas output cleaning...")
    print("=" * 60)

    # Start jupyter console with xeus-sas kernel
    proc = subprocess.Popen(
        ['jupyter', 'console', '--kernel=xeus-sas'],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    time.sleep(3)  # Wait for kernel to start

    # Test: Simple %PUT should output only the message
    print("\nTest: Running %put hello world;")
    print("Expected output: 'hello world' (clean)")
    print("Unwanted: 'XEUS_SAS_END_*' or numbered input echo")

    test_code = "%put hello world;\n"

    proc.stdin.write(test_code)
    proc.stdin.flush()
    time.sleep(2)

    # Exit
    proc.stdin.write("exit\n")
    proc.stdin.flush()

    # Wait for process to finish
    try:
        stdout, stderr = proc.communicate(timeout=10)
        print("\n" + "=" * 60)
        print("STDOUT:")
        print(stdout)
        print("\n" + "=" * 60)
        print("STDERR:")
        print(stderr)
        print("=" * 60)

        # Check for markers (should NOT be present)
        if "XEUS_SAS_END" in stdout or "XEUS_SAS_END" in stderr:
            print("\n✗ FAILURE: Execution marker found in output!")
            return 1

        # Check for input echo with line numbers (should NOT be present)
        # Pattern: number followed by spaces and the input
        if re.search(r'\d+\s+%put hello world;', stdout) or re.search(r'\d+\s+%put hello world;', stderr):
            print("\n✗ FAILURE: Input echo (numbered line) found in output!")
            return 1

        # Check that we DO see the expected output
        if "hello world" in stdout or "hello world" in stderr:
            print("\n✓ SUCCESS: Output is clean!")
            print("  - No execution markers")
            print("  - No input echo")
            print("  - Only actual SAS output visible")
            return 0
        else:
            print("\n✗ FAILURE: Expected output 'hello world' not found!")
            return 1

    except subprocess.TimeoutExpired:
        proc.kill()
        print("✗ Test timed out")
        return 1

if __name__ == "__main__":
    sys.exit(test_output_cleaning())
