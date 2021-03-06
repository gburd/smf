#pragma once
// seastar
#include <net/api.hh>
#include <core/iostream.hh>

namespace smf {

class rpc_connection {
  public:
  rpc_connection(connected_socket fd)
    : socket(std::move(fd))
    , istream(socket.input())
    , ostream(socket.output()) {}
  connected_socket socket;
  input_stream<char> istream;
  output_stream<char> ostream;

  uint32_t istream_active_parser{0};

  void disable() { enabled_ = false; }
  bool is_enabled() const { return enabled_; }
  bool is_valid() { return !istream.eof() && !has_error() && enabled_; }
  bool has_error() const { return error_.operator bool(); }
  void set_error(const char *e) { error_ = sstring(e); }
  sstring get_error() const { return error_.value(); }

  private:
  std::experimental::optional<sstring> error_;
  bool enabled_{true};
};
}
