//
//  ycsbc.cc
//  YCSB-C
//
//  Created by Jinglei Ren on 12/19/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#include <cstring>
#include <future>
#include <iostream>
#include <string>
#include <vector>

#include "core/client.h"
#include "core/core_workload.h"
#include "core/timer.h"
#include "core/utils.h"
#include "db/db_factory.h"

using namespace std;

void UsageMessage(const char *command);
bool StrStartWith(const char *str, const char *pre);
string ParseCommandLine(int argc, const char *argv[], utils::Properties &props);

uint64_t DelegateClient(ycsbc::DB *db, ycsbc::CoreWorkload *wl,
                        const uint64_t num_ops, bool is_loading) {
  db->Init();
  ycsbc::Client client(*db, *wl);
  uint64_t oks = 0;
  for (uint64_t i = 0; i < num_ops; ++i) {
    if (i % 1000000 == 0) {
      cerr << "finished ops: " << i << "\r";
    }
    if (is_loading) {
      oks += client.DoInsert();
    } else {
      oks += client.DoTransaction();
    }
  }
  db->Close();
  return oks;
}

int main(const int argc, const char *argv[]) {
  utils::Properties props;
  string file_name = ParseCommandLine(argc, argv, props);

  ycsbc::DB *db = ycsbc::DBFactory::CreateDB(props);
  if (!db) {
    cout << "Unknown database name " << props["dbname"] << endl;
    exit(0);
  }

  ycsbc::CoreWorkload wl;
  wl.Init(props);

  const int num_threads = stoi(props.GetProperty("threadcount", "1"));
  const bool skipLoad = utils::StrToBool(props.GetProperty("skipLoad", "false"));

  // Loads data
  vector<future<uint64_t>> actual_ops;
  uint64_t total_ops =
      stoull(props[ycsbc::CoreWorkload::RECORD_COUNT_PROPERTY]);
  utils::Timer<double> timer;
  double duration;
  uint64_t sum;
  if (!skipLoad) {
    timer.Start();
    for (int i = 0; i < num_threads; ++i) {
      actual_ops.emplace_back(async(launch::async, DelegateClient, db, &wl,
                                    total_ops / num_threads, true));
    }
    assert((int)actual_ops.size() == num_threads);

    sum = 0;
    for (auto &n : actual_ops) {
      assert(n.valid());
      sum += n.get();
    }
    duration = timer.End();
    cerr << "# Loading records:\t" << sum << "\t kops:";
    cerr << total_ops / duration / 1000 << endl;
  } else {
    cerr << "# Skip loading record count:\t" << total_ops << endl;
  }

  // Peforms transactions
  actual_ops.clear();
  total_ops = stoi(props[ycsbc::CoreWorkload::OPERATION_COUNT_PROPERTY]);
  timer.Start();
  for (int i = 0; i < num_threads; ++i) {
    actual_ops.emplace_back(async(launch::async, DelegateClient, db, &wl,
                                  total_ops / num_threads, false));
  }
  assert((int)actual_ops.size() == num_threads);

  sum = 0;
  for (auto &n : actual_ops) {
    assert(n.valid());
    sum += n.get();
  }
  duration = timer.End();
  cerr << "# Transaction throughput (KTPS)" << endl;
  cerr << "DB:" << props["dbname"] << "\t Workload:" << file_name
       << "\t Threads:" << num_threads << "\t kops:";
  cerr << total_ops / duration / 1000 << endl;

  delete db;
}

string ParseCommandLine(int argc, const char *argv[],
                        utils::Properties &props) {
  int argindex = 1;
  string filename;
  while (argindex < argc && StrStartWith(argv[argindex], "-")) {
    if (strcmp(argv[argindex], "-threads") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("threadcount", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-db") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("dbname", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-host") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("host", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-port") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("port", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-slaves") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("slaves", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-P") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      filename.assign(argv[argindex]);
      ifstream input(argv[argindex]);
      try {
        props.Load(input);
      } catch (const string &message) {
        cout << message << endl;
        exit(0);
      }
      input.close();
      argindex++;
    } else if (strcmp(argv[argindex], "-dbfilename") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("dbfilename", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-skipLoad") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("skipLoad", argv[argindex]);
      argindex++;
    } else {
      cout << "Unknown option '" << argv[argindex] << "'" << endl;
      exit(0);
    }
  }

  if (argindex == 1 || argindex != argc) {
    UsageMessage(argv[0]);
    exit(0);
  }

  return filename;
}

void UsageMessage(const char *command) {
  cout << "Usage: " << command << " [options]" << endl;
  cout << "Options:" << endl;
  cout << "  -threads n: execute using n threads (default: 1)" << endl;
  cout << "  -db dbname: specify the name of the DB to use (default: basic)"
       << endl;
  cout << "  -P propertyfile: load properties from the given file. Multiple "
          "files can"
       << endl;
  cout << "                   be specified, and will be processed in the order "
          "specified"
       << endl;
}

inline bool StrStartWith(const char *str, const char *pre) {
  return strncmp(str, pre, strlen(pre)) == 0;
}
