//===- ARMBaseRegisterInfo.cpp - ARM Register Information -----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the base ARM implementation of TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#include "ARM.h"
#include "ARMAddressingModes.h"
#include "ARMBaseInstrInfo.h"
#include "ARMBaseRegisterInfo.h"
#include "ARMInstrInfo.h"
#include "ARMMachineFunctionInfo.h"
#include "ARMSubtarget.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineLocation.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetFrameInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/SmallVector.h"
using namespace llvm;

unsigned ARMBaseRegisterInfo::getRegisterNumbering(unsigned RegEnum) {
  using namespace ARM;
  switch (RegEnum) {
  case R0:  case S0:  case D0:  return 0;
  case R1:  case S1:  case D1:  return 1;
  case R2:  case S2:  case D2:  return 2;
  case R3:  case S3:  case D3:  return 3;
  case R4:  case S4:  case D4:  return 4;
  case R5:  case S5:  case D5:  return 5;
  case R6:  case S6:  case D6:  return 6;
  case R7:  case S7:  case D7:  return 7;
  case R8:  case S8:  case D8:  return 8;
  case R9:  case S9:  case D9:  return 9;
  case R10: case S10: case D10: return 10;
  case R11: case S11: case D11: return 11;
  case R12: case S12: case D12: return 12;
  case SP:  case S13: case D13: return 13;
  case LR:  case S14: case D14: return 14;
  case PC:  case S15: case D15: return 15;
  case S16: return 16;
  case S17: return 17;
  case S18: return 18;
  case S19: return 19;
  case S20: return 20;
  case S21: return 21;
  case S22: return 22;
  case S23: return 23;
  case S24: return 24;
  case S25: return 25;
  case S26: return 26;
  case S27: return 27;
  case S28: return 28;
  case S29: return 29;
  case S30: return 30;
  case S31: return 31;
  default:
    LLVM_UNREACHABLE("Unknown ARM register!");
  }
}

unsigned ARMBaseRegisterInfo::getRegisterNumbering(unsigned RegEnum,
                                                   bool &isSPVFP) {
  isSPVFP = false;

  using namespace ARM;
  switch (RegEnum) {
  default:
    LLVM_UNREACHABLE("Unknown ARM register!");
  case R0:  case D0:  return 0;
  case R1:  case D1:  return 1;
  case R2:  case D2:  return 2;
  case R3:  case D3:  return 3;
  case R4:  case D4:  return 4;
  case R5:  case D5:  return 5;
  case R6:  case D6:  return 6;
  case R7:  case D7:  return 7;
  case R8:  case D8:  return 8;
  case R9:  case D9:  return 9;
  case R10: case D10: return 10;
  case R11: case D11: return 11;
  case R12: case D12: return 12;
  case SP:  case D13: return 13;
  case LR:  case D14: return 14;
  case PC:  case D15: return 15;

  case S0: case S1: case S2: case S3:
  case S4: case S5: case S6: case S7:
  case S8: case S9: case S10: case S11:
  case S12: case S13: case S14: case S15:
  case S16: case S17: case S18: case S19:
  case S20: case S21: case S22: case S23:
  case S24: case S25: case S26: case S27:
  case S28: case S29: case S30: case S31:  {
    isSPVFP = true;
    switch (RegEnum) {
    default: return 0; // Avoid compile time warning.
    case S0: return 0;
    case S1: return 1;
    case S2: return 2;
    case S3: return 3;
    case S4: return 4;
    case S5: return 5;
    case S6: return 6;
    case S7: return 7;
    case S8: return 8;
    case S9: return 9;
    case S10: return 10;
    case S11: return 11;
    case S12: return 12;
    case S13: return 13;
    case S14: return 14;
    case S15: return 15;
    case S16: return 16;
    case S17: return 17;
    case S18: return 18;
    case S19: return 19;
    case S20: return 20;
    case S21: return 21;
    case S22: return 22;
    case S23: return 23;
    case S24: return 24;
    case S25: return 25;
    case S26: return 26;
    case S27: return 27;
    case S28: return 28;
    case S29: return 29;
    case S30: return 30;
    case S31: return 31;
    }
  }
  }
}

ARMBaseRegisterInfo::ARMBaseRegisterInfo(const ARMBaseInstrInfo &tii,
                                         const ARMSubtarget &sti)
  : ARMGenRegisterInfo(ARM::ADJCALLSTACKDOWN, ARM::ADJCALLSTACKUP),
    TII(tii), STI(sti),
    FramePtr((STI.isTargetDarwin() || STI.isThumb()) ? ARM::R7 : ARM::R11) {
}

unsigned ARMBaseRegisterInfo::
getOpcode(int Op) const {
  return TII.getOpcode((ARMII::Op)Op);
}

const unsigned*
ARMBaseRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  static const unsigned CalleeSavedRegs[] = {
    ARM::LR, ARM::R11, ARM::R10, ARM::R9, ARM::R8,
    ARM::R7, ARM::R6,  ARM::R5,  ARM::R4,

    ARM::D15, ARM::D14, ARM::D13, ARM::D12,
    ARM::D11, ARM::D10, ARM::D9,  ARM::D8,
    0
  };

  static const unsigned DarwinCalleeSavedRegs[] = {
    // Darwin ABI deviates from ARM standard ABI. R9 is not a callee-saved
    // register.
    ARM::LR,  ARM::R7,  ARM::R6, ARM::R5, ARM::R4,
    ARM::R11, ARM::R10, ARM::R8,

    ARM::D15, ARM::D14, ARM::D13, ARM::D12,
    ARM::D11, ARM::D10, ARM::D9,  ARM::D8,
    0
  };
  return STI.isTargetDarwin() ? DarwinCalleeSavedRegs : CalleeSavedRegs;
}

const TargetRegisterClass* const *
ARMBaseRegisterInfo::getCalleeSavedRegClasses(const MachineFunction *MF) const {
  static const TargetRegisterClass * const CalleeSavedRegClasses[] = {
    &ARM::GPRRegClass, &ARM::GPRRegClass, &ARM::GPRRegClass,
    &ARM::GPRRegClass, &ARM::GPRRegClass, &ARM::GPRRegClass,
    &ARM::GPRRegClass, &ARM::GPRRegClass, &ARM::GPRRegClass,

    &ARM::DPRRegClass, &ARM::DPRRegClass, &ARM::DPRRegClass, &ARM::DPRRegClass,
    &ARM::DPRRegClass, &ARM::DPRRegClass, &ARM::DPRRegClass, &ARM::DPRRegClass,
    0
  };

  static const TargetRegisterClass * const ThumbCalleeSavedRegClasses[] = {
    &ARM::GPRRegClass, &ARM::GPRRegClass, &ARM::GPRRegClass,
    &ARM::GPRRegClass, &ARM::GPRRegClass, &ARM::tGPRRegClass,
    &ARM::tGPRRegClass,&ARM::tGPRRegClass,&ARM::tGPRRegClass,

    &ARM::DPRRegClass, &ARM::DPRRegClass, &ARM::DPRRegClass, &ARM::DPRRegClass,
    &ARM::DPRRegClass, &ARM::DPRRegClass, &ARM::DPRRegClass, &ARM::DPRRegClass,
    0
  };

  static const TargetRegisterClass * const DarwinCalleeSavedRegClasses[] = {
    &ARM::GPRRegClass, &ARM::GPRRegClass, &ARM::GPRRegClass,
    &ARM::GPRRegClass, &ARM::GPRRegClass, &ARM::GPRRegClass,
    &ARM::GPRRegClass, &ARM::GPRRegClass,

    &ARM::DPRRegClass, &ARM::DPRRegClass, &ARM::DPRRegClass, &ARM::DPRRegClass,
    &ARM::DPRRegClass, &ARM::DPRRegClass, &ARM::DPRRegClass, &ARM::DPRRegClass,
    0
  };

  static const TargetRegisterClass * const DarwinThumbCalleeSavedRegClasses[] ={
    &ARM::GPRRegClass,  &ARM::tGPRRegClass, &ARM::tGPRRegClass,
    &ARM::tGPRRegClass, &ARM::tGPRRegClass, &ARM::GPRRegClass,
    &ARM::GPRRegClass,  &ARM::GPRRegClass,

    &ARM::DPRRegClass, &ARM::DPRRegClass, &ARM::DPRRegClass, &ARM::DPRRegClass,
    &ARM::DPRRegClass, &ARM::DPRRegClass, &ARM::DPRRegClass, &ARM::DPRRegClass,
    0
  };

  if (STI.isThumb1Only()) {
    return STI.isTargetDarwin()
      ? DarwinThumbCalleeSavedRegClasses : ThumbCalleeSavedRegClasses;
  }
  return STI.isTargetDarwin()
    ? DarwinCalleeSavedRegClasses : CalleeSavedRegClasses;
}

