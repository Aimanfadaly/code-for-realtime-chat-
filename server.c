import socket
import threading
import secrets
import base64
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives import padding

# Server configuration
HOST = '127.0.0.1'
PORT = 5000
BUFFER_SIZE = 1024

# Generate a random AES key for encryption/decryption
key = secrets.token_bytes(32)

# List to store connected clients
clients = []

# Function to encrypt a message using AES
def encrypt_message(message, key):
    iv = secrets.token_bytes(16)  # Generate a random initialization vector
    cipher = Cipher(algorithms.AES(key), modes.CBC(iv))
    encryptor = cipher.encryptor()
    padder = padding.PKCS7(128).padder()
    padded_data = padder.update(message.encode()) + padder.finalize()
    encrypted_data = encryptor.update(padded_data) + encryptor.finalize()
    return base64.b64encode(iv + encrypted_data)

# Function to decrypt a message using AES
def decrypt_message(encrypted_message, key):
    encrypted_data = base64.b64decode(encrypted_message)
    iv = encrypted_data[:16]
    cipher = Cipher(algorithms.AES(key), modes.CBC(iv))
    decryptor = cipher.decryptor()
    padded_data = decryptor.update(encrypted_data[16:]) + decryptor.finalize()
    unpadder = padding.PKCS7(128).unpadder()
    decrypted_data = unpadder.update(padded_data) + unpadder.finalize()
    return decrypted_data.decode()

# Function to handle client connections
def handle_client(conn, addr):
    print(f'New connection from {addr}')
    clients.append(conn)

    # Send the encryption key to the client
    conn.send(key)

    while True:
        try:
            # Receive encrypted message from the client
            encrypted_message = conn.recv(BUFFER_SIZE)
            if not encrypted_message:
                break

            # Decrypt the message
            decrypted_message = decrypt_message(encrypted_message, key)

            # Broadcast the decrypted message to all connected clients
            for client in clients:
                if client != conn:
                    encrypted_message = encrypt_message(decrypted_message, key)
                    client.send(encrypted_message)

        except Exception as e:
            print(f'Error: {e}')
            break

    # Remove the disconnected client from the list
    clients.remove(conn)
    conn.close()
    print(f'Connection closed with {addr}')

# Start the server
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind((HOST, PORT))
server_socket.listen(5)
print(f'Server listening on {HOST}:{PORT}')

while True:
    conn, addr = server_socket.accept()
    client_thread = threading.Thread(target=handle_client, args=(conn, addr))
    client_thread.start()
