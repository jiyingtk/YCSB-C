#include "leveldb_db.h"

#include <cstring>

using namespace std;

namespace ycsbc {
LevelDB::LevelDB(const char *dbfilename) {
  leveldb::Options options;
  options.create_if_missing = true;
  options.compression = leveldb::kNoCompression;  // compression is disabled.
  leveldb::Status status = leveldb::DB::Open(options, dbfilename, &db_);
  if (!status.ok()) {
    cerr << "Can't open leveldb" << endl;
    exit(0);
  }
}

int LevelDB::Read(const string &table, const string &key,
                  const vector<string> *fields, vector<DB::KVPair> &result) {
  std::string value;
  leveldb::Status s = db_->Get(leveldb::ReadOptions(), key, &value);
  if (s.IsNotFound()) {
    cerr << "not found!" << endl;
    return DB::kOK;
  } else if (!s.ok()) {
    cerr << "read error" << endl;
    exit(0);
  }
  return DB::kOK;
}

int LevelDB::Insert(const string &table, const string &key,
                    vector<DB::KVPair> &values) {
  leveldb::Status s;
  int count = 0;
  // cout<<key<<endl;
  for (KVPair &p : values) {
    // cout<<p.second.length()<<endl;
    s = db_->Put(leveldb::WriteOptions(), key, p.second);
    count++;
    if (!s.ok()) {
      cerr << "insert error " << s.ToString() << endl;
      exit(0);
    }
  }
  if (count != 1) {
    cerr << "insert error " << s.ToString() << endl;
    exit(0);
  }
  return DB::kOK;
}

int LevelDB::Delete(const string &table, const string &key) {
  vector<DB::KVPair> values;
  return Insert(table, key, values);
}

int LevelDB::Scan(const string &table, const string &key, int len,
                  const vector<string> *fields,
                  vector<vector<DB::KVPair>> &result) {
  auto it = db_->NewIterator(leveldb::ReadOptions());
  it->Seek(key);
  int cnt = 0;
  int i;
  for (i = 0; i < len && it->Valid(); i++) {
    auto val = it->value();
    if (val.empty()) cnt++;
    it->Next();
  }
  if (cnt > 0) cout << "[scan] # of empty value:" << cnt << endl;
  if (i < len) {
    cout << "[scan] get " << i << " for length " << len << "." << endl;
    cerr << "[scan] get " << i << " for length " << len << "." << endl;
  }

  delete it;
  return DB::kOK;
}

int LevelDB::Update(const string &table, const string &key,
                    vector<DB::KVPair> &values) {
  return Insert(table, key, values);
}

LevelDB::~LevelDB() { delete db_; }

}  // namespace ycsbc