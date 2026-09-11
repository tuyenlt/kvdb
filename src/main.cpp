#include "io/poll_event.hpp"
#include "logger/logger.hpp"
#include "net/tcp_connection.hpp"
#include "net/tcp_server.hpp"
#include <exception>
#include <iostream>
#include <string>
#include <system_error>

int main(int, char **) {
  try {
    TcpServer server = TcpServer();

    server.set_read_callback_handler([](TcpConnection *conn) {
      auto &in_buf = conn->get_in_buffer();
      std::string msg(in_buf.begin(), in_buf.end());
      std::cout << "[Server] Received " << in_buf.size() << " bytes: " << msg
                << std::endl;

      conn->add_write_buffer(in_buf);
      conn->set_interest_mask(POLL_WANT_WRITE | POLL_WANT_READ);
      in_buf.clear();
    });

    std::cout << "Server is running on 0.0.0.0:8080..." << std::endl;
    server.start();
  } catch (const std::system_error &e) {
    Logger::error(std::string("System error [") + e.code().category().name() +
                  ":" + std::to_string(e.code().value()) + "]: " + e.what());
  } catch (const std::exception &e) {
    Logger::error(std::string("Standard error: ") + e.what());
  }
}
