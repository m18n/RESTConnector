
#include "include/connector_manager.h"
static connector::Logger log_empty;
connector::Logger* connector::connector_log = &log_empty;
void connector::Logger::close_file(file* file) {
  scope_lock_mutex s_mt(&mt);
  if (file->file.is_open()) {
    file->file.close();
  }
}
void connector::Logger::delete_file(std::string filename) {
  scope_lock_mutex s_mt(&mt);
  for (int i = 0; i < logFiles.size(); i++) {
    if (logFiles[i].name_file == filename) {
      close_file(&logFiles[i]);
      logFiles.erase(logFiles.begin() + i);
      break;
    }
  }
}
int connector::Logger::find_file(std::string filename) {
  scope_lock_mutex s_mt(&mt);
  for (int i = 0; i < logFiles.size(); i++) {
    if (logFiles[i].name_file == filename) {
      return i;
    }
  }
  return -1;
}
void connector::Logger::set_current_file(std::string filename) {
  scope_lock_mutex s_mt(&mt);
  this->currentfile = filename;
}
void connector::Logger::add_file(std::string filename) {
  scope_lock_mutex s_mt(&mt);
  delete_file(filename);
  file logFile;
  logFile.name_file = filename;
  logFile.file.open(
      filename,
      std::ios::out | std::ios::app);  // Відкриваємо файл для логування
                                       // (додаємо до вже існуючого)
  if (!logFile.file.is_open()) {
    exit(1);
  }
  this->currentfile = filename;
  logFiles.push_back(std::move(logFile));
}
void connector::Logger::add_log_level(int log_level) {
  scope_lock_mutex s_mt(&mt);
  log_levels.push_back(log_level);
}
void connector::Logger::delete_log_level(int log_level) {
  scope_lock_mutex s_mt(&mt);

  for (int i = 0; i < log_levels.size(); i++) {
    if (log_levels[i] == log_level) {
      log_levels.erase(log_levels.begin() + i);
      return;
    }
  }
}
bool connector::Logger::find(int log_level) {
  for (int i = 0; i < log_levels.size(); i++) {
    if (log_level == log_levels[i]) {
      return true;
    }
  }
  return false;
}
void connector::Logger::log(int log_level,
                            std::string name_log,
                            std::string message) {
  scope_lock_mutex s_mt(&mt);
  std::string time = getCurrentTime();
  if (find(log_level) == false)
    return;
  if (logFiles.size() == 0) {
    std::cout << time << " - " << name_log << " - " << message << std::endl;
    return;
  }
  int index = find_file(currentfile);
  if (index == -1) {
    exit(1);
  }
  if (logFiles[index].file.is_open()) {
    logFiles[index].file << time << " - " << name_log << " - " << message
                         << std::endl;  // Записуємо повідомлення з часом у файл
  }
}
void connector::Logger::log(int log_level,
                            std::string filename,
                            std::string name_log,
                            std::string message) {
  scope_lock_mutex s_mt(&mt);
  this->currentfile = filename;
  log(log_level, name_log, message);
}

