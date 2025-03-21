//
// Created by wyk on 11/2/23.
//

#pragma once
#include "clock.h"
#include "systemc.h"

namespace cimsim {

template <class T>
struct FSMPayload {
    T payload;
    bool valid;

    bool operator==(const FSMPayload& another) {
        return valid == another.valid && payload == another.payload;
    }

    friend void sc_trace(sc_core::sc_trace_file*, const FSMPayload&, const std::string&) {}

    friend std::ostream& operator<<(std::ostream& out, const FSMPayload&) {
        out << "FSMPayload Type\n";
        return out;
    }
};

template <class T>
class FSM : public sc_core::sc_module {
public:
    SC_HAS_PROCESS(FSM);

    FSM(const sc_core::sc_module_name& name, Clock* clk) : sc_core::sc_module(name), clk_(clk), is_ready_(true) {
        SC_METHOD(process)
        sensitive << trigger_;

        SC_METHOD(processInput)
        sensitive << input_;

        SC_METHOD(processEnable)
        sensitive << enable_;

        SC_METHOD(finishExec)
        sensitive << finish_exec_;
    }

    void process() {
        if (is_ready_ && enable_.read() && clk_->posEdge()) {
            if (auto fsm_payload = input_.read(); fsm_payload.valid) {
                value_ = fsm_payload.payload;
                is_ready_ = false;
                start_exec_.notify(SC_ZERO_TIME);
                output_.write(value_);
            }
        }
    }

    void processInput() {
        if (input_.read().valid) {
            clk_->notifyNextPosEdge(&trigger_);
        }
    }

    void processEnable() {
        if (enable_.read()) {
            clk_->notifyNextPosEdge(&trigger_);
        }
    }

    void finishExec() {
        is_ready_ = true;
        clk_->notifyNextPosEdge(&trigger_);
    }

    bool isReady() const {
        return is_ready_;
    }

    bool isBusy() const {
        return !is_ready_;
    }

public:
    sc_core::sc_in<FSMPayload<T>> input_{"input"};
    sc_core::sc_in<bool> enable_{"enable"};
    sc_core::sc_out<T> output_{"output"};

    sc_core::sc_event start_exec_;
    sc_core::sc_event finish_exec_;

private:
    Clock* clk_;
    sc_core::sc_event trigger_;
    bool is_ready_;
    T value_;
};

}  // namespace cimsim
