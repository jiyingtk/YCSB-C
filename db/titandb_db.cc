//
// Created by 吴加禹 on 2019-07-17.
//

#include "titandb_db.h"

#include <iostream>

#include "leveldb_config.h"
#include "rocksdb/cache.h"
#include "rocksdb/filter_policy.h"
#include "rocksdb/flush_block_policy.h"
#include "rocksdb/statistics.h"

using namespace std;

namespace ycsbc {
TitanDB::TitanDB(const char *dbfilename) : noResult(0) {
  // get leveldb config
  ConfigLevelDB config = ConfigLevelDB();
  int bloomBits = config.getBloomBits();
  size_t blockCache = config.getBlockCache();
  bool seekCompaction = config.getSeekCompaction();
  bool compression = config.getCompression();
  bool directIO = config.getDirectIO();
  size_t memtable = config.getMemtable();
  double gcRatio = config.getGCRatio();
  // uint64_t blockWriteSize = config.getBlockWriteSize();

  // set optionssc
  rocksdb::titandb::TitanOptions options;
  rocksdb::BlockBasedTableOptions bbto;
  options.create_if_missing = true;
  options.write_buffer_size = memtable;
  options.disable_background_gc = true;
  options.compaction_pri = rocksdb::kMinOverlappingRatio;
  options.max_bytes_for_level_base = memtable;
  options.target_file_size_base = 16 << 20;
  options.statistics = rocksdb::CreateDBStatistics();
  if (!compression) options.compression = rocksdb::kNoCompression;
  if (bloomBits > 0) {
    bbto.filter_policy.reset(rocksdb::NewBloomFilterPolicy(bloomBits));
  }
  options.min_gc_batch_size = 32 << 20;
  options.max_gc_batch_size = 64 << 20;
  options.max_sorted_runs = config.getMaxSortedRuns();
  bbto.block_cache = rocksdb::NewLRUCache(blockCache);
  options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(bbto));
  options.blob_file_target_size = 8 << 20;
  options.level_merge = config.getLevelMerge();
  options.range_merge = config.getRangeMerge();
  nowal = !options.level_merge;
  options.max_background_gc = config.getGCThreads();
  //   options.block_write_size = blockWriteSize;
  //   cerr << "block write size " << options.block_write_size <<
  //   endl;
  options.blob_file_discardable_ratio = gcRatio;
  if (options.level_merge) {
    // options.base_level_for_dynamic_level_bytes = 4;
    options.level_compaction_dynamic_level_bytes = true;
    // options.num_foreground_builders = 1;
    cerr << "set intro compaction true" << endl;
    // options.intra_compact_small_l0 = true;
  }

  //   cerr << "intro compaction " << options.intra_compact_small_l0
  //             << endl;

  // options.sep_before_flush = config.getSepBeforeFlush();
  if (config.getTiered())
    options.compaction_style = rocksdb::kCompactionStyleUniversal;
  options.max_background_jobs = config.getNumThreads();
  options.disable_auto_compactions = config.getNoCompaction();
  //   options.mid_blob_size = config.getMidThresh();
  options.min_blob_size = config.getSmallThresh();

  rocksdb::Status s =
      rocksdb::titandb::TitanDB::Open(options, dbfilename, &db_);
  if (!s.ok()) {
    cerr << "Can't open titandb " << dbfilename << endl;
    exit(0);
  }
  cerr << "dbname " << dbfilename << "\nbloom bits:" << bloomBits
       << "bits\ndirectIO:" << (bool)directIO
       << "\nseekCompaction:" << (bool)seekCompaction
       << "dynamic level:" << options.level_compaction_dynamic_level_bytes
       << endl;
}

int TitanDB::Read(const string &table, const string &key,
                  const vector<string> *fields, vector<KVPair> &result) {
  string value;
  static atomic<uint64_t> miss{0};
  rocksdb::Status s = db_->Get(rocksdb::ReadOptions(), key, &value);
  if (s.ok()) return DB::kOK;
  if (s.IsNotFound()) {
    noResult++;
    if (noResult % 1000 == 0) {
      cerr << noResult << endl;
    }
    return DB::kOK;
  } else {
    miss += 1;
    if (miss.load() % 1000 == 0) {
      cerr << "miss " << miss << "values" << endl;
      cerr << "last reason: " << s.ToString() << endl;
    }
    return DB::kOK;
  }
}

int TitanDB::Scan(const string &table, const string &key, int len,
                  const vector<string> *fields,
                  vector<vector<KVPair>> &result) {
  int i;
  int cnt = 0;

  auto it = db_->NewIterator(rocksdb::ReadOptions());
  it->Seek(key);

  string val;
  string k;
  for (i = 0; i < len && it->Valid(); i++) {
    // cnt++;
    k = it->key().ToString();
    val = it->value().ToString();
    it->Next();
    if (val.size() == 0) cnt++;
  }
  if (cnt > 0) {
    cerr << "[scan] missed " << cnt << " vals for length " << len << "."
         << endl;
    cerr << "[scan] missed " << cnt << " vals for length " << len << "."
         << endl;
  } else if (i < len) {
    cerr << "[scan] get " << i << " for length " << len << "." << endl;
    cerr << "[scan] get " << i << " for length " << len << "." << endl;
  }
  delete it;

  //   vector<string> keys;
  //   vector<string> vals;
  //   i = db_->Scan(rocksdb::ReadOptions(), key, len, keys, vals);

  return DB::kOK;
}

int TitanDB::Insert(const string &table, const string &key,
                    vector<KVPair> &values) {
  rocksdb::Status s;
  auto op = rocksdb::WriteOptions();
  op.disableWAL = nowal;
  for (KVPair &p : values) {
    s = db_->Put(op, key, p.second);
    if (!s.ok()) {
      cerr << "insert error\n";
      cerr << s.ToString() << endl;
      // exit(0);
    }
  }
  return DB::kOK;
}

int TitanDB::Update(const string &table, const string &key,
                    vector<KVPair> &values) {
  return Insert(table, key, values);
}

int TitanDB::Delete(const string &table, const string &key) {
  vector<DB::KVPair> values;
  return Insert(table, key, values);
}

void TitanDB::printStats() {
  string stats;
  cerr << ">> db stats <<" << endl;
  db_->GetProperty("rocksdb.stats", &stats);
  cerr << stats << endl;
}

TitanDB::~TitanDB() {
  printStats();
  delete db_;
}
}  // namespace ycsbc
