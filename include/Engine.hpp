#pragma once

#include <cstdint>
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <thread>
#include "candle.hpp"
#include "ring_buffer.hpp"
#include "order.hpp"

namespace backtester
{

    static constexpr int BUFFER_CAPACITY = 16384;
    struct Candle_memory 
    {
        volatile uint64_t write_idx; 
        uint8_t           pad1[56];
        volatile uint64_t read_idx; 
        uint8_t           pad2[56];
        Candle            buffer[16384];
    };

    struct Order_memory 
    {
        volatile uint64_t write_idx; 
        uint8_t           pad1[56];
        volatile uint64_t read_idx; 
        uint8_t           pad2[56];
        Order            buffer[16384];
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
                candle_mem = map_mem<Candle_memory>("/dev/shm/hft_candle", MemoryFlags::CONSUMER);
                order_mem = map_mem<Order_memory>("/dev/shm/hft_order", MemoryFlags::PRODUCER);
            }

            // Add for order.
            void run ()
            {
                {
                }
                uint64_t local_read_idx = candle_mem->write_idx;
                candle_mem->read_idx = local_read_idx;
                std::cout << "Watching memory...\n";

                while (true)
                {
                    uint64_t current_write_idx = candle_mem->write_idx;
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
                    else 
                    {
                        std::this_thread::yield();
                    }
                }
            }

            void buy(float price, float quantity)
            {
                Order order = Order(price, quantity);
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

            void sold(float price, float quantity)
            {
                Order order = Order(price, quantity);
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

            void cancel(size_t id) 
            {
                // Order order = Order(price, quantity);
                // uint64_t local_write_idx = order_mem->write_idx;
                // uint64_t cached_read_idx = order_mem->read_idx; 
                // if (local_write_idx - cached_read_idx >= BUFFER_CAPACITY) 
                // {
                //     cached_read_idx = order_mem->read_idx;
                //
                // }
                // if (local_write_idx - cached_read_idx >= BUFFER_CAPACITY) 
                // {
                //     return;
                // }
                //
                // order_mem->buffer[local_write_idx % BUFFER_CAPACITY] = order;
                // std::atomic_thread_fence(std::memory_order_release);
                // order_mem->write_idx = local_write_idx + 1;
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
            Strategy strategy;
            Candle_memory* candle_mem;
            Order_memory* order_mem;
            Ring_buffer ring_buffer;
            size_t warm_count = 0;
    };
};