BitVector ARMBaseRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  // FIXME: avoid re-calculating this everytime.
  BitVector Reserved(getNumRegs());
  Reserved.set(ARM::SP);
  Reserved.set(ARM::PC);
  if (STI.isTargetDarwin() || hasFP(MF))
    Reserved.set(FramePtr);
  // Some targets reserve R9.
  if (STI.isR9Reserved())
    Reserved.set(ARM::R9);
  return Reserved;
}

bool
ARMBaseRegisterInfo::isReservedReg(const MachineFunction &MF, unsigned Reg) const {
  switch (Reg) {
  default: break;
  case ARM::SP:
  case ARM::PC:
    return true;
  case ARM::R7:
  case ARM::R11:
    if (FramePtr == Reg && (STI.isTargetDarwin() || hasFP(MF)))
      return true;
    break;
  case ARM::R9:
    return STI.isR9Reserved();
  }

  return false;
}

const TargetRegisterClass *ARMBaseRegisterInfo::getPointerRegClass() const {
  return &ARM::GPRRegClass;
}

/// getAllocationOrder - Returns the register allocation order for a specified
/// register class in the form of a pair of TargetRegisterClass iterators.
std::pair<TargetRegisterClass::iterator,TargetRegisterClass::iterator>
ARMBaseRegisterInfo::getAllocationOrder(const TargetRegisterClass *RC,
                                        unsigned HintType, unsigned HintReg,
                                        const MachineFunction &MF) const {
  // Alternative register allocation orders when favoring even / odd registers
  // of register pairs.

  // No FP, R9 is available.
  static const unsigned GPREven1[] = {
    ARM::R0, ARM::R2, ARM::R4, ARM::R6, ARM::R8, ARM::R10,
    ARM::R1, ARM::R3, ARM::R12,ARM::LR, ARM::R5, ARM::R7,
    ARM::R9, ARM::R11
  };
  static const unsigned GPROdd1[] = {
    ARM::R1, ARM::R3, ARM::R5, ARM::R7, ARM::R9, ARM::R11,
    ARM::R0, ARM::R2, ARM::R12,ARM::LR, ARM::R4, ARM::R6,
    ARM::R8, ARM::R10
  };

  // FP is R7, R9 is available.
  static const unsigned GPREven2[] = {
    ARM::R0, ARM::R2, ARM::R4,          ARM::R8, ARM::R10,
    ARM::R1, ARM::R3, ARM::R12,ARM::LR, ARM::R5, ARM::R6,
    ARM::R9, ARM::R11
  };
  static const unsigned GPROdd2[] = {
    ARM::R1, ARM::R3, ARM::R5,          ARM::R9, ARM::R11,
    ARM::R0, ARM::R2, ARM::R12,ARM::LR, ARM::R4, ARM::R6,
    ARM::R8, ARM::R10
  };

  // FP is R11, R9 is available.
  static const unsigned GPREven3[] = {
    ARM::R0, ARM::R2, ARM::R4, ARM::R6, ARM::R8,
    ARM::R1, ARM::R3, ARM::R10,ARM::R12,ARM::LR, ARM::R5, ARM::R7,
    ARM::R9
  };
  static const unsigned GPROdd3[] = {
    ARM::R1, ARM::R3, ARM::R5, ARM::R6, ARM::R9,
    ARM::R0, ARM::R2, ARM::R10,ARM::R12,ARM::LR, ARM::R4, ARM::R7,
    ARM::R8
  };

  // No FP, R9 is not available.
  static const unsigned GPREven4[] = {
    ARM::R0, ARM::R2, ARM::R4, ARM::R6,          ARM::R10,
    ARM::R1, ARM::R3, ARM::R12,ARM::LR, ARM::R5, ARM::R7, ARM::R8,
    ARM::R11
  };
  static const unsigned GPROdd4[] = {
    ARM::R1, ARM::R3, ARM::R5, ARM::R7,          ARM::R11,
    ARM::R0, ARM::R2, ARM::R12,ARM::LR, ARM::R4, ARM::R6, ARM::R8,
    ARM::R10
  };

  // FP is R7, R9 is not available.
  static const unsigned GPREven5[] = {
    ARM::R0, ARM::R2, ARM::R4,                   ARM::R10,
    ARM::R1, ARM::R3, ARM::R12,ARM::LR, ARM::R5, ARM::R6, ARM::R8,
    ARM::R11
  };
  static const unsigned GPROdd5[] = {
    ARM::R1, ARM::R3, ARM::R5,                   ARM::R11,
    ARM::R0, ARM::R2, ARM::R12,ARM::LR, ARM::R4, ARM::R6, ARM::R8,
    ARM::R10
  };

  // FP is R11, R9 is not available.
  static const unsigned GPREven6[] = {
    ARM::R0, ARM::R2, ARM::R4, ARM::R6,
    ARM::R1, ARM::R3, ARM::R10,ARM::R12,ARM::LR, ARM::R5, ARM::R7, ARM::R8
  };
  static const unsigned GPROdd6[] = {
    ARM::R1, ARM::R3, ARM::R5, ARM::R7,
    ARM::R0, ARM::R2, ARM::R10,ARM::R12,ARM::LR, ARM::R4, ARM::R6, ARM::R8
  };


  if (HintType == ARMRI::RegPairEven) {
    if (isPhysicalRegister(HintReg) && getRegisterPairEven(HintReg, MF) == 0)
      // It's no longer possible to fulfill this hint. Return the default
      // allocation order.
      return std::make_pair(RC->allocation_order_begin(MF),
                            RC->allocation_order_end(MF));

    if (!STI.isTargetDarwin() && !hasFP(MF)) {
      if (!STI.isR9Reserved())
        return std::make_pair(GPREven1,
                              GPREven1 + (sizeof(GPREven1)/sizeof(unsigned)));
      else
        return std::make_pair(GPREven4,
                              GPREven4 + (sizeof(GPREven4)/sizeof(unsigned)));
    } else if (FramePtr == ARM::R7) {
      if (!STI.isR9Reserved())
        return std::make_pair(GPREven2,
                              GPREven2 + (sizeof(GPREven2)/sizeof(unsigned)));
      else
        return std::make_pair(GPREven5,
                              GPREven5 + (sizeof(GPREven5)/sizeof(unsigned)));
    } else { // FramePtr == ARM::R11
      if (!STI.isR9Reserved())
        return std::make_pair(GPREven3,
                              GPREven3 + (sizeof(GPREven3)/sizeof(unsigned)));
      else
        return std::make_pair(GPREven6,
                              GPREven6 + (sizeof(GPREven6)/sizeof(unsigned)));
    }
  } else if (HintType == ARMRI::RegPairOdd) {
    if (isPhysicalRegister(HintReg) && getRegisterPairOdd(HintReg, MF) == 0)
      // It's no longer possible to fulfill this hint. Return the default
      // allocation order.
      return std::make_pair(RC->allocation_order_begin(MF),
                            RC->allocation_order_end(MF));

    if (!STI.isTargetDarwin() && !hasFP(MF)) {
      if (!STI.isR9Reserved())
        return std::make_pair(GPROdd1,
                              GPROdd1 + (sizeof(GPROdd1)/sizeof(unsigned)));
      else
        return std::make_pair(GPROdd4,
                              GPROdd4 + (sizeof(GPROdd4)/sizeof(unsigned)));
    } else if (FramePtr == ARM::R7) {
      if (!STI.isR9Reserved())
        return std::make_pair(GPROdd2,
                              GPROdd2 + (sizeof(GPROdd2)/sizeof(unsigned)));
      else
        return std::make_pair(GPROdd5,
                              GPROdd5 + (sizeof(GPROdd5)/sizeof(unsigned)));
    } else { // FramePtr == ARM::R11
      if (!STI.isR9Reserved())
        return std::make_pair(GPROdd3,
                              GPROdd3 + (sizeof(GPROdd3)/sizeof(unsigned)));
      else
        return std::make_pair(GPROdd6,
                              GPROdd6 + (sizeof(GPROdd6)/sizeof(unsigned)));
    }
  }
  return std::make_pair(RC->allocation_order_begin(MF),
                        RC->allocation_order_end(MF));
}

