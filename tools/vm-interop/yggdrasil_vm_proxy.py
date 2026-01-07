#!/usr/bin/env python3
"""
Yggdrasil to VM Proxy
Forwards connections from Yggdrasil mesh to 9front VM services

This allows blockchain nodes on the Yggdrasil mesh to reach
the cmdexec JSON API running inside the VM.
"""

import socket
import threading
import sys
import json

# Configuration
YGGDRASIL_LISTEN_PORT = 9999  # Port to listen on Yggdrasil interface
VM_HOST = "10.0.2.2"           # VM sees host at this address (from inside VM)
VM_PORT = 9999                 # cmdexec blockchain API port in VM

# Note: Since we can't reach 10.0.2.15 from host, we need the VM
# to connect OUT to us, or restart QEMU with port forwarding

def handle_client(client_sock, client_addr):
    """Forward connection from Yggdrasil client to VM"""
    print(f"🌐 Yggdrasil connection from {client_addr}")

    try:
        # For now, since VM is behind NAT, we'd need either:
        # 1. VM to establish reverse connection
        # 2. Restart QEMU with -netdev user,hostfwd=tcp::9999-:9999
        # 3. Use tap/bridge networking

        # Placeholder for when we have proper connectivity:
        # vm_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # vm_sock.connect((VM_HOST, VM_PORT))

        # For testing, just echo back a response
        data = client_sock.recv(4096)
        if data:
            try:
                request = json.loads(data.decode())
                print(f"📦 Blockchain event: {request}")

                # Mock response until VM connectivity is fixed
                response = {
                    "status": "pending",
                    "message": "VM connectivity being established",
                    "request": request
                }

                client_sock.send(json.dumps(response).encode() + b'\n')
            except json.JSONDecodeError:
                client_sock.send(b'{"error": "Invalid JSON"}\n')

    except Exception as e:
        print(f"❌ Proxy error: {e}")
    finally:
        client_sock.close()

def main():
    # Get host's Yggdrasil IPv6 address
    ygg_addr = "200:97d7:cf23:7439:21d3:4139:5a0e:9e18"

    print(f"🚀 Yggdrasil-to-VM Proxy Starting")
    print(f"📍 Listening on [{ygg_addr}]:{YGGDRASIL_LISTEN_PORT}")
    print(f"🎯 Will forward to VM at {VM_HOST}:{VM_PORT}")
    print()
    print("⚠️  Note: VM connectivity requires either:")
    print("   1. Restart QEMU with: -netdev user,hostfwd=tcp::9999-:9999")
    print("   2. Set up tap/bridge networking")
    print("   3. Have VM establish reverse connection")
    print()

    # Create IPv6 socket for Yggdrasil
    server = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    # Bind to Yggdrasil address
    server.bind((ygg_addr, YGGDRASIL_LISTEN_PORT))
    server.listen(5)

    print(f"✅ Proxy listening on Yggdrasil mesh!")
    print(f"📝 Blockchain nodes can connect to: [{ygg_addr}]:{YGGDRASIL_LISTEN_PORT}")

    while True:
        client_sock, client_addr = server.accept()
        thread = threading.Thread(target=handle_client, args=(client_sock, client_addr))
        thread.daemon = True
        thread.start()

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n👋 Proxy shutting down")
    except Exception as e:
        print(f"❌ Fatal error: {e}")
        sys.exit(1)