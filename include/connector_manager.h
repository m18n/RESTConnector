#pragma once
#include "curl_wrapper.h"
#include "mutex"
#include <arpa/inet.h>
#include <chrono>
#include <condition_variable>
#include <cstdlib> // Include the C Standard Library for random number generation
#include <ctime>
#include <ifaddrs.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
namespace connector {

std::string GetLocalIP();
struct return_data {
  t_json json_send;

  std::string server_hash;
  int respon_id = -1;
  void (*callback)(t_json jsonsend, t_json json_answer) = NULL;
};
void init_return_data(return_data *data);
class connector_manager;
struct handler {
  std::string nameobj = "";
  void (*callback)(connector_manager *conn, t_json json_req) = NULL;
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
void init_task(task *ev);
class manager_task {
public:
  manager_task() { buffer.resize(25); }
  void add(t_json json) {
    //std::cout<<"ADD: "<<json.dump()<<"\n";
    // for(int i=0;i<buffer_events.size();i++){
    //   if(!buffer_events[i].empty()&&buffer_events[i]["id"]==json["id"]){
    //     return;
    //   }
    // }
    task ev;
    ev.json = json;
    ev.note = true;
    // std::cout<<"$$$ADD\n";
    m.lock();
    for (int i = 0; i < buffer.size(); i++) {
      if (buffer[i].empty == true) {
        ev.empty=false;
        buffer[i] = ev;
        m.unlock();
        return;
      }
    }
    buffer.push_back(ev);
    m.unlock();
  }
  void show() {
    int c = 1;
    // std::cout<<"$$$SHOW\n";
    m.lock();
    for (int i = 0; i < buffer.size(); i++) {
      if (!buffer[i].empty) {
        std::cout << "\n C: " << c << " JSON_OBJECT: " << buffer[i].json.dump()
                  << "\n";
        c++;
      }
    }
    m.unlock();
  }
  bool check_id(std::string id) {
    // std::cout<<"$$$CHECK\n";
    m.lock();
    for (int i = 0; i < buffer.size(); i++) {
      if (!buffer[i].empty && buffer[i].json["id"] == id) {
        buffer[i].note = true;
        m.unlock();
        return true;
      }
    }
    m.unlock();
    return false;
  }
  t_json get_task() {
    // std::cout<<"$$$GET TASK\n";
    t_json t;
    m.lock();
    for (int i = 0; i < buffer.size(); i++) {
      if (!buffer[i].empty) {
        t = buffer[i].json;

        break;
      }
    }
    m.unlock();
    return t;
  }
  void delete_notnote() {
    // std::cout<<"$$$DELETE\n";
    m.lock();
    for (int i = 0; i < buffer.size(); i++) {
      if (!buffer[i].empty && buffer[i].note == false) {
        init_task(&buffer[i]);
      }
    }
    m.unlock();
  }

  void note_all() {
    // std::cout<<"$$$NOT ALL\n";
    m.lock();
    for (int i = 0; i < buffer.size(); i++) {
      if (!buffer[i].empty) {
        buffer[i].note = false;
      }
    }
    m.unlock();
  }
  void delete_object(std::string id) {
    // std::cout<<"$$$DELETE OBJ\n";
    m.lock();
    for (int i = 0; i < buffer.size(); i++) {
      if (!buffer[i].empty && buffer[i].json["id"] == id) {
        init_task(&buffer[i]);
        m.unlock();
        return;
      }
    }
    m.unlock();
  }
  ~manager_task() {}

private:
  std::mutex m;
  std::vector<task> buffer;
};
class manager_returns {
public:
  manager_returns() { returns.resize(25); }
  ~manager_returns() {}
  void add(return_data d) {
    mt_ret.lock();
    for (int i = 0; i < returns.size(); i++) {
      if (returns[i].respon_id == -1) {
        returns[i] = d;
        mt_ret.unlock();
        return;
      }
    }

    returns.push_back(d);
    mt_ret.unlock();
  }
  void call(int respon_id, std::string server_hash, t_json answer) {
    mt_ret.lock();
    for (int i = 0; i < returns.size(); i++) {
      if (returns[i].respon_id == respon_id &&
          returns[i].server_hash == server_hash) {
        returns[i].callback(returns[i].json_send, answer);
        init_return_data(&returns[i]);
      }
    }
    mt_ret.unlock();
  }
  bool check(int respon_id, std::string server_hash) {
    mt_ret.lock();
    for (int i = 0; i < returns.size(); i++) {
      if (returns[i].respon_id == respon_id &&
          returns[i].server_hash == server_hash) {
        mt_ret.unlock();
        return true;
      }
    }
    mt_ret.unlock();
    return false;
  }
  void delete_object(return_data d) {
    mt_ret.lock();
    for (int i = 0; i < returns.size(); i++) {
      if (returns[i].respon_id == d.respon_id) {
        returns[i].respon_id = -1;
        mt_ret.unlock();
        return;
      }
    }
    mt_ret.unlock();
  }

private:
  std::vector<return_data> returns;
  std::mutex mt_ret;
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
  manager_task m_task;
  manager_returns m_returns;
  std::thread *th;
  std::thread *th_worker;

