#ifndef SDGBC_CPU_CMD_MODE_H_
#define SDGBC_CPU_CMD_MODE_H_

#include "debug/serial_stdout.h"
#include "hw/gbc.h"

class CpuCmdMode {
public:
  CpuCmdMode();

  int Run(const std::string& romFilePath = {});

private:
  Gbc gbc_;
  SerialStdout serialStdout_;

  unsigned long long totalCpuSteps_, totalCpuCycles_;
  bool autoResume_, updateGbcOnStep_;

  void ResetInputStream() const;

  std::string QueryUserCommand() const;
  bool ExecuteUserCommand(const std::string& cmd);
  RomLoadResult ExecuteLoadRomCommand(std::string romFilePath = {});
  void ExecuteViewMemoryAreaCommand();

  void ExecuteStepCpuCommand();
  void StepCpu(unsigned int steps = 1);

  void PrintCpuStatus() const;
  void PrintMemoryArea(u16 middleLoc, unsigned int linesAboveBelow) const;
  void PrintMemoryRow(u16 loc, bool markLoc = false) const;
  std::string GetCpuStatusString() const;
};

#endif // SDGBC_CPU_CMD_MODE_H_
