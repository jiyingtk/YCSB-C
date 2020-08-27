#ifndef YCSB_C_LEVELDB_H
#define YCSB_C_LEVELDB_H
#include "core/db.h"
#include <iostream>
#include <string>
#include "core/properties.h"
#include <leveldb/db.h>
using std::cout;
using std::endl;

namespace ycsbc
{
  class LevelDB : public DB
  {
  public:
    LevelDB(const char *dbfilename);
    virtual ~LevelDB();

    int Read(const std::string &table, const std::string &key,
             const std::vector<std::string> *fields,
             std::vector<KVPair> &result);

    int Scan(const std::string &table, const std::string &key,
             int len, const std::vector<std::string> *fields,
             std::vector<std::vector<KVPair>> &result);

    int Update(const std::string &table, const std::string &key,
               std::vector<KVPair> &values);

    int Insert(const std::string &table, const std::string &key,
               std::vector<KVPair> &values);

    int Delete(const std::string &table, const std::string &key);

  private:
    leveldb::DB *db_;
  };
} // namespace ycsbc
#endif