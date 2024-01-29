#ifndef bash_cf_h
#define bash_cf_h

#include <string>
#include <sstream>
#include <fstream>
#include <chrono>
#include <iostream>

namespace atf
{

namespace cf
{

auto bash(const std::string &script, const std::string &costfile) {
  return [=](configuration &configuration) {
      std::stringstream ss;
      for (auto &tp : configuration) {
        ss << tp.first << "=" << tp.second << " ";
      }
      ss << script;
      auto ret = system(ss.str().c_str());
      if (ret != 0) {
        throw std::exception();
      }

      std::ifstream cost_in;
      cost_in.open(costfile, std::ifstream::in);
      size_t runtime = 0;
      ss.clear();
      if (!(cost_in >> runtime)) {
        std::cerr << "could not read runtime from costfile: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
      }
      cost_in.close();
   
      return runtime;
  };
}

} // namespace cf

} // namespace atf

#endif /* bash_cf_h */
