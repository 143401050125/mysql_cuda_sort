mysql_cuda_sort
===============

sorting mysql result set on gpu using thrust, and bechmarking against standard sql order by

*  **dump_generator** - creates table and inserts rows, with random values to it

compile:
```
$ g++ -o dump_generator dump_generator.cpp ../lib/inih/ini.c ../lib/inih/cpp/INIReader.cpp
```
execute:
```
$ dump_generator <table_name>[ <step=10000>[ <numberOfLoops=1>]]
```

*  **sort** - executes a select query and sorts it, first with thrust, then with order by, and prints execution time

compile:
```
$ nvcc -o sort sort.cu ../lib/inih/cpp/INIReader.cpp ../lib/inih/ini.c -I/usr/include/mysql -lmysqlclient
```
execute:
```
$ sort <table_name>
```
