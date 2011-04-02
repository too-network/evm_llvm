//===-- InterferenceCache.h - Caching per-block interference ---*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// InterferenceCache remembers per-block interference in LiveIntervalUnions.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "regalloc"
#include "InterferenceCache.h"
#include "llvm/Target/TargetRegisterInfo.h"

using namespace llvm;

void InterferenceCache::init(MachineFunction *mf,
                             LiveIntervalUnion *liuarray,
                             SlotIndexes *indexes,
                            const TargetRegisterInfo *tri) {
  MF = mf;
  LIUArray = liuarray;
  TRI = tri;
  PhysRegEntries.assign(TRI->getNumRegs(), 0);
  for (unsigned i = 0; i != CacheEntries; ++i)
    Entries[i].clear(indexes);
}

InterferenceCache::Entry *InterferenceCache::get(unsigned PhysReg) {
  unsigned E = PhysRegEntries[PhysReg];
  if (E < CacheEntries && Entries[E].getPhysReg() == PhysReg) {
    if (!Entries[E].valid(LIUArray, TRI))
      Entries[E].revalidate();
    return &Entries[E];
  }
  // No valid entry exists, pick the next round-robin entry.
  E = RoundRobin;
  if (++RoundRobin == CacheEntries)
    RoundRobin = 0;
  Entries[E].reset(PhysReg, LIUArray, TRI, MF);
  PhysRegEntries[PhysReg] = E;
  return &Entries[E];
}

/// revalidate - LIU contents have changed, update tags.
void InterferenceCache::Entry::revalidate() {
  // Invalidate all block entries.
  ++Tag;
  // Invalidate all iterators.
  PrevPos = SlotIndex();
  for (unsigned i = 0, e = Aliases.size(); i != e; ++i)
    Aliases[i].second = Aliases[i].first->getTag();
}

void InterferenceCache::Entry::reset(unsigned physReg,
                                     LiveIntervalUnion *LIUArray,
                                     const TargetRegisterInfo *TRI,
                                     const MachineFunction *MF) {
  // LIU's changed, invalidate cache.
  ++Tag;
  PhysReg = physReg;
  Blocks.resize(MF->getNumBlockIDs());
  Aliases.clear();
  for (const unsigned *AS = TRI->getOverlaps(PhysReg); *AS; ++AS) {
    LiveIntervalUnion *LIU = LIUArray + *AS;
    Aliases.push_back(std::make_pair(LIU, LIU->getTag()));
  }

  // Reset iterators.
  PrevPos = SlotIndex();
  unsigned e = Aliases.size();
  Iters.resize(e);
  for (unsigned i = 0; i != e; ++i)
    Iters[i].setMap(Aliases[i].first->getMap());
}

bool InterferenceCache::Entry::valid(LiveIntervalUnion *LIUArray,
                                     const TargetRegisterInfo *TRI) {
  unsigned i = 0, e = Aliases.size();
  for (const unsigned *AS = TRI->getOverlaps(PhysReg); *AS; ++AS, ++i) {
    LiveIntervalUnion *LIU = LIUArray + *AS;
    if (i == e ||  Aliases[i].first != LIU)
      return false;
    if (LIU->changedSince(Aliases[i].second))
      return false;
  }
  return i == e;
}

void InterferenceCache::Entry::update(unsigned MBBNum) {
  BlockInterference *BI = &Blocks[MBBNum];
  BI->Tag = Tag;
  BI->First = BI->Last = SlotIndex();

  SlotIndex Start, Stop;
  tie(Start, Stop) = Indexes->getMBBRange(MBBNum);

  // Use advanceTo only when possible.
  if (!PrevPos.isValid() || Start < PrevPos)
    for (unsigned i = 0, e = Iters.size(); i != e; ++i)
      Iters[i].find(Start);
  else
    for (unsigned i = 0, e = Iters.size(); i != e; ++i)
      Iters[i].advanceTo(Start);
  PrevPos = Start;

  // Check for first interference.
  for (unsigned i = 0, e = Iters.size(); i != e; ++i) {
    Iter &I = Iters[i];
    if (!I.valid())
      continue;
    SlotIndex StartI = I.start();
    if (StartI >= Stop)
      continue;
    if (!BI->First.isValid() || StartI < BI->First)
      BI->First = StartI;
  }

  // No interference in block.
  if (!BI->First.isValid())
    return;

  // Check for last interference.
  for (unsigned i = 0, e = Iters.size(); i != e; ++i) {
    Iter &I = Iters[i];
    if (!I.valid() || I.start() >= Stop)
      continue;
    I.advanceTo(Stop);
    if (!I.valid() || I.start() >= Stop)
      --I;
    SlotIndex StopI = I.stop();
    if (!BI->Last.isValid() || StopI > BI->Last)
      BI->Last = StopI;
  }
  PrevPos = Stop;
}
