#include <fstream>
#include <string>
#include <regex>
#include <iostream>


using namespace std;

int main (int argc, char* argv[])
{
  const char* path = argv[1];
  const char* dest = argv[2];
  
  ifstream infile; 
  infile.open(path);  
     
  ofstream myfile;
  myfile.open(dest);

  // output help if insufficient params
  if(argc > 3) {
    cerr << "source destination " << endl;
    return 1;
  }

  long long  id;
  
  cerr << "running..." << endl;
  
    while (infile >>  id)
    { 
        myfile << "0 " << id << " 1" << endl; 
    }

  infile.close();
  myfile.close();
  return 0;
}
