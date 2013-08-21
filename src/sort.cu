#include <iostream>
#include <string>
#include <sstream>
#include <stdio.h>
#include <mysql.h>
#include <cstring>
#include <stdlib.h>
#include <sys/time.h>

#include <thrust/host_vector.h>
#include <thrust/device_vector.h>
#include <thrust/sort.h>

#include "../lib/inih/cpp/INIReader.h"

class Sortable
{
  public:
    int originalIndex;
    int sortColumn;
    
    __host__ __device__ Sortable() {};
    
    __host__ __device__ Sortable(int _originalIndex, int _sortColumn)
    {
      originalIndex = _originalIndex;
      sortColumn = _sortColumn;      
    }
};

class Sorter
{
  public:
    __host__ __device__ bool operator() (const Sortable& ls, const Sortable& rs) const
    {
      return ls.sortColumn < rs.sortColumn;
    }
};

void printElapsedTime(const timeval& stopTime, const timeval& startTime, const char* processorType)
{
  int elapsedSeconds = stopTime.tv_sec - startTime.tv_sec;
  double elapsedMilliSeconds;
  
  if (stopTime.tv_usec < startTime.tv_usec) {
    elapsedSeconds -= 1;
    elapsedMilliSeconds = (1000 + (stopTime.tv_usec / 1000)) - (startTime.tv_usec / 1000);
  } else {
    elapsedMilliSeconds = (stopTime.tv_usec / 1000) - (startTime.tv_usec / 1000);
  }
  
  std::cout<<elapsedMilliSeconds<<std::endl;
  
  double elapsedTime = elapsedSeconds + (elapsedMilliSeconds / 1000); 
  
  std::cout<<processorType<<" query execution and sorting took: "<<elapsedTime<<" seconds"<<std::endl;  
}

void verifyResult(const thrust::host_vector<MYSQL_ROW>& h_vec, int sortColumnIndex, const char* processorType)
{
  for (int i = 0; i < h_vec.size(); i++) {
    if (i < (h_vec.size() - 2) && atoi(h_vec[i][sortColumnIndex]) > atoi(h_vec[i + 1][sortColumnIndex])) {
      std::cout<<"Error in "<<processorType<<" sorting at index "<<i<<". "<<h_vec[i][sortColumnIndex]<<" should be less than "<<h_vec[i + 1][sortColumnIndex]<<std::endl;
    }
  }
}

void gpuSort(const char *dbServer, const char *dbUser, const char *dbPassword, const char *dbDatabase, std::string query, int sortColumnIndex)
{
  MYSQL *conn;
  MYSQL_RES *result;
  MYSQL_ROW row;
  int num_fields;
  
  timeval startTime, stopTime;

  thrust::host_vector<Sortable> h_vec;

  thrust::host_vector<MYSQL_ROW> h_row_vec, h_row_vec_temp;
  
  conn = mysql_init(NULL);
  mysql_real_connect(conn, dbServer, dbUser, dbPassword, dbDatabase, 0, NULL, 0);

  gettimeofday(&startTime, NULL);
  
  mysql_query(conn, query.c_str());
  result = mysql_store_result(conn);

  num_fields = mysql_num_fields(result);
  if (num_fields <= sortColumnIndex) {
    std::cout<<"sortColumnIndex is greater than number of rows."<<std::endl;
    return;
  }

  int loopIndex = 0;
  while ((row = mysql_fetch_row(result)))
  {
    h_row_vec_temp.push_back(row);
    Sortable sortable(loopIndex, atoi(row[sortColumnIndex]));
    h_vec.push_back(sortable);
    loopIndex++;
  }
  
  thrust::device_vector<Sortable> d_vec = h_vec;
  
  thrust::sort(d_vec.begin(), d_vec.end(), Sorter());
  
  thrust::copy(d_vec.begin(), d_vec.end(), h_vec.begin());
  
  for (int i = 0; i < h_vec.size(); i++) {
    h_row_vec.push_back(h_row_vec_temp[h_vec[i].originalIndex]);
  }
  
  h_vec.clear();
  h_vec.shrink_to_fit();
  h_row_vec_temp.clear();
  h_row_vec_temp.shrink_to_fit();
  d_vec.clear();
  d_vec.shrink_to_fit();

  gettimeofday(&stopTime, NULL);
  
  verifyResult(h_row_vec, sortColumnIndex, "gpu");
  
  h_row_vec.clear();
  h_row_vec.shrink_to_fit();

  mysql_free_result(result);
  mysql_close(conn);
  
  printElapsedTime(stopTime, startTime, "gpu");
}

void cpuSort(const char *dbServer, const char *dbUser, const char *dbPassword, const char *dbDatabase, std::string query, const std::string sortColumnName, int sortColumnIndex)
{
  MYSQL *conn;
  MYSQL_RES *result;
  MYSQL_ROW row;
//   int num_fields;
  
  timeval startTime, stopTime;

  thrust::host_vector<MYSQL_ROW> h_row_vec;
  
  conn = mysql_init(NULL);
  mysql_real_connect(conn, dbServer, dbUser, dbPassword, dbDatabase, 0, NULL, 0);

  std::ostringstream fullQuery;
  fullQuery<<query<<" ORDER BY "<<sortColumnName;
  query = fullQuery.str();
  
  gettimeofday(&startTime, NULL);
  
  mysql_query(conn, query.c_str());
  result = mysql_store_result(conn);

//   num_fields = mysql_num_fields(result);

  while ((row = mysql_fetch_row(result)))
  {
    h_row_vec.push_back(row);
  }
  
  gettimeofday(&stopTime, NULL);

  verifyResult(h_row_vec, sortColumnIndex, "cpu");

  h_row_vec.clear();
  h_row_vec.shrink_to_fit();

  mysql_free_result(result);
  mysql_close(conn);
  
  printElapsedTime(stopTime, startTime, "cpu");
}

int main(int argc, char* argv[])
{
  char* tableName;
  
  if (argc <= 1 || argc > 2) {
    std::cout<<"Usage: "<<argv[0]<<" <table_name>"<<std::endl;
    return 1;
  }
  
  tableName = argv[1];

  INIReader iniReader("config/config.ini");
  if (iniReader.ParseError() < 0) {
      std::cout << "Can't load 'test.ini'\n";
      return 1;
  }
  
  const char *dbServer = iniReader.Get("database", "server", "localhost").c_str();
  const char *dbUser = iniReader.Get("database", "user", "root").c_str();
  const char *dbPassword = iniReader.Get("database", "password", "passwd").c_str();
  const char *dbDatabase = iniReader.Get("database", "database", "test").c_str();
  
  std::ostringstream queryBuilder;
  queryBuilder<<"SELECT SQL_NO_CACHE id, text_col, int_col, double_col FROM "<<tableName;

  std::string query = queryBuilder.str();
  std::string sortColumnName("int_col");
  
  gpuSort(dbServer, dbUser, dbPassword, dbDatabase, query, 2);
  cpuSort(dbServer, dbUser, dbPassword, dbDatabase, query, sortColumnName, 2);

  return 0;
}
