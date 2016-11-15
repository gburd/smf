// source header
#include "filesystem/wal_writer.h"
// third party
#include <fmt/format.h>
#include <core/reactor.h>
// smf
#include "log.h"
#include "filesystem/wal_writer_utils.h"
#include "filesystem/wal_writer_node.h"
#include "filesystem/wal_head_file_functor.h"


namespace smf {

future<> wal_writer::open() {
  return open_directory(directory_)
    .then([this](file f) {
      auto l = make_lw_shared<wal_head_file_functor>(std::move(f));
      return l->close().then([l, this]() mutable {
        return open_file_dma(l->last_file, open_flags::ro)
          .then([this, auto prefix = l->name_parser.prefix](file last) {
            auto lastf = make_lw_shared<file>(std::move(last));
            return lastf->size().then([this, prefix, lastf](uint64_t size) {
              auto filename = prefix + to_sstring(size);
              current_ = std::make_unique<wal_writer_node>(filename);
              return lastf->close();
            });
          });
      });
    });
}

future<> wal_writer::append(temporary_buffer<char> &&buf) {
  assert(current_ != nullptr);
  // maybe gate this runtime error
  if(buf.size() > max_single_write_size) {
    auto s = fmt::format(
      "call to write buffer of size: `{}` is bigger than limit of: `{}`",
      buf.size(), max_single_write_size);
    throw std::runtime_error(s);
  }
  if(buf.size() > current_.space_left()) {
    return current_.close().then([this, b = std::move(buf)]() mutable {
      using namespace std::chrono;
      auto t = high_resolution_clock::now();
      const auto t_version = duration_cast<milliseconds>(t).count();
      current_ =
        wal_writer_node(current_.name + ":" + to_sstring(current_.current_size)
                        + ":" + to_sstring(t_version));
      return current_.append(std::move(b));
    });
  }
  return current_.append(std::move(buf));
}
}
