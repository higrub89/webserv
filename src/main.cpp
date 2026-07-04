#include <iostream>

int main(int argc, char* argv[]) {
  if (argc > 2) {
    std::cerr << "Usage: ./webserver [config_file]" << std::endl;
    return 1;
  }
  (void)argv;
}
