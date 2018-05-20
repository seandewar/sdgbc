#include "debug/cpu_cmd_mode.h"
#include "util.h"
#include <cassert>
#include <iostream>
#include <sstream>

CpuCmdMode::CpuCmdMode()
    : autoResume_(false), updateGbcOnStep_(true), totalCpuSteps_(0),
      totalCpuCycles_(0) {
  gbc_.GetHardware().serial.SetSerialOutput(&serialStdout_);
}

int CpuCmdMode::Run(const std::string& romFilePath) {
  std::cout << "       *-*-*-*  sdgbc cpu-cmd mode  *-*-*-*\n";
  std::cout << "basic tool for debugging sdgbc's LR35902 emulation\n";
  std::cout << std::endl;

  // try to load the ROM we've been given from command-line args first
  // (will query for ROM if none given)
  if (ExecuteLoadRomCommand(romFilePath) != RomLoadResult::Ok) {
    do {
      std::cout << std::endl;
    } while (ExecuteLoadRomCommand() != RomLoadResult::Ok);
  }

  std::cout << "\ntype \"help\" for a list of commands\n";
  std::cout << "NOTE: commands are case sensitive!\n\n";

  std::cout << "stepgbc is on by default\n\n";

  while (true) {
    if(!ExecuteUserCommand(QueryUserCommand())) {
      break; // exit requested
    }
  }

  return EXIT_SUCCESS;
}

RomLoadResult CpuCmdMode::ExecuteLoadRomCommand(std::string romFilePath) {
  if (romFilePath.empty()) {
    std::cout << "input ROM file path > ";
    std::getline(std::cin, romFilePath);
  }

  const auto result = gbc_.LoadCartridgeRomFile(romFilePath);
  switch (result) {
    case RomLoadResult::Ok:
      std::cout << "ROM file loaded!\n";
      break;

    default:
      std::cout << "failed to load ROM - \""
                << Cartridge::GetRomLoadResultAsMessage(result) << "\"\n";
  }

  return result;
}

void CpuCmdMode::ExecuteStepCpuCommand() {
  std::cout << "input number of steps > ";
  unsigned int steps;

  if (!(std::cin >> steps)) {
    std::cout << "steps must be an integral number!\n";
  } else if (steps < 0) {
    std::cout << "invalid number of steps - must be a positive integer!\n";
  } else {
    StepCpu(steps);
  }

  ResetInputStream();
}

void CpuCmdMode::StepCpu(unsigned int steps) {
  auto& cpu = gbc_.GetHardware().cpu;

  auto stepped = 0u;
  for (; stepped < steps; ++stepped) {
    // check if the CPU can be auto-resumed from here
    if (autoResume_ && (cpu.GetStatus() == CpuStatus::Halted ||
                        cpu.GetStatus() == CpuStatus::Stopped)) {
      cpu.Resume();
    }

    if (cpu.GetStatus() == CpuStatus::Running || updateGbcOnStep_) {
      totalCpuCycles_ += updateGbcOnStep_ ? gbc_.Update() : cpu.Update();
    } else {
      std::cout << "failed to step the CPU after " << stepped << " step(s)\n";
      std::cout << "NOTE: the CPU might have halted or is hung...\n\n";
      break;
    }
  }

  totalCpuSteps_ += stepped;
  printf("stepped the CPU %d time(s) -- new PC=$%04x\n",
         stepped, cpu.GetRegisters().pc.Get());
}

void CpuCmdMode::PrintCpuStatus() const {
  // NOTE: we use printf for hexadecimal formatting because C++'s stream
  // formatters are awful. This is really only code for debug purposes, anyway
  const auto& cpu = gbc_.GetHardware().cpu;

  // print cpu run status
  std::cout << "stat: " << GetCpuStatusString() << "\n";
  // print cpu total cycles & steps ran
  std::cout << "cycl: " << totalCpuCycles_ << "\n";
  std::cout << "step: " << totalCpuSteps_ << "\n\n";

  // print register pair values
  printf("regs: PC=$%04x SP=$%04x\n", cpu.GetRegisters().pc.Get(),
                                      cpu.GetRegisters().sp.Get());
  printf("      AF=$%04x BC=$%04x DE=$%04x HL=$%04x\n",
         cpu.GetRegisters().af.Get(),
         cpu.GetRegisters().bc.Get(),
         cpu.GetRegisters().de.Get(),
         cpu.GetRegisters().hl.Get());

  // state of status flags in register F
  std::cout << "flag: "
            << (cpu.GetRegisters().f.GetZFlag() ? "Z " : "- ")
            << (cpu.GetRegisters().f.GetNFlag() ? "N " : "- ")
            << (cpu.GetRegisters().f.GetHFlag() ? "H " : "- ")
            << (cpu.GetRegisters().f.GetCFlag() ? "C"  : "-")
            << "\n";

  // state of interrupt master enable register
  std::cout << "inme: " << (cpu.GetIntme() ? "on" : "off") << "\n\n";


  std::cout << "memory around PC:\n";
  PrintMemoryArea(cpu.GetRegisters().pc.Get(), 2);
  std::cout << "\nmemory around SP:\n";
  PrintMemoryArea(cpu.GetRegisters().sp.Get(), 2);
}

void CpuCmdMode::ExecuteViewMemoryAreaCommand() {
  std::cout << "input memory address > $";
  u16 loc;

  if (!(std::cin >> std::hex >> loc)) {
    std::cout << "address must be a hexadecimal value! ($0000 to $ffff)\n";
  } else {
    printf("memory around $%04x:\n", loc);
    PrintMemoryArea(loc, 2);
  }

  std::cin.unsetf(std::ios::hex);
  ResetInputStream();
}