  std::vector<handler> handlers;
  t_json last_events;
  std::condition_variable cv;
  bool empty_thread = false;
  std::mutex mt;
  curl_wrapper cw;

  bool work_loop = false;
  std::vector<connection> connections;
  void (*transfer)(connector::connector_manager *m_conn, t_json json);
public:
  std::string name_client="test";
private:

  int find_conn(std::string address) {
    for (int i = 0; i < connections.size(); i++) {
      if (connections[i].address == address) {
        return i;
      }
    }
    return -1;
  }
  void get_myid(std::string address, bool loop) {
    std::string hash_worker = "";
    std::string server_hash;
    int index = find_conn(address);
    t_json json;
    
    json["$time"] = std::to_string(start_time);
    json["$ip"] = local_ip;
    if (loop == false) {
      try {
        int res_code = 0;
        t_json jcode =
            cw.get_page_json(address, "/api/client/" + name_client + "/getid",
                             json.dump(), res_code);
        if (!jcode.empty()) {
          hash_worker = jcode["$hash_worker"];
          server_hash = jcode["$server_hash"];
        }
      } catch (const t_json::exception &e) {
        // std::cout<<"ERROR: "<< e.what()<<"\n";
      }
    } else {
      while (hash_worker == "") {
        try {
          int res_code = 0;
          t_json jcode =
              cw.get_page_json(address, "/api/client/" + name_client + "/getid",
                               json.dump(), res_code);
          if (!jcode.empty()) {
            hash_worker = jcode["$hash_worker"];
            server_hash = jcode["$server_hash"];
          }
          if (hash_worker == "") {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
          }
        } catch (const t_json::exception &e) {
          // std::cout<<"ERROR: "<< e.what()<<"\n";
        }
      }
    }

    connections[index].hash_worker = hash_worker;
    connections[index].server_hash = server_hash;
    std::cout << "SERVER ID: " << server_hash << " ID: " << hash_worker << "\n";
  }

public:
  connector_manager() {
    start_time = time(nullptr);
    local_ip = GetLocalIP();
    transfer = NULL;
    
  }
  void on() {
    std::cout<<"NAME CLIENT: "<<name_client<<"\n";
    for (int i = 0; i < connections.size(); i++) {
      get_myid(connections[i].address, true);
    }
    start_loop();
  }
  void set_transfer(void (*transfer)(connector::connector_manager *m_conn,
                                     t_json json)) {
    this->transfer = transfer;
  }
  void exit(std::string address) {
    int index = find_conn(address);
    std::string code = "";
    
    while (code == "") {
      try {
        int res_code = 0;
        code = cw.get_page(address,
                           "/api/client/" + connections[index].server_hash +
                               "/" + name_client + "/command/" +
                               connections[index].hash_worker + "/exit",
                           res_code);
      } catch (const t_json::exception &e) {
      }
    }
  }
  void off() {
    finish_loop();
    for (int i = 0; i < connections.size(); i++) {
      exit(connections[i].address);
    }
  }
  // std::string get_auth_code() {
  //   std::string code = "0";
  //   while(code=="0"){
  //       try {

