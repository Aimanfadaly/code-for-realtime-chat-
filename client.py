import socket
import threading

SERVER_ADDRESS = "127.0.0.1"
SERVER_PORT = 8000
BUFFER_SIZE = 1024

def receive_messages(client_socket):
    while True:
        try:
            message = client_socket.recv(BUFFER_SIZE).decode()
            if not message:
                break
            print(message)
        except Exception as e:
            print(f"Error receiving message: {e}")
            break

def main():
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect((SERVER_ADDRESS, SERVER_PORT))

    name = input("Enter your name: ").strip()
    client_socket.send(name.encode())

    password = input("Enter the server password: ").strip()

    receive_thread = threading.Thread(target=receive_messages, args=(client_socket,))
    receive_thread.start()

    while True:
        message = input()
        client_socket.send(message.encode())

        if message.lower() == "quit":
            break

    client_socket.close()

if __name__ == "__main__":
    main()
