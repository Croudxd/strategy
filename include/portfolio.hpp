#pragma once
#include <unordered_map>
#include <cstdint>
#include <iostream>
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
        double fee_rate;

        // Tracks exactly how much was locked per Order ID to prevent drift
        std::unordered_map<uint64_t, double> order_locks;

        Portfolio(double start, double fee) 
            : starting_cash(start)
            , cash(start)
            , locked_cash(0.0)
            , position(0.0)
            , total_fees(0.0)
            , fee_rate(fee)
        {}

        double get_equity(double current_price) const 
        {
            return cash + locked_cash + (position * (current_price / 100.0));
        }

        double get_cash() const 
        {
            return cash; 
        }

        double get_position() const
        {
            return position;
        }

        /**
         * @brief Updates the portfolio based on an execution report.
         * @param rep The execution report from the engine.
         * @param limit_price The ORIGINAL limit price used when the order was placed.
         */
        void update(const Report& rep, double limit_price)
        {
            // 1. Standardize Units
            double real_limit_price = limit_price / 100.0;
            double real_last_price  = (double)rep.last_price / 100.0;
            
            double real_last_qty    = (double)rep.last_quantity   / 1000000.0;
            double real_leaves_qty  = (double)rep.leaves_quantity / 1000000.0;

            // --- Case: New Order ---
            if (rep.status == Status::NEW)
            {
                if (rep.side == Side::BUY) 
                {
                    double amount_to_lock = real_limit_price * real_leaves_qty;
                    cash -= amount_to_lock;
                    locked_cash += amount_to_lock;
                    
                    // Track exactly what we moved so we can refund it perfectly
                    order_locks[rep.order_id] = amount_to_lock;
                }
            }
            // --- Case: Execution (Fill or Partial) ---
            else if (rep.status == Status::FILLED || rep.status == Status::PARTIALLY_FILLED)
            {
                double trade_value = real_last_price * real_last_qty;
                
                if (rep.side == Side::BUY) 
                {
                    // Calculate how much of our "Locked Pot" this fill represents
                    double lock_portion = real_limit_price * real_last_qty;
                    
                    position    += real_last_qty;
                    locked_cash -= lock_portion;
                    
                    // Price Improvement: If fill price < limit price, return the extra to cash
                    cash += (lock_portion - trade_value);
                    
                    // Update our internal tracker for this order
                    if (order_locks.count(rep.order_id)) {
                        order_locks[rep.order_id] -= lock_portion;
                    }
                }
                else // SELL
                { 
                    position -= real_last_qty;
                    cash     += trade_value;
                }

                // Apply Fees (always from liquid cash)
                double local_fee = trade_value * fee_rate;
                total_fees += local_fee;
                cash -= local_fee;
            }
            // --- Case: Cancellation or Rejection ---
            else if (rep.status == Status::CANCELED || rep.status == Status::REJECTED)
            {
                if (rep.side == Side::BUY) 
                {
                    // Instead of recalculating, refund the EXACT balance of what was locked
                    if (order_locks.count(rep.order_id)) {
                        double refund = order_locks[rep.order_id];
                        locked_cash -= refund;
                        cash += refund;
                        order_locks.erase(rep.order_id);
                    }
                }
            }

            // --- Final Cleanup ---
            if (rep.status == Status::FILLED)
            {
                order_locks.erase(rep.order_id);
            }

            // Zero-out floating point noise (prevents the "0.1 increase" drift)
            if (locked_cash < 1e-8) locked_cash = 0.0;
        }
    };
}
