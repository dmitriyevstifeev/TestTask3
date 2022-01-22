#include "json.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

const std::string PROGRAM_NAME_KEY{"progname"};
const std::string URL_NAME_KEY{"url"};
const std::string DEFAULT_URL{"https://www.ya.ru"};

bool IsFileExist(const std::string &file_name) {
  std::ifstream infile(file_name);
  return infile.good();
}

std::string GetFilePath(const char *file_path) {
  std::string result{file_path};
  size_t slash_pos = 0;
  for (size_t i = 0; i < result.size(); ++i) {
    if (result[i] == '/') {
      slash_pos = i;
    }
  }

  return result.substr(0, slash_pos);
}

std::pair<bool, int> RunProgram(const std::string &program_path,
               const std::string &program_name,
               const std::string &url,
               std::string &program_result) {
  int program_return_code = 0;

  //program_return_code = execl(program_path.c_str(), program_name.c_str(), url.c_str(), (char *) nullptr);

  int filedes[2];
  if (pipe(filedes) == -1) {
    std::cout << "can't create pipe";
    return {false, -1};
  }

  pid_t pid = fork();
  if (pid == -1) {
    std::cout << "can't fork process";
    return {false, -1};
  } else if (pid == 0) {
    while ((dup2(filedes[1], STDOUT_FILENO) == -1) && (errno == EINTR)) {}
    close(filedes[1]);
    close(filedes[0]);
    program_return_code = execl(program_path.c_str(), program_name.c_str(), url.c_str(), (char *) nullptr);
    std::cout << "can't exec process";
    return {false, -1};
  }
  close(filedes[1]);

  char buffer[4096];
  while (true) {
    ssize_t count = read(filedes[0], buffer, sizeof(buffer));
    if (count == -1) {
      if (errno == EINTR) {
        continue;
      } else {
        std::cout << "can't read program output";
        return {false, -1};
      }
    } else if (count == 0) {
      break;
    } else {
      program_result += buffer;
    }
  }
  close(filedes[0]);
  wait(0);

  return {true, program_return_code};
}

int main(int argc, char *argv[]) {

  if (argc != 2) {
    std::cout << "Please, enter a json!";
    return 1;
  }
  std::string str = argv[1];
  //nlohmann::json jdata;
  nlohmann::json jdata;
  try {
    jdata = nlohmann::json::parse(str);
  } catch (...) {
    std::cout << "Incorrect json!";
  }
  if (jdata.is_object()) {
    std::map<std::string, std::string> params;
    for (const auto &element: jdata.items()) {
      params.insert({element.key(), element.value()});
    }
    const auto found_key_it = params.find(PROGRAM_NAME_KEY);
    if (found_key_it == params.end()) {
      std::cout << "Progname wasn't found!";
      return 2;
    }
    std::string url;
    const auto found_url_it = params.find(URL_NAME_KEY);
    if (found_url_it == params.end()) {
      std::cout << "Url wasn't found! Url will be " << DEFAULT_URL << '\n';
      url = DEFAULT_URL;
    } else {
      url = found_url_it->second;
    }
    std::string program_name = found_key_it->second;

    bool program_exist = IsFileExist(program_name);
    if (!program_exist) {
      std::cout << "Program " << program_name << " wasn't found!";
      return 2;
    }
    std::string program_path = GetFilePath(argv[0]);
    program_path += '/';
    program_path += program_name;
    std::string program_result;
    auto program_code = RunProgram(program_path, program_name, url, program_result);

    if (program_code.first && program_code.second == 0){
      //TODO : Если есть результат то сериализация его в json
      std::cout << program_result;
    }


    //TODO : Makefile
  } else {
    std::cout << "Incorrect json!";
  }
}