/// ResolveRegAllocHint - Resolves the specified register allocation hint
/// to a physical register. Returns the physical register if it is successful.
unsigned
ARMBaseRegisterInfo::ResolveRegAllocHint(unsigned Type, unsigned Reg,
                                         const MachineFunction &MF) const {
  if (Reg == 0 || !isPhysicalRegister(Reg))
    return 0;
  if (Type == 0)
    return Reg;
  else if (Type == (unsigned)ARMRI::RegPairOdd)
    // Odd register.
    return getRegisterPairOdd(Reg, MF);
  else if (Type == (unsigned)ARMRI::RegPairEven)
    // Even register.
    return getRegisterPairEven(Reg, MF);
  return 0;
}

void
ARMBaseRegisterInfo::UpdateRegAllocHint(unsigned Reg, unsigned NewReg,
                                        MachineFunction &MF) const {
  MachineRegisterInfo *MRI = &MF.getRegInfo();
  std::pair<unsigned, unsigned> Hint = MRI->getRegAllocationHint(Reg);
  if ((Hint.first == (unsigned)ARMRI::RegPairOdd ||
       Hint.first == (unsigned)ARMRI::RegPairEven) &&
      Hint.second && TargetRegisterInfo::isVirtualRegister(Hint.second)) {
    // If 'Reg' is one of the even / odd register pair and it's now changed
    // (e.g. coalesced) into a different register. The other register of the
    // pair allocation hint must be updated to reflect the relationship
    // change.
    unsigned OtherReg = Hint.second;
    Hint = MRI->getRegAllocationHint(OtherReg);
    if (Hint.second == Reg)
      // Make sure the pair has not already divorced.
      MRI->setRegAllocationHint(OtherReg, Hint.first, NewReg);
  }
}

/// hasFP - Return true if the specified function should have a dedicated frame
/// pointer register.  This is true if the function has variable sized allocas
/// or if frame pointer elimination is disabled.
///
bool ARMBaseRegisterInfo::hasFP(const MachineFunction &MF) const {
  const MachineFrameInfo *MFI = MF.getFrameInfo();
  return (NoFramePointerElim ||
          MFI->hasVarSizedObjects() ||
          MFI->isFrameAddressTaken());
}

static unsigned estimateStackSize(MachineFunction &MF, MachineFrameInfo *MFI) {
  const MachineFrameInfo *FFI = MF.getFrameInfo();
  int Offset = 0;
  for (int i = FFI->getObjectIndexBegin(); i != 0; ++i) {
    int FixedOff = -FFI->getObjectOffset(i);
    if (FixedOff > Offset) Offset = FixedOff;
  }
  for (unsigned i = 0, e = FFI->getObjectIndexEnd(); i != e; ++i) {
    if (FFI->isDeadObjectIndex(i))
      continue;
    Offset += FFI->getObjectSize(i);
    unsigned Align = FFI->getObjectAlignment(i);
    // Adjust to alignment boundary
    Offset = (Offset+Align-1)/Align*Align;
  }
  return (unsigned)Offset;
}

