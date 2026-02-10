#pragma once

#include <cstdint>
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <thread>
#include "candle.hpp"
#include "ring_buffer.hpp"

namespace backtester
{

    struct Shared_memory_layout 
    {
        volatile uint64_t write_idx; 
        uint8_t           pad1[56];
        volatile uint64_t read_idx; 
        uint8_t           pad2[56];
        Candle            buffer[16384];
    };

    template <typename Strategy>
    class Engine
    {
        public:
            Engine()
            {
                ring_buffer = backtester::Ring_buffer();
            }

            void connect()
            {
                int fd = open("/dev/shm/hft_candle", O_RDWR);
                while (fd == -1) 
                {
                    std::cout << "Waiting for Rust feeder...\n";
                    sleep(1);
                    fd = open("/dev/shm/hft_candle", O_RDWR);
                }

                void* ptr = mmap(NULL, sizeof(Shared_memory_layout), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                if (ptr == MAP_FAILED) 
                {
                    perror("mmap failed"); 
                    return;
                }
                std::cout << "Connected to memory" << std::endl;

                shm = static_cast<Shared_memory_layout*>(ptr);
            }

            void run ()
            {
                uint64_t local_read_idx = shm->write_idx;
                shm->read_idx = local_read_idx;
                std::cout << "Watching memory...\n";

                while (true)
                {
                    uint64_t current_write_idx = shm->write_idx;
                    if (local_read_idx < current_write_idx) 
                    {
                        int slot = local_read_idx % 16384;
                        Candle raw = shm->buffer[slot];

                        ring_buffer.add(raw);
                        local_read_idx++;
                        shm->read_idx = local_read_idx; 

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

            void buy()
            {
                std::cout << "BOUGHT" << std::flush;
            }

            void sold()
            {
                std::cout << "SOLD";
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
            Shared_memory_layout* shm;
            Ring_buffer ring_buffer;
            size_t warm_count = 0;
    };
};




