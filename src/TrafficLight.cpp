#include <iostream>
#include <random>
#include <thread>
#include <chrono>
#include <random>
#include <future>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */


template <typename T>
T MessageQueue<T>::receive()
{
    
    std::unique_lock<std::mutex> lck(_mtx);
    _condition.wait(lck, [this] {return !_queue.empty();});
    T msg = std::move(_queue.back());
    _queue.pop_back();

    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    std::lock_guard<std::mutex> lck(_mtx);
    _queue.clear();
    _queue.push_back(std::move(msg));
    _condition.notify_one();
}


/* Implementation of class "TrafficLight" */


TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
    _queue = std::make_shared<MessageQueue<TrafficLightPhase>>();
}

void TrafficLight::waitForGreen()
{
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto current = _queue->receive();
        if(current == TrafficLightPhase::green)
        {
            return;
        }
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    std::lock_guard<std::mutex> lck(_mutex);
    return _currentPhase;
}

void TrafficLight::simulate()
{
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this)); 
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{ 
    std::random_device rdm;
    std::mt19937 gen(rdm());
    std::uniform_int_distribution<int> dis(4000, 6000);
    int cycleDuration = dis(gen);
    std::chrono::time_point<std::chrono::system_clock> lastUpdate = std::chrono::system_clock::now();

    while(true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto temp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastUpdate);
        int difference = temp.count();

        if(difference >= cycleDuration)
        {
            if(TrafficLight::getCurrentPhase() == TrafficLightPhase::red)
            {
                _currentPhase = TrafficLightPhase::green;
            }
            else
            {
                _currentPhase = TrafficLightPhase::red;
            }

            auto futures = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, _queue,std::move(_currentPhase));
            
            futures.wait();

            lastUpdate = std::chrono::system_clock::now();
            cycleDuration = dis(gen);
        }
        
    }

}

