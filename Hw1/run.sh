#!/bin/bash
make clean
rm *.log
rm file.txt
clear
make
echo -e "\n\n"

echo "***** Example 1-1 *****"
./logger config.txt ./ex1-1
echo -e "***********************\n\n"

echo "***** Example 1-2 *****"
./logger config.txt ./ex1-2
echo -e "***********************\n\n"

echo "***** Example  2  *****"
./logger config.txt ./ex2
echo -e "***********************\n\n"

echo "***** Example 3-1 *****"
./logger config.txt ./ex3-1
echo -e "***********************\n\n"

echo "***** Example 3-2 *****"
./logger config.txt ./ex3-2
echo -e "***********************\n\n"

echo "***** Example 4-1  *****"
./logger config.txt ./ex4 www.cs.nycu.edu.tw
echo -e "***********************\n\n"

echo "***** Example 4-2  *****"
./logger config.txt ./ex4 www.google.com
echo -e "***********************\n\n"

echo "***** Example 5-1  *****"
./logger config.txt ./ex5 172.217.160.100
echo -e "***********************\n\n"

echo "***** Example 5-2  *****"
./logger config.txt ./ex5 20.27.177.113
echo -e "***********************\n\n"

echo "***** Example  6  *****"
./logger config.txt ./ex6
echo -e "***********************"


