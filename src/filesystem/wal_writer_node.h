#pragma once

namespace smf {
class wal_writer_node {
  wal_writer_node(sstring filename,
                  const uint64_t file_size = wal_file_size_aligned())
    : name(filename), max_size(file_size) {
    file_output_stream_options opts;
    opts.buffer_size = file_size;
    opts.preallocation_size = opts.buffer_size;
    fstream = make_file_output_stream(filename, opts);
  }
  future<> append(temporary_buffer<char> &&buf) {
    if(buf.size() > spae_left()) {
      auto s = fmt::format(
        "File: `{}` of size: `{}` cannot write buffer of size: `{}` ", name,
        max_size, buf.size());
      throw std::runtime_error(s);
    }
    return fstream.write(std::move(buf));
  }

  future<> close() {
    if(!closed) {
      closed = true;
      return fstream.flush().then([this] { return fstream.close(); });
    }
  }
  ~wal_writer_node() {
    if(!closed) {
      LOG_ERROR("File {} was not closed, possible data loss", name);
    }
  }
  inline uint64_t space_left() const { return current_size - max_size; }

  const sstring name;
  const uint64_t max_size;

  private:
  uint64_t current_size_ = 0;
  bool closed_ = false;
  output_stream<char> fstream_;
};

} // namespace smf
