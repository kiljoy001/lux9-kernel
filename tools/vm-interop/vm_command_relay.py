#!/usr/bin/env python3
"""
VM Command Relay Server
Accepts commands from user and relays them to the VM
The VM connects OUT to this server (works through NAT)
"""

import socket
import threading
import queue
import json
import sys
from datetime import datetime

# Configuration
RELAY_PORT = 8888  # Port for VM to connect to
command_queue = queue.Queue()
vm_connections = []
results = []

def handle_vm_connection(client_sock, client_addr):
    """Handle connection from VM"""
    print(f"✅ VM connected from {client_addr}")
    vm_connections.append(client_sock)

    try:
        while True:
            # Check for queued commands
            try:
                cmd = command_queue.get(timeout=1)
                msg = json.dumps({"type": "command", "cmd": cmd}) + "\n"
                client_sock.send(msg.encode())
                print(f"📤 Sent to VM: {cmd}")

                # Wait for result
                data = client_sock.recv(4096)
                if data:
                    result = data.decode().strip()
                    results.append({
                        "time": datetime.now().isoformat(),
                        "command": cmd,
                        "result": result
                    })
                    print(f"📥 VM result: {result[:100]}...")

            except queue.Empty:
                # Send heartbeat
                client_sock.send(b'{"type":"ping"}\n')

    except Exception as e:
        print(f"❌ VM disconnected: {e}")
    finally:
        if client_sock in vm_connections:
            vm_connections.remove(client_sock)
        client_sock.close()

def start_relay_server():
    """Start the relay server"""
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(('0.0.0.0', RELAY_PORT))
    server.listen(5)

    print(f"🚀 VM Command Relay Server")
    print(f"📍 Listening on port {RELAY_PORT}")
    print(f"⌨️  Commands:")
    print(f"   'cmd <command>' - Send command to VM")
    print(f"   'status' - Show connection status")
    print(f"   'results' - Show recent results")
    print(f"   'quit' - Exit")
    print()

    # Accept connections in background
    def accept_connections():
        while True:
            client_sock, client_addr = server.accept()
            thread = threading.Thread(target=handle_vm_connection,
                                    args=(client_sock, client_addr))
            thread.daemon = True
            thread.start()

    accept_thread = threading.Thread(target=accept_connections)
    accept_thread.daemon = True
    accept_thread.start()

    # Just keep running
    print("Server is running. VM can now connect.")
    print("To send commands, use: echo 'date' | nc localhost 8889")

    # Start a simple command input server on another port
    def command_input_server():
        cmd_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        cmd_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        cmd_sock.bind(('127.0.0.1', 8889))
        cmd_sock.listen(1)

        while True:
            conn, addr = cmd_sock.accept()
            data = conn.recv(1024).decode().strip()
            if data:
                if vm_connections:
                    command_queue.put(data)
                    conn.send(b"Command queued\n")
                else:
                    conn.send(b"No VM connected\n")
            conn.close()

    cmd_thread = threading.Thread(target=command_input_server)
    cmd_thread.daemon = True
    cmd_thread.start()

    # Keep main thread alive
    try:
        while True:
            threading.Event().wait(1)
    except KeyboardInterrupt:
        print("\n👋 Shutting down")
        sys.exit(0)

if __name__ == "__main__":
    start_relay_server()