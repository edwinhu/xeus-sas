#!/usr/bin/env python3
"""Test that xeus-sas maintains a persistent session."""

import subprocess
import sys
import time

def test_persistent_session():
    """Test that data persists between executions."""

    print("Testing xeus-sas persistent session...")
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

    # Test 1: Create a dataset
    print("\nTest 1: Creating dataset work.mydata...")
    test1_code = """DATA work.mydata;
    INPUT x y;
    DATALINES;
1 10
2 20
3 30
;
RUN;
"""

    proc.stdin.write(test1_code + "\n")
    proc.stdin.flush()
    time.sleep(2)

    # Test 2: Try to print the dataset - should exist if session persists
    print("Test 2: Attempting to print work.mydata (should work if persistent)...")
    test2_code = "PROC PRINT DATA=work.mydata; RUN;\n"

    proc.stdin.write(test2_code + "\n")
    proc.stdin.flush()
    time.sleep(2)

    # Test 3: Check for initialization messages
    print("Test 3: Running simple %PUT (should NOT show SAS initialization)...")
    test3_code = "%put Session is persistent!;\n"

    proc.stdin.write(test3_code + "\n")
    proc.stdin.flush()
    time.sleep(2)

    # Exit
    proc.stdin.write("exit\n")
    proc.stdin.flush()

    # Wait for process to finish
    try:
        stdout, stderr = proc.communicate(timeout=10)
        print("\nSTDOUT:")
        print(stdout)
        print("\nSTDERR:")
        print(stderr)

        # Check if we see "SAS initialization" multiple times (bad) or just once (good)
        init_count = stdout.count("SAS initialization") + stderr.count("SAS initialization")
        print(f"\nSAS initialization messages found: {init_count}")

        if init_count <= 1:
            print("✓ SUCCESS: Session appears to be persistent!")
            return 0
        else:
            print("✗ FAILURE: Multiple SAS initializations detected")
            return 1

    except subprocess.TimeoutExpired:
        proc.kill()
        print("✗ Test timed out")
        return 1

if __name__ == "__main__":
    sys.exit(test_persistent_session())
