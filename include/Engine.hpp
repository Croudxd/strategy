#pragma once

#include <cstdint>
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <thread>
#include <unordered_map>
#include <vector>
#include "candle.hpp"
#include "ring_buffer.hpp"
#include "order.hpp"
#include "report.hpp"

namespace backtester
{

    static constexpr int BUFFER_CAPACITY = 16384;

    template <typename T>
    struct memory_struct 
    {
        volatile uint64_t write_idx; 
        uint8_t           pad1[56];
        volatile uint64_t read_idx; 
        uint8_t           pad2[56];
        T                 buffer[16384];
    };


    enum class MemoryFlags
    {
        PRODUCER,
        CONSUMER,
    };

    template <typename Strategy>
    class Engine
    {
        public:
            Engine()
            {
                ring_buffer = backtester::Ring_buffer();
                history.reserve(1000);
            }

            template<typename T> 
            T* map_mem(const char* path, MemoryFlags flag )
            {

                int fd = 0;
                if (flag == MemoryFlags::CONSUMER)
                {
                    fd = open(path, O_RDWR);
                    while (fd == -1) 
                    {
                        sleep(1);
                        fd = open(path, O_RDWR);
                    }
                }
                if ( flag == MemoryFlags::PRODUCER)
                {
                        fd = open(path, O_RDWR | O_CREAT, 0666); 
                        ftruncate(fd, sizeof(T)); 
                }

                void* ptr = mmap(NULL, sizeof(T), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                if (ptr == MAP_FAILED) 
                {
                    perror("mmap failed"); 
                    return nullptr;
                }
                std::cout << "Connected to memory" << std::endl;
                return static_cast<T*>(ptr);
            }

            void connect()
            {
                candle_mem = map_mem<memory_struct<Candle>>("/dev/shm/hft_candle", MemoryFlags::CONSUMER);
                order_mem = map_mem<memory_struct<Order>>("/dev/shm/hft_order", MemoryFlags::PRODUCER);
                report_mem = map_mem<memory_struct<Report>>("/dev/shm/hft_report", MemoryFlags::CONSUMER);
            }

            void run ()
            {
                uint64_t local_read_idx = candle_mem->write_idx;
                candle_mem->read_idx = local_read_idx;

                uint64_t local_report_read_idx = report_mem->write_idx;
                report_mem->read_idx = local_report_read_idx;

                while (true)
                {
                    uint64_t current_write_idx = candle_mem->write_idx;
                    uint64_t current_report_write_idx = report_mem->write_idx;

                    if (local_read_idx < current_write_idx) 
                    {
                        int slot = local_read_idx % 16384;
                        Candle raw = candle_mem->buffer[slot];

                        ring_buffer.add(raw);
                        local_read_idx++;
                        candle_mem->read_idx = local_read_idx; 

                        if (warm_count > 0)
                        {
                            warm_count--;
                            continue;
                        }
                        strategy.run(ring_buffer, *this);
                    }
                    if (local_report_read_idx < current_report_write_idx)
                    {
                        int slot = local_report_read_idx % 16384;
                        Report raw = report_mem->buffer[slot];



                        local_report_read_idx++;
                        report_mem->read_idx = local_report_read_idx; 
                    }
                    else 
                    {
                        std::this_thread::yield();
                    }
                }
            }

            void order(float size, float price, Order_side side, bool cancel)
            {
                int8_t int_side = (side == Order_side::BUY) ? 0 : 1; 
                int8_t int_action = (cancel == true) ? 1 : 0; 
                uint64_t p = price * 100.0;
                uint64_t s = size * 1000000.0;
                Order order = Order(order_id++, price, size, int_side, int_action, 0 /** status*/);
                uint64_t local_write_idx = order_mem->write_idx;
                uint64_t cached_read_idx = order_mem->read_idx; 
                if (local_write_idx - cached_read_idx >= BUFFER_CAPACITY) 
                {
                    cached_read_idx = order_mem->read_idx;
                    
                }
                if (local_write_idx - cached_read_idx >= BUFFER_CAPACITY) 
                {
                    return;
                }

                order_mem->buffer[local_write_idx % BUFFER_CAPACITY] = order;
                std::atomic_thread_fence(std::memory_order_release);
                order_mem->write_idx = local_write_idx + 1;
            }

            void cancel_order(uint64_t order_id)
            {
                Order order = Order(order_id++, 0/**SIZE*/, 0/**PRICE*/, 0/**SIDE*/, 1/**ACTION*/, 0/**Status*/);
                uint64_t local_write_idx = order_mem->write_idx;
                uint64_t cached_read_idx = order_mem->read_idx; 
                if (local_write_idx - cached_read_idx >= BUFFER_CAPACITY) 
                {
                    cached_read_idx = order_mem->read_idx;
                }
                if (local_write_idx - cached_read_idx >= BUFFER_CAPACITY) 
                {
                    return;
                }
                order_mem->buffer[local_write_idx % BUFFER_CAPACITY] = order;
                std::atomic_thread_fence(std::memory_order_release);
                order_mem->write_idx = local_write_idx + 1;
            }

            void warmup(size_t size)
            {
                warm_count = size;
            }

            backtester::Ring_buffer* get_ring_buffer()
            {
                return &ring_buffer;
            }

        private:


            void on_report(const Report& raw)
            {
                if (raw.status != Status::NEW) {
                    history.push_back(raw);
                }

                switch (raw.status) 
                {
                    case Status::NEW:
                        active_orders[raw.order_id] = { 
                            raw.order_id, 
                            raw.leaves_quantity, 
                            (uint64_t)raw.last_price, 
                            raw.side,
                            raw.timestamp 
                        };
                        break;

                    case Status::PARTIALLY_FILLED:
                        if (auto it = active_orders.find(raw.order_id); it != active_orders.end()) {
                            it->second.leaves_quantity = raw.leaves_quantity;
                        }
                        break;

                    case Status::FILLED:
                    case Status::CANCELED:
                    case Status::REJECTED:
                        active_orders.erase(raw.order_id);
                        break;
                }
            }


            Strategy strategy;
            memory_struct<Candle>* candle_mem;
            memory_struct<Order>* order_mem;
            memory_struct<Report>* report_mem;
            Ring_buffer ring_buffer;
            size_t warm_count = 0;
            uint64_t order_id;

            std::vector<Report> history;
            std::unordered_map<uint64_t, Active_orders> active_orders;
    };
};
