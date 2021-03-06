// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#ifndef ARROW_IO_HDFS
#define ARROW_IO_HDFS

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "arrow/io/interfaces.h"
#include "arrow/util/macros.h"
#include "arrow/util/visibility.h"

namespace arrow {

class Buffer;
class Status;

namespace io {

class HdfsClient;
class HdfsReadableFile;
class HdfsOutputStream;

struct HdfsPathInfo {
  ObjectType::type kind;

  std::string name;
  std::string owner;
  std::string group;

  // Access times in UNIX timestamps (seconds)
  int64_t size;
  int64_t block_size;

  int32_t last_modified_time;
  int32_t last_access_time;

  int16_t replication;
  int16_t permissions;
};

enum class HdfsDriver : char { LIBHDFS, LIBHDFS3 };

struct HdfsConnectionConfig {
  std::string host;
  int port;
  std::string user;
  std::string kerb_ticket;
  HdfsDriver driver;
};

class ARROW_EXPORT HdfsClient : public FileSystemClient {
 public:
  ~HdfsClient();

  // Connect to an HDFS cluster given a configuration
  //
  // @param config (in): configuration for connecting
  // @param fs (out): the created client
  // @returns Status
  static Status Connect(
      const HdfsConnectionConfig* config, std::shared_ptr<HdfsClient>* fs);

  // Create directory and all parents
  //
  // @param path (in): absolute HDFS path
  // @returns Status
  Status CreateDirectory(const std::string& path);

  // Delete file or directory
  // @param path: absolute path to data
  // @param recursive: if path is a directory, delete contents as well
  // @returns error status on failure
  Status Delete(const std::string& path, bool recursive = false);

  // Disconnect from cluster
  //
  // @returns Status
  Status Disconnect();

  // @param path (in): absolute HDFS path
  // @returns bool, true if the path exists, false if not (or on error)
  bool Exists(const std::string& path);

  // @param path (in): absolute HDFS path
  // @param info (out)
  // @returns Status
  Status GetPathInfo(const std::string& path, HdfsPathInfo* info);

  // @param nbytes (out): total capacity of the filesystem
  // @returns Status
  Status GetCapacity(int64_t* nbytes);

  // @param nbytes (out): total bytes used of the filesystem
  // @returns Status
  Status GetUsed(int64_t* nbytes);

  Status ListDirectory(const std::string& path, std::vector<HdfsPathInfo>* listing);

  // @param path file path to change
  // @param owner pass nullptr for no change
  // @param group pass nullptr for no change
  Status Chown(const std::string& path, const char* owner, const char* group);

  Status Chmod(const std::string& path, int mode);

  // Move file or directory from source path to destination path within the
  // current filesystem
  Status Rename(const std::string& src, const std::string& dst);

  // TODO(wesm): GetWorkingDirectory, SetWorkingDirectory

  // Open an HDFS file in READ mode. Returns error
  // status if the file is not found.
  //
  // @param path complete file path
  Status OpenReadable(const std::string& path, int32_t buffer_size,
      std::shared_ptr<HdfsReadableFile>* file);

  Status OpenReadable(const std::string& path, std::shared_ptr<HdfsReadableFile>* file);

  // FileMode::WRITE options
  // @param path complete file path
  // @param buffer_size, 0 for default
  // @param replication, 0 for default
  // @param default_block_size, 0 for default
  Status OpenWriteable(const std::string& path, bool append, int32_t buffer_size,
      int16_t replication, int64_t default_block_size,
      std::shared_ptr<HdfsOutputStream>* file);

  Status OpenWriteable(
      const std::string& path, bool append, std::shared_ptr<HdfsOutputStream>* file);

 private:
  friend class HdfsReadableFile;
  friend class HdfsOutputStream;

  class ARROW_NO_EXPORT HdfsClientImpl;
  std::unique_ptr<HdfsClientImpl> impl_;

  HdfsClient();
  DISALLOW_COPY_AND_ASSIGN(HdfsClient);
};

class ARROW_EXPORT HdfsReadableFile : public RandomAccessFile {
 public:
  ~HdfsReadableFile();

  Status Close() override;

  Status GetSize(int64_t* size) override;

  // NOTE: If you wish to read a particular range of a file in a multithreaded
  // context, you may prefer to use ReadAt to avoid locking issues
  Status Read(int64_t nbytes, int64_t* bytes_read, uint8_t* buffer) override;

  Status Read(int64_t nbytes, std::shared_ptr<Buffer>* out) override;

  Status ReadAt(
      int64_t position, int64_t nbytes, int64_t* bytes_read, uint8_t* buffer) override;

  Status ReadAt(int64_t position, int64_t nbytes, std::shared_ptr<Buffer>* out) override;

  bool supports_zero_copy() const override;

  Status Seek(int64_t position) override;
  Status Tell(int64_t* position) override;

  void set_memory_pool(MemoryPool* pool);

 private:
  explicit HdfsReadableFile(MemoryPool* pool = nullptr);

  class ARROW_NO_EXPORT HdfsReadableFileImpl;
  std::unique_ptr<HdfsReadableFileImpl> impl_;

  friend class HdfsClient::HdfsClientImpl;

  DISALLOW_COPY_AND_ASSIGN(HdfsReadableFile);
};

// Naming this file OutputStream because it does not support seeking (like the
// WriteableFile interface)
class ARROW_EXPORT HdfsOutputStream : public OutputStream {
 public:
  ~HdfsOutputStream();

  Status Close() override;

  Status Write(const uint8_t* buffer, int64_t nbytes) override;

  Status Write(const uint8_t* buffer, int64_t nbytes, int64_t* bytes_written);

  Status Flush() override;

  Status Tell(int64_t* position) override;

 private:
  class ARROW_NO_EXPORT HdfsOutputStreamImpl;
  std::unique_ptr<HdfsOutputStreamImpl> impl_;

  friend class HdfsClient::HdfsClientImpl;

  HdfsOutputStream();

  DISALLOW_COPY_AND_ASSIGN(HdfsOutputStream);
};

Status ARROW_EXPORT HaveLibHdfs();
Status ARROW_EXPORT HaveLibHdfs3();

}  // namespace io
}  // namespace arrow

#endif  // ARROW_IO_HDFS
