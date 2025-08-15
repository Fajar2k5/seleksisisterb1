#!/bin/bash

SOURCE_FILE="ntt_calculator.c"

EXECUTABLE_NAME="ntt_calculator"

echo "Compiling ${SOURCE_FILE}..."

gcc -O3 -Wall -Wextra "${SOURCE_FILE}" -o "${EXECUTABLE_NAME}"

if [ $? -eq 0 ]; then
  echo "Success! Program compiled."
  echo "Run with: ./${EXECUTABLE_NAME}"
else
  echo "Compilation failed."
fi
