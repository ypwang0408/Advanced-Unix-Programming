from pwn import *
import sys
import time
flags = {}
def send_file(number):
    print('Send file ' + str(number) + '.asm')
    p = remote('up.zoolab.org', number)
    file = open(str(number) + '.asm', 'r')
    codes = file.read()
    for code in codes.split('\n'):
        code = bytes(code, 'utf-8')
        p.sendline(code)
    p.sendline(b'done:')
    r = p.recvall().decode()
    print(r)
    p.close()
    if 'FLAG' in r:
        flag = r.split('FLAG: ')[1].split('\n')[0]
        flags[number] = flag

try:
    number = int(sys.argv[1])
except:
    number = -1

if number != -1:
    if number < 100:
        number += 2500
    send_file(number)
else:
    for i in range(0, 23):
        print('Send file ' + str(i + 2500) + '.asm')
        send_file(i + 2500)
        time.sleep(1)
    for i in range(0, 23):
        if i + 2500 in flags:
            print('\x1b[6;30;42m' + 'FLAG ' + str(i + 2500) + ': ' + flags[i + 2500] + '\x1b[0m')
        else:
            print('\x1b[6;30;41m' + 'FLAG ' + str(i + 2500) + ': Not found' + '\x1b[0m')
    
