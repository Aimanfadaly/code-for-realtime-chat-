import socket
import threading
import base64
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives import padding

# Server configuration
HOST = '127.0.0.1'
PORT = 5000
BUFFER_SIZE = 1024

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

# Function to receive messages from the server
def receive_messages(client_socket, key):
    while True:
        try:
            # Receive encrypted message from the server
            encrypted_message = client_socket.recv(BUFFER_SIZE)
            if not encrypted_message:
                break

            # Decrypt the message
            decrypted_message = decrypt_message(encrypted_message, key)
            print(decrypted_message)

        except Exception as e:
            print(f'Error: {e}')
            break

# Function to send messages to the server
def send_messages(client_socket, key):
    while True:
        message = input()

        # Encrypt the message
        encrypted_message = encrypt_message(message, key)

        try:
            # Send the encrypted message to the server
            client_socket.send(encrypted_message)
        except Exception as e:
            print(f'Error: {e}')
            break

# Connect to the server
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client_socket.connect((HOST, PORT))

# Receive the encryption key from the server
key = client_socket.recv(32)

# Start threads for sending and receiving messages
receive_thread = threading.Thread(target=receive_messages, args=(client_socket, key))
send_thread = threading.Thread(target=send_messages, args=(client_socket, key))

receive_thread.start()
send_thread.start()
