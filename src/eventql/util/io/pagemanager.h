/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#ifndef _libstx_FILEBACKEND_PAGEMANAGER_H
#define _libstx_FILEBACKEND_PAGEMANAGER_H

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <atomic>
#include <eventql/util/exception.h>
#include <eventql/util/autoref.h>


namespace io {

/**
 * This is an internal class. For usage instructions and extended documentation
 * please refer to "storagebackend.h" and "database.h"
 */
class PageManager {
public:

  struct Page {
    Page(uint64_t offset_, uint64_t size_);
    Page();
    uint64_t offset;
    uint64_t size;
  };

  struct PageRef {
  public:
    PageRef(const Page& page);
    PageRef(const PageRef& copy) = delete;
    PageRef& operator=(const PageRef& copy) = delete;
    virtual ~PageRef();
    void* operator*() const;
    template<typename T> T* structAt(size_t position) const;
    virtual void* getPtr() const = 0;
    inline void* ptr() const { return getPtr(); }
    inline size_t size() const { return page_.size; }
    virtual void sync(bool async = false) const {};
    const PageManager::Page page_;
  };

  PageManager(size_t block_size, size_t end_pos = 0);
  //PageManager(size_t block_size, const LogSnapshot& log_snapshot);
  PageManager(const PageManager& copy) = delete;
  PageManager& operator=(const PageManager& copy) = delete;
  PageManager(const PageManager&& move);

  /**
   * Request a new page from the page manager
   */
  virtual Page allocPage(size_t min_size);

  /**
   * Return a page to the pagemanager. Adds this page to the freelist
   */
  virtual void freePage(const Page& page);

  /**
   * Request a page to be mapped into memory. Returns a smart pointer.
   */
  virtual std::unique_ptr<PageRef> getPage(const PageManager::Page& page) = 0;

protected:

  /**
   * Try to find a free page with a size larger than or equal to min_size
   *
   * Returns true if a matching free page was found and returns the page into
   * the destination parameter. Returns false if no matching page was found and
   * does not change the destination parameter.
   */
  bool findFreePage(size_t min_size, Page* destination);

  /**
   * Index of the first unused byte in the file
   */
  size_t end_pos_;

  /**
   * Optimal block size for the underlying file
   */
  const size_t block_size_;

  /**
   * Page free list
   *
   * tuple is (size, offset)
   */
  std::vector<std::pair<uint64_t, uint64_t>> freelist_;

  std::mutex mutex_;
};

class MmapPageManager : public PageManager {
protected:
  // FIXPAUL replace with io::MmapedFile
  class MmappedFile : public RefCounted {
  public:
    MmappedFile(void* __data, const size_t __size);
    MmappedFile(const MmappedFile& copy) = delete;
    MmappedFile& operator=(const MmappedFile& copy) = delete;
    ~MmappedFile();
    void* const data;
    const size_t size;
  };

public:
  /**
   * Size of the initially create mmaping in bytes. All mmapings will be a
   * multiple of this size!
   */
  static const size_t kMmapSizeMultiplier = 128; /* 128 * PAGE_SIZE */

  class MmappedPageRef : public PageManager::PageRef {
  public:
    MmappedPageRef(
        const PageManager::Page& page,
        RefPtr<MmappedFile> file,
        size_t sys_page_size);

    MmappedPageRef(MmappedPageRef&& move);
    MmappedPageRef(const MmappedPageRef& copy) = delete;
    MmappedPageRef& operator=(const MmappedPageRef& copy) = delete;
    ~MmappedPageRef();
    void sync(bool async = false) const override;
  protected:
    size_t sys_page_size_;
    void* getPtr() const override;
    RefPtr<MmappedFile> file_;
  };

  /**
   * Create a new mmap page manager for a new file
   */
  explicit MmapPageManager(const std::string& filename, size_t file_size);


  MmapPageManager(MmapPageManager&& move);
  MmapPageManager(const MmapPageManager& copy) = delete;
  MmapPageManager& operator=(const MmapPageManager& copy) = delete;
  ~MmapPageManager();

  /**
   * Request a page to be mapped into memory. Returns a smart pointer.
   */
  std::unique_ptr<PageManager::PageRef> getPage(
      const PageManager::Page& page) override;

  /**
   * Request a page to be mapped into memory but disallow padding the file
   * . Returns a smart pointer.
   */
  struct kNoPadding {};
  std::unique_ptr<PageManager::PageRef> getPage(
      const PageManager::Page& page, kNoPadding no_padding);


  /**
   * Truncate the file to the actual used size
   */
  void shrinkFile();

protected:

  std::unique_ptr<PageManager::PageRef> getPageImpl(
      const PageManager::Page& page, bool allow_padding);
  /**
   * Returns a mmap()ed memory region backend by the managed file spans until
   * at least last_byte
   */
  RefPtr<MmappedFile> getMmappedFile(uint64_t last_byte);

  const std::string filename_;
  size_t used_bytes_;
  size_t file_size_;
  RefPtr<MmappedFile> current_mapping_;
  std::mutex mmap_mutex_;
  size_t sys_page_size_;
};


/**
 * IMPLEMENTATION
 */
template<typename T>
T* PageManager::PageRef::structAt(size_t position) const {
  if (position >= page_.size) {
    RAISE(
        kIndexError,
        "invalid access at address %x, page size=%x offset=%x",
        position,
        page_.size,
        page_.offset);
  }

  return (T*) (((char *) getPtr()) + page_.offset + position);
}

}
#endif
