#! /bin/bash
cd cgi-bin
gcc -o command command.c
cd ..
gcc -o sw sw.c
echo "Start server on port 9088"
./sw