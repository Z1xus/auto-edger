@echo off
IF NOT EXIST "build" (
  mkdir build
)
cd build
cmake ..
cmake --build . --config Release