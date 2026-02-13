#pragma once
#include <cstdint>

namespace backtester
{
    enum class Status 
    {
        NEW,
        FILLED,
        PARTIALLY_FILLED,
        CANCELED,
        REJECTED,
    };

    enum class Side
    {
        BUY,
        SELL,
    };

    enum class Rejection_code
    {
        NO_FUNDS,
        PRICE_OUT_OF_BOUNDS,
        SYSTEM_ERROR,
    };

    struct Report 
    {
        uint64_t order_id;
        Status status;
        uint64_t last_quantity;
        uint64_t last_price;
        uint64_t leaves_quantity;
        Side side;
        Rejection_code reject_code;
        uint64_t trade_id;
        uint64_t timestamp;
    };

    struct Active_orders 
    {
        uint64_t order_id;
        uint64_t leaves_quantity;
        uint64_t price;
        Side side;
        uint64_t timestamp;
    };

};