std::string CpuCmdMode::QueryUserCommand() const {
  const auto& hw = gbc_.GetHardware();

  std::cout << std::endl;

  std::ostringstream oss;
  oss << GetCpuStatusString()
      << (hw.dma.IsOamDmaInProgress() ? " oamdma" : "")
      << (hw.dma.IsHdmaInProgress()   ? " hdma"   : "")
      << (hw.dma.IsGdmaInProgress()   ? " gdma"   : "");

  printf("(%s, PC=$%04x, (PC)=[$%02x $%02x $%02x]) > ",
         oss.str().c_str(),
         hw.cpu.GetRegisters().pc.Get(),
         hw.mmu.Read8(hw.cpu.GetRegisters().pc.Get()),
         hw.mmu.Read8(hw.cpu.GetRegisters().pc.Get() + 1),
         hw.mmu.Read8(hw.cpu.GetRegisters().pc.Get() + 2));

  std::string cmd;
  std::getline(std::cin, cmd);
  return cmd;
}

bool CpuCmdMode::ExecuteUserCommand(const std::string& cmd) {
  if (cmd == "quit" || cmd == "exit" || cmd == "q") {
    return false; // indicate that we want to exit now
  } else if (cmd == "print" || cmd == "p") {
    PrintCpuStatus();
  } else if (cmd == "memory" || cmd == "mem" || cmd == "m") {
    ExecuteViewMemoryAreaCommand();
  } else if (cmd == "" || cmd == "next" || cmd == "n") {
    StepCpu();
  } else if (cmd == "step" || cmd == "s") {
    ExecuteStepCpuCommand();
  } else if (cmd == "unhalt" || cmd == "u") {
    if (gbc_.GetHardware().cpu.Resume()) {
      std::cout << "OK - CPU resumed!\n";
    } else {
      std::cout << "could not resume the CPU! (is it hung?)\n";
    }
  } else if (cmd == "autoresume" || cmd == "a") {
    autoResume_ = !autoResume_;
    std::cout << "autoresume is now " << (autoResume_ ? "on" : "off") << "\n";
  } else if (cmd == "stepgbc" || cmd == "g") {
    updateGbcOnStep_ = !updateGbcOnStep_;
    std::cout << "stepgbc is now " << (updateGbcOnStep_ ? "on" : "off") << "\n";
    if (updateGbcOnStep_) {
      std::cout << "NOTE: the GBC can be stepped while stepgbc is on even if\n";
      std::cout << "      the CPU is halted or hung\n";
    }
  } else if (cmd == "reset" || cmd == "r") {
    gbc_.Reset();
    totalCpuSteps_ = totalCpuCycles_ = 0;
    std::cout << "OK - system reset!\n";
  } else if (cmd == "load" || cmd == "l") {
    ExecuteLoadRomCommand();
  } else if (cmd == "help" || cmd == "h" || cmd == "?") {
    std::cout << "help | h | ?       - print a list of commands\n";
    std::cout << "load | l           - load a new program ROM file and reset\n";
    std::cout << "print | p          - print CPU status information\n";
    std::cout << "memory | mem | m   - print contents of a memory location\n";
    std::cout << "reset | r          - reset the system\n";
    std::cout << "unhalt | u         - resume the CPU if suspended\n";
    std::cout << "autoresume | a     - toggle automatic CPU unhalting\n";
    std::cout << "stepgbc | g        - toggle steps updating all hardware\n";
    std::cout << "<empty> | next | n - step to the next CPU instruction\n";
    std::cout << "step | s           - step a number of CPU instructions\n";
    std::cout << "quit | q | exit    - quit the application\n";
  } else {
    std::cout << "bad command - type \"help\" for a list of commands\n";
    std::cout << "NOTE: commands are case sensitive!\n";
  }

  return true;
}

void CpuCmdMode::PrintMemoryArea(u16 loc, unsigned int linesAboveBelow) const {
  // print column hex offsets
  std::cout << "      | 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f | ";
  std::cout << "0123456789abcdef\n";
  std::cout << "======|=================================================|=";
  std::cout << "================\n";

  for (auto i = 1u; i <= linesAboveBelow; ++i) {
    PrintMemoryRow((loc & 0xfff0) - (linesAboveBelow - i + 1) * 0x10);
    std::cout << "\n";
  }

  PrintMemoryRow(loc, true);
  std::cout << "\n";

  for (auto i = 1u; i <= linesAboveBelow; ++i) {
    PrintMemoryRow((loc & 0xfff0) + i * 0x10);
    std::cout << "\n";
  }
}

void CpuCmdMode::PrintMemoryRow(u16 loc, bool markLoc) const {
  const u16 startLoc = loc & 0xfff0;
  printf("$%04x |", startLoc);

  // print hex values
  for (auto i = 0u; i < 0x10; ++i) {
    const auto formatCStr = markLoc && (loc & 0xfu) == i ? ">%02x" : " %02x";
    printf(formatCStr, gbc_.GetHardware().mmu.Read8(startLoc + i));
  }

  std::cout << " | ";

  // print ascii values
  for (auto i = 0u; i < 0x10; ++i) {
    // make sure we don't print escape/control characters
    const u8 val = gbc_.GetHardware().mmu.Read8(startLoc + i);
    printf("%c", val < 32 || val > 126 ? '.' : val);
  }
}

std::string CpuCmdMode::GetCpuStatusString() const {
  switch (gbc_.GetHardware().cpu.GetStatus()) {
    case CpuStatus::Running:
      return "running";
    case CpuStatus::Halted:
      return "halt";
    case CpuStatus::Stopped:
      return "stop";
    case CpuStatus::Hung:
      return "hung";

    default: assert(!"unhandled cpu status string!");
      return "unknown";
  }
}

void CpuCmdMode::ResetInputStream() const {
  std::cin.clear();
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
