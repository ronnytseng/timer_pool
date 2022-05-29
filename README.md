# Simple Timer Pool
#### Introduction
* use only one prepared thread to handle multiple timers. The callback function of the timer needs to be as short as possible, you could integrate thread-pool to do this.
####Compiling
* This shoud compile on c++17 compiler.
####Example
```cpp
cd example
cmake -Bbuild
cmake --build build
./build/test
```