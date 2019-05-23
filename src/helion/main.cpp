// [License]
// MIT - See LICENSE.md file in the package.

#include <gc/gc.h>
#include <helion.h>
#include <stdio.h>
#include <sys/socket.h>
#include <uv.h>
#include <iostream>
#include <map>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>

using namespace helion;
using namespace std::string_literals;

/*

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


*/


int main(int argc, char **argv) {
  // print every token from the file
  text src = read_file(argv[1]);
  auto res = parse_module(src);
  puts(res->str());
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


struct storage_cell {
  size_t ind = 0;
  size_t size = 0;
  char *buf = nullptr;
};

std::vector<storage_cell> cells;



class storage_conn : public connection {
 public:
  inline void on_connect() { printf("new connection\n"); }
  inline void on_disconnect() { printf("disconnect\n"); }
  inline void on_recv(int len, const char *buf) {
    if (len > 0) {
      char cmd = buf[0];
      if (cmd == 'a') {
        size_t size = *(size_t *)(buf + 1);

        size_t ind = cells.size();

        storage_cell sc;
        sc.size = size;
        sc.ind = ind;
        sc.buf = new char[size];
        cells.push_back(sc);

        send(sizeof(size_t), (const char *)&ind);
        return;
      }


      if (cmd == 'r') {
        size_t *args = (size_t *)(buf + 1);

        auto addr = args[0];
        auto size = args[1];


        if (addr >= 0 && addr < cells.size()) {
          auto &sc = cells[addr];

          if (sc.size >= size) {
            send(size, (const char *)sc.buf);
            return;
          }
        }
        send("");
        return;
      }


      if (cmd == 'w') {
        size_t *args = (size_t *)(buf + 1);

        auto addr = args[0];
        auto size = args[1];

        char *data = (char *)&args[2];

        if (addr >= 0 && addr < cells.size()) {
          auto &sc = cells[addr];

          if (sc.size >= size) {
            memcpy(sc.buf, data, size);
            send("1");
            return;
          }
        }
        send("0");
      }
    } else {
      send("");
    }
  }
};




struct remote_storage_connection {
  int sock = -1;
  std::mutex lock;


  inline remote_storage_connection(const char *addr = "127.0.0.1",
                                   short port = 7000) {
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      printf("socket creation error \n");
      exit(-1);
    }
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, addr, &serv_addr.sin_addr) <= 0) {
      exit(-1);
    }
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
      exit(-1);
    }
  }

  /**
   * allocate a block of memory
   */
  inline size_t alloc(size_t sz) {
    std::unique_lock lk(lock);
    size_t addr = 0;
    int blen = sizeof(char) + sizeof(size_t);
    char buf[blen];
    buf[0] = 'a';
    *(size_t *)(buf + 1) = sz;
    ::send(sock, buf, blen, 0);
    ::read(sock, &addr, sizeof(size_t));
    return addr;
  }

  inline int read(size_t addr, size_t size, char *dst) {
    std::unique_lock lk(lock);

    int blen = sizeof(char) + sizeof(size_t[2]);
    char buf[blen];
    buf[0] = 'r';
    size_t *nums = (size_t *)(buf + 1);
    nums[0] = addr;
    nums[1] = size;
    ::send(sock, buf, blen, 0);
    return ::read(sock, dst, size);
  }

  inline int write(size_t addr, int size, char *data) {
    std::unique_lock lk(lock);

    int cmdlen = sizeof(char) + sizeof(size_t) + sizeof(size_t) + size;
    char *cmd = new char[cmdlen];
    cmd[0] = 'w';
    size_t *args = (size_t *)(cmd + 1);
    args[0] = addr;
    args[1] = size;
    void *dst = &args[2];
    memcpy(dst, data, size);
    ::send(sock, cmd, cmdlen, 0);
    ::read(sock, dst, 1);
    delete[] cmd;
    return 0;
  }
};



/**
 * primary remote storage connection
 */
remote_storage_connection *rsc;

template <typename T>
class remote_ref {
 public:
  size_t ind = -1;

  operator T() {
    T v;
    rsc->read(ind, sizeof(T), (char *)&v);
    return v;
  }

  T operator+(T &o) {
    T self = *this;
    return self + o;
  }

  remote_ref &operator=(const T &value) {
    // write the changes over the network
    rsc->write(ind, sizeof(T), (char *)&value);
    return *this;
  }
};

template <typename T>
remote_ref<T> remote_alloc(void) {
  remote_ref<T> r;
  r.ind = rsc->alloc(sizeof(T));
  return r;
}


int client_main() {
  rsc = new remote_storage_connection();


  auto r = remote_alloc<int>();
  r = 0;

  while (true) {
    int i = r;
    i++;
    printf("%d\n", i);
    r = i;
  }

  return 0;
}



uv_loop_t *loop;
tcp_server<storage_conn> *server;


int _main(int argc, char **argv) {
  if (argc == 2) {
    loop = uv_default_loop();


    if (!strcmp(argv[1], "client")) {
      return client_main();
    }
    if (!strcmp(argv[1], "server")) {
      server = new tcp_server<storage_conn>(loop, "0.0.0.0", 7000);
    }


    return uv_run(loop, UV_RUN_DEFAULT);
  }
  return 0;
}
