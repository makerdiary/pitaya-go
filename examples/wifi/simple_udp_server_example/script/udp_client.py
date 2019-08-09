import socket, sys
from sys import argv
import time

# Application settings
if len(argv)!= 3:
    print 'Unknown parameters'
    print 'Usage python ./udp_client.py <server_ip> <port>'
    sys.exit()

ip = argv[1]
port = int(argv[2])

print 'UDP client settings:\nServer IP:   %s\nPort: %d' % (ip, port)
print 'Please make sure above values are pointing to your board.\n'

# Open UDP client socket.
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

try:
    # Send UDP packets.
    for i in range(0,10):
        sock.sendto(str(i), (ip, port))
        print 'Sent', i, 'to Pitaya Go'
        time.sleep(1)
finally:
    # Close socket.
    sock.close()
