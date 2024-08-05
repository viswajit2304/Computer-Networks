import socket
import sys

# Define server address and port
SERVER_IP = '127.0.0.1'
SERVER_PORT = 12340

# Create a UDP socket
server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.bind((SERVER_IP, SERVER_PORT))

# Set the maximum packet size
MAX_PACKET_SIZE = 25600
n=len(sys.argv)
if len(sys.argv) != 4:
    print('Expected filename, maxpacketsize, RTO  as command line argument')
    exit()

file_name=sys.argv[1]
MAX_PACKET_SIZE=int(sys.argv[2])
RTO=float(sys.argv[3])

def send_file(filename, client_address):
    with open(filename, 'rb') as file:
        seq_num = 1
        while True:
            # Read data from the file
            data = file.read(MAX_PACKET_SIZE)
            # print(type(data))
            # Check for end of file
            if not data:
                # Send a packet with sequence number 0 to indicate end of file
                seq_num=0
                seq_num2=seq_num.to_bytes(4,'big')

                server_socket.sendto(seq_num2, client_address)
                ack_received = False
                while not ack_received:
                    server_socket.settimeout(RTO)  # Set a timeout for receiving ACK
                    try:
                        ack, _ = server_socket.recvfrom(MAX_PACKET_SIZE)
                        if ack == str(seq_num).encode():
                            ack_received = True
                        else:
                            server_socket.sendto(seq_num2, client_address)
                            # print(seq_num)
                    except socket.timeout:
                        # print(f"Timeout occurred for packet {seq_num}. Retransmitting...")
                        # Retransmit the packet
                        server_socket.sendto(seq_num2, client_address)
                
                break
            
            # Send data to the client with packet sequence number
            # print(data)
            # print(type(data))
            # data2=data.decode()
            # seq_num2=str(seq_num)
            # packet = f"{seq_num}|{data}"
            seq_num2=seq_num.to_bytes(4,'big')+data
            # print(seq_num2)
            # print(type(seq_num2))
            # packet= seq_num+"|"+data2
            # print(type(packet))
            # print(packet)
            # print(packet.encode())
            server_socket.sendto(seq_num2, client_address)
            # xx=seq_num2.decode()
            # Wait for ACK from client with a timeout
            ack_received = False
            while not ack_received:
                server_socket.settimeout(RTO)  # Set a timeout for receiving ACK
                try:
                    ack, _ = server_socket.recvfrom(MAX_PACKET_SIZE)
                    if ack == str(seq_num).encode():
                        ack_received = True
                    else:
                        server_socket.sendto(seq_num2, client_address)
                        # print(seq_num)
                except socket.timeout:
                    # print(f"Timeout occurred for packet {seq_num}. Retransmitting...")
                    # Retransmit the packet
                    server_socket.sendto(seq_num2, client_address)
            
            seq_num += 1
            server_socket.settimeout(None)

def main(): 
    print("Server is running...")
    
    while True:
        # Receive the "I want file" message from the client
        message, client_address = server_socket.recvfrom(MAX_PACKET_SIZE)
        if message.decode() == "I want file":
            # Send the file to the client
            send_file(file_name, client_address)
    
    # Close the socket
    print("ending")
    server_socket.close()

if __name__ == "__main__":
    main()
