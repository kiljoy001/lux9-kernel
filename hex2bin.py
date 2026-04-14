import sys; data=''.join(sys.stdin.read().split()); bytes=bytearray.fromhex(data); sys.stdout.buffer.write(bytes)
