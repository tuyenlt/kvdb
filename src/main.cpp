#include "logger/logger.hpp"
#include "net/tcp_server.hpp"
#include <exception>
#include <iostream>
#include <string>
#include <system_error>

int main(int, char **) {
  try {
    TcpServer server = TcpServer();
    std::cout << "Server is running on 0.0.0.0:8080..." << std::endl;
    server.start();
  } catch (const std::system_error &e) {
    Logger::error(std::string("System error [") + e.code().category().name() +
                  ":" + std::to_string(e.code().value()) + "]: " + e.what());
  } catch (const std::exception &e) {
    Logger::error(std::string("Standard error: ") + e.what());
  }
}
