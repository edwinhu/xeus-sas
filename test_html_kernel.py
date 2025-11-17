#!/usr/bin/env python3
"""Test script to verify xeus-sas HTML output."""

import jupyter_client

# Create a kernel manager and client
km = jupyter_client.KernelManager(kernel_name='xeus-sas')
km.start_kernel()
kc = km.client()
kc.start_channels()
kc.wait_for_ready()

# Send test code
code = """PROC PRINT DATA=sashelp.class;
RUN;
"""

print("Sending code to kernel:")
print(code)
print("\n" + "=" * 60 + "\n")

msg_id = kc.execute(code)

# Collect responses
outputs = []
while True:
    try:
        msg = kc.get_iopub_msg(timeout=10)
        msg_type = msg['msg_type']
        content = msg['content']

        print(f"Message type: {msg_type}")

        if msg_type == 'status' and content['execution_state'] == 'idle':
            break
        elif msg_type == 'stream':
            print(f"Stream ({content['name']}): {content['text'][:500]}...")
            outputs.append(('stream', content))
        elif msg_type == 'display_data':
            print(f"Display data MIME types: {list(content['data'].keys())}")
            if 'text/html' in content['data']:
                html = content['data']['text/html']
                print(f"HTML output length: {len(html)} chars")
                print(f"HTML preview: {html[:500]}...")
            outputs.append(('display_data', content))
        elif msg_type == 'execute_result':
            print(f"Execute result: {content}")
            outputs.append(('execute_result', content))
        elif msg_type == 'error':
            print(f"Error: {content}")
            outputs.append(('error', content))

    except Exception as e:
        print(f"Exception: {e}")
        break

print("\n" + "=" * 60 + "\n")
print(f"Total outputs collected: {len(outputs)}")

# Check for HTML output
html_found = False
for output_type, content in outputs:
    if output_type == 'display_data' and 'text/html' in content['data']:
        html_found = True
        print("\n✅ SUCCESS: HTML output detected!")
        print(f"HTML length: {len(content['data']['text/html'])} characters")
        break

if not html_found:
    print("\n❌ FAILED: No HTML output found")
    print("Output types received:", [o[0] for o in outputs])

# Cleanup
kc.stop_channels()
km.shutdown_kernel()
