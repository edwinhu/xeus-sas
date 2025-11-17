#!/usr/bin/env python3
"""Check what HTML xeus-sas is actually returning"""

import jupyter_client
import time

km = jupyter_client.KernelManager(kernel_name='xeus-sas')
km.start_kernel()
kc = km.client()
kc.start_channels()

time.sleep(3)

code = "PROC PRINT DATA=sashelp.class; RUN;"

print("Executing:", code)
msg_id = kc.execute(code)

html_received = None
outputs = []

while True:
    try:
        msg = kc.get_iopub_msg(timeout=15)
        msg_type = msg['header']['msg_type']
        content = msg['content']

        if msg_type == 'display_data':
            if 'data' in content and 'text/html' in content['data']:
                html_received = content['data']['text/html']
                print(f"\n✓ Received HTML: {len(html_received)} bytes")
                print(f"  Starts with: {html_received[:50]}...")
                print(f"  Ends with: ...{html_received[-50:]}")

                # Count table rows
                row_count = html_received.count('</tr>')
                print(f"  Table rows: {row_count}")

                # Check if complete
                if html_received.strip().endswith('</html>'):
                    print("  ✓ Complete HTML (ends with </html>)")
                else:
                    print("  ✗ INCOMPLETE HTML!")
                    print(f"    Last 100 chars: {html_received[-100:]}")

        elif msg_type == 'status' and content['execution_state'] == 'idle':
            break

    except Exception as e:
        print(f"Error: {e}")
        break

kc.stop_channels()
km.shutdown_kernel()

if html_received:
    # Write to file for inspection
    with open('/tmp/xeus_sas_captured.html', 'w') as f:
        f.write(html_received)
    print(f"\n✓ HTML written to /tmp/xeus_sas_captured.html")
    print(f"Compare with: diff /tmp/sas_html_test.html /tmp/xeus_sas_captured.html")
else:
    print("\n✗ No HTML received!")
