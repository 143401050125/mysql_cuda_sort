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
$ nvcc -o bin/sort src/sort.cu lib/inih/cpp/INIReader.cpp lib/inih/ini.c -I/usr/include/mysql -lmysqlclient -lboost_regex
```
execute:
```
$ bin/sort <table_name>
```

an example on my laptop, with geforce 610m, for an unindexed table with 1M lines, and default mysql config:

```
$ optirun bin/sort test_data_1000000
gpu query execution and sorting took: 3.922 seconds
cpu query execution and sorting took: 7.574 seconds
```