void connector::manager_task::add(t_json json) {
  connector_log->log(0, "manager_task|add", "START FUNCTION\n");
  // std::cout<<"ADD: "<<json.dump()<<"\n";
  //  for(int i=0;i<buffer_events.size();i++){
  //    if(!buffer_events[i].empty()&&buffer_events[i]["id"]==json["id"]){
  //      return;
  //    }
  //  }
  task ev;
  ev.json = json;
  scope_lock_mutex s_ret(&mt);

  // std::cout<<"$$$ADD\n";

  for (int i = 0; i < buffer.size(); i++) {
    if (buffer[i].empty == true) {
      ev.empty = false;
      buffer[i] = ev;
      connector_log->log(0, "manager_task|add", "ADD TO BUFFER\n");
      return;
    }
  }
  buffer.push_back(ev);
}
void connector::manager_task::show() {
  connector_log->log(0, "manager_task|show", "START FUNCTION\n");
  int c = 1;
  // std::cout<<"$$$SHOW\n";
  scope_lock_mutex s_ret(&mt);

  for (int i = 0; i < buffer.size(); i++) {
    if (!buffer[i].empty) {
      std::cout << "\n C: " << c << " JSON_OBJECT: " << buffer[i].json.dump()
                << "\n";
      c++;
    }
  }
}
bool connector::manager_task::check_id(std::string id) {
  connector_log->log(0, "manager_task|check_id", "START FUNCTION\n");
  // std::cout<<"$$$CHECK\n";
  scope_lock_mutex s_ret(&mt);
  for (int i = 0; i < buffer.size(); i++) {
    if (!buffer[i].empty && buffer[i].json["id"] == id) {
      return true;
    }
  }

  return false;
}
connector::t_json connector::manager_task::get_task() {
  connector_log->log(0, "manager_task|get_task", "START FUNCTION\n");
  // std::cout<<"$$$GET TASK\n";
  t_json t;
  scope_lock_mutex s_ret(&mt);

  for (int i = 0; i < buffer.size(); i++) {
    if (!buffer[i].empty) {
      t = buffer[i].json;

      break;
    }
  }

  return t;
}
void connector::manager_task::delete_notnote() {
  connector_log->log(0, "manager_task|delete_notnote", "START FUNCTION\n");
  // std::cout<<"$$$DELETE\n";
  scope_lock_mutex s_ret(&mt);

  for (int i = 0; i < buffer.size(); i++) {
    if (!buffer[i].empty && buffer[i].note == false) {
      init_task(&buffer[i]);
    }
  }
}

void connector::manager_task::note_all() {
  connector_log->log(0, "manager_task|not_all", "START FUNCTION\n");
  scope_lock_mutex s_ret(&mt);
  // std::cout<<"$$$NOT ALL\n";

  for (int i = 0; i < buffer.size(); i++) {
    if (!buffer[i].empty) {
      buffer[i].note = false;
    }
  }
}
void connector::manager_task::delete_object(std::string id) {
  connector_log->log(0, "manager_task|delete_object", "START FUNCTION\n");
  scope_lock_mutex s_ret(&mt);
  // std::cout<<"$$$DELETE OBJ\n";

  for (int i = 0; i < buffer.size(); i++) {
    if (!buffer[i].empty && buffer[i].json["id"] == id) {
      init_task(&buffer[i]);

      return;
    }
  }
}
int connector::manager_returns::get_empty_id() {
  connector_log->log(0, "manager_returns|get_empty_id", "START FUNCTION\n");
  scope_lock_mutex s_ret(&mt_ret);
  for (int i = 0; i < returns.size(); i++) {
    if (returns[i].respon_id == -1) {
      returns[i].respon_id = i;

      return i;
    }
  }
  return_data r;
  r.respon_id = returns.size();
  returns.push_back(r);
  return r.respon_id;
}
void connector::manager_returns::add(return_data d) {
  connector_log->log(0, "manager_returns|add", "START FUNCTION\n");
  scope_lock_mutex s_ret(&mt_ret);

  for (int i = 0; i < returns.size(); i++) {
    if (returns[i].respon_id == d.respon_id) {
      returns[i] = d;
    }
  }
}
void connector::manager_returns::call(int respon_id, t_json answer) {
  connector_log->log(0, "manager_returns|call", "START FUNCTION\n");
  scope_lock_mutex s_ret(&mt_ret);

  for (int i = 0; i < returns.size(); i++) {
    if (returns[i].respon_id == respon_id) {
      t_json j = returns[i].json_send;
      t_json a = answer;
      returns[i].callback(j, a);
      init_return_data(&returns[i]);
    }
  }
}
bool connector::manager_returns::check(int respon_id) {
  connector_log->log(0, "manager_returns|check", "START FUNCTION\n");
  scope_lock_mutex s_ret(&mt_ret);
  for (int i = 0; i < returns.size(); i++) {
    if (returns[i].respon_id == respon_id) {
      return true;
    }
  }

  return false;
}
void connector::manager_returns::delete_object(return_data d) {
  connector_log->log(0, "manager_returns|delete_object", "START FUNCTION\n");
  scope_lock_mutex s_ret(&mt_ret);

  for (int i = 0; i < returns.size(); i++) {
    if (returns[i].respon_id == d.respon_id) {
      returns[i].respon_id = -1;

      return;
    }
  }
}

