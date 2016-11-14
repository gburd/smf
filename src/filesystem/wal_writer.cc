// source header
#include "filesystem/wal_writer.h"
// third party
#include <fmt/format.h>
#include <core/reactor.h>
// smf
#include "log.h"
#include "filesystem/wal_writer_utils.h"
#include "filesystem/wal_writer_node.h"

namespace smf {


/// \brief used from a subscriber to find the last file on the file system
/// according to our naming scheme of `file`_`size`_`date`, i.e.:
/// for a given directory.
/// Given `/tmp/smf/wal` as a directory, we'll find files that looks like this
/// "smf_s_2345234_d_23421234.wal"
///
struct head_file_functor {
  file_visitor(file _f)
    : f(std::move(_f))
    , listing(
        f.list_directory([this](directory_entry de) { return visit(de); })) {}

  future<> visit(directory_entry de) {
    static const boost::regex file_re("(\\d+)_(\\d+)\.wal");
    if(de.type && de.type == directory_entry_type::regular
       && regex_match(de.name.c_str(), file_re)) {
      if(de.name > last_file) {
        last_file = de.name;
      }
    }
    return make_ready_future<>();
  }

  future<> close() { return listing.done(); }

  sstring last_file;
  file f;
  subscription<directory_entry> listing;

}

future<>
wal_writer::open() {
  return engine()
    .open_directory(directory_)
    .then([this](file f) {
      auto l = make_lw_shared<file_visitor>(std::move(f));
      return l->close();
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
