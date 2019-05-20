// [License]
// MIT - See LICENSE.md file in the package.

#include <gc/gc.h>
#include <helion.h>
#include <stdio.h>
#include <uv.h>
#include <iostream>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>

using namespace helion;
using namespace std::string_literals;


class array_literal : public node {
 public:
  std::vector<node *> vals;

  inline text str(void) {
    text t;
    t += "[";
    for (size_t i = 0; i < vals.size(); i++) {
      auto &v = vals[i];
      if (v != nullptr) {
        t += v->str();
      } else {
        t += "UNKNOWN!";
      }
      if (i < vals.size() - 1) t += ", ";
    }
    t += "]";
    return t;
  }
};

class number_literal : public node {
 public:
  text num;
  inline text str(void) { return num; }
};

class var_literal : public node {
 public:
  text var;
  inline text str(void) { return var; }
};



class string_literal : public node {
 public:
  text val;
  inline text str(void) {
    text s;
    s += "'";
    s += val;
    s += "'";
    return s;
  }
};

presult parse_num(pstate);
presult parse_var(pstate);
presult parse_string(pstate);
presult parse_array(pstate);

auto parse_expr = options({parse_num, parse_var, parse_string, parse_array});

parse_func accept(enum tok_type t) {
  return [t](pstate s) -> presult {
    if (s.first().type == t) {
      presult p;
      p.state = s.next();
      return p;
    }
    return pfail();
  };
}

auto epsilon = [](pstate s) {
  presult r;
  r.state = s;
  return r;
};


presult parse_num(pstate s) {
  token t = s;
  if (t.type == tok_num) {
    auto *n = new number_literal();
    n->num = t.val;
    return presult(n, s.next());
  } else {
    return pfail();
  }
}

presult parse_var(pstate s) {
  token t = s;
  if (t.type == tok_var) {
    auto *n = new var_literal();
    n->var = t.val;
    return presult(n, s.next());
  } else {
    return pfail();
  }
}


presult parse_string(pstate s) {
  token t = s;
  if (t.type == tok_str) {
    auto *n = new string_literal();
    n->val = t.val;
    return presult(n, s.next());
  } else {
    return pfail();
  }
}




/**
 * list grammar is as follows:
 * list -> [ list_body ]
 * list_body -> expr list_tail | _
 * list_tail -> , list_body | _
 */
// decl
presult p_array_tail(pstate);
presult p_array_body(pstate);
// impl
presult p_array_body(pstate s) {
  return ((parse_expr & p_array_tail) | (parse_func)epsilon)(s);
}
presult p_array_tail(pstate s) {
  return ((accept(tok_comma) & p_array_body) | (parse_func)epsilon)(s);
}

presult parse_array(pstate s) {
  auto p_list =
      accept(tok_left_square) & p_array_body & accept(tok_right_square);
  auto res = p_list(s);
  array_literal *ln = new array_literal();
  ln->vals = res.vals;
  return presult(ln, res.state);
}




int _main(int argc, char **argv) {
  // print every token from the file
  text src = read_file(argv[1]);
  tokenizer s(src);
  for (pstate s = src; s; s++) {
    puts(s.first());
  }
  puts("-------------------------------------");
  auto res = parse_expr(src);
  puts("res:", res.str());
  return 0;
}
/**
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */



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
    buf->base = (char *)malloc(suggested_size);
    buf->len = suggested_size;
  }

  void on_connection(uv_stream_t *server, int status) {
    if (status < 0) {
      fprintf(stderr, "New connection error %s\n", uv_strerror(status));
      return;
    }

    auto client = new uv_tcp_t();
    auto conn = new C();
    client->data = (void *)conn;
    conn->_connected(client);
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
    static_assert(std::is_base_of<connection, C>::value,
                  "C must be a descendant of MyBase");
    loop = lp;
    // initialize the server on the loop lp
    uv_tcp_init(loop, &sv);
    sv.data = this;
    // initialize the address...
    uv_ip4_addr(ip.c_str(), port, &addr);
    // bind the address so the server can listen on it
    uv_tcp_bind(&sv, (const struct sockaddr *)&addr, 0);
    int r =
        uv_listen((uv_stream_t *)&sv, 128, tcp_server::translate_on_connection);
    if (r) {
      std::string err = "tcp_server listen error: "s + uv_strerror(r);
      throw std::runtime_error(err.c_str());
    }
  }
};



class connection {
  uv_tcp_t *client = nullptr;

  static void translate_write_res(uv_write_t *req, int status) {
    connection *c = (connection *)req->data;
    c->on_write(req, status);
    free(req);
  }

 public:
  static void translate_read(uv_stream_t *client, ssize_t nread,
                             const uv_buf_t *buf) {
    auto *conn = static_cast<connection *>(client->data);
    if (nread < 0) {
      if (nread != UV_EOF) {
        // fprintf(stderr, "Client Lost %s\n", uv_err_name(nread));
        delete conn;
      }
    } else if (nread > 0) {
      conn->on_recv(nread, buf->base);
    }
  }

  inline connection() {}

  inline virtual ~connection(void) {
    if (client != nullptr) {
      printf("connection closed\n");
      uv_close((uv_handle_t *)client, NULL);
    }
  }


  inline void _connected(uv_tcp_t *c) {
    client = c;
    on_connect();
  }

  inline virtual void on_connect() {}

  inline void send(int nbytes, const char *buf) {
    uv_write_t *req = (uv_write_t *)malloc(sizeof(uv_write_t));
    req->data = (void *)this;
    printf("writing %d bytes: ", nbytes);

    for (int i = 0; i < nbytes; i++) {
      printf("%c", buf[i]);
    }
    printf("\n");
    uv_buf_t wrbuf = uv_buf_init(const_cast<char *>(buf), nbytes);
    uv_write(req, (uv_stream_t *)client, &wrbuf, 1,
             connection::translate_write_res);
  }

  inline void on_write(uv_write_t *req, int status) {
    if (status) {
      fprintf(stderr, "Write error %s\n", uv_strerror(status));
    }
  }

  virtual inline void on_recv(int nread, const char *data) {
    // printf("read %d bytes\n", nread);
  }
};



class echo_conn : public connection {
 public:
  inline void on_recv(int n, const char *data) { send(n, data); }
  inline ~echo_conn() {}
};



class len_conn : public connection {
 public:
  inline void on_recv(int n, const char *data) {
    int sz = ceil(log10(n)) + 2;
    char *buf = new char[sz];
    sprintf(buf, "%d\n", n);
    send(sz, buf);
  }
  inline ~len_conn() {}
};


struct connect_data {
  connection *conn;
  uv_tcp_t *sock;
};

void on_connect(uv_connect_t *connection, int status) {
  auto *data = (connect_data *)connection->data;


  auto *conn = data->conn;
  auto *sock = data->sock;

  conn->_connected(sock);

  delete data;
}


template <typename C>
connection *connect(uv_loop_t *loop, std::string ip4, int port) {
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
  uv_tcp_connect(connect, socket, (const struct sockaddr *)&addr, on_connect);
  return data->conn;
}




class spammer : public connection {

  public:
    inline void on_connect() {
      while (true) {
        char *buf = "hello\n";
        send(sizeof(buf), buf-1);
      }
    }
};


uv_loop_t *loop;

int main(int argc, char **argv) {
  loop = uv_default_loop();

  auto *c = connect<spammer>(loop, "127.0.0.1", 7000);
  // auto *server = new tcp_server<len_conn>(loop, "127.0.0.1", 7000);
  // printf("server addr: %p\n", server);
  return uv_run(loop, UV_RUN_DEFAULT);
}