  //           json jcode =
  //           cw.get_page_json("/api/telegram/command/"+std::to_string(my_id)+"/getauthcode");
  //           code = jcode["code"];
  //       } catch(const json::exception &e){
  //       }
  //   }
  //   if(code=="-1"){
  //     this->~connector_manager();
  //     exit(1);
  //   }
  //   return code;
  // }
  void add_connection(std::string conn) {
    connection con;
    con.address = conn;
    connections.push_back(con);
  }
  void send(std::string address, t_json json,
            void (*callback)(t_json jsonsend, t_json jsonanswer)) {
    int id = -1;
    std::string server_id;
   
    int index = find_conn(address);
    while (id == -1) {
      try {
        int res_code = 0;
        t_json jsonres = cw.get_page_json(
            address,
            "/api/send/" + connections[index].server_hash + "/" + name_client +
                "/command/" + connections[index].hash_worker + "/event",
            json.dump(), res_code);
        // std::cout<<"RES: "<<jsonres.dump()<<"\n";
        if (jsonres.contains("$error")) {
          get_myid(address, true);
        } else {
          id = jsonres["$respon_id"];
        }
        if (id == -1)
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
      } catch (const t_json::exception &e) {
      }
    }
    // std::cout<<"ID RESPON: "<<id<<"\n";
    return_data d;
    d.callback = callback;
    d.respon_id = id;
    d.json_send = json;
    d.server_hash = connections[index].server_hash;
    m_returns.add(d);
  }
  void send_response(t_json json_req, t_json json_res) {
    int id = -1;
    std::string server_id;
    
    t_json jdata;
    jdata["meta"] = json_req["meta"];
    jdata["data"] = json_res["data"];
    jdata["meta"]["$type_event"] = "res";
    jdata["meta"]["$type_obj"] = json_res["meta"]["$type_obj"];
    std::reverse(jdata["meta"]["$list_servers"].begin(),
                 jdata["meta"]["$list_servers"].end());
    int index = find_conn(json_req["address"]);
    while (id == -1) {
      try {
        int res_code = 0;
        t_json jsonres = cw.get_page_json(
            json_req["address"],
            "/api/send/" + connections[index].server_hash + "/" + name_client +
                "/command/" + connections[index].hash_worker + "/event",
            jdata.dump(), res_code);
        // std::cout<<"RES: "<<jsonres.dump()<<"\n";
        if (jsonres.contains("$error")) {
          get_myid(json_req["address"], true);
        } else {
          id = jsonres["$respon_id"];
        }
        if (id == -1)
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
      } catch (const t_json::exception &e) {
      }
    }
    std::cout << "RESPON: " << json_req["meta"]["$respon_id"].dump() << "\n";
    if (m_returns.check(json_req["meta"]["$respon_id"],
                        json_req["meta"]["$server_hash"])) {
      int ret = -1;
      while (ret == -1) {
        ret = end_event(json_req);
      }
    }
  }
  void add_handler(std::string nameobj,
                   void (*callback)(connector_manager *conn, t_json json_req)) {
    handler h;
    h.callback = callback;
    h.nameobj = nameobj;
    handlers.push_back(h);
  }
  t_json get_all_events() { return last_events; }
  int start_event(t_json &json_event) {
    int id = -1;
    std::string server_id;
    
    std::string st = (std::string)json_event["id"];
    int index = find_conn(json_event["address"]);
    try {
      int res_code = 0;
      t_json jsonres = cw.get_page_json(
          json_event["address"],
          "/api/send/" + connections[index].server_hash + "/" + name_client +
              "/command/" + connections[index].hash_worker + "/event/start/" +
              (std::string)json_event["id"],
          res_code);
      std::cout << "START EVENT: " << jsonres.dump()
                << " Hash ID:" << json_event["id"]
                << " RESPON ID: " << json_event["meta"]["$respon_id"] << "\n";
      if (!jsonres.empty()) {
        if (jsonres.contains("$error")) {
          get_myid(json_event["address"], false);
        } else {
          id = jsonres["$status"];
        }
      }

    } catch (const t_json::exception &e) {
    }

    return id;
  }
  int clear_event(t_json &json_event) {
    int id = -1;
    std::string server_id;
    
    std::string st = (std::string)json_event["id"];
    int index = find_conn(json_event["address"]);
    try {
      int res_code = 0;
      t_json jsonres = cw.get_page_json(
          json_event["address"],
          "/api/send/" + connections[index].server_hash + "/" + name_client +
              "/command/" + connections[index].hash_worker + "/event/clear/" +
              (std::string)json_event["id"],
          res_code);
      if (!jsonres.empty()) {
        if (jsonres.contains("$error")) {
          get_myid(json_event["address"], false);
        } else {
          id = jsonres["$status"];
        }
      }

    } catch (const t_json::exception &e) {
    }

    return id;
  }
  int end_event(t_json &json_event) {
    int id = -1;
    std::string server_id;
    
    int index = find_conn(json_event["address"]);
    try {
      int res_code = 0;
      t_json jsonres = cw.get_page_json(
          json_event["address"],
          "/api/send/" + connections[index].server_hash + "/" + name_client +
              "/command/" + connections[index].hash_worker + "/event/finish/" +
              (std::string)json_event["id"],
          res_code);
      std::cout << "END EVENT: " << jsonres.dump()
                << " Hash ID:" << json_event["id"]
                << " RESPON ID: " << json_event["meta"]["$respon_id"] << "\n";
      // std::cout<<"RES: "<<jsonres.dump()<<"\n";
      if (!jsonres.empty()) {
        if (jsonres.contains("$error")) {
          get_myid(json_event["address"], false);
        } else {
          id = jsonres["$status"];
        }
      }

    } catch (const t_json::exception &e) {
    }

    return id;
  }
  void worker_task() {
    auto start_time = std::chrono::high_resolution_clock::now();
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    while (work_loop == true) {
      t_json json = m_task.get_task();

      if (json.empty()) {
        end_time = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        if (duration.count() >= 1000) {
          std::cout << "DURACTION: " << duration.count() << "\n";
          std::unique_lock<std::mutex> lock(mt);
          empty_thread = false;
          cv.wait(lock, [this] { return this->empty_thread; });
          lock.unlock();
        }
        continue;
      }
      bool job = false;
      start_time = std::chrono::high_resolution_clock::now();
      int size_arr = json["meta"]["$list_servers"].size();
      
      //std::cout<<"LIST\n";
      //std::cout<<"JSON: "<<json.dump()<<" SIZE ARR: "<<size_arr<<"\n";
      if (json["meta"]["$list_servers"][size_arr - 1]["name"] == name_client) {
        if (json["meta"]["$type_event"] == "res") {
          //std::cout<<"RES\n";
          if (start_event(json) == 0) {
            m_returns.call(json["meta"]["$respon_id"],
                           json["meta"]["$server_hash"], json);
            end_event(json);
          }
        } else if (json["meta"]["$type_event"] == "req") {
          for (int j = 0; j < handlers.size(); j++) {
            //std::cout<<"REQ\n";
            if (handlers[j].nameobj == json["meta"]["$type_obj"]) {
              std::cout << "START JSON: " << json["meta"]["$respon_id"] << "\n";
              if (start_event(json) == 0) {
                std::cout << "CALLBACK: " << json["meta"]["$respon_id"] << "\n";
                handlers[j].callback(this, json);
                end_event(json);
              }
              break;
            }
          }
        } else {
          continue;
        }
      } else {
        
          if (this->transfer != NULL) {
            transfer(this, json);
          }

          
      }

      m_task.delete_object(json["id"]);
    }
  }
  void getevent() {

    t_json json_temp;
    
    int col = 0;

    int col_try = 0;
    auto start_event = std::chrono::high_resolution_clock::now();
    auto end_event = std::chrono::high_resolution_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_event - start_event);
    bool all_successfull = false;
    // for (int i = 0; i < connections.size(); i++) {
    //   connections[i].count_try = 0;
    // }
    while (true) {
      if (dur.count() > 100 || all_successfull == true) {
        break;
      }
      all_successfull = true;
      for (int i = 0; i < connections.size(); i++) {

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - connections[i].last_try);

        if (connections[i].count_try >= 20) {
          if (duration.count() < 1000) {
            continue;
          }
        } else if (connections[i].count_try > 5) {
          if (duration.count() < 100) {
            continue;
          }
        }
        int res_code = 0;
        std::string res_str = "";
        res_str =
            cw.get_page(connections[i].address,
                        "/api/get/" + connections[i].server_hash + "/" + name_client +
                            "/command/" + connections[i].hash_worker + "/event",
                        res_code);

        if (res_code == 200) {
          connections[i].respon_str = std::move(res_str);
          connections[i].count_try=0;

        } else {
          connections[i].count_try++;
          all_successfull = false;
        }
        connections[i].last_try = std::chrono::high_resolution_clock::now();
      }
      col_try++;
      end_event = std::chrono::high_resolution_clock::now();
      dur = std::chrono::duration_cast<std::chrono::milliseconds>(end_event -
                                                                  start_event);
    }
    for (int i = 0; i < connections.size(); i++) {
      if (connections[i].count_try != 0) {
        continue;
      }
      json_temp.clear();
      if (connections[i].respon_str[0] == '{') {
        json_temp = t_json::parse(connections[i].respon_str);
        if (json_temp.contains("$error")) {
          get_myid(connections[i].address, false);
        }
        continue;
      }
      size_t pos = 0; // Знаходимо перше входження
      t_json id;
      t_json meta;
      t_json data;

      int n = 0;
      std::string str_size_json;
      str_size_json.resize(10);
      int int_size_json = 0;
      int res_size = connections[i].respon_str.size();
      char temp;
      int index_object = 0;
      bool jump = false;

      for (int j = 0; j < res_size; j++) {
        if (connections[i].respon_str[j] == '{') {
          memcpy(&str_size_json[0], &connections[i].respon_str[n], j - n);
          str_size_json[j - n] = '\0';
          int_size_json = atoi(str_size_json.c_str());
          if (!jump) {
            temp = connections[i].respon_str[j + int_size_json];
            connections[i].respon_str[j + int_size_json] = '\0';
            json_temp = t_json::parse(&connections[i].respon_str[j]);
            connections[i].respon_str[j + int_size_json] = temp;
            connections[i].respon_str[res_size] = '\0';
          }
          j += int_size_json;
          if (index_object == 0) { // id
            id = json_temp;

            if (m_task.check_id(id["id"])) {
              jump = true;
            }

          } else if (index_object == 1) { // meta
            meta = json_temp;
          } else if (index_object == 2) { // data

            data = json_temp;
            index_object = -1;
            j++;
            json_temp.clear();
            if (!jump) {
              json_temp["id"] = id["id"];
              json_temp["meta"] = meta;
              json_temp["data"] = data;
              json_temp["address"] = connections[i].address;
              m_task.add(json_temp);

            } else {
              jump = false;
            }
          }
          n = j;
          index_object++;
        }
      }

      m_task.delete_notnote();
      m_task.note_all();
      std::cout << "\nEVENT: " << connections[i].respon_str << "\n";
      mt.lock();
      if (empty_thread == false) {
        empty_thread = true;
        cv.notify_all();
      }
      mt.unlock();
    }
  }
  void start_loop() {
    work_loop = true;
    th = new std::thread(&connector_manager::loop, this);
    th_worker = new std::thread(&connector_manager::worker_task, this);
  }
  void loop() {
    while (work_loop == true) {
      getevent();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
  void finish_loop() {
    work_loop = false;
    th->join();
    th_worker->join();
    delete th;
    delete th_worker;
  }
  ~connector_manager() { off(); }
};
} // namespace connector