#pragma once
#include <chrono>
#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <thread>
#include <vector>

class Timer {
   public:
    using Ptr = std::shared_ptr<Timer>;
    Timer(std::chrono::milliseconds d, std::function<bool()> callback) : duration_(d), callback_(callback) {
    }
    Timer(const Timer &t) : duration_(t.duration_), callback_(t.callback_), time_point_(t.time_point_) {
    }
    Timer &operator=(const Timer &r) {
        if (this != &r) {
            duration_ = r.duration_;
            callback_ = r.callback_;
            time_point_ = r.time_point_;
        }
        return *this;
    }
    void UpdateCurrentTimePoint() {
        time_point_ = std::chrono::steady_clock::now() + duration_;
    }
    std::chrono::steady_clock::time_point GetTimePoint() const {
        return time_point_;
    }
    bool Run() {
        if (callback_) {
            return callback_();
        }
        return false;
    }
    std::chrono::milliseconds GetDuration() const {
        return duration_;
    }
    std::function<bool()> GetCallback() const {
        return callback_;
    }

   private:
    std::chrono::milliseconds duration_;
    std::function<bool()> callback_;
    std::chrono::steady_clock::time_point time_point_;
};
class TimerPool {
   public:
    TimerPool() {
        std::chrono::minutes time_100years(60 * 24 * 365 * 100);
        PushTimer(time_100years, []() { return true; });
        vec_thread.push_back(std::thread(&TimerPool::worker, this));
    }
    ~TimerPool() {
        for (auto &iter : vec_thread) {
            iter.join();
        }
    }
    Timer::Ptr PushTimer(std::chrono::milliseconds d, std::function<bool()> f) {
        Timer::Ptr timer_ptr = std::make_shared<Timer>(d, f);
        TimerToList(timer_ptr);
        return timer_ptr;
    }
    Timer::Ptr PushTimer(Timer::Ptr timer_ptr) {
        TimerToList(timer_ptr);
        return timer_ptr;
    }
    void StopTimer(Timer::Ptr timer_ptr) {
        const std::scoped_lock lock(list_lock_);
        if (timer_ptr == nullptr) {
            return;
        }
        if (timer_list_.empty()) {
            return;
        }
        for (auto iter : timer_list_) {
            if (iter == timer_ptr) {
                timer_list_.remove(iter);
                break;
            }
        }
        NotifyAndUpdate();
    }
    bool IsRunning(Timer::Ptr timer_ptr) {
        const std::scoped_lock lock(list_lock_);
        if (timer_ptr == nullptr) {
            return false;
        }
        if (timer_list_.empty()) {
            return false;
        }
        auto pos = std::find(timer_list_.begin(), timer_list_.end(), timer_ptr);
        return (pos != timer_list_.end()) ? true : false;
    }
    void ShutDown() {
        quit_ = true;
        NotifyAndUpdate();
    }

   private:
    std::list<Timer::Ptr> timer_list_;
    std::condition_variable timer_cv_;
    std::mutex timer_cv_lock_;
    std::mutex list_lock_;
    bool quit_ = false;
    bool timer_updated_ = false;
    std::vector<std::thread> vec_thread;

    void TimerToList(Timer::Ptr timer_ptr) {
        const std::scoped_lock lock(list_lock_);
        if (timer_ptr == nullptr) return;
        timer_ptr->UpdateCurrentTimePoint();
        if (timer_list_.empty()) {
            timer_list_.push_back(timer_ptr);
            return;
        }
        auto timer_iter = timer_list_.begin();
        while (timer_iter != timer_list_.end()) {
            if (timer_ptr->GetTimePoint() > timer_iter->get()->GetTimePoint()) {
                ++timer_iter;
            } else {
                timer_list_.insert(timer_iter, timer_ptr);
                break;
            }
        }
        NotifyAndUpdate();
    }
    void NotifyAndUpdate() {
        const std::scoped_lock lock(timer_cv_lock_);
        timer_updated_ = true;
        timer_cv_.notify_one();
    }
    Timer::Ptr GetLatestTimerPtr() {
        const std::scoped_lock lock(list_lock_);
        Timer::Ptr latest_timer_ptr(nullptr);
        if (timer_list_.size()) {
            latest_timer_ptr = timer_list_.front();
        }
        return latest_timer_ptr;
    }
    void PopLatestTimerPtr() {
        const std::scoped_lock lock(list_lock_);
        timer_list_.pop_front();
    }
    void worker() {
        while (!quit_) {
            Timer::Ptr latest_timer_ptr = GetLatestTimerPtr();
            if (latest_timer_ptr == nullptr) {
                assert(0);  // should not be here
            }
            std::chrono::steady_clock::time_point timeout_point = latest_timer_ptr->GetTimePoint();
            bool ret;
            {
                std::unique_lock<std::mutex> lock(timer_cv_lock_);
                ret = timer_cv_.wait_until(lock, timeout_point, [this]() { return timer_updated_; });
                timer_updated_ = false;
            }
            if (ret == false) {
                PopLatestTimerPtr();
                if (latest_timer_ptr->Run() == true) {
                    PushTimer(latest_timer_ptr);
                }
            }
        }
    }
};
