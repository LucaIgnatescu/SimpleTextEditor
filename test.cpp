#include <iostream>
#include <sstream>

using std::string, std::cout;

int main() {
  string test = "ekjehfdkjdsahlkjfhaewoiufhdslck,nbfds";
  const unsigned short int chunkSize = 3;
  char *buffer = new char[chunkSize + 1];
  std::stringstream ss(test);
  while (ss.get(buffer, chunkSize)) {
    // buffer[chunkSize] ='\0';
    cout <<buffer<<"\n";
  }
  delete[] buffer;
  return 0;
}
