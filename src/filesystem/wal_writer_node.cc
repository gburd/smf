#pragma once
// seastar
#include <core/file.hh>
#include <core/fstream.hh>

// smf
#include "likely.h"
#include "filesystem/wal_writer_utils.h"

namespace smf {


wal_writer_node::wal_writer_node(sstring prefix,
                                 uint64_t epoch,
                                 const uint64_t file_size)
  : prefix_name(prefix), max_size(file_size), epoch_(epoch) {
  opts_.buffer_size = file_size;
  opts_.preallocation_size = file_size;
  const auto name = wal_file_name(prefix_name, epoch_);
  fstream_ = make_file_output_stream(name, opts_);
}

future<> wal_writer_node::append(temporary_buffer<char> &&buf) {
  auto const write_size = buf.size();
  if(likely(write_size < spae_left())) {
    current_size_ += write_size;
    return fstream.write(std::move(buf));
  }
  // import flatbuffers headers for wal
  // TODO: fix
  // write partial & return the rest
  return make_ready_future<>();
}
future<> wal_writer_node::close() {
  if(!closed) {
    closed = true;
    return fstream_.flush().then([this] { return fstream_.close(); });
  }
}
wal_writer_node::~wal_writer_node() {
  if(!closed) {
    const auto name = wal_file_name(prefix_name, epoch_);
    LOG_ERROR("File {} was not closed, possible data loss", name);
  }
}
future<> wal_writer_node::rotate_fstream() {
  return fstream_.flush().then([this] {
    return fstream_.close().then([this] {
      epoch_ += max_size;
      const auto name = wal_file_name(prefix_name, epoch_);
      fstream_ = make_file_output_stream(name, opts_);
      current_size_ = 0;
      return make_ready_future<>();
    });
  });
}


} // namespace smf