void
ARMBaseRegisterInfo::processFunctionBeforeCalleeSavedScan(MachineFunction &MF,
                                                          RegScavenger *RS) const {
  // This tells PEI to spill the FP as if it is any other callee-save register
  // to take advantage the eliminateFrameIndex machinery. This also ensures it
  // is spilled in the order specified by getCalleeSavedRegs() to make it easier
  // to combine multiple loads / stores.
  bool CanEliminateFrame = true;
  bool CS1Spilled = false;
  bool LRSpilled = false;
  unsigned NumGPRSpills = 0;
  SmallVector<unsigned, 4> UnspilledCS1GPRs;
  SmallVector<unsigned, 4> UnspilledCS2GPRs;
  ARMFunctionInfo *AFI = MF.getInfo<ARMFunctionInfo>();

  // Don't spill FP if the frame can be eliminated. This is determined
  // by scanning the callee-save registers to see if any is used.
  const unsigned *CSRegs = getCalleeSavedRegs();
  const TargetRegisterClass* const *CSRegClasses = getCalleeSavedRegClasses();
  for (unsigned i = 0; CSRegs[i]; ++i) {
    unsigned Reg = CSRegs[i];
    bool Spilled = false;
    if (MF.getRegInfo().isPhysRegUsed(Reg)) {
      AFI->setCSRegisterIsSpilled(Reg);
      Spilled = true;
      CanEliminateFrame = false;
    } else {
      // Check alias registers too.
      for (const unsigned *Aliases = getAliasSet(Reg); *Aliases; ++Aliases) {
        if (MF.getRegInfo().isPhysRegUsed(*Aliases)) {
          Spilled = true;
          CanEliminateFrame = false;
        }
      }
    }

    if (CSRegClasses[i] == &ARM::GPRRegClass) {
      if (Spilled) {
        NumGPRSpills++;

        if (!STI.isTargetDarwin()) {
          if (Reg == ARM::LR)
            LRSpilled = true;
          CS1Spilled = true;
          continue;
        }

        // Keep track if LR and any of R4, R5, R6, and R7 is spilled.
        switch (Reg) {
        case ARM::LR:
          LRSpilled = true;
          // Fallthrough
        case ARM::R4:
        case ARM::R5:
        case ARM::R6:
        case ARM::R7:
          CS1Spilled = true;
          break;
        default:
          break;
        }
      } else {
        if (!STI.isTargetDarwin()) {
          UnspilledCS1GPRs.push_back(Reg);
          continue;
        }

        switch (Reg) {
        case ARM::R4:
        case ARM::R5:
        case ARM::R6:
        case ARM::R7:
        case ARM::LR:
          UnspilledCS1GPRs.push_back(Reg);
          break;
        default:
          UnspilledCS2GPRs.push_back(Reg);
          break;
        }
      }
    }
  }

  bool ForceLRSpill = false;
  if (!LRSpilled && AFI->isThumb1OnlyFunction()) {
    unsigned FnSize = TII.GetFunctionSizeInBytes(MF);
    // Force LR to be spilled if the Thumb function size is > 2048. This enables
    // use of BL to implement far jump. If it turns out that it's not needed
    // then the branch fix up path will undo it.
    if (FnSize >= (1 << 11)) {
      CanEliminateFrame = false;
      ForceLRSpill = true;
    }
  }

  bool ExtraCSSpill = false;
  if (!CanEliminateFrame || hasFP(MF)) {
    AFI->setHasStackFrame(true);

    // If LR is not spilled, but at least one of R4, R5, R6, and R7 is spilled.
    // Spill LR as well so we can fold BX_RET to the registers restore (LDM).
    if (!LRSpilled && CS1Spilled) {
      MF.getRegInfo().setPhysRegUsed(ARM::LR);
      AFI->setCSRegisterIsSpilled(ARM::LR);
      NumGPRSpills++;
      UnspilledCS1GPRs.erase(std::find(UnspilledCS1GPRs.begin(),
                                    UnspilledCS1GPRs.end(), (unsigned)ARM::LR));
      ForceLRSpill = false;
      ExtraCSSpill = true;
    }

    // Darwin ABI requires FP to point to the stack slot that contains the
    // previous FP.
    if (STI.isTargetDarwin() || hasFP(MF)) {
      MF.getRegInfo().setPhysRegUsed(FramePtr);
      NumGPRSpills++;
    }

    // If stack and double are 8-byte aligned and we are spilling an odd number
    // of GPRs. Spill one extra callee save GPR so we won't have to pad between
    // the integer and double callee save areas.
    unsigned TargetAlign = MF.getTarget().getFrameInfo()->getStackAlignment();
    if (TargetAlign == 8 && (NumGPRSpills & 1)) {
      if (CS1Spilled && !UnspilledCS1GPRs.empty()) {
        for (unsigned i = 0, e = UnspilledCS1GPRs.size(); i != e; ++i) {
          unsigned Reg = UnspilledCS1GPRs[i];
          // Don't spill high register if the function is thumb1
          if (!AFI->isThumb1OnlyFunction() ||
              isARMLowRegister(Reg) || Reg == ARM::LR) {
            MF.getRegInfo().setPhysRegUsed(Reg);
            AFI->setCSRegisterIsSpilled(Reg);
            if (!isReservedReg(MF, Reg))
              ExtraCSSpill = true;
            break;
          }
        }
      } else if (!UnspilledCS2GPRs.empty() &&
                 !AFI->isThumb1OnlyFunction()) {
        unsigned Reg = UnspilledCS2GPRs.front();
        MF.getRegInfo().setPhysRegUsed(Reg);
        AFI->setCSRegisterIsSpilled(Reg);
        if (!isReservedReg(MF, Reg))
          ExtraCSSpill = true;
      }
    }

    // Estimate if we might need to scavenge a register at some point in order
    // to materialize a stack offset. If so, either spill one additional
    // callee-saved register or reserve a special spill slot to facilitate
    // register scavenging.
    if (RS && !ExtraCSSpill && !AFI->isThumb1OnlyFunction()) {
      MachineFrameInfo  *MFI = MF.getFrameInfo();
      unsigned Size = estimateStackSize(MF, MFI);
      unsigned Limit = (1 << 12) - 1;
      for (MachineFunction::iterator BB = MF.begin(),E = MF.end();BB != E; ++BB)
        for (MachineBasicBlock::iterator I= BB->begin(); I != BB->end(); ++I) {
          for (unsigned i = 0, e = I->getNumOperands(); i != e; ++i)
            if (I->getOperand(i).isFI()) {
              unsigned Opcode = I->getOpcode();
              const TargetInstrDesc &Desc = TII.get(Opcode);
              unsigned AddrMode = (Desc.TSFlags & ARMII::AddrModeMask);
              if (AddrMode == ARMII::AddrMode3) {
                Limit = (1 << 8) - 1;
                goto DoneEstimating;
              } else if (AddrMode == ARMII::AddrMode5) {
                unsigned ThisLimit = ((1 << 8) - 1) * 4;
                if (ThisLimit < Limit)
                  Limit = ThisLimit;
              }
            }
        }
    DoneEstimating:
      if (Size >= Limit) {
        // If any non-reserved CS register isn't spilled, just spill one or two
        // extra. That should take care of it!
        unsigned NumExtras = TargetAlign / 4;
        SmallVector<unsigned, 2> Extras;
        while (NumExtras && !UnspilledCS1GPRs.empty()) {
          unsigned Reg = UnspilledCS1GPRs.back();
          UnspilledCS1GPRs.pop_back();
          if (!isReservedReg(MF, Reg)) {
            Extras.push_back(Reg);
            NumExtras--;
          }
        }
        while (NumExtras && !UnspilledCS2GPRs.empty()) {
          unsigned Reg = UnspilledCS2GPRs.back();
          UnspilledCS2GPRs.pop_back();
          if (!isReservedReg(MF, Reg)) {
            Extras.push_back(Reg);
            NumExtras--;
          }
        }
        if (Extras.size() && NumExtras == 0) {
          for (unsigned i = 0, e = Extras.size(); i != e; ++i) {
            MF.getRegInfo().setPhysRegUsed(Extras[i]);
            AFI->setCSRegisterIsSpilled(Extras[i]);
          }
        } else {
          // Reserve a slot closest to SP or frame pointer.
          const TargetRegisterClass *RC = &ARM::GPRRegClass;
          RS->setScavengingFrameIndex(MFI->CreateStackObject(RC->getSize(),
                                                           RC->getAlignment()));
        }
      }
    }
  }

  if (ForceLRSpill) {
    MF.getRegInfo().setPhysRegUsed(ARM::LR);
    AFI->setCSRegisterIsSpilled(ARM::LR);
    AFI->setLRIsSpilledForFarJump(true);
  }
}

unsigned ARMBaseRegisterInfo::getRARegister() const {
  return ARM::LR;
}

unsigned ARMBaseRegisterInfo::getFrameRegister(MachineFunction &MF) const {
  if (STI.isTargetDarwin() || hasFP(MF))
    return FramePtr;
  return ARM::SP;
}

unsigned ARMBaseRegisterInfo::getEHExceptionRegister() const {
  LLVM_UNREACHABLE("What is the exception register");
  return 0;
}

unsigned ARMBaseRegisterInfo::getEHHandlerRegister() const {
  LLVM_UNREACHABLE("What is the exception handler register");
  return 0;
}

