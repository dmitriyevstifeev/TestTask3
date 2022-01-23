#include "curl_easy.h"
#include "curl_ios.h"
#include <iostream>

using curl::curl_easy;
using curl::curl_easy_exception;
using curl::curlcpp_traceback;

int main(int argc, char *argv[]) {

  if (argc != 2) {
    std::cout << "Please, enter a website url!";
    return 1;
  }
  std::stringstream result;
  curl::curl_ios<std::stringstream> writer(result);
  curl_easy easy(writer);

  easy.add<CURLOPT_URL>(argv[1]);
  easy.add<CURLOPT_FOLLOWLOCATION>(1L);

  try {
    easy.perform();
  } catch (curl_easy_exception &error) {
    std::cerr << error.what();
    return 2;
  }
  std::cout << result.str();

  return 0;
}