#include <termios.h>
#include <unistd.h>

#include <boost/io/ios_state.hpp>
#include <boost/program_options.hpp>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>

#include "core/common/clint.h"
#include "core/common/lwrt_clint.h"
#include "debug.h"
#include "debug_memory.h"
#include "elf_loader.h"
#include "fu540_plic.h"
#include "gdb-mc/gdb_runner.h"
#include "gdb-mc/gdb_server.h"
#include "iss.h"
#include "mem.h"
#include "memory.h"
#include "memory_mapped_file.h"
#include "mmu.h"
#include "platform/common/options.h"
#include "platform/common/sifive_test.h"
#include "platform/common/slip.h"
#include "platform/common/uart.h"
#include "platform/common/vncsimplefb.h"
#include "platform/common/vncsimpleinputkbd.h"
#include "platform/common/vncsimpleinputptr.h"
#include "prci.h"
#include "syscall.h"
#include "util/options.h"
#include "util/vncserver.h"

/* if not defined externally fall back to TARGET_RV64 */
#if !defined(TARGET_RV32) && !defined(TARGET_RV64)
#define TARGET_RV64
#endif

/* if not defined externally fall back to four worker cores */
#if !defined(NUM_CORES)
#define NUM_CORES (4 + 1)
#endif

#if defined(TARGET_RV32)
using namespace rv32;
#define MEM_SIZE_MB 1024  // MB ram
/*
 * on RV32 linux vmalloc size is very limited
 * -> only small memory areas (images sizes) possible
 */
#define MRAM_SIZE_MB 64  // MB mem mapped file (rootfs)

#elif defined(TARGET_RV64)
using namespace rv64;
#define MEM_SIZE_MB 2048  // MB ram
#define MRAM_SIZE_MB 512  // MB mem mapped file (rootfs)

#endif /* TARGET_RVxx */

namespace po = boost::program_options;

struct LinuxOptions : public Options {
   public:
	typedef unsigned int addr_t;

	addr_t mem_size = 1024u * 1024u * (unsigned int)(MEM_SIZE_MB);
	addr_t mem_start_addr = 0x80000000;
	addr_t mem_end_addr = mem_start_addr + mem_size - 1;
	addr_t clint_start_addr = 0x02000000;
	addr_t clint_end_addr = 0x0200ffff;
	addr_t sys_start_addr = 0x02010000;
	addr_t sys_end_addr = 0x020103ff;
	addr_t dtb_rom_start_addr = 0x00001000;
	addr_t dtb_rom_size = 0x2000;
	addr_t dtb_rom_end_addr = dtb_rom_start_addr + dtb_rom_size - 1;
	addr_t uart0_start_addr = 0x10010000;
	addr_t uart0_end_addr = 0x10010fff;
	addr_t uart1_start_addr = 0x10011000;
	addr_t uart1_end_addr = 0x10011fff;
	addr_t plic_start_addr = 0x0C000000;
	addr_t plic_end_addr = 0x10000000;
	addr_t prci_start_addr = 0x10000000;
	addr_t prci_end_addr = 0x1000FFFF;
	addr_t sifive_test_start_addr = 0x100000;
	addr_t sifive_test_end_addr = 0x100fff;
	addr_t vncsimplefb_start_addr = 0x11000000;
	addr_t vncsimplefb_end_addr = 0x11ffffff; /* 16MiB */
	addr_t vncsimpleinputptr_start_addr = 0x12000000;
	addr_t vncsimpleinputptr_end_addr = 0x12000fff;
	addr_t vncsimpleinputkbd_start_addr = 0x12001000;
	addr_t vncsimpleinputkbd_end_addr = 0x12001fff;
	addr_t mram_root_start_addr = 0x40000000;
	addr_t mram_root_size = 1024u * 1024u * (unsigned int)(MRAM_SIZE_MB);
	addr_t mram_root_end_addr = mram_root_start_addr + mram_root_size - 1;
	addr_t mram_data_start_addr = 0x60000000;
	addr_t mram_data_size = 1024u * 1024u * (unsigned int)(MRAM_SIZE_MB);
	addr_t mram_data_end_addr = mram_data_start_addr + mram_data_size - 1;

	OptionValue<unsigned long> entry_point;
	std::string dtb_file;
	std::string tun_device = "tun0";
	std::string mram_root_image;
	std::string mram_data_image;

	unsigned int vnc_port = 5900;

	LinuxOptions(void) {
		// clang-format off
		add_options()
			("memory-start", po::value<unsigned int>(&mem_start_addr),"set memory start address")
			("memory-size", po::value<unsigned int>(&mem_size), "set memory size")
			("entry-point", po::value<std::string>(&entry_point.option),"set entry point address (ISS program counter)")
			("dtb-file", po::value<std::string>(&dtb_file)->required(), "dtb file for boot loading")
			("tun-device", po::value<std::string>(&tun_device), "tun device used by SLIP")
			("mram-root-image", po::value<std::string>(&mram_root_image)->default_value(""),"MRAM root image file")
			("mram-root-image-size", po::value<unsigned int>(&mram_root_size), "MRAM root image size")
			("mram-data-image", po::value<std::string>(&mram_data_image)->default_value(""),"MRAM data image file for persistency")
			("mram-data-image-size", po::value<unsigned int>(&mram_data_size), "MRAM data image size")
			("vnc-port", po::value<unsigned int>(&vnc_port), "select port number to connect with VNC");
		// clang-format on
	}

