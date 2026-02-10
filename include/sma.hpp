#pragma once
#include "indicator.hpp"
#include <cstddef>

/**
 * SMA is Moving average.
 *
 * We calculate by firstly getting an average of X days
 * so sma(14);
 *
 * (Average of past 14) in order to calculate this we need atleast 14 days to calculate the an average.
 * Then we can simply remove the last and add the first
 *
 * To simplify lets do sma (3)
 * candle 1 = 100 > not enough candles just add to the average. sum = 100
 * candle 2 = 101 > not enough add acg = 201
 * candle 3  = 100 > add to candle 301
 * Now we have candle 3 which is 300.33. / 3 so 100.1
 *
 * Then we get candle 4: = 102
 * so we do
 * x= 100.1 +((102 - 100) / 3) 
 * and then again for next candle so
 * x+1 = x + ((candle5 - 102 ) / 3)
 *
 * Which means we need this:
 * int incoming;
 * int old;
 * the average;
 * to calculate after warmup.
 *
 * To warmup we simple just add every single candle we get and then divide when we hit the number.
 */

namespace backtester
{
    class SMA : Indicator 
    {
        public:
            SMA(size_t size) : period(size) {}

            operator int() 
            {
                return backtester::SMA::calculate();
            }

            long calculate() 
            {
                size_t count = Indicator::ring_buffer->size();
                if (period > Indicator::ring_buffer->size())
                {
                    return 0;
                }
                else if (period == Indicator::ring_buffer->size()) 
                {
                    for ( size_t i = {}; i < period; i++)
                    {
                        sum += ring_buffer->get(i).get_open();
                    }
                    sum /= period;
                }
                else if ( count > last_count_size)
                {
                        incoming = ring_buffer->get(0).get_open();
                        outgoing = ring_buffer->get(period - 1).get_open();
                        sum = sum + ( incoming - outgoing ) ;
                }

                last_count_size = count;
                return sum / period;
            }

        private:
            long outgoing = 0;
            long incoming = 0;
            long sum = 0;
            long period = 0;
            size_t last_count_size = 0;
            /** might need to remove this every now and then */
    };
};
