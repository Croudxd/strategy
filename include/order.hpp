#pragma once


namespace backtester
{
    enum class FLAGS
    {
        IOC,
        NORMAL,
    };

    struct Order 
    {

        long price;
        long size;
        FLAGS flag;

    };
}