int ARMBaseRegisterInfo::getDwarfRegNum(unsigned RegNum, bool isEH) const {
  return ARMGenRegisterInfo::getDwarfRegNumFull(RegNum, 0);
}

unsigned ARMBaseRegisterInfo::getRegisterPairEven(unsigned Reg,
                                               const MachineFunction &MF) const {
  switch (Reg) {
  default: break;
  // Return 0 if either register of the pair is a special register.
  // So no R12, etc.
  case ARM::R1:
    return ARM::R0;
  case ARM::R3:
    // FIXME!
    return STI.isThumb1Only() ? 0 : ARM::R2;
  case ARM::R5:
    return ARM::R4;
  case ARM::R7:
    return isReservedReg(MF, ARM::R7)  ? 0 : ARM::R6;
  case ARM::R9:
    return isReservedReg(MF, ARM::R9)  ? 0 :ARM::R8;
  case ARM::R11:
    return isReservedReg(MF, ARM::R11) ? 0 : ARM::R10;

  case ARM::S1:
    return ARM::S0;
  case ARM::S3:
    return ARM::S2;
  case ARM::S5:
    return ARM::S4;
  case ARM::S7:
    return ARM::S6;
  case ARM::S9:
    return ARM::S8;
  case ARM::S11:
    return ARM::S10;
  case ARM::S13:
    return ARM::S12;
  case ARM::S15:
    return ARM::S14;
  case ARM::S17:
    return ARM::S16;
  case ARM::S19:
    return ARM::S18;
  case ARM::S21:
    return ARM::S20;
  case ARM::S23:
    return ARM::S22;
  case ARM::S25:
    return ARM::S24;
  case ARM::S27:
    return ARM::S26;
  case ARM::S29:
    return ARM::S28;
  case ARM::S31:
    return ARM::S30;

  case ARM::D1:
    return ARM::D0;
  case ARM::D3:
    return ARM::D2;
  case ARM::D5:
    return ARM::D4;
  case ARM::D7:
    return ARM::D6;
  case ARM::D9:
    return ARM::D8;
  case ARM::D11:
    return ARM::D10;
  case ARM::D13:
    return ARM::D12;
  case ARM::D15:
    return ARM::D14;
  }

  return 0;
}

unsigned ARMBaseRegisterInfo::getRegisterPairOdd(unsigned Reg,
                                             const MachineFunction &MF) const {
  switch (Reg) {
  default: break;
  // Return 0 if either register of the pair is a special register.
  // So no R12, etc.
  case ARM::R0:
    return ARM::R1;
  case ARM::R2:
    // FIXME!
    return STI.isThumb1Only() ? 0 : ARM::R3;
  case ARM::R4:
    return ARM::R5;
  case ARM::R6:
    return isReservedReg(MF, ARM::R7)  ? 0 : ARM::R7;
  case ARM::R8:
    return isReservedReg(MF, ARM::R9)  ? 0 :ARM::R9;
  case ARM::R10:
    return isReservedReg(MF, ARM::R11) ? 0 : ARM::R11;

  case ARM::S0:
    return ARM::S1;
  case ARM::S2:
    return ARM::S3;
  case ARM::S4:
    return ARM::S5;
  case ARM::S6:
    return ARM::S7;
  case ARM::S8:
    return ARM::S9;
  case ARM::S10:
    return ARM::S11;
  case ARM::S12:
    return ARM::S13;
  case ARM::S14:
    return ARM::S15;
  case ARM::S16:
    return ARM::S17;
  case ARM::S18:
    return ARM::S19;
  case ARM::S20:
    return ARM::S21;
  case ARM::S22:
    return ARM::S23;
  case ARM::S24:
    return ARM::S25;
  case ARM::S26:
    return ARM::S27;
  case ARM::S28:
    return ARM::S29;
  case ARM::S30:
    return ARM::S31;

  case ARM::D0:
    return ARM::D1;
  case ARM::D2:
    return ARM::D3;
  case ARM::D4:
    return ARM::D5;
  case ARM::D6:
    return ARM::D7;
  case ARM::D8:
    return ARM::D9;
  case ARM::D10:
    return ARM::D11;
  case ARM::D12:
    return ARM::D13;
  case ARM::D14:
    return ARM::D15;
  }

  return 0;
}

// FIXME: Dup in ARMBaseInstrInfo.cpp
static inline
const MachineInstrBuilder &AddDefaultPred(const MachineInstrBuilder &MIB) {
  return MIB.addImm((int64_t)ARMCC::AL).addReg(0);
}

static inline
const MachineInstrBuilder &AddDefaultCC(const MachineInstrBuilder &MIB) {
  return MIB.addReg(0);
}

/// emitLoadConstPool - Emits a load from constpool to materialize the
/// specified immediate.
void ARMBaseRegisterInfo::
emitLoadConstPool(MachineBasicBlock &MBB,
                  MachineBasicBlock::iterator &MBBI,
                  DebugLoc dl,
                  unsigned DestReg, int Val,
                  ARMCC::CondCodes Pred,
                  unsigned PredReg) const {
  MachineFunction &MF = *MBB.getParent();
  MachineConstantPool *ConstantPool = MF.getConstantPool();
  Constant *C = ConstantInt::get(Type::Int32Ty, Val);
  unsigned Idx = ConstantPool->getConstantPoolIndex(C, 4);

  BuildMI(MBB, MBBI, dl, TII.get(ARM::LDRcp), DestReg)
    .addConstantPoolIndex(Idx)
    .addReg(0).addImm(0).addImm(Pred).addReg(PredReg);
}

bool ARMBaseRegisterInfo::
requiresRegisterScavenging(const MachineFunction &MF) const {
  return true;
}

// hasReservedCallFrame - Under normal circumstances, when a frame pointer is
// not required, we reserve argument space for call sites in the function
// immediately on entry to the current function. This eliminates the need for
// add/sub sp brackets around call sites. Returns true if the call frame is
// included as part of the stack frame.
bool ARMBaseRegisterInfo::
hasReservedCallFrame(MachineFunction &MF) const {
  const MachineFrameInfo *FFI = MF.getFrameInfo();
  unsigned CFSize = FFI->getMaxCallFrameSize();
  // It's not always a good idea to include the call frame as part of the
  // stack frame. ARM (especially Thumb) has small immediate offset to
  // address the stack frame. So a large call frame can cause poor codegen
  // and may even makes it impossible to scavenge a register.
  if (CFSize >= ((1 << 12) - 1) / 2)  // Half of imm12
    return false;

  return !MF.getFrameInfo()->hasVarSizedObjects();
}

/// emitARMRegPlusImmediate - Emits a series of instructions to materialize
/// a destreg = basereg + immediate in ARM code.
static
void emitARMRegPlusImmediate(MachineBasicBlock &MBB,
                             MachineBasicBlock::iterator &MBBI,
                             unsigned DestReg, unsigned BaseReg, int NumBytes,
                             ARMCC::CondCodes Pred, unsigned PredReg,
                             const ARMBaseInstrInfo &TII,
                             DebugLoc dl) {
  bool isSub = NumBytes < 0;
  if (isSub) NumBytes = -NumBytes;

  while (NumBytes) {
    unsigned RotAmt = ARM_AM::getSOImmValRotate(NumBytes);
    unsigned ThisVal = NumBytes & ARM_AM::rotr32(0xFF, RotAmt);
    assert(ThisVal && "Didn't extract field correctly");

    // We will handle these bits from offset, clear them.
    NumBytes &= ~ThisVal;

    assert(ARM_AM::getSOImmVal(ThisVal) != -1 && "Bit extraction didn't work?");

    // Build the new ADD / SUB.
    BuildMI(MBB, MBBI, dl, TII.get(TII.getOpcode(isSub ? ARMII::SUBri : ARMII::ADDri)), DestReg)
      .addReg(BaseReg, RegState::Kill).addImm(ThisVal)
      .addImm((unsigned)Pred).addReg(PredReg).addReg(0);
    BaseReg = DestReg;
  }
}

