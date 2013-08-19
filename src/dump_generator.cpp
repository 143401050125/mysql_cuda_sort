#include <iostream>
#include <cstdio>
#include <string>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <time.h>
#include <iomanip>

#include "../lib/inih/cpp/INIReader.h"

using namespace std;

void generate_random_string(char *s, const int len)
{
    static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}

int main(int argc, char* argv[])
{
  char* tableName;
  int step = 100000;
  int numberOfLoops = 1;
  
  if (argc <= 1 || argc > 4) {
    cout<<"Usage: "<<argv[0]<<" <table_name>[ <step=10000>[ <numberOfLoops=1>]]"<<endl;
    return 1;
  }
  
  tableName = argv[1];
  
  if (argc >= 3) {
    step = atoi(argv[2]);
  }
  
  if (argc == 4) {
    numberOfLoops = atoi(argv[3]);
  }
  
  INIReader iniReader("../config/config.ini");
  if (iniReader.ParseError() < 0) {
      std::cout << "Can't load 'test.ini'\n";
      return 1;
  }
  
  string dbServer = iniReader.Get("database", "server", "localhost");
  string dbUser = iniReader.Get("database", "user", "root");
  string dbPassword = iniReader.Get("database", "password", "passwd");
  string dbDatabase = iniReader.Get("database", "database", "test");
  
  ostringstream fileName, command;
  fileName<<"/tmp/create_"<<tableName<<".sql";
  command<<"mysql -u"<<dbUser<<" -p"<<dbPassword<<" -h"<<dbServer<<" "<<dbDatabase<<" < "<<fileName.str().c_str();
  
  ofstream createFile;  
  createFile.open(fileName.str().c_str());
  
  createFile<<"DROP TABLE IF EXISTS `"<<tableName<<"`;\n"
            <<"CREATE TABLE `"<<tableName<<"` (\n"
            <<"`id` int(11) NOT NULL AUTO_INCREMENT,\n"
            <<"`text_col` varchar(255) DEFAULT NULL,\n"
            <<"`int_col` int(11) DEFAULT NULL,\n"
            <<"`double_col` double DEFAULT NULL,\n"
            <<"PRIMARY KEY (`id`)\n"
            <<") ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=latin1;\n"
  ;
  
  createFile.close();
  
  system(command.str().c_str());
  
  if (remove(fileName.str().c_str()) != 0) {
    cout<<"Error deleting file"<<endl;
  } else {
    cout<<"Table creation finished."<<endl;
  }

  int str_length;
  char* str_col = new char[255];
  int int_col;
  double double_col;
  
  fileName.str(string());
  fileName<<"/tmp/dump_"<<tableName<<"_"<<step<<".sql";
  
  command.str(string());
  command<<"mysql -u"<<dbUser<<" -p"<<dbPassword<<" -h"<<dbServer<<" "<<dbDatabase<<" < "<<fileName.str().c_str();
  
  ofstream myfile;
  
  for (int j = 0; j < numberOfLoops; j++) {
    myfile.open(fileName.str().c_str());
    
    myfile<<"INSERT INTO `"<<tableName<<"` (`text_col`, `int_col`, `double_col`) VALUES \n";
    
    for (int i = 0; i < step; i++) {
      str_length = (rand() % 255);
      
      generate_random_string(str_col, str_length);
      int_col = rand();
      double_col = rand() + (double)rand() / (double)RAND_MAX;

      myfile<<"('"<<str_col<<"', "<<int_col<<", "<< std::fixed<<std::setprecision(8)<<double_col<<")";
      
      if (i == (step - 1)) {
	myfile<<";\n";
      } else {
	myfile<<",\n";
      }
    }
    
    myfile.close();
    
    system(command.str().c_str());
    
    if (remove(fileName.str().c_str()) != 0) {
      cout<<"Error deleting file"<<endl;
    }
    
    cout<<step * (j + 1)<<" of "<<step * numberOfLoops<<" rows inserted"<<endl;
  }
  
  return 0;
}
