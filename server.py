import socket
import threading

SERVER_PORT = 8000
BUFFER_SIZE = 1024
MAX_NAME_LENGTH = 32
MAX_CLIENTS = 10
SHIFT = 3  # Shift value for Caesar cipher

clients = []
clients_lock = threading.Lock()

def caesar_cipher(text, shift):
    result = ""
    for char in text:
        if char.isalpha():
            shifted = ord(char) - shift
            if char.islower():
                result += chr((shifted - ord('a')) % 26 + ord('a'))
            else:
                result += chr((shifted - ord('A')) % 26 + ord('A'))
        else:
            result += char
    return result

def handle_client(client_socket, client_address):
    name = client_socket.recv(MAX_NAME_LENGTH).decode().strip()
    print(f"Welcome {name} to our awesome server.")

    with clients_lock:
        clients.append((client_socket, name))

    while True:
        try:
            message = client_socket.recv(BUFFER_SIZE).decode()
            if not message:
                break
            decrypted_message = caesar_cipher(message, SHIFT)
            print(f"{name}: {decrypted_message}")

            with clients_lock:
                for c in clients:
                    if c[0] != client_socket:
                        c[0].send(f"{name}: {message}".encode())
        except Exception as e:
            print(f"Error handling client: {e}")
            break

    print(f"Bye bye {name} see you again.")
    with clients_lock:
        clients.remove((client_socket, name))
    client_socket.close()

def main():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(('0.0.0.0', SERVER_PORT))
    server_socket.listen(MAX_CLIENTS)
    print(f"Awesome server is up. Kindly ask the client to enter awesome for the password")

    while True:
        client_socket, client_address = server_socket.accept()
        client_thread = threading.Thread(target=handle_client, args=(client_socket, client_address))
        client_thread.start()

if __name__ == "__main__":
    main()
