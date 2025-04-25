#ifndef RISCV_ISA_BASIC_TIMER_H
#define RISCV_ISA_BASIC_TIMER_H

#include <systemc>

#include <tlm_utils/simple_target_socket.h>

#include "core/common/irq_if.h"

struct BasicTimer : public sc_core::sc_module {
	tlm_utils::simple_target_socket<BasicTimer> tsock;
	interrupt_gateway *plic = 0;
	
	uint32_t irq_number = 0;
	uint32_t interval_reg = 10;
	bool interrupt_enabled = false;

	enum {
		INTERVAL_REG_ADDR = 0x10
	};

	SC_HAS_PROCESS(BasicTimer);

	BasicTimer(sc_core::sc_module_name, uint32_t irq_number) : irq_number(irq_number) {
		tsock.register_b_transport(this, &BasicTimer::transport);
		SC_THREAD(run);
	}

	void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
		if (trans.get_command() == tlm::TLM_WRITE_COMMAND){
			if(trans.get_address() == INTERVAL_REG_ADDR){
				interval_reg = *((uint32_t *)(trans.get_data_ptr()));
				interrupt_enabled = true;
			}
		}
	}

	void run() {
		while (true) {
			sc_core::wait(sc_core::sc_time(interval_reg, sc_core::SC_MS));
			if(interrupt_enabled){
				trigger_interrupt();
			}
		}
	}

	void trigger_interrupt(){
		plic->gateway_trigger_interrupt(irq_number);
	}
};

#endif  // RISCV_ISA_BASIC_TIMER_H
