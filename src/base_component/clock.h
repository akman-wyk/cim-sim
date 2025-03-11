//
// Created by wyk on 11/1/23.
//

#pragma once
#include <set>

#include "systemc.h"

namespace cimsim {

class Clock : public sc_core::sc_module {
    SC_HAS_PROCESS(Clock);

public:
    Clock(const sc_core::sc_module_name& name, double period);

    // update clock, process events
    [[noreturn]] void process();

    // registers call this method to trigger its update
    void notifyNextPosEdge(sc_core::sc_event* event);

    // return it is positive edge or not
    bool posEdge() const;

private:
    void endPosEdge();

private:
    std::set<sc_core::sc_event*> pos_edge_events_;
    sc_core::sc_event end_pos_edge_;
    bool is_pos_edge_ = false;
    double period_;
};

}  // namespace cimsim
