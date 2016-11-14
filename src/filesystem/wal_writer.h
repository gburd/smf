#pragma once
// smf
#include "log.h"
#include "filesystem/wal_writer_node.h"

namespace smf {
class wal_writer {
  public:
  explicit wal_writer(sstring _directory) : directory(_directory) {}
  wal_writer(const wal_writer &other) = delete;

  future<> open();
  future<> append(temporary_buffer<char> &&buf);

  const sstring directory;

  private:
  std::unique_ptr<wal_writer_node> current_ = nullptr;
};
}