static void
emitSPUpdate(MachineBasicBlock &MBB, MachineBasicBlock::iterator &MBBI,
             const ARMBaseInstrInfo &TII, DebugLoc dl,
             int NumBytes,
             ARMCC::CondCodes Pred = ARMCC::AL, unsigned PredReg = 0) {
  emitARMRegPlusImmediate(MBB, MBBI, ARM::SP, ARM::SP, NumBytes,
                          Pred, PredReg, TII, dl);
}

void ARMBaseRegisterInfo::
eliminateCallFramePseudoInstr(MachineFunction &MF, MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator I) const {
  if (!hasReservedCallFrame(MF)) {
    // If we have alloca, convert as follows:
    // ADJCALLSTACKDOWN -> sub, sp, sp, amount
    // ADJCALLSTACKUP   -> add, sp, sp, amount
    MachineInstr *Old = I;
    DebugLoc dl = Old->getDebugLoc();
    unsigned Amount = Old->getOperand(0).getImm();
    if (Amount != 0) {
      // We need to keep the stack aligned properly.  To do this, we round the
      // amount of space needed for the outgoing arguments up to the next
      // alignment boundary.
      unsigned Align = MF.getTarget().getFrameInfo()->getStackAlignment();
      Amount = (Amount+Align-1)/Align*Align;

      // Replace the pseudo instruction with a new instruction...
      unsigned Opc = Old->getOpcode();
      ARMCC::CondCodes Pred = (ARMCC::CondCodes)Old->getOperand(1).getImm();
      if (Opc == ARM::ADJCALLSTACKDOWN || Opc == ARM::tADJCALLSTACKDOWN) {
        // Note: PredReg is operand 2 for ADJCALLSTACKDOWN.
        unsigned PredReg = Old->getOperand(2).getReg();
        emitSPUpdate(MBB, I, TII, dl, -Amount, Pred, PredReg);
      } else {
        // Note: PredReg is operand 3 for ADJCALLSTACKUP.
        unsigned PredReg = Old->getOperand(3).getReg();
        assert(Opc == ARM::ADJCALLSTACKUP || Opc == ARM::tADJCALLSTACKUP);
        emitSPUpdate(MBB, I, TII, dl, Amount, Pred, PredReg);
      }
    }
  }
  MBB.erase(I);
}

/// findScratchRegister - Find a 'free' ARM register. If register scavenger
/// is not being used, R12 is available. Otherwise, try for a call-clobbered
/// register first and then a spilled callee-saved register if that fails.
static
unsigned findScratchRegister(RegScavenger *RS, const TargetRegisterClass *RC,
                             ARMFunctionInfo *AFI) {
  unsigned Reg = RS ? RS->FindUnusedReg(RC, true) : (unsigned) ARM::R12;
  assert (!AFI->isThumb1OnlyFunction());
  if (Reg == 0)
    // Try a already spilled CS register.
    Reg = RS->FindUnusedReg(RC, AFI->getSpilledCSRegisters());

  return Reg;
}

