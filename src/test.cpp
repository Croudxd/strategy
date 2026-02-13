#include <iostream>
#include <chrono>
#include <vector>
#include "Engine.hpp" 
#include "ring_buffer.hpp"
#include <sma.hpp>
#include <indicator.hpp>
// Include your strategy header or define it here if it's in main.cpp

// Mock Engine to catch orders without using shared memory
template<typename Strategy>
class MockEngine : public backtester::Engine<Strategy> {
public:
    MockEngine() : backtester::Engine<Strategy>(10000, 0) {}
    // Override order to do nothing but count
    void order(float size, float price, backtester::Order_side side, bool cancel) {} 
};

// Paste your Strategy class here (or include it)
class Strategy {
    backtester::SMA short_ma { 3 };
    backtester::SMA long_ma { 12 };
public:
    void run(backtester::Ring_buffer& ring_buffer, auto& engine) {
        double current_price = ring_buffer.get(0).get_open();
        long sma_s = short_ma.calculate(); 
        long sma_l = long_ma.calculate();
        if (sma_s > sma_l) {
            engine.order(1.0, current_price, backtester::Order_side::BUY, false);
        }
    }
};

int main() {
    backtester::Ring_buffer buffer;
    MockEngine<Strategy> engine;
    backtester::Indicator::set_ring_buffer(engine.get_ring_buffer());
    Strategy strategy;
    
    // 1. Warmup
    std::cout << "Warming up..." << std::endl;
    for(int i=0; i<100; i++) {
        buffer.add(Candle(100+i, 105+i, 95+i, 102+i, 1000));
    }

    // 2. Benchmark
    int iterations = 1000000;
    auto start = std::chrono::high_resolution_clock::now();

    for(int i=0; i<iterations; i++) {
        // Feed a fake candle
        buffer.add(Candle(100, 100, 100, 100, 100)); 
        // Run logic
        strategy.run(buffer, engine);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    std::cout << "Total Time: " << duration / 1000000.0 << " ms" << std::endl;
    std::cout << "Avg Latency per Tick: " << duration / iterations << " ns" << std::endl;
    std::cout << "Ticks per Second: " << (iterations * 1e9) / duration << std::endl;

    return 0;
}
