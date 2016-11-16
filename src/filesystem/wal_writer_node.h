#pragma once
// seastar
#include <core/file.hh>
#include <core/fstream.hh>

// smf
#include "likely.h"
#include "filesystem/wal_writer_utils.h"

namespace smf {

class wal_writer_node {
  wal_writer_node(sstring prefix,
                  uint64_t epoch = 0,
                  const uint64_t file_size = wal_file_size_aligned())
    : prefix_name(prefix), max_size(file_size), epoch_(epoch) {
    opts_.buffer_size = file_size;
    opts_.preallocation_size = file_size;
    fstream_ = make_file_output_stream(name, opts_);
  }
  future<> append(temporary_buffer<char> &&buf) {
    auto const write_size = buf.size();
    if(likely(write_size < spae_left())) {
      current_size_ += write_size;
      return fstream.write(std::move(buf));
    }
    // TODO: fix
    // write partial & return the rest
    return make_ready_future<>();
  }
  future<> close() {
    if(!closed) {
      closed = true;
      return fstream_.flush().then([this] { return fstream_.close(); });
    }
  }
  ~wal_writer_node() {
    if(!closed) {
      LOG_ERROR("File {} was not closed, possible data loss", name);
    }
  }
  inline uint64_t space_left() const { return current_size_ - max_size; }
  const sstring prefix_name;
  const uint64_t max_size;


  private:
  future<> rotate_fstream() {
    return fstream_.flush().then([this] {
      auto f = std::move(fstream_);
      epoch_ += max_size;
      const auto name = wal_file_name(prefix_name, epoch_);
      fstream_ = make_file_output_stream(name, opts_);
      current_size_ = 0;
      return f.close();
    });
  }


  private:
  uint64_t epoch_;
  output_stream<char> fstream_;
  file_output_stream_options opts_;

  uint64_t current_size_ = 0;
  bool closed_ = false;
};

} // namespace smf
