/** 
 * I want to create a way to create strategies based on multiple types of indicators.
 * Integrated into the cpp order-book/engine. Will recieve candles from live trade data via a SPSC (or some other form of IPC).
 * Will send buy and sell notifications to it.
 * Will recieve updates such as amount of money and stocks from each buy/sell.
 * Be able to create a portfolio, show the amount that has been gained/lost, and other metrics.
 * Would be cool to have some form of GUI/TUI rather than a crappy terminal screen.
 *
 * 1. Get candles flowing into the program.
 * 2. Able to manually send buy/sell requests and get anwsers results.
 * 2. Ability to get a simple indicator working (probably SMI)
 * 3. Able to calculate profits/loss
 * */

/**
 * When we get data from the SPSC we put that into a vector/Array (memory pool so we dont keep reallocating mem.)
 * Then we can just use rsi/smi/ema on the past length x - array.length.
 *
 * Then we send a request to the SPSC for order-book to insert a match. The SPSC will open at the start of the program and loop. So idk how we will do this actually.
 */

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "Engine.hpp"
#include "indicator.hpp"
#include "order.hpp"
#include "ring_buffer.hpp"
#include "sma.hpp"

class Strategy
{
    public:
        void run(backtester::Ring_buffer ring_buffer, backtester::Engine<Strategy>& engine)
        {
            if (backtester::SMA(3) > backtester::SMA(12))
                engine.order(100.0, 100.0, backtester::Order_side::SELL, true);
            if (backtester::SMA(12) > backtester::SMA(3))
                engine.order(100.0, 100.0, backtester::Order_side::BUY, true);
        }
};

int main() 
{
    backtester::Engine<Strategy> engine(10000.0, 0.000001);
    backtester::Indicator::set_ring_buffer(engine.get_ring_buffer());
    engine.connect();
    engine.warmup(12);
    engine.run();
    return 0;
}


