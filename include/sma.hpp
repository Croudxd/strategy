#pragma once
#include "indicator.hpp"
#include <cstddef>


namespace backtester
{
    class SMA : Indicator 
    {
        public:
            SMA(size_t size) : period(size) {}

            operator int() const 
            {
                return backtester::SMA::calculate();
            }

            int calculate() const
            {
                return period;
            }

        private:
            int period;

    };
};
