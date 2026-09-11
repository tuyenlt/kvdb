#include <iostream>
#include <string>

#include <arpa/inet.h>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  int clientFd = socket(AF_INET, SOCK_STREAM, 0);

  if (clientFd == -1) {
    perror("socket");
    return 1;
  }

  sockaddr_in serverAddr{};

  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(8080);

  if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0) {
    perror("inet_pton");
    close(clientFd);
    return 1;
  }

  if (connect(clientFd, reinterpret_cast<sockaddr *>(&serverAddr),
              sizeof(serverAddr)) == -1) {
    perror("connect");
    close(clientFd);
    return 1;
  }

  std::cout << "Connected to server on 127.0.0.1:8080!\n";
  std::cout << "Type a message and press Enter (or 'exit' to quit):\n";

  std::string message;
  while (true) {
    std::cout << "\nclient> ";
    if (!std::getline(std::cin, message)) {
      break;
    }

    if (message == "exit") {
      std::cout << "Closing connection and exiting...\n";
      break;
    }

    if (message.empty()) {
      continue;
    }

    ssize_t sent = send(clientFd, message.data(), message.size(), 0);
    if (sent <= 0) {
      perror("send failed");
      break;
    }
    std::cout << "[WRITE] Sent " << sent << " bytes to server: \"" << message
              << "\"\n";

    char recv_buffer[64 * 1024];
    ssize_t bytes_read =
        recv(clientFd, recv_buffer, sizeof(recv_buffer) - 1, 0);

    if (bytes_read > 0) {
      recv_buffer[bytes_read] = '\0';
      std::cout << "[READ]  Received " << bytes_read << " bytes from server: \""
                << recv_buffer << "\"\n";
    } else if (bytes_read == 0) {
      std::cout << "[READ]  Server closed the connection.\n";
      break;
    } else {
      perror("recv failed");
      break;
    }
  }

  close(clientFd);
  return 0;
}