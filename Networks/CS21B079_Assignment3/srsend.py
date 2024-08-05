import socket
import random
import sys
import _thread 
import time

class Timer(object):
    TIMER_STOP = -1

    def __init__(self, duration):
        self._start_time = self.TIMER_STOP
        self._duration = duration

    # Starts the timer
    def start(self):
        if self._start_time == self.TIMER_STOP:
            self._start_time = time.time()

    # Stops the timer
    def stop(self):
        if self._start_time != self.TIMER_STOP:
            self._start_time = self.TIMER_STOP

    # Determines whether the timer is runnning
    def running(self):
        return self._start_time != self.TIMER_STOP

    # Determines whether the timer timed out
    def timeout(self):
        if not self.running():
            return False
        else:
            return time.time() - self._start_time >= self._duration

MAX_PACKET_SIZE = 25600
RTO = 0.5
WINDOW_SIZE = 4

# ======Update when necessary=====
# Mention IP address and port number
RECEIVER_ADDR = ('localhost', 8071)
SENDER_ADDR = ('localhost', 0)

# Shared resources across threads
base = 0
mutex = _thread.allocate_lock()
send_timer = Timer(RTO)

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

def set_window_size(num_packets):
    global base
    return min(WINDOW_SIZE, num_packets - base)

# Send thread for selective repeat
def send_sr(sock,filename):
    global mutex
    global base
    global send_timer

    # Open the file
    try:
        file = open(filename, 'rb')
    except IOError:
        #print('Unable to open', filename)
        return
    
    # Add all the packets to the buffer
    packets = []
    seq_num = 0
    while True:
        data = file.read(MAX_PACKET_SIZE)
        if not data:
            break
        packets.append(pacmake(seq_num, data))
        seq_num += 1

    num_packets = len(packets)
    #print('I gots', num_packets)
    window_size = set_window_size(num_packets)
    base = 0
    next_to_send = 0

    # Start the receiver thread
    _thread.start_new_thread(receive_sr, (sock,))

    while base < num_packets:
        mutex.acquire()
        # Send all the packets in the window
        if next_to_send > base :
            #print('Sending packet', base)
            udt_send(packets[base], sock, RECEIVER_ADDR)
        while next_to_send < base + window_size:
            #print('Sending packet', next_to_send)
            udt_send(packets[next_to_send], sock, RECEIVER_ADDR)
            next_to_send += 1

        # Start the timer
        if not send_timer.running():
            #print('Starting timer')
            send_timer.start()

        # Wait until a timer goes off or we get an ACK
        while send_timer.running() and not send_timer.timeout():
            mutex.release()
            #print('Sleeping')
            time.sleep(0.1)
            mutex.acquire()

        if send_timer.timeout():
            # Looks like we timed out
            #print('Timeout')
            send_timer.stop()
        else:
            #print('Shifting window')
            window_size = set_window_size(num_packets)
        mutex.release()

    # Send empty packet as sentinel
    udt_send(pacmake_empty(), sock, RECEIVER_ADDR)
    file.close()

#Recieve thread for selective repeat
def receive_sr(sock):
    global mutex
    global base
    global send_timer

    while True:
        pkt, _ = udt_recv(sock)
        ack, _ = pacextract(pkt)

        # If we get an ACK for the first in-flight packet
        #print('Got ACK', ack)
        if (ack > base):
            mutex.acquire()
            send_timer.stop()
            #print('Base updated', ack)
            base = ack
            mutex.release()

if __name__ == '__main__':
    if len(sys.argv) !=5:
        #print('Arguments missing ')
        exit()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(SENDER_ADDR)
    filename = sys.argv[1]
    MAX_PACKET_SIZE = int(sys.argv[2])
    WINDOW_SIZE = int(sys.argv[3])
    RTO = float(sys.argv[4])

    send_sr(sock, filename)
    sock.close()
