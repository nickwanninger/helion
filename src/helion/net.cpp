// [License]
// MIT - See LICENSE.md file in the package.


#include <helion/net.h>


using namespace helion;






void connection::send(int nbytes, const char *buf) {
  if (client != nullptr) {
    uv_write_t *req = (uv_write_t *)malloc(sizeof(uv_write_t));
    req->data = (void *)this;

    int conn_fd;
    uv_fileno((uv_handle_t *)client, &conn_fd);

    // use the OS send. This is slower than the libuv version, but it should
    // be fine.
    int res = ::send(conn_fd, buf, nbytes, 0);
    if (res == -1) {
      disconnect();
    }
  }
}




connection::~connection(void) {
  if (client != nullptr) {
    disconnect();
  }
}


void connection::disconnect() {
  connected = false;
  uv_close((uv_handle_t *)client, NULL);
  client = nullptr;
  on_disconnect();
  if (disconnect_callback) {
    disconnect_callback();
  }
}



struct connect_data {
  connection *conn;
  uv_tcp_t *sock;
};

void on_connect_cb(uv_connect_t *connection, int status) {
  auto *data = (connect_data *)connection->data;

  auto *conn = data->conn;
  auto *sock = data->sock;
  if (status) {
    conn->connection_failed(status);
  } else {
    conn->_connected(sock);
  }
  delete data;
}


template <typename C>
connection *helion::connect(uv_loop_t *loop, std::string ip4, int port) {
  struct sockaddr_in addr;
  uv_ip4_addr(const_cast<char *>(ip4.c_str()), port, &addr);

  uv_tcp_t *socket = new uv_tcp_t();
  uv_tcp_init(loop, socket);
  uv_tcp_keepalive(socket, 1, 60);

  uv_connect_t *connect = new uv_connect_t();
  auto data = new connect_data();
  data->conn = new C();
  data->sock = socket;

  connect->data = (void *)data;
  uv_tcp_connect(connect, socket, (const struct sockaddr *)&addr, on_connect_cb);
  return data->conn;
}

