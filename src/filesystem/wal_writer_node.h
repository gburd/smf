#pragma once
// seastar
#include <core/fstream.hh>
// smf
#include "filesystem/wal_writer_utils.h"

namespace smf {

class wal_writer_node {
  wal_writer_node(sstring prefix,
                  uint64_t epoch = 0,
                  const uint64_t file_size = wal_file_size_aligned());

  future<> append(temporary_buffer<char> &&buf);
  future<> close();
  ~wal_writer_node();

  inline uint64_t space_left() const { return current_size_ - max_size; }
  const sstring prefix_name;
  const uint64_t max_size;

  private:
  future<> rotate_fstream();

  private:
  uint64_t epoch_;
  output_stream<char> fstream_;
  file_output_stream_options opts_;
  uint64_t current_size_ = 0;
  bool closed_ = false;
};

} // namespace smf
