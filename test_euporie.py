#!/usr/bin/env python3
"""Test xeus-sas kernel with a simple SAS program."""

import jupyter_client
import time

# Create a kernel manager for xeus-sas
km = jupyter_client.KernelManager(kernel_name='xeus-sas')
km.start_kernel()

# Create a client
kc = km.client()
kc.start_channels()

# Wait for kernel to be ready
time.sleep(2)

# Simple SAS test code
sas_code = """
DATA work.test;
    INPUT name $ age;
    DATALINES;
Alice 25
Bob 30
Charlie 35
;
RUN;

PROC PRINT DATA=work.test;
RUN;
"""

print("Executing SAS code...")
msg_id = kc.execute(sas_code)

# Collect outputs
outputs = []
while True:
    try:
        msg = kc.get_iopub_msg(timeout=10)
        msg_type = msg['header']['msg_type']
        content = msg['content']

        print(f"Message type: {msg_type}")

        if msg_type == 'stream':
            print(f"Stream ({content['name']}): {content['text']}")
            outputs.append(content['text'])
        elif msg_type == 'execute_result':
            print(f"Result: {content['data']}")
            outputs.append(str(content['data']))
        elif msg_type == 'error':
            print(f"Error: {content['ename']}: {content['evalue']}")
            outputs.append(f"Error: {content['ename']}: {content['evalue']}")
        elif msg_type == 'status' and content['execution_state'] == 'idle':
            break

    except Exception as e:
        print(f"Exception: {e}")
        break

print("\n" + "="*60)
print("OUTPUTS:")
print("="*60)
for output in outputs:
    print(output)

# Cleanup
kc.stop_channels()
km.shutdown_kernel()

print("\nTest completed!")
