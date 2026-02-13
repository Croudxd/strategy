#pragma once
#include "report.hpp"

namespace backtester
{
    struct Portfolio
    {
        double starting_cash;
        double cash;
        double locked_cash;
        double position;
        double total_fees;
        double fee;

        Portfolio(double start, double fee) 
            : starting_cash(start)
            , cash(start)
            , locked_cash(0.0)
            , position(0.0)
            , total_fees(0.0)
        {}

        double get_equity(double current_price) const 
        {
            return cash + locked_cash + (position * current_price);
        }

        double get_cash() const 
        {
            return cash; 
        }

        void update(const Report& rep, double limit_price)
        {
            if (rep.status == Status::NEW)
            {
                if (rep.side == Side::BUY) 
                {
                    double amount = limit_price * (double)rep.leaves_quantity;
                    cash        -= amount;
                    locked_cash += amount;
                }
            }
            else if (rep.status == Status::FILLED || rep.status == Status::PARTIALLY_FILLED)
            {
                double trade_value = (double)rep.last_price * (double)rep.last_quantity;
                double lock_value  = limit_price * (double)rep.last_quantity;
                
                double local_fee = trade_value * fee; 
                total_fees += local_fee;
                cash -= local_fee; 

                if (rep.side == Side::BUY) 
                {
                    position    += rep.last_quantity;
                    locked_cash -= lock_value; 
                    cash += (lock_value - trade_value);
                }
                else 
                { 
                    position -= rep.last_quantity;
                    cash     += trade_value;
                }
            }
            else if (rep.status == Status::CANCELED || rep.status == Status::REJECTED)
            {
                if (rep.side == Side::BUY) 
                {
                    double refund = limit_price * (double)rep.leaves_quantity;
                    locked_cash -= refund;
                    cash        += refund;
                }
            }
            
            if (locked_cash < 0) locked_cash = 0;
        }
    };
};
