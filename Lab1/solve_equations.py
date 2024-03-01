#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import base64
import hashlib
import time
import sys
from pwn import *
def trans(ma):
    if ma[2][2] == '╳':
        return "*"
    elif ma[2][2] == '┼':
        return "+"
    elif ma[1][2] == '•' and ma[1][2] == '•':
        return "/"
    else:
        tmp = ma[0][0] + ma[0][2] + ma[0][4] + ma[2][0] + ma[2][2] + ma[2][4] + ma[4][0] + ma[4][2] + ma[4][4]
        print(tmp)
        if tmp == '┌──└─┐└─┘':
            return "5"
        elif tmp == '│ │└─┤  │':
            return "4"
        elif tmp == '┌─┐┌─┘└─┘':
            return "2"
        elif tmp == '┌─┐ ─┤└─┘':
            return "3"
        elif tmp == '┌─┐└─┤└─┘':
            return "9"
        elif tmp == '┌─┐├─┤└─┘':
            return "8"
        elif tmp == '┌─┐├─┐└─┘':
            return "6"
        elif tmp == ' ┐  │  ┴ ':
            return "1"
        elif tmp == '┌─┐  │  │':
            return "7"
        elif tmp == '┌─┐│ │└─┘':
            return "0"
            
    return "?"
    

def solve_pow(r):
    prefix = r.recvline().decode().split("'")[1];
    print(time.time(), "solving pow ...");
    solved = b''
    for i in range(1000000000):
        h = hashlib.sha1((prefix + str(i)).encode()).hexdigest();
        if h[:6] == '000000':
            solved = str(i).encode();
            print("solved =", solved);
            break;
    print(time.time(), "done.");
    r.sendlineafter(b'string S: ', base64.b64encode(solved));

if __name__ == "__main__":
    r = None
    if len(sys.argv) == 2:
        r = remote('localhost', int(sys.argv[1]))
    elif len(sys.argv) == 3:
        r = remote(sys.argv[2], int(sys.argv[1]))
    else:
        r = process('./pow.py')
    solve_pow(r)
    print(r.recvline().strip().decode())
    print(r.recvline().strip().decode())
    print(r.recvline().strip().decode())
    tmp = r.recvline().strip().decode()
    print(tmp)
    num_line = int(tmp.split(' ')[3])
    ##print("num_line =", num_line)
    for i in range(num_line):
        rec = r.recv().decode()
        print(rec) 
        print('-----------------')
        q = base64.b64decode(rec.split(' ')[2]).decode()
        print(q)
        print(len(q))
        a = [[' ' for x in range(5)] for y in range(5)] 
        b = [[' ' for x in range(5)] for y in range(5)]
        c = [[' ' for x in range(5)] for y in range(5)]
        d = [[' ' for x in range(5)] for y in range(5)]
        e = [[' ' for x in range(5)] for y in range(5)]
        f = [[' ' for x in range(5)] for y in range(5)]
        g = [[' ' for x in range(5)] for y in range(5)]
        
        
        
        cnt = 0
        l = 0
        eq = ''
        for i in range(len(q)):
            if q[i] == '\n' or q[i] == '\r':
                cnt = 0
                l += 1
            elif cnt > 0 and cnt < 6 :
                a[l][cnt-1] = q[i]
                cnt += 1
            elif cnt > 7 and cnt < 13:
                b[l][cnt-8] = q[i]
                cnt += 1
            elif cnt > 14 and cnt < 20:
                c[l][cnt-15] = q[i]
                cnt += 1
            elif cnt > 21 and cnt < 27:
                d[l][cnt-22] = q[i]
                cnt += 1
            elif cnt > 28 and cnt < 34:
                e[l][cnt-29] = q[i]
                cnt += 1
            elif cnt > 35 and cnt < 41:
                f[l][cnt-36] = q[i]
                cnt += 1
            elif cnt > 42 and cnt < 48:
                g[l][cnt-43] = q[i]
                cnt += 1
            else:
                cnt += 1
        for i in range(5):
            print(a[i])
        eq += trans(a)
        for i in range(5):
            print(b[i])
        eq += trans(b)
        for i in range(5):
            print(c[i])
        eq += trans(c)
        for i in range(5):
            print(d[i])
        eq += trans(d)
        for i in range(5):
            print(e[i])
        eq += trans(e)
        for i in range(5):
            print(f[i])
        eq += trans(f)
        for i in range(5):
            print(g[i])
        eq += trans(g)
        
        print(eq)
        res = str(int(eval(eq)))
        print(res)
        
        #res = eval(rec.split(' ')[2])
        #print(res)
        print('-----------------')
        r.sendline(res)
        
         
    
        
    r.interactive()
    r.close()

# vim: set tabstop=4 expandtab shiftwidth=4 softtabstop=4 number cindent fileencoding=utf-8 :

