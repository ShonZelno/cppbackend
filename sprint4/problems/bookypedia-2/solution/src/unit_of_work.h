#pragma once

#include <memory>

#include "author_fwd.h"
#include "book_fwd.h"
#include "book_tag_fwd.h"

namespace app {

class UnitOfWork {
public:
  virtual void Commit() = 0;
  virtual domain::AuthorRepository &Authors() = 0;
  virtual domain::BookRepository &Books() = 0;
  virtual domain::BookTagRepository &BookTags() = 0;

public:
  ~UnitOfWork() = default;
};

class UnitOfWorkFactory {
public:
  virtual std::unique_ptr<UnitOfWork> CreateUnitOfWork() = 0;

protected:
  ~UnitOfWorkFactory() = default;
};

} // namespace app