void connector::init_logg_connector(Logger* log) {
  connector_log = log;
}
void connector::init_return_data(return_data* data) {
  data->callback = NULL;
  data->json_send.clear();
  data->respon_id = -1;
}
void connector::init_task(task* ev) {
  ev->json.clear();
  ev->empty = true;
  ev->note=false;
}
std::string connector::GetLocalIP() {
  struct ifaddrs* ifAddrStruct = nullptr;
  void* tmpAddrPtr = nullptr;
  std::string ipAddress;

  getifaddrs(&ifAddrStruct);

  struct ifaddrs* ifa = ifAddrStruct;
  while (ifa != nullptr) {
    if (ifa->ifa_addr->sa_family == AF_INET) {
      tmpAddrPtr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
      char addressBuffer[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
      if (std::string(ifa->ifa_name) == "lo") {
        ipAddress = std::string(addressBuffer);
      } else {
        ipAddress = std::string(addressBuffer);
        break;
      }
    }
    ifa = ifa->ifa_next;
  }

  freeifaddrs(ifAddrStruct);

  return ipAddress;
}

int connector::connector_manager::find_conn(std::string address) {
  connector_log->log(0, "connector_manager|find_conn", "START FUNCTION\n");
  scope_lock_mutex s_mt(&mt_n);
  for (int i = 0; i < connections.size(); i++) {
    if (connections[i].address == address) {
      return i;
    }
  }
  return -1;
}

void connector::connector_manager::on() {
  connector_log->log(0, "connector_manager|on", "START FUNCTION\n");
  scope_lock_mutex s_mt(&mt_n);
  std::cout << "NAME CLIENT: " << name_client << "\n";
  start_loop();
}
void connector::connector_manager::set_transfer(
    void (*transfer)(connector::connector_manager* m_conn, t_json json)) {
  connector_log->log(0, "connector_manager|set_transfer", "START FUNCTION\n");
  scope_lock_mutex s_mt(&mt_n);
  this->transfer = transfer;
}

void connector::connector_manager::off() {
  connector_log->log(0, "connector_manager|off", "START FUNCTION\n");
  scope_lock_mutex s_mt(&mt_n);
  finish_loop();
}
void connector::connector_manager::add_connection(std::string conn) {
  connector_log->log(0, "connector_manager|add_connection", "START FUNCTION\n");
  scope_lock_mutex s_mt(&mt_n);
  connection con;
  con.address = conn;
  connections.push_back(con);
}
void connector::connector_manager::send(std::string address,
                                        t_json json,
                                        void (*callback)(t_json jsonsend,
                                                         t_json jsonanswer)) {
  connector_log->log(0, "connector_manager|send", "START FUNCTION\n");
  scope_lock_mutex s_mt(&mt_n);
  int res_code = -1;
  std::string server_id;
  int id_returns = m_returns.get_empty_id();
  json["meta"]["$respon_id"] = id_returns;
  while (res_code < 0) {
    try {
      int http_code;
      t_json jsonres = cw.get_page_json(
          address,
          "/api/send/" + hash_worker + "/" + name_client + "/command/event",
          json.dump(), http_code);
      // std::cout<<"RES: "<<jsonres.dump()<<"\n";
      if (jsonres.contains("$error")) {
        connector_log->log(-1, "connector_manager|send",
                           "\nERROR SEND" + jsonres.dump() + "\n");
      } else {
        res_code = jsonres["$respon_id"];
      }

    } catch (const t_json::exception& e) {
    }
  }
  // std::cout<<"ID RESPON: "<<id<<"\n";
  return_data d;
  d.callback = callback;
  d.respon_id = id_returns;
  d.json_send = json;
  m_returns.add(d);
}
void connector::connector_manager::send_response(t_json json_req,
                                                 t_json json_res) {
  connector_log->log(0, "connector_manager|send_respone", "START FUNCTION\n");
  scope_lock_mutex s_mt(&mt_n);
  int res_code = -1;
  std::string server_id;

  t_json jdata;
  jdata["meta"] = json_req["meta"];
  jdata["data"] = json_res["data"];
  jdata["meta"]["$type_event"] = "res";
  jdata["meta"]["$type_obj"] = json_res["meta"]["$type_obj"];
  std::reverse(jdata["meta"]["$list_servers"].begin(),
               jdata["meta"]["$list_servers"].end());
  int index = find_conn(json_req["address"]);
  while (res_code < 0) {
    try {
      int http_code = 0;
      t_json jsonres = cw.get_page_json(
          json_req["address"],
          "/api/send/" + hash_worker + "/" + name_client + "/command/event",
          jdata.dump(), http_code);
      // std::cout<<"RES: "<<jsonres.dump()<<"\n";
      if (jsonres.contains("$error")) {
        connector_log->log(-1, "connector_manager|send_respone",
                           "\nERROR SEND" + jsonres.dump() + "\n");
      } else {
        res_code = jsonres["$respon_id"];
      }

    } catch (const t_json::exception& e) {
    }
  }
  std::cout << "RESPON: " << json_req["meta"]["$respon_id"].dump() << "\n";
  if (m_returns.check(json_req["meta"]["$respon_id"])) {
    int ret = -1;
    while (ret == -1) {
      ret = end_event(json_req);
    }
  }
}
void connector::connector_manager::add_handler(
    std::string nameobj,
    void (*callback)(connector_manager* conn, t_json json_req)) {
  connector_log->log(0, "connector_manager|add_handler", "START FUNCTION\n");
  scope_lock_mutex s_mt(&mt_n);
  handler h;
  h.callback = callback;
  h.nameobj = nameobj;
  handlers.push_back(h);
}
connector::t_json connector::connector_manager::get_all_events() {
  connector_log->log(0, "connector_manager|get_all_events", "START FUNCTION\n");
  scope_lock_mutex s_mt(&mt_n);
  t_json j = last_events;
  return std::move(j);
}
int connector::connector_manager::start_event(t_json& json_event) {
  connector_log->log(0, "connector_manager|start_event", "START FUNCTION\n");
  scope_lock_mutex s_mt(&mt_n);
  int id = -1;
  std::string server_id;

  std::string st = (std::string)json_event["id"];
  int index = find_conn(json_event["address"]);
  try {
    int res_code = 0;
    t_json jsonres = cw.get_page_json(
        json_event["address"],
        "/api/send/" + hash_worker + "/" + name_client +
            "/command/event/start/" + (std::string)json_event["id"],
        res_code);
    std::cout << "START EVENT: " << jsonres.dump()
              << " Hash ID:" << json_event["id"]
              << " RESPON ID: " << json_event["meta"]["$respon_id"] << "\n";
    if (!jsonres.empty()) {
      if (jsonres.contains("$error")) {
        connector_log->log(-1, "connector_manager|start_event",
                           "\nERROR SEND" + jsonres.dump() + "\n");
      } else {
        id = jsonres["$status"];
      }
    }

  } catch (const t_json::exception& e) {
  }

  return id;
}
int connector::connector_manager::clear_event(t_json& json_event) {
  connector_log->log(0, "connector_manager|clear_event", "START FUNCTION\n");
  scope_lock_mutex s_mt(&mt_n);
  int id = -1;
  std::string server_id;

  std::string st = (std::string)json_event["id"];
  int index = find_conn(json_event["address"]);
  try {
    int res_code = 0;
    t_json jsonres = cw.get_page_json(
        json_event["address"],
        "/api/send/" + hash_worker + "/" + name_client +
            "/command/event/clear/" + (std::string)json_event["id"],
        res_code);
    if (!jsonres.empty()) {
      if (jsonres.contains("$error")) {
        connector_log->log(-1, "connector_manager|clear_event",
                           "\nERROR SEND" + jsonres.dump() + "\n");
      } else {
        id = jsonres["$status"];
      }
    }

  } catch (const t_json::exception& e) {
  }

  return id;
}
int connector::connector_manager::end_event(t_json& json_event) {
  connector_log->log(0, "connector_manager|end_event", "START FUNCTION\n");
  scope_lock_mutex s_mt(&mt_n);
  int id = -1;
  std::string server_id;

  int index = find_conn(json_event["address"]);
  try {
    int res_code = 0;
    t_json jsonres = cw.get_page_json(
        json_event["address"],
        "/api/send/" + hash_worker + "/" + name_client +
            "/command/event/finish/" + (std::string)json_event["id"],
        res_code);
    std::cout << "END EVENT: " << jsonres.dump()
              << " Hash ID:" << json_event["id"]
              << " RESPON ID: " << json_event["meta"]["$respon_id"] << "\n";
    // std::cout<<"RES: "<<jsonres.dump()<<"\n";
    if (!jsonres.empty()) {
      if (jsonres.contains("$error")) {
        connector_log->log(-1, "connector_manager|end_event",
                           "\nERROR SEND" + jsonres.dump() + "\n");
      } else {
        id = jsonres["$status"];
      }
    }

  } catch (const t_json::exception& e) {
  }

  return id;
}
void connector::connector_manager::worker_task() {
  connector_log->log(0, "connector_manager|worker_task", "START FUNCTION\n");
  static int g = 1;
  int num_miss = 0;
  int middle_timer = 0;
  for (int i = 0; i < m_task.buffer.size(); i++) {
    if (m_task.buffer[i].empty == false) {
      int size_arr = m_task.buffer[i].json["meta"]["$list_servers"].size();

      // std::cout<<"LIST\n";
      // std::cout<<"JSON: "<<json.dump()<<" SIZE ARR: "<<size_arr<<"\n";
      if (m_task.buffer[i].json["meta"]["$list_servers"][size_arr - 1]
                               ["name"] == name_client) {
        if (m_task.buffer[i].json["meta"]["$type_event"] == "res") {
          // std::cout<<"RES\n";
          if (start_event(m_task.buffer[i].json) == 0) {
            m_returns.call(m_task.buffer[i].json["meta"]["$respon_id"], m_task.buffer[i].json);
            end_event(m_task.buffer[i].json);
          } else {
            num_miss++;
          }
        } else if (m_task.buffer[i].json["meta"]["$type_event"] == "req") {
          for (int j = 0; j < handlers.size(); j++) {
            // std::cout<<"REQ\n";
            if (handlers[j].nameobj ==
                m_task.buffer[i].json["meta"]["$type_obj"]) {
              std::cout << "START JSON: "
                        << m_task.buffer[i].json["meta"]["$respon_id"] << "\n";
              if (start_event(m_task.buffer[i].json) == 0) {
                std::cout << "CALLBACK: "
                          << m_task.buffer[i].json["meta"]["$respon_id"]
                          << "\n";
                handlers[j].callback(this, m_task.buffer[i].json);
                end_event(m_task.buffer[i].json);
              } else {
                num_miss++;
              }
              break;
            }
          }
        } else {
          continue;
        }
      } else {
        if (this->transfer != NULL) {
          transfer(this, m_task.buffer[i].json);
        }
      }
      init_task(&m_task.buffer[i]);
    }
  }
  
}
bool connector::compareByupdate(const connection* a, const connection* b) {
  return a->number_update < b->number_update;  // Сортування за зростанням віку
}

void connector::connector_manager::getevent() {
  connector_log->log(0, "connector_manager|getevent", "START FUNCTION\n");
  scope_lock_mutex s_mt(&mt_n);
  t_json json_temp;

  int col = 0;

  int col_try = 0;
  auto start_event = std::chrono::high_resolution_clock::now();
  auto end_event = std::chrono::high_resolution_clock::now();
  auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end_event -
                                                                   start_event);
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
      res_str = cw.get_page(
          connections[i].address,
          "/api/get/" + hash_worker + "/" + name_client + "/command/event",
          res_code);

      if (res_code == 200) {
        connections[i].respon_str = std::move(res_str);
        connections[i].count_try = 0;

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
  start_event = std::chrono::high_resolution_clock::now();
  std::vector<connection*> sort_connections;
  sort_connections.resize(connections.size());

  for (int i = 0; i < sort_connections.size(); i++) {
    sort_connections[i] = &connections[i];
  }
  std::sort(sort_connections.begin(), sort_connections.end(), compareByupdate);
  int max = 0;
  for (int i = 0; i < sort_connections.size(); i++) {
    if (sort_connections[i]->number_update > max) {
      max = sort_connections[i]->number_update;
    }
  }
  if (max > sort_connections.size() + 2) {
    for (int i = 0; i < sort_connections.size(); i++) {
      sort_connections[i]->number_update = 0;
    }
  }
  for (int i = 0; i < sort_connections.size(); i++) {
    if (sort_connections[i]->count_try != 0) {
      continue;
    }
    json_temp.clear();
    if (sort_connections[i]->respon_str[0] == '{') {
      json_temp = t_json::parse(sort_connections[i]->respon_str);
      if (json_temp.contains("$error")) {
        connector_log->log(-1, "connector_manager|getevent",
                           "\nERROR SEND" + json_temp.dump() + "\n");
      }
      continue;
    }
    size_t pos = 0;  // Знаходимо перше входження
    t_json id;
    t_json meta;
    t_json data;

    int n = 0;
    std::string str_size_json;
    str_size_json.resize(10);
    int int_size_json = 0;
    int res_size = sort_connections[i]->respon_str.size();
    char temp;
    int index_object = 0;
    bool jump = false;
    int g = 0;
    for (int j = 0; j < res_size; j++) {
      if (sort_connections[i]->respon_str[j] == '{') {
        memcpy(&str_size_json[0], &sort_connections[i]->respon_str[n], j - n);
        str_size_json[j - n] = '\0';
        int_size_json = atoi(str_size_json.c_str());
        if (!jump) {
          temp = sort_connections[i]->respon_str[j + int_size_json];
          sort_connections[i]->respon_str[j + int_size_json] = '\0';
          json_temp = t_json::parse(&sort_connections[i]->respon_str[j]);
          sort_connections[i]->respon_str[j + int_size_json] = temp;
          sort_connections[i]->respon_str[res_size] = '\0';
        }
        j += int_size_json;
        if (index_object == 0) {  // id
          id = json_temp;

          if (m_task.check_id(id["id"])) {
            jump = true;
          }

        } else if (index_object == 1) {  // meta
          meta = json_temp;
        } else if (index_object == 2) {  // data

          data = json_temp;
          index_object = -1;
          j++;
          json_temp.clear();
          if (!jump) {
            json_temp["id"] = id["id"];
            json_temp["meta"] = meta;
            json_temp["data"] = data;
            json_temp["address"] = sort_connections[i]->address;
            m_task.add(json_temp);
            end_event = std::chrono::high_resolution_clock::now();
            dur = std::chrono::duration_cast<std::chrono::milliseconds>(
                end_event - start_event);
            if (dur.count() > 20) {
              break;
            }
          } else {
            jump = false;
          }
        }
        n = j;
        index_object++;
      }
    }
    sort_connections[i]->number_update++;

    std::cout << "\nEVENT: " << sort_connections[i]->respon_str << "\n";
    m_task.delete_notnote();
    m_task.note_all();
    end_event = std::chrono::high_resolution_clock::now();
    dur = std::chrono::duration_cast<std::chrono::milliseconds>(end_event -
                                                                start_event);
    if (dur.count() > 20) {
      break;
    }
  }
  worker_task();
}
void connector::connector_manager::start_loop() {
  connector_log->log(0, "connector_manager|start_loop", "START FUNCTION\n");
  scope_lock_mutex s_mt(&mt_n);
  work_loop = true;
  th = new std::thread(&connector_manager::loop, this);
}
void connector::connector_manager::loop() {
  connector_log->log(0, "connector_manager|loop", "START FUNCTION\n");
  while (work_loop == true) {
    getevent();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}
void connector::connector_manager::finish_loop() {
  connector_log->log(0, "connector_manager|finish_loop", "START FUNCTION\n");
  scope_lock_mutex s_mt(&mt_n);
  work_loop = false;
  th->join();
  delete th;
}