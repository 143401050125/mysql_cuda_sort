mysql_cuda_sort
===============

sorting mysql result set on gpu using thrust, and bechmarking against standard sql order by

*  **dump_generator** - creates table and inserts rows with random values to it

compile:
```
$ g++ -o bin/dump_generator src/dump_generator.cpp lib/inih/ini.c lib/inih/cpp/INIReader.cpp
```
execute:
```
$ bin/dump_generator <table_name>[ <step=10000>[ <numberOfLoops=1>]]
```

*  **sort** - executes a select query and sorts it, first with thrust, then with order by, and prints execution time

compile:
```
$ nvcc -o bin/sort src/sort.cu lib/inih/cpp/INIReader.cpp lib/inih/ini.c -I/usr/include/mysql -lmysqlclient -lboost_regex -arch=sm_20
```
execute:
```
$ bin/sort <table_name>
```

an example on my laptop, with geforce 610m, for an unindexed table with 1M lines, and default mysql config:

```
$ optirun bin/sort test_data_1000000
query: SELECT SQL_NO_CACHE id, text_col, int_col, double_col FROM test_data_1000000 ORDER BY int_col
calling gpuSort<int>
Errors: 0
gpu query execution and sorting took: 4.153 seconds
calling cpuSort
Errors: 0
cpu query execution and sorting took: 8.185 seconds
query: SELECT SQL_NO_CACHE id, text_col, int_col, double_col FROM test_data_1000000 ORDER BY double_col
calling gpuSort<double>
Errors: 0
gpu query execution and sorting took: 4.686 seconds
calling cpuSort
Errors: 0
cpu query execution and sorting took: 8.272 seconds
```