	void parse(int argc, char **argv) override {
		Options::parse(argc, argv);
		entry_point.finalize(parse_ulong_option);
		mem_end_addr = mem_start_addr + mem_size - 1;
		mram_root_end_addr = mram_root_start_addr + mram_root_size - 1;
		assert(mram_root_end_addr < mram_data_start_addr && "MRAM root too big, would overlap MRAM root");
		mram_data_end_addr = mram_data_start_addr + mram_data_size - 1;
		assert(mram_data_end_addr < mem_start_addr && "MRAM too big, would overlap memory");
	}
};

class Core {
   public:
	ISS iss;
	MMU mmu;
	CombinedMemoryInterface memif;
	InstrMemoryProxy imemif;

	Core(unsigned int id, MemoryDMI dmi)
	    : iss(id), mmu(iss), memif(("MemoryInterface" + std::to_string(id)).c_str(), iss, &mmu), imemif(dmi, iss) {
		return;
	}

	void init(bool use_data_dmi, bool use_instr_dmi, clint_if *clint, uint64_t entry, uint64_t addr) {
		if (use_data_dmi)
			memif.dmi_ranges.emplace_back(imemif.dmi);
		iss.init(get_instr_memory_if(use_instr_dmi), &memif, clint, entry, addr);
	}

   private:
	instr_memory_if *get_instr_memory_if(bool use_instr_dmi) {
		if (use_instr_dmi)
			return &imemif;
		else
			return &memif;
	}
};

