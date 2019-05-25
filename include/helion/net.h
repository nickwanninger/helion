// [License]
// MIT - See LICENSE.md file in the package.

#pragma once

#ifndef __HELION_NET_H__
#define __HELION_NET_H__


#include <stdio.h>
#include <sys/socket.h>
#include <uv.h>
#include <string>
#include <type_traits>
#include <vector>
#include <functional>

namespace helion {


  class connection;

  template <typename C>
  class tcp_server {
   private:
    struct sockaddr_in addr;
    // a handle to the TCP server inside libuv
    uv_tcp_t sv;
    uv_loop_t *loop = nullptr;
    static void alloc_buffer(uv_handle_t *handle, size_t suggested_size,
                             uv_buf_t *buf) {
      buf->base = new char[suggested_size];
      buf->len = suggested_size;
    }

    /**
     * when a connection occurs, this function is called by libuv and a C
     * is constructed to handle it. C is a connection object that can handle
     * the async events from the connection
     */
    void on_connection(uv_stream_t *server, int status) {
      if (status < 0) {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        return;
      }
      auto client = new uv_tcp_t();
      auto conn = new C();
      client->data = (void *)conn;
      conn->_connected(client);
      conn->disconnect_callback = [conn] () {
        delete conn;
      };
      uv_tcp_init(loop, client);
      if (uv_accept(server, (uv_stream_t *)client) == 0) {
        uv_read_start((uv_stream_t *)client, alloc_buffer, C::translate_read);
      } else {
        uv_close((uv_handle_t *)client, NULL);
      }
    }

    static void translate_on_connection(uv_stream_t *server, int status) {
      auto *self = (tcp_server *)server->data;
      self->on_connection(server, status);
    }

   public:
    tcp_server(uv_loop_t *lp, std::string ip = "0.0.0.0", int port = 7000) {
      using namespace std::string_literals;
      static_assert(std::is_base_of<connection, C>::value,
                    "C must be a descendant of connection");
      loop = lp;
      // initialize the server on the loop lp
      uv_tcp_init(loop, &sv);
      sv.data = this;
      // initialize the address...
      uv_ip4_addr(ip.c_str(), port, &addr);
      // bind the address so the server can listen on it
      uv_tcp_bind(&sv, (const struct sockaddr *)&addr, 0);
      int r = uv_listen((uv_stream_t *)&sv, 512,
                        tcp_server::translate_on_connection);
      if (r) {
        std::string err = "tcp_server listen error: "s + uv_strerror(r);
        throw std::runtime_error(err.c_str());
      }
    }

    ~tcp_server(void) {
      uv_close((uv_handle_t*)sv, NULL);
    }
  };


  class connection {
   protected:
    bool connected = false;
    uv_tcp_t *client = nullptr;


   public:
    std::function<void(void)> disconnect_callback;
    static void translate_read(uv_stream_t *client, ssize_t nread,
                               const uv_buf_t *buf) {
      auto *conn = static_cast<connection *>(client->data);
      if (nread < 0) {
        if (nread != UV_EOF) {
          conn->disconnect();
        }
      } else if (nread > 0) {
        conn->on_recv(nread, buf->base);
      } else {
        conn->on_recv(0, nullptr);
      }
      delete buf->base;
    }

    virtual ~connection(void);

    /**
     * cause a disconnect to occur. closes the client
     * and removes the reference to it.
     */
    void disconnect();

    /**
     * called when the socket connects. Updates local vars
     * and defers to client virtual function
     */
    inline void _connected(uv_tcp_t *c) {
      connected = true;
      client = c;
      on_connect();
    }

    inline void _connection_failed(int status) { connection_failed(status); }

    void send(int nbytes, const char *buf);
    inline void send(std::string s) { send(s.size(), s.c_str()); }


    /**
     * virtual functions that are overloaded by child/client classes.
     * This is the primary API.
     */
    virtual inline void connection_failed(int status) {}
    virtual inline void on_connect() {}
    virtual inline void on_disconnect() {}
    virtual inline void on_recv(int nread, const char *data) {}
  };

  template <typename C>
  connection *connect(uv_loop_t *loop, std::string ip4, int port);


}  // namespace helion


#endif
