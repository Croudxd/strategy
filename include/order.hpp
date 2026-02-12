#pragma once


namespace backtester
{
    enum class FLAGS
    {
        IOC,
        NORMAL,
        CANCEL,
        //...
    };

    struct Order 
    {

        long price;
        long size;
        FLAGS flag;

    };
}

