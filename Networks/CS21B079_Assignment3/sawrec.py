import socket
import time
# Define server address and port
SERVER_IP = '127.0.0.1'
SERVER_PORT = 12340

# Create a UDP socket
client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Set the maximum packet size (server size+13)
MAX_PACKET_SIZE = 25600

def receive_file(filename):
    start_time=time.monotonic()
    seq_num = 1
    with open(filename, 'wb') as file:
        while True:
            # Receive data from the server
            data, _ = client_socket.recvfrom(MAX_PACKET_SIZE)
            # print(data)
            # print(type(data))
            # packet_data = data.decode()
            
            received_seq_num = int.from_bytes(data[:4], byteorder='big')
            # print(received_seq_num)
            packet_data=data[4:]
            # received_seq_num, packet_data = data.decode().split('|', 1)
            # received_seq_num = int(received_seq_num)
            
            # print(type(packet_data))
            # If the packet indicates end of file (seq_num is 0), break the loop
            if received_seq_num == 0:
                end_time=time.monotonic()
                # print(start_time)
                # print(end_time)
                # print(type(start_time))
                print(end_time-start_time)
                break

            # Check if the received sequence number matches the expected sequence number
            if received_seq_num == seq_num:
                # Write data to the file
                file.write(packet_data)  # Convert packet_data to bytes before writing
                seq_num += 1
            else:
                # If the received sequence number is out of order, request retransmission of the missing packet
                ack = str(seq_num - 1).encode()  # Send ACK for the last successfully received packet
                client_socket.sendto(ack, (SERVER_IP, SERVER_PORT))
                continue
            
            # Send ACK to the server for the received packet
            ack = str(seq_num - 1).encode()  # Send ACK for the last successfully received packet
            client_socket.sendto(ack, (SERVER_IP, SERVER_PORT))
 
def main():
    # print("Client is running...")
    
    # Send a request to the server to start file transfer
    client_socket.sendto(b"I want file", (SERVER_IP, SERVER_PORT))
    # start_time=time.monotonic()
    # print(start_time)
    # print(end_time)
    # Receive the file from the server
    receive_file('received_file.jpg')
    # print("HIHI")
    # print(end_time)
    # print(start_time)
    # print(end_time-start_time)
    # Close the socket
    client_socket.close()

if __name__ == "__main__":
    main()
