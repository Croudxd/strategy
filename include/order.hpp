#pragma once
#include <iostream>


#include <cstdint>
#include <sys/types.h>
namespace backtester
{
    enum class Order_side
    {
        BUY,
        SELL,
    };

    struct Order 
    {
        uint64_t id;
        uint64_t size;
        int64_t  price;
        int8_t   side;   //sell / buy
        int8_t   action; // cancel order 
        int8_t   status; // trade/order
        uint8_t  pad1[1];

     };
}

