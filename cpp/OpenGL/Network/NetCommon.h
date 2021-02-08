#pragma once

#include <thread>
#include <mutex>
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <random>

#include "AES.h"
#include "Sockets.h"


struct Coder {
  
  static inline char DELIM = char(1);
  
  static std::string encode(const std::vector<std::string>& data, bool checkForDelim=true) {
    std::string result;
    for (size_t i = 0;i<data.size();++i) {
      if (checkForDelim)
        result += removeDelim(data[i]);
      else
        result += data[i];
      if (i<data.size()-1) result += DELIM;
    }
    return result;
  }

  static std::vector<std::string> decode(const std::string& input) {
    std::vector<std::string> result;
    size_t start = 0;
    size_t delimPos = input.find(DELIM,start);
    
    while (delimPos != std::string::npos) {
      result.push_back( input.substr(start,delimPos-start) );
      start = delimPos+1;
      delimPos = input.find(DELIM,start);
    }
    
    result.push_back( input.substr(start) );
    
    return result;
  }

  static std::string removeDelim(std::string input) {
    size_t pos=0;
    while(pos<input.size()) {
      pos=input.find(DELIM,pos);
      if(pos==std::string::npos) break;
      input.replace(pos,1,"");
    }
    return input;
  }
};

std::string genHandshake(const std::string& iv, const std::string& key);
std::string getIVFromHandshake(const std::string& message, const std::string& key);
