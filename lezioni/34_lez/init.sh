#! /bin/bash
cd cgi
gcc -o cgiexe cgiexe.c
cd ..
gcc -o sw-cgi sw-cgi.c
echo "Start server on port 17999"
./sw-cgi