void ARMBaseRegisterInfo::
eliminateFrameIndex(MachineBasicBlock::iterator II,
                    int SPAdj, RegScavenger *RS) const{
  unsigned i = 0;
  MachineInstr &MI = *II;
  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  ARMFunctionInfo *AFI = MF.getInfo<ARMFunctionInfo>();
  DebugLoc dl = MI.getDebugLoc();

  while (!MI.getOperand(i).isFI()) {
    ++i;
    assert(i < MI.getNumOperands() && "Instr doesn't have FrameIndex operand!");
  }

  unsigned FrameReg = ARM::SP;
  int FrameIndex = MI.getOperand(i).getIndex();
  int Offset = MF.getFrameInfo()->getObjectOffset(FrameIndex) +
               MF.getFrameInfo()->getStackSize() + SPAdj;

  if (AFI->isGPRCalleeSavedArea1Frame(FrameIndex))
    Offset -= AFI->getGPRCalleeSavedArea1Offset();
  else if (AFI->isGPRCalleeSavedArea2Frame(FrameIndex))
    Offset -= AFI->getGPRCalleeSavedArea2Offset();
  else if (AFI->isDPRCalleeSavedAreaFrame(FrameIndex))
    Offset -= AFI->getDPRCalleeSavedAreaOffset();
  else if (hasFP(MF)) {
    assert(SPAdj == 0 && "Unexpected");
    // There is alloca()'s in this function, must reference off the frame
    // pointer instead.
    FrameReg = getFrameRegister(MF);
    Offset -= AFI->getFramePtrSpillOffset();
  }

  unsigned Opcode = MI.getOpcode();
  const TargetInstrDesc &Desc = MI.getDesc();
  unsigned AddrMode = (Desc.TSFlags & ARMII::AddrModeMask);
  bool isSub = false;

  // Memory operands in inline assembly always use AddrMode2.
  if (Opcode == ARM::INLINEASM)
    AddrMode = ARMII::AddrMode2;

  if (Opcode == getOpcode(ARMII::ADDri)) {
    Offset += MI.getOperand(i+1).getImm();
    if (Offset == 0) {
      // Turn it into a move.
      MI.setDesc(TII.get(getOpcode(ARMII::MOVr)));
      MI.getOperand(i).ChangeToRegister(FrameReg, false);
      MI.RemoveOperand(i+1);
      return;
    } else if (Offset < 0) {
      Offset = -Offset;
      isSub = true;
      MI.setDesc(TII.get(getOpcode(ARMII::SUBri)));
    }

    // Common case: small offset, fits into instruction.
    if (ARM_AM::getSOImmVal(Offset) != -1) {
      // Replace the FrameIndex with sp / fp
      MI.getOperand(i).ChangeToRegister(FrameReg, false);
      MI.getOperand(i+1).ChangeToImmediate(Offset);
      return;
    }

    // Otherwise, we fallback to common code below to form the imm offset with
    // a sequence of ADDri instructions.  First though, pull as much of the imm
    // into this ADDri as possible.
    unsigned RotAmt = ARM_AM::getSOImmValRotate(Offset);
    unsigned ThisImmVal = Offset & ARM_AM::rotr32(0xFF, RotAmt);

    // We will handle these bits from offset, clear them.
    Offset &= ~ThisImmVal;

    // Get the properly encoded SOImmVal field.
    assert(ARM_AM::getSOImmVal(ThisImmVal) != -1 &&
           "Bit extraction didn't work?");
    MI.getOperand(i+1).ChangeToImmediate(ThisImmVal);
  } else {
    unsigned ImmIdx = 0;
    int InstrOffs = 0;
    unsigned NumBits = 0;
    unsigned Scale = 1;
    switch (AddrMode) {
    case ARMII::AddrMode2: {
      ImmIdx = i+2;
      InstrOffs = ARM_AM::getAM2Offset(MI.getOperand(ImmIdx).getImm());
      if (ARM_AM::getAM2Op(MI.getOperand(ImmIdx).getImm()) == ARM_AM::sub)
        InstrOffs *= -1;
      NumBits = 12;
      break;
    }
    case ARMII::AddrMode3: {
      ImmIdx = i+2;
      InstrOffs = ARM_AM::getAM3Offset(MI.getOperand(ImmIdx).getImm());
      if (ARM_AM::getAM3Op(MI.getOperand(ImmIdx).getImm()) == ARM_AM::sub)
        InstrOffs *= -1;
      NumBits = 8;
      break;
    }
    case ARMII::AddrMode5: {
      ImmIdx = i+1;
      InstrOffs = ARM_AM::getAM5Offset(MI.getOperand(ImmIdx).getImm());
      if (ARM_AM::getAM5Op(MI.getOperand(ImmIdx).getImm()) == ARM_AM::sub)
        InstrOffs *= -1;
      NumBits = 8;
      Scale = 4;
      break;
    }
    case ARMII::AddrModeT2_i12: {
      ImmIdx = i+1;
      InstrOffs = MI.getOperand(ImmIdx).getImm();
      NumBits = 12;
      break;
    }
    case ARMII::AddrModeT2_i8: {
      ImmIdx = i+1;
      InstrOffs = MI.getOperand(ImmIdx).getImm();
      NumBits = 8;
      break;
    }
    case ARMII::AddrModeT2_so: {
      ImmIdx = i+2;
      InstrOffs = MI.getOperand(ImmIdx).getImm();
      break;
    }
    default:
      LLVM_UNREACHABLE("Unsupported addressing mode!");
      break;
    }

    Offset += InstrOffs * Scale;
    assert((Offset & (Scale-1)) == 0 && "Can't encode this offset!");
    if (Offset < 0) {
      Offset = -Offset;
      isSub = true;
    }

    // Common case: small offset, fits into instruction.
    MachineOperand &ImmOp = MI.getOperand(ImmIdx);
    int ImmedOffset = Offset / Scale;
    unsigned Mask = (1 << NumBits) - 1;
    if ((unsigned)Offset <= Mask * Scale) {
      // Replace the FrameIndex with sp
      MI.getOperand(i).ChangeToRegister(FrameReg, false);
      if (isSub)
        ImmedOffset |= 1 << NumBits;
      ImmOp.ChangeToImmediate(ImmedOffset);
      return;
    }

    // Otherwise, it didn't fit. Pull in what we can to simplify the immed.
    ImmedOffset = ImmedOffset & Mask;
    if (isSub)
      ImmedOffset |= 1 << NumBits;
    ImmOp.ChangeToImmediate(ImmedOffset);
    Offset &= ~(Mask*Scale);
  }

  // If we get here, the immediate doesn't fit into the instruction.  We folded
  // as much as possible above, handle the rest, providing a register that is
  // SP+LargeImm.
  assert(Offset && "This code isn't needed if offset already handled!");

  // Insert a set of r12 with the full address: r12 = sp + offset
  // If the offset we have is too large to fit into the instruction, we need
  // to form it with a series of ADDri's.  Do this by taking 8-bit chunks
  // out of 'Offset'.
  unsigned ScratchReg = findScratchRegister(RS, &ARM::GPRRegClass, AFI);
  if (ScratchReg == 0)
    // No register is "free". Scavenge a register.
    ScratchReg = RS->scavengeRegister(&ARM::GPRRegClass, II, SPAdj);
  int PIdx = MI.findFirstPredOperandIdx();
  ARMCC::CondCodes Pred = (PIdx == -1)
    ? ARMCC::AL : (ARMCC::CondCodes)MI.getOperand(PIdx).getImm();
  unsigned PredReg = (PIdx == -1) ? 0 : MI.getOperand(PIdx+1).getReg();
  emitARMRegPlusImmediate(MBB, II, ScratchReg, FrameReg,
                          isSub ? -Offset : Offset, Pred, PredReg, TII, dl);
  MI.getOperand(i).ChangeToRegister(ScratchReg, false, false, true);
}

/// Move iterator pass the next bunch of callee save load / store ops for
/// the particular spill area (1: integer area 1, 2: integer area 2,
/// 3: fp area, 0: don't care).
static void movePastCSLoadStoreOps(MachineBasicBlock &MBB,
                                   MachineBasicBlock::iterator &MBBI,
                                   int Opc, unsigned Area,
                                   const ARMSubtarget &STI) {
  while (MBBI != MBB.end() &&
         MBBI->getOpcode() == Opc && MBBI->getOperand(1).isFI()) {
    if (Area != 0) {
      bool Done = false;
      unsigned Category = 0;
      switch (MBBI->getOperand(0).getReg()) {
      case ARM::R4:  case ARM::R5:  case ARM::R6: case ARM::R7:
      case ARM::LR:
        Category = 1;
        break;
      case ARM::R8:  case ARM::R9:  case ARM::R10: case ARM::R11:
        Category = STI.isTargetDarwin() ? 2 : 1;
        break;
      case ARM::D8:  case ARM::D9:  case ARM::D10: case ARM::D11:
      case ARM::D12: case ARM::D13: case ARM::D14: case ARM::D15:
        Category = 3;
        break;
      default:
        Done = true;
        break;
      }
      if (Done || Category != Area)
        break;
    }

    ++MBBI;
  }
}

