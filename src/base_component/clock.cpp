//
// Created by wyk on 11/1/23.
//

#include "clock.h"

namespace cimsim {

Clock::Clock(const sc_module_name& name, double period) : sc_module(name), period_(period) {
    SC_THREAD(process)

    SC_METHOD(endPosEdge)
    sensitive << end_pos_edge_;
}

void Clock::process() {
    while (true) {
        wait(period_, SC_NS);

        // at positive edge
        is_pos_edge_ = true;
        for (const auto& event : pos_edge_events_) {
            event->notify();
        }
        pos_edge_events_.clear();

        // Wait until all events currently waiting to be executed are processed, notify end_pos_edge_.
        // That is, notify end_pos_edge_ at next delta cycle
        end_pos_edge_.notify(SC_ZERO_TIME);
        // positive edge end
    }
}

void Clock::notifyNextPosEdge(sc_event* event) {
    pos_edge_events_.insert(event);
}

bool Clock::posEdge() const {
    return is_pos_edge_;
}

void Clock::endPosEdge() {
    is_pos_edge_ = false;
}

}  // namespace cimsim
