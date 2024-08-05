import socket
import random
import sys
import signal
import time

RECEIVER_ADDR = ('localhost', 8080)

# Handler function to raise exception when timeout occurs
def handle_timeout(signum, frame):
    raise TimeoutError("Process timed out and will be terminated.")

# Set the signal handler for SIGALRM
signal.signal(signal.SIGALRM, handle_timeout)



# Creates a packet from a sequence number and byte data
def pacmake(seq_num, data = b''):
    seq_bytes = seq_num.to_bytes(4, byteorder = 'little', signed = True)
    return seq_bytes + data

# Creates an empty packet
def pacmake_empty():
    return b''

# Extracts sequence number and data from a non-empty packet
def pacextract(packet):
    seq_num = int.from_bytes(packet[0:4], byteorder = 'little', signed = True)
    return seq_num, packet[4:]

# Packet may be lost
def udt_send(packet, sock, addr):
    if random.randint(0, 8) > 0:
        sock.sendto(packet, addr)
    return

# Receive a packet from the unreliable channel
def udt_recv(sock):
    packet, addr = sock.recvfrom(25620)
    return packet, addr

# Receive packets from the sender
def receive(sock, filename):
    # Open the file for writing
    try:
        file = open(filename, 'wb')
    except IOError:
        # print('Unable to open', filename)
        return
    # Define a timeout duration in seconds
    signal.alarm(40)
    st=0
    en=0
    try:
        
        expected_num = 0
        cl=0
        while True:
            # Get the next packet from the sender
            pkt, addr = udt_recv(sock)
            if (cl==0):
                st=time.monotonic()
                cl=1
            if not pkt:
                break
            seq_num, data = pacextract(pkt)
            # print('Got packet', seq_num)
            
            # Send back an ACK
            if seq_num == expected_num:
                # print('Got expected packet')
                # print('Sending ACK', expected_num)
                pkt = pacmake(expected_num)
                udt_send(pkt, sock, addr)
                expected_num += 1
                file.write(data)
            else:
                # print('Sending ACK', expected_num - 1)
                pkt = pacmake(expected_num - 1)
                udt_send(pkt, sock, addr)
            en=time.monotonic()
    except TimeoutError as e:
        x=2
   
    print(en-st)
    file.close()

# Main function
if __name__ == '__main__':
    if len(sys.argv) != 2:
        # print('Expected filename as command line argument')
        exit()
    
    filename = sys.argv[1]
        
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(RECEIVER_ADDR) 
    
    receive(sock, filename)
    
    sock.close()