void ARMBaseRegisterInfo::
emitPrologue(MachineFunction &MF) const {
  MachineBasicBlock &MBB = MF.front();
  MachineBasicBlock::iterator MBBI = MBB.begin();
  MachineFrameInfo  *MFI = MF.getFrameInfo();
  ARMFunctionInfo *AFI = MF.getInfo<ARMFunctionInfo>();
  unsigned VARegSaveSize = AFI->getVarArgsRegSaveSize();
  unsigned NumBytes = MFI->getStackSize();
  const std::vector<CalleeSavedInfo> &CSI = MFI->getCalleeSavedInfo();
  DebugLoc dl = (MBBI != MBB.end() ?
                 MBBI->getDebugLoc() : DebugLoc::getUnknownLoc());

  // Determine the sizes of each callee-save spill areas and record which frame
  // belongs to which callee-save spill areas.
  unsigned GPRCS1Size = 0, GPRCS2Size = 0, DPRCSSize = 0;
  int FramePtrSpillFI = 0;

  if (VARegSaveSize)
    emitSPUpdate(MBB, MBBI, TII, dl, -VARegSaveSize);

  if (!AFI->hasStackFrame()) {
    if (NumBytes != 0)
      emitSPUpdate(MBB, MBBI, TII, dl, -NumBytes);
    return;
  }

  for (unsigned i = 0, e = CSI.size(); i != e; ++i) {
    unsigned Reg = CSI[i].getReg();
    int FI = CSI[i].getFrameIdx();
    switch (Reg) {
    case ARM::R4:
    case ARM::R5:
    case ARM::R6:
    case ARM::R7:
    case ARM::LR:
      if (Reg == FramePtr)
        FramePtrSpillFI = FI;
      AFI->addGPRCalleeSavedArea1Frame(FI);
      GPRCS1Size += 4;
      break;
    case ARM::R8:
    case ARM::R9:
    case ARM::R10:
    case ARM::R11:
      if (Reg == FramePtr)
        FramePtrSpillFI = FI;
      if (STI.isTargetDarwin()) {
        AFI->addGPRCalleeSavedArea2Frame(FI);
        GPRCS2Size += 4;
      } else {
        AFI->addGPRCalleeSavedArea1Frame(FI);
        GPRCS1Size += 4;
      }
      break;
    default:
      AFI->addDPRCalleeSavedAreaFrame(FI);
      DPRCSSize += 8;
    }
  }

  // Build the new SUBri to adjust SP for integer callee-save spill area 1.
  emitSPUpdate(MBB, MBBI, TII, dl, -GPRCS1Size);
  movePastCSLoadStoreOps(MBB, MBBI, getOpcode(ARMII::STR), 1, STI);

  // Darwin ABI requires FP to point to the stack slot that contains the
  // previous FP.
  if (STI.isTargetDarwin() || hasFP(MF)) {
    MachineInstrBuilder MIB =
      BuildMI(MBB, MBBI, dl, TII.get(getOpcode(ARMII::ADDri)), FramePtr)
      .addFrameIndex(FramePtrSpillFI).addImm(0);
    AddDefaultCC(AddDefaultPred(MIB));
  }

  // Build the new SUBri to adjust SP for integer callee-save spill area 2.
  emitSPUpdate(MBB, MBBI, TII, dl, -GPRCS2Size);

  // Build the new SUBri to adjust SP for FP callee-save spill area.
  movePastCSLoadStoreOps(MBB, MBBI, getOpcode(ARMII::STR), 2, STI);
  emitSPUpdate(MBB, MBBI, TII, dl, -DPRCSSize);

  // Determine starting offsets of spill areas.
  unsigned DPRCSOffset  = NumBytes - (GPRCS1Size + GPRCS2Size + DPRCSSize);
  unsigned GPRCS2Offset = DPRCSOffset + DPRCSSize;
  unsigned GPRCS1Offset = GPRCS2Offset + GPRCS2Size;
  AFI->setFramePtrSpillOffset(MFI->getObjectOffset(FramePtrSpillFI) + NumBytes);
  AFI->setGPRCalleeSavedArea1Offset(GPRCS1Offset);
  AFI->setGPRCalleeSavedArea2Offset(GPRCS2Offset);
  AFI->setDPRCalleeSavedAreaOffset(DPRCSOffset);

  NumBytes = DPRCSOffset;
  if (NumBytes) {
    // Insert it after all the callee-save spills.
    movePastCSLoadStoreOps(MBB, MBBI, getOpcode(ARMII::FSTD), 3, STI);
    emitSPUpdate(MBB, MBBI, TII, dl, -NumBytes);
  }

  if (STI.isTargetELF() && hasFP(MF)) {
    MFI->setOffsetAdjustment(MFI->getOffsetAdjustment() -
                             AFI->getFramePtrSpillOffset());
  }

  AFI->setGPRCalleeSavedArea1Size(GPRCS1Size);
  AFI->setGPRCalleeSavedArea2Size(GPRCS2Size);
  AFI->setDPRCalleeSavedAreaSize(DPRCSSize);
}

static bool isCalleeSavedRegister(unsigned Reg, const unsigned *CSRegs) {
  for (unsigned i = 0; CSRegs[i]; ++i)
    if (Reg == CSRegs[i])
      return true;
  return false;
}

static bool isCSRestore(MachineInstr *MI,
                        const ARMBaseInstrInfo &TII, 
                        const unsigned *CSRegs) {
  return ((MI->getOpcode() == (int)TII.getOpcode(ARMII::FLDD) ||
           MI->getOpcode() == (int)TII.getOpcode(ARMII::LDR)) &&
          MI->getOperand(1).isFI() &&
          isCalleeSavedRegister(MI->getOperand(0).getReg(), CSRegs));
}

void ARMBaseRegisterInfo::
emitEpilogue(MachineFunction &MF,
             MachineBasicBlock &MBB) const {
  MachineBasicBlock::iterator MBBI = prior(MBB.end());
  assert(MBBI->getOpcode() == (int)getOpcode(ARMII::BX_RET) &&
         "Can only insert epilog into returning blocks");
  DebugLoc dl = MBBI->getDebugLoc();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  ARMFunctionInfo *AFI = MF.getInfo<ARMFunctionInfo>();
  unsigned VARegSaveSize = AFI->getVarArgsRegSaveSize();
  int NumBytes = (int)MFI->getStackSize();

  if (!AFI->hasStackFrame()) {
    if (NumBytes != 0)
      emitSPUpdate(MBB, MBBI, TII, dl, NumBytes);
  } else {
    // Unwind MBBI to point to first LDR / FLDD.
    const unsigned *CSRegs = getCalleeSavedRegs();
    if (MBBI != MBB.begin()) {
      do
        --MBBI;
      while (MBBI != MBB.begin() && isCSRestore(MBBI, TII, CSRegs));
      if (!isCSRestore(MBBI, TII, CSRegs))
        ++MBBI;
    }

    // Move SP to start of FP callee save spill area.
    NumBytes -= (AFI->getGPRCalleeSavedArea1Size() +
                 AFI->getGPRCalleeSavedArea2Size() +
                 AFI->getDPRCalleeSavedAreaSize());

    // Darwin ABI requires FP to point to the stack slot that contains the
    // previous FP.
    if ((STI.isTargetDarwin() && NumBytes) || hasFP(MF)) {
      NumBytes = AFI->getFramePtrSpillOffset() - NumBytes;
      // Reset SP based on frame pointer only if the stack frame extends beyond
      // frame pointer stack slot or target is ELF and the function has FP.
      if (AFI->getGPRCalleeSavedArea2Size() ||
          AFI->getDPRCalleeSavedAreaSize()  ||
          AFI->getDPRCalleeSavedAreaOffset()||
          hasFP(MF)) {
        if (NumBytes)
          BuildMI(MBB, MBBI, dl, TII.get(getOpcode(ARMII::SUBri)), ARM::SP).addReg(FramePtr)
            .addImm(NumBytes)
            .addImm((unsigned)ARMCC::AL).addReg(0).addReg(0);
        else
          BuildMI(MBB, MBBI, dl, TII.get(getOpcode(ARMII::MOVr)), ARM::SP).addReg(FramePtr)
            .addImm((unsigned)ARMCC::AL).addReg(0).addReg(0);
      }
    } else if (NumBytes) {
      emitSPUpdate(MBB, MBBI, TII, dl, NumBytes);
    }

    // Move SP to start of integer callee save spill area 2.
    movePastCSLoadStoreOps(MBB, MBBI, getOpcode(ARMII::FLDD), 3, STI);
    emitSPUpdate(MBB, MBBI, TII, dl, AFI->getDPRCalleeSavedAreaSize());

    // Move SP to start of integer callee save spill area 1.
    movePastCSLoadStoreOps(MBB, MBBI, getOpcode(ARMII::LDR), 2, STI);
    emitSPUpdate(MBB, MBBI, TII, dl, AFI->getGPRCalleeSavedArea2Size());

    // Move SP to SP upon entry to the function.
    movePastCSLoadStoreOps(MBB, MBBI, getOpcode(ARMII::LDR), 1, STI);
    emitSPUpdate(MBB, MBBI, TII, dl, AFI->getGPRCalleeSavedArea1Size());
  }

  if (VARegSaveSize)
    emitSPUpdate(MBB, MBBI, TII, dl, VARegSaveSize);

}

#include "ARMGenRegisterInfo.inc"
