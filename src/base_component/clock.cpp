//
// Created by wyk on 11/1/23.
//

#include "clock.h"

namespace pimsim {

Clock::Clock(const sc_core::sc_module_name& name11111111, double period)
    : sc_core::sc_module(name11111111), period_(period) {
    SC_THREAD(process)

    SC_METHOD(endPosEdge)
    sensitive << end_pos_edge_;
}

void Clock::process() {
    while (true) {
        wait(period_, sc_core::SC_NS);

        // at positive edge
        is_pos_edge_ = true;
        for (const auto& event : pos_edge_events_) {
            event->notify();
        }
        pos_edge_events_.clear();

        // Wait until all events currently waiting to be executed are processed, notify end_pos_edge_.
        // That is, notify end_pos_edge_ at next delta cycle
        end_pos_edge_.notify(sc_core::SC_ZERO_TIME);
        // positive edge end
    }
}

void Clock::notifyNextPosEdge(sc_core::sc_event* event) {
    pos_edge_events_.insert(event);
}

bool Clock::posEdge() const {
    return is_pos_edge_;
}

void Clock::endPosEdge() {
    is_pos_edge_ = false;
}

}  // namespace pim
