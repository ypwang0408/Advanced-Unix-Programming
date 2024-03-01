from pwn import *

def get_ip_address():
    # Define the URL without the protocol
    url = "ipinfo.io"

    # Use Pwntools to connect to the URL and retrieve the IP address
    with remote(url, 80) as conn:
        conn.send("GET /ip HTTP/1.1\r\n")
        conn.send("Host: {}\r\n".format(url))
        conn.send("User-Agent: curl/7.88.1\r\n")
        conn.send("Accept: */*\r\n\r\n")
        response = conn.recvall()
        headers, body = response.split(b'\r\n\r\n', 1)
        ip_address = body.strip().decode()

    return ip_address

if __name__ == '__main__':
    ip_address = get_ip_address()
    print("IP Address: {}".format(ip_address))
