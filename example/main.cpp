#include <iostream>
#include "../timer_pool.hpp"
int main() {
    TimerPool timer_pool;
    Timer::Ptr timer_ptr1(nullptr), timer_ptr2(nullptr), timer_ptr3(nullptr);

    /*create repeated timer*/
    std::cout << "create repeating 1000ms timer" << std::endl;
    timer_ptr1 = timer_pool.PushTimer(std::chrono::milliseconds(1000), []() {std::cout<<"1000ms"<<std::endl;return true; });
    std::cout << "create repeating 2000ms timer" << std::endl;
    timer_ptr2 = timer_pool.PushTimer(std::chrono::milliseconds(2000), []() {std::cout<<"2000ms"<<std::endl;return true; });

    std::cout << "create repeating 500ms timer" << std::endl;
    Timer::Ptr timer_ptr4 = std::make_shared<Timer>(std::chrono::milliseconds(500), []() {std::cout<<"500ms"<<std::endl;return true; });
    timer_pool.PushTimer(timer_ptr4);
    /*create one shot timer*/
    std::cout << "create one-shot 3000ms timer" << std::endl;
    timer_ptr3 = timer_pool.PushTimer(std::chrono::milliseconds(3000), []() {std::cout<<"3000ms"<<std::endl;return false; });
    std::this_thread::sleep_for(std::chrono::seconds(10));
    std::cout << "stop 1000ms timer" << std::endl;
    timer_pool.StopTimer(timer_ptr1);
    std::this_thread::sleep_for(std::chrono::seconds(10));
    std::cout << "stop 2000ms timer" << std::endl;
    timer_pool.StopTimer(timer_ptr2);
    std::this_thread::sleep_for(std::chrono::seconds(10));
    std::cout << "close timer pool" << std::endl;
    timer_pool.ShutDown();
    return 0;
}
