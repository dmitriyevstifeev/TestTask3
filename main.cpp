#include "json.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

const std::string PROGRAM_NAME_KEY{"progname"};
const std::string CONTENT_NAME_KEY{"content"};
const std::string ERROR_NAME_KEY{"error"};
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

  int filedes[2];
  if (pipe(filedes) == -1) {
    std::cout << "can't create pipe";
    return {false, -1};
  }
  int filedes_err[2];
  if (pipe(filedes_err) == -1) {
    std::cout << "can't create pipe";
    return {false, -1};
  }
  pid_t pid = fork();
  if (pid == -1) {
    std::cout << "can't fork process";
    return {false, -1};
  } else if (pid == 0) {
    while ((dup2(filedes[1], STDOUT_FILENO) == -1) && (errno == EINTR)) {}
    while ((dup2(filedes_err[1], STDERR_FILENO) == -1) && (errno == EINTR)) {}
    close(filedes[1]);
    close(filedes[0]);
    close(filedes_err[1]);
    close(filedes_err[0]);
    execl(program_path.c_str(), program_name.c_str(), url.c_str(), (char *) nullptr);
    std::cout << "can't exec process";
    return {false, -1};
  }
  close(filedes[1]);
  close(filedes_err[1]);

  int status;
  if (waitpid(pid, &status,0) < 0)
  {
    std::cout << "can't call wait";
  } else
  {
    program_return_code = WEXITSTATUS(status);
  }

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
      program_result += std::string(buffer, count);
    }
  }
  close(filedes[0]);

  while (true) {
    ssize_t count = read(filedes_err[0], buffer, sizeof(buffer));
    if (count == -1) {
      if (errno == EINTR) {
        continue;
      } else {
        std::cout << "can't read program error output";
        return {false, -1};
      }
    } else if (count == 0) {
      break;
    } else {
      program_result += std::string(buffer, count);
    }
  }
  close(filedes_err[0]);

  return {true, program_return_code};
}

nlohmann::json CreateJsonResult(const std::string &program_name,
                                std::string program_result,
                                bool program_result_is_error,
                                int code = 0) {
  std::map<std::string, std::string> result_map;
  result_map.insert({PROGRAM_NAME_KEY, program_name});
  if (!program_result_is_error) {
    result_map.insert({CONTENT_NAME_KEY, std::move(program_result)});
  } else {
    result_map.insert({CONTENT_NAME_KEY, std::string()});
    if (program_result.empty()) {
      std::string error{"No error description, only error code #<"};
      error += std::to_string(code);
      error += "> was reported";
      result_map.insert({ERROR_NAME_KEY, error});
    } else {
      result_map.insert({ERROR_NAME_KEY, std::move(program_result)});
    }
  }

  return nlohmann::json(result_map);

}
int main(int argc, char *argv[]) {

  if (argc != 2) {
    std::cout << "Please, enter a json!";
    return 1;
  }
  std::string str = argv[1];
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

    if (program_code.first && program_code.second == 0) {
      std::cout << CreateJsonResult(program_name, program_result, false);
    } else if (program_code.first && program_code.second != 0) {
      std::cout << CreateJsonResult(program_name, program_result, true, program_code.second);
    }


    //TODO : Makefile
  } else {
    std::cout << "Incorrect json!";
  }
}