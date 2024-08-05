# import packet
import socket
import random
import signal
import time
import sys
# import udt

RECEIVER_ADDR = ('localhost', 8071)

# Time details
begin_time=0
end_time=0

# Handler function to raise exception when timeout occurs
def handle_timeout(signum, frame):
    raise TimeoutError("Process timed out and will be terminated.")

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

# Set the signal handler for SIGALRM
signal.signal(signal.SIGALRM, handle_timeout)

def receive_sr(sock, filename):
    global begin_time
    global end_time

    # Open the file for writing
    try:
        file = open(filename, 'wb')
    except IOError:
        # print('Unable to open', filename)
        return
    
    expected_num = 0
    received_seq_nums=[]
    received_packets={}
    begin_time=time.time()
    signal.alarm(40)
    try:
        while True:
            # Get the next packet from the sender
            pkt, addr = udt_recv(sock)
            if not pkt:
                break
            seq_num, data = pacextract(pkt)
            # print('Got packet', seq_num)
            # print('Expected packet', expected_num)
            received_seq_nums.append(seq_num)
            received_packets[seq_num]=data
            
            # Send back an ACK
            if seq_num == expected_num:
                # print('Got expected packet')
                while expected_num in received_seq_nums:
                    file.write(received_packets[expected_num])
                    expected_num += 1
                # print('Sending ACK', expected_num)
                pkt = pacmake(expected_num)
                udt_send(pkt, sock, addr)
            else:
                # print('Sending ACK', expected_num)
                pkt = pacmake(expected_num)
                udt_send(pkt, sock, addr)
            end_time=time.time()
    except TimeoutError as e:
        x=2
    
    file.close()

# Main function
if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('Arguments missing')
        exit()
    
    filename = sys.argv[1]

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(RECEIVER_ADDR) 

    receive_sr(sock, filename)
    sock.close()
    print(end_time-begin_time)
