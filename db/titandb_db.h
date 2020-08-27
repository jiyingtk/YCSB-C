//
// Created by 吴加禹 on 2019-07-17.
//

#ifndef YCSB_C_TITANDB_DB_H
#define YCSB_C_TITANDB_DB_H

#include <string>

#include "core/db.h"
#include "leveldb_config.h"
#include "titan/db.h"

namespace ycsbc {
class TitanDB : public DB {
 public:
  TitanDB(const char *dbfilename);
  int Read(const std::string &table, const std::string &key,
           const std::vector<std::string> *fields, std::vector<KVPair> &result);

  int Scan(const std::string &table, const std::string &key, int len,
           const std::vector<std::string> *fields,
           std::vector<std::vector<KVPair>> &result);

  int Insert(const std::string &table, const std::string &key,
             std::vector<KVPair> &values);

  int Update(const std::string &table, const std::string &key,
             std::vector<KVPair> &values);

  int Delete(const std::string &table, const std::string &key);

  void printStats();

  ~TitanDB();

 private:
  rocksdb::titandb::TitanDB *db_;
  unsigned noResult;
  bool nowal{false};
  rocksdb::Iterator *it{nullptr};
};
}  // namespace ycsbc

#endif  // YCSB_C_TITANDB_DB_H
