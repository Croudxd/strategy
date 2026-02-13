#include <iostream>
#include <chrono>
#include <thread>
#include "Engine.hpp" // Re-use your Engine struct definitions

using namespace backtester;

int main() {
    // Manually map the order memory as a PRODUCER
    // (Copy the map_mem logic or use the Engine class helper if accessible)
    // For simplicity, assuming you can instantiate Engine to get access:
    
    Engine<int> helper(0,0); // Dummy engine just to use helper methods
    auto* order_mem = helper.map_mem<memory_struct<Order>>("/dev/shm/hft_order", MemoryFlags::CONSUMER); // We pretend to be consumer to write to it? 
    // Wait, your Engine.hpp logic says PRODUCER creates the file. 
    // Let's just assume we attach to the existing one.
    
    if(!order_mem) { std::cout << "Start the main Engine first!" << std::endl; return -1; }

    uint64_t count = 1000000; // 1 Million Orders
    std::cout << "Spamming " << count << " orders..." << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    for(uint64_t i = 0; i < count; i++) {
        // Create a dummy order
        Order o(i, 100, 10000, 0, 0, 0); 
        
        // Write to Ring Buffer (No checks, just speed)
        // Note: In real benchmark, you should check for buffer full, 
        // but for pure raw speed, we blast.
        uint64_t idx = order_mem->write_idx;
        order_mem->buffer[idx % 16384] = o;
        
        // Atomic commit (simulated)
        std::atomic_thread_fence(std::memory_order_release);
        order_mem->write_idx++;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    std::cout << "Sent 1M orders in " << ns / 1e9 << " seconds." << std::endl;
    std::cout << "Throughput: " << (count * 1e9) / ns << " Orders/Sec" << std::endl;
}
