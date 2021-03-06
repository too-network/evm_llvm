//===-- llvm/MC/MCEVMObjectWriter.h - EVM Object Writer -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_MC_MCEVMOBJECTWRITER_H
#define LLVM_MC_MCEVMOBJECTWRITER_H

#include "llvm/MC/MCObjectWriter.h"
#include <memory>

namespace llvm {

class MCFixup;
class MCValue;
class raw_pwrite_stream;

class MCEVMObjectTargetWriter : public MCObjectTargetWriter {
protected:
  explicit MCEVMObjectTargetWriter();

public:
  virtual ~MCEVMObjectTargetWriter();

  virtual Triple::ObjectFormatType getFormat() const { return Triple::EVMBinary; }
  static bool classof(const MCObjectTargetWriter *W) {
    return W->getFormat() == Triple::EVMBinary;
  }

  virtual unsigned getRelocType(const MCValue &Target,
                                const MCFixup &Fixup) const = 0;
};

/// Construct a new EVM writer instance.
///
/// \param MOTW - The target specific EVM writer subclass.
/// \param OS - The stream to write to.
/// \returns The constructed object writer.
std::unique_ptr<MCObjectWriter>
createEVMObjectWriter(std::unique_ptr<MCEVMObjectTargetWriter> MOTW,
                      raw_pwrite_stream &OS);

} // namespace llvm

#endif