int sc_main(int argc, char **argv) {
	LinuxOptions opt;
	opt.parse(argc, argv);

	std::srand(std::time(nullptr));  // use current time as seed for random generator

	tlm::tlm_global_quantum::instance().set(sc_core::sc_time(opt.tlm_global_quantum, sc_core::SC_NS));

	VNCServer vncServer("RISC-V VP++ VNCServer", opt.vnc_port);

	SimpleMemory mem("SimpleMemory", opt.mem_size);
	SimpleMemory dtb_rom("DBT_ROM", opt.dtb_rom_size);
	ELFLoader loader(opt.input_program.c_str());
	SimpleBus<NUM_CORES + 1, 14> bus("SimpleBus");
	SyscallHandler sys("SyscallHandler");
	FU540_PLIC plic("PLIC", NUM_CORES);
	LWRT_CLINT<NUM_CORES> clint("CLINT");
	PRCI prci("PRCI");
	UART uart0("UART0", 3);
	SLIP slip("SLIP", 4, opt.tun_device);
	SIFIVE_Test sifive_test("SIFIVE_Test");
	VNCSimpleFB vncsimplefb("VNCSimpleFB", vncServer);
	VNCSimpleInputPtr vncsimpleinputptr("VNCSimpleInputPtr", vncServer, 10);
	VNCSimpleInputKbd vncsimpleinputkbd("VNCSimpleInputKbd", vncServer, 11);
	DebugMemoryInterface dbg_if("DebugMemoryInterface");
	MemoryDMI dmi = MemoryDMI::create_start_size_mapping(mem.data, opt.mem_start_addr, mem.size);
	MemoryMappedFile mramRoot("MRAM_Root", opt.mram_root_image, opt.mram_root_size);
	MemoryMappedFile mramData("MRAM_Data", opt.mram_data_image, opt.mram_data_size);

	Core *cores[NUM_CORES];
	for (unsigned i = 0; i < NUM_CORES; i++) {
		cores[i] = new Core(i, dmi);
	}

	std::shared_ptr<BusLock> bus_lock = std::make_shared<BusLock>();
	for (size_t i = 0; i < NUM_CORES; i++) {
		cores[i]->memif.bus_lock = bus_lock;
		cores[i]->mmu.mem = &cores[i]->memif;
	}

	uint64_t entry_point = loader.get_entrypoint();
	if (opt.entry_point.available)
		entry_point = opt.entry_point.value;

	loader.load_executable_image(mem, mem.size, opt.mem_start_addr);
	sys.init(mem.data, opt.mem_start_addr, loader.get_heap_addr());
	for (size_t i = 0; i < NUM_CORES; i++) {
		cores[i]->init(opt.use_data_dmi, opt.use_instr_dmi, &clint, entry_point, rv64_align_address(opt.mem_end_addr));

		sys.register_core(&cores[i]->iss);
		if (opt.intercept_syscalls)
			cores[i]->iss.sys = &sys;
	}

	// setup port mapping
	bus.ports[0] = new PortMapping(opt.mem_start_addr, opt.mem_end_addr);
	bus.ports[1] = new PortMapping(opt.clint_start_addr, opt.clint_end_addr);
	bus.ports[2] = new PortMapping(opt.sys_start_addr, opt.sys_end_addr);
	bus.ports[3] = new PortMapping(opt.dtb_rom_start_addr, opt.dtb_rom_end_addr);
	bus.ports[4] = new PortMapping(opt.uart0_start_addr, opt.uart0_end_addr);
	bus.ports[5] = new PortMapping(opt.uart1_start_addr, opt.uart1_end_addr);
	bus.ports[6] = new PortMapping(opt.plic_start_addr, opt.plic_end_addr);
	bus.ports[7] = new PortMapping(opt.prci_start_addr, opt.prci_end_addr);
	bus.ports[8] = new PortMapping(opt.sifive_test_start_addr, opt.sifive_test_end_addr);
	bus.ports[9] = new PortMapping(opt.vncsimplefb_start_addr, opt.vncsimplefb_end_addr);
	bus.ports[10] = new PortMapping(opt.vncsimpleinputptr_start_addr, opt.vncsimpleinputptr_end_addr);
	bus.ports[11] = new PortMapping(opt.vncsimpleinputkbd_start_addr, opt.vncsimpleinputkbd_end_addr);
	bus.ports[12] = new PortMapping(opt.mram_root_start_addr, opt.mram_root_end_addr);
	bus.ports[13] = new PortMapping(opt.mram_data_start_addr, opt.mram_data_end_addr);

	// connect TLM sockets
	for (size_t i = 0; i < NUM_CORES; i++) {
		cores[i]->memif.isock.bind(bus.tsocks[i]);
	}
	dbg_if.isock.bind(bus.tsocks[NUM_CORES]);
	bus.isocks[0].bind(mem.tsock);
	bus.isocks[1].bind(clint.tsock);
	bus.isocks[2].bind(sys.tsock);
	bus.isocks[3].bind(dtb_rom.tsock);
	bus.isocks[4].bind(uart0.tsock);
	bus.isocks[5].bind(slip.tsock);
	bus.isocks[6].bind(plic.tsock);
	bus.isocks[7].bind(prci.tsock);
	bus.isocks[8].bind(sifive_test.tsock);
	bus.isocks[9].bind(vncsimplefb.tsock);
	bus.isocks[10].bind(vncsimpleinputptr.tsock);
	bus.isocks[11].bind(vncsimpleinputkbd.tsock);
	bus.isocks[12].bind(mramRoot.tsock);
	bus.isocks[13].bind(mramData.tsock);

	// connect interrupt signals/communication
	for (size_t i = 0; i < NUM_CORES; i++) {
		plic.target_harts[i] = &cores[i]->iss;
		clint.target_harts[i] = &cores[i]->iss;
	}
	uart0.plic = &plic;
	slip.plic = &plic;
	vncsimpleinputptr.plic = &plic;
	vncsimpleinputkbd.plic = &plic;

	for (size_t i = 0; i < NUM_CORES; i++) {
		// switch for printing instructions
		cores[i]->iss.trace = opt.trace_mode;

		// ignore WFI instructions (handle them as a NOP, which is ok according to the RISC-V ISA) to avoid running too
		// fast ahead with simulation time when the CPU is idle
		cores[i]->iss.ignore_wfi = true;

		// emulate RISC-V core boot loader
		cores[i]->iss.regs[RegFile::a0] = cores[i]->iss.get_hart_id();
		cores[i]->iss.regs[RegFile::a1] = opt.dtb_rom_start_addr;

#ifdef TARGET_RV32
		// configure supported instructions
		cores[i]->iss.csrs.misa.fields.extensions |= cores[i]->iss.csrs.misa.M | cores[i]->iss.csrs.misa.A |
		                                             cores[i]->iss.csrs.misa.F | cores[i]->iss.csrs.misa.D;
#endif /* TARGET_RV32 */
	}

	// OpenSBI boots all harts except hart 0 by default.
	//
	// To prevent this hart from being scheduled when stuck in
	// the OpenSBI `sbi_hart_hang()` function do not ignore WFI on
	// this hart.
	//
	// See: https://github.com/riscv/opensbi/commit/d70f8aab45d1e449b3b9be26e050b20ed76e12e9
	cores[0]->iss.ignore_wfi = false;

	// load DTB (Device Tree Binary) file
	dtb_rom.load_binary_file(opt.dtb_file, 0);

	std::vector<mmu_memory_if *> mmus;
	std::vector<debug_target_if *> dharts;
	if (opt.use_debug_runner) {
		for (size_t i = 0; i < NUM_CORES; i++) {
			dharts.push_back(&cores[i]->iss);
			mmus.push_back(&cores[i]->memif);
		}

		auto server = new GDBServer("GDBServer", dharts, &dbg_if, opt.debug_port, mmus);
		for (size_t i = 0; i < dharts.size(); i++)
			new GDBServerRunner(("GDBRunner" + std::to_string(i)).c_str(), server, dharts[i]);
	} else {
		for (size_t i = 0; i < NUM_CORES; i++) {
			new DirectCoreRunner(cores[i]->iss);
		}
	}

	sc_core::sc_start();
	for (size_t i = 0; i < NUM_CORES; i++) {
		cores[i]->iss.show();
	}

	return 0;
}
