#pragma once
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <condition_variable>
#include <cstdlib>  // Include the C Standard Library for random number generation
#include <ctime>
#include <fstream>
#include <iostream>
#include <thread>
#include "curl_wrapper.h"
#include "mutex"
namespace connector {
class mutex_n {
 public:
  mutex_n() = default;
  void lock() {
    if (n == 0) {
      mt.lock();
    }
    n++;
  }
  void unlock() {
    if (n == 1) {
      mt.unlock();
    }
    if (n != 0)
      n--;
  }

 private:
  int n = 0;
  std::mutex mt;
};
class scope_lock_mutex {
 public:
  scope_lock_mutex() { b = false; }
  scope_lock_mutex(mutex_n* m) : scope_lock_mutex() {
    this->m = m;
    lock();
  }
  void lock() {
    if (b == false)
      m->lock();
    b = true;
  }
  void unlock() {
    if (b == true)
      m->unlock();
    b = false;
  }
  ~scope_lock_mutex() { unlock(); }

 private:
  mutex_n* m;
  bool b;
};
struct file {
  std::ofstream file;
  std::string name_file;
};
class Logger {
 private:
  std::vector<file> logFiles;
  std::vector<int> log_levels;
  std::string currentfile;
  mutex_n mt;
  std::string getCurrentTime() {
    std::time_t currentTime = std::time(nullptr);
    char* timeString = std::ctime(&currentTime);
    timeString[std::strlen(timeString) - 1] =
        '\0';  // Видаляємо символ нового рядка \n
    return std::string(timeString);
  }

 public:
  Logger() {}
  void close_file(file* file);
  void delete_file(std::string filename);
  int find_file(std::string filename);
  void set_current_file(std::string filename);
  void add_file(std::string filename);
  void add_log_level(int log_level);
  void delete_log_level(int log_level);
  bool find(int log_level);
  void log(int log_level, std::string name_log, std::string message);
  void log(int log_level,
           std::string filename,
           std::string name_log,
           std::string message);
  ~Logger() {
    scope_lock_mutex s_mt(&mt);
    for (int i = 0; i < logFiles.size(); i++) {
      close_file(&logFiles[i]);
    }
  }
};

extern Logger* connector_log;
void init_logg_connector(Logger* log);
std::string GetLocalIP();
struct return_data {
  t_json json_send;

  std::string server_hash;
  int respon_id = -1;
  void (*callback)(t_json jsonsend, t_json json_answer) = NULL;
};
void init_return_data(return_data* data);
class connector_manager;
struct handler {
  std::string nameobj = "";
  void (*callback)(connector_manager* conn, t_json json_req) = NULL;
};
struct event {
  void (*handling)(t_json json) = NULL;
  t_json json;
};
struct task {
  t_json json;
  bool note = false;
  bool empty = true;
};
void init_task(task* ev);

class manager_task {
 public:
  manager_task() {
    scope_lock_mutex s_ret(&mt);
    buffer.resize(25);
  }
  void add(t_json json);
  void show();
  bool check_id(std::string id);
  t_json get_task();
  void delete_notnote();
  void note_all();
  void delete_object(std::string id);
  ~manager_task() {}

 private:
  mutex_n mt;
  std::vector<task> buffer;
};
class manager_returns {
 public:
  manager_returns() {
    connector_log->log(0, "manager_returns|manager_returns",
                       "START FUNCTION\n");
    scope_lock_mutex s_ret(&mt_ret);
    returns.resize(25);
  }
  ~manager_returns() {}
  void add(return_data d);
  void call(int respon_id, std::string server_hash, t_json answer);
  bool check(int respon_id, std::string server_hash);
  void delete_object(return_data d);

 private:
  std::vector<return_data> returns;
  mutex_n mt_ret;
};
struct connection {
  std::string address;
  std::chrono::_V2::system_clock::time_point last_try;
  int count_try = 0;
  std::string respon_str;
  std::string server_hash = "1";
  std::string hash_worker = "1";
};

class connector_manager {
 private:
  time_t start_time;
  std::string local_ip;
  connector::manager_task m_task;
  connector::manager_returns m_returns;
  std::thread* th;
  std::thread* th_worker;
  connector::mutex_n mt_n;
  std::vector<handler> handlers;
  connector::t_json last_events;
  std::condition_variable cv;
  bool empty_thread = false;
  std::mutex mt;
  curl_wrapper cw;

  bool work_loop = false;
  std::vector<connector::connection> connections;
  void (*transfer)(connector::connector_manager* m_conn, t_json json);

 public:
  std::string name_client = "test";

 private:
  int find_conn(std::string address);

 public:
  connector_manager() {
    connector_log->log(0, "connector_manager|connector_manager",
                       "START FUNCTION\n");
    scope_lock_mutex s_mt(&mt_n);
    start_time = time(nullptr);
    local_ip = GetLocalIP();
    transfer = NULL;
  }
  void on();
  void set_transfer(void (*transfer)(connector::connector_manager* m_conn,
                                     t_json json));
  void off();
  void add_connection(std::string conn);
  void send(std::string address,
            t_json json,
            void (*callback)(t_json jsonsend, t_json jsonanswer));
  void send_response(t_json json_req, t_json json_res);
  void add_handler(std::string nameobj,
                   void (*callback)(connector_manager* conn, t_json json_req));
  t_json get_all_events();
  int start_event(t_json& json_event);
  int clear_event(t_json& json_event);
  int end_event(t_json& json_event);
  void worker_task();
  void getevent();
  void start_loop();
  void loop();
  void finish_loop();
  ~connector_manager() {
    connector_log->log(0, "connector_manager|~connector_manager",
                       "START FUNCTION\n");
    off();
  }
};
}  // namespace connector