//===- llvm/unittest/Analysis/LoopPassManagerTest.cpp - LPM tests ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/LoopPassManager.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/AsmParser/Parser.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/SourceMgr.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace {

using testing::DoDefault;
using testing::Return;
using testing::Expectation;
using testing::Invoke;
using testing::InvokeWithoutArgs;
using testing::_;

template <typename DerivedT, typename IRUnitT,
          typename AnalysisManagerT = AnalysisManager<IRUnitT>,
          typename... ExtraArgTs>
class MockAnalysisHandleBase {
public:
  class Analysis : public AnalysisInfoMixin<Analysis> {
    friend AnalysisInfoMixin<Analysis>;
    friend MockAnalysisHandleBase;
    static AnalysisKey Key;

    DerivedT *Handle;

    Analysis(DerivedT &Handle) : Handle(&Handle) {}

  public:
    class Result {
      friend MockAnalysisHandleBase;

      DerivedT *Handle;

      Result(DerivedT &Handle) : Handle(&Handle) {}

    public:
      // Forward invalidation events to the mock handle.
      bool invalidate(IRUnitT &IR, const PreservedAnalyses &PA,
                      typename AnalysisManagerT::Invalidator &Inv) {
        return Handle->invalidate(IR, PA, Inv);
      }
    };

    Result run(IRUnitT &IR, AnalysisManagerT &AM, ExtraArgTs... ExtraArgs) {
      return Handle->run(IR, AM, ExtraArgs...);
    }
  };

  Analysis getAnalysis() { return Analysis(static_cast<DerivedT &>(*this)); }
  typename Analysis::Result getResult() {
    return typename Analysis::Result(static_cast<DerivedT &>(*this));
  }

protected:
  /// Derived classes should call this in their constructor to set up default
  /// mock actions. (We can't do this in our constructor because this has to
  /// run after the DerivedT is constructed.)
  void setDefaults() {
    ON_CALL(static_cast<DerivedT &>(*this),
            run(_, _, testing::Matcher<ExtraArgTs>(_)...))
        .WillByDefault(Return(this->getResult()));
    ON_CALL(static_cast<DerivedT &>(*this), invalidate(_, _, _))
        .WillByDefault(Invoke([](IRUnitT &, const PreservedAnalyses &PA,
                                 typename AnalysisManagerT::Invalidator &Inv) {
          auto PAC = PA.getChecker<Analysis>();
          return !PAC.preserved() &&
                 !PAC.template preservedSet<AllAnalysesOn<IRUnitT>>();
        }));
  }
};

template <typename DerivedT, typename IRUnitT, typename AnalysisManagerT,
          typename... ExtraArgTs>
AnalysisKey MockAnalysisHandleBase<DerivedT, IRUnitT, AnalysisManagerT,
                                   ExtraArgTs...>::Analysis::Key;

/// Mock handle for loop analyses.
///
/// This is provided as a template accepting an (optional) integer. Because
/// analyses are identified and queried by type, this allows constructing
/// multiple handles with distinctly typed nested 'Analysis' types that can be
/// registered and queried. If you want to register multiple loop analysis
/// passes, you'll need to instantiate this type with different values for I.
/// For example:
///
///   MockLoopAnalysisHandleTemplate<0> h0;
///   MockLoopAnalysisHandleTemplate<1> h1;
///   typedef decltype(h0)::Analysis Analysis0;
///   typedef decltype(h1)::Analysis Analysis1;
template <size_t I = static_cast<size_t>(-1)>
struct MockLoopAnalysisHandleTemplate
    : MockAnalysisHandleBase<MockLoopAnalysisHandleTemplate<I>, Loop,
                             LoopAnalysisManager,
                             LoopStandardAnalysisResults &> {
  typedef typename MockLoopAnalysisHandleTemplate::Analysis Analysis;

  MOCK_METHOD3_T(run, typename Analysis::Result(Loop &, LoopAnalysisManager &,
                                                LoopStandardAnalysisResults &));

  MOCK_METHOD3_T(invalidate, bool(Loop &, const PreservedAnalyses &,
                                  LoopAnalysisManager::Invalidator &));

  MockLoopAnalysisHandleTemplate() { this->setDefaults(); }
};

typedef MockLoopAnalysisHandleTemplate<> MockLoopAnalysisHandle;

struct MockFunctionAnalysisHandle
    : MockAnalysisHandleBase<MockFunctionAnalysisHandle, Function> {
  MOCK_METHOD2(run, Analysis::Result(Function &, FunctionAnalysisManager &));

  MOCK_METHOD3(invalidate, bool(Function &, const PreservedAnalyses &,
                                FunctionAnalysisManager::Invalidator &));

  MockFunctionAnalysisHandle() { setDefaults(); }
};

template <typename DerivedT, typename IRUnitT,
          typename AnalysisManagerT = AnalysisManager<IRUnitT>,
          typename... ExtraArgTs>
class MockPassHandleBase {
public:
  class Pass : public PassInfoMixin<Pass> {
    friend MockPassHandleBase;

    DerivedT *Handle;

    Pass(DerivedT &Handle) : Handle(&Handle) {}

  public:
    PreservedAnalyses run(IRUnitT &IR, AnalysisManagerT &AM,
                          ExtraArgTs... ExtraArgs) {
      return Handle->run(IR, AM, ExtraArgs...);
    }
  };

  Pass getPass() { return Pass(static_cast<DerivedT &>(*this)); }

protected:
  /// Derived classes should call this in their constructor to set up default
  /// mock actions. (We can't do this in our constructor because this has to
  /// run after the DerivedT is constructed.)
  void setDefaults() {
    ON_CALL(static_cast<DerivedT &>(*this),
            run(_, _, testing::Matcher<ExtraArgTs>(_)...))
        .WillByDefault(Return(PreservedAnalyses::all()));
  }
};

struct MockLoopPassHandle
    : MockPassHandleBase<MockLoopPassHandle, Loop, LoopAnalysisManager,
                         LoopStandardAnalysisResults &, LPMUpdater &> {
  MOCK_METHOD4(run,
               PreservedAnalyses(Loop &, LoopAnalysisManager &,
                                 LoopStandardAnalysisResults &, LPMUpdater &));
  MockLoopPassHandle() { setDefaults(); }
};

struct MockFunctionPassHandle
    : MockPassHandleBase<MockFunctionPassHandle, Function> {
  MOCK_METHOD2(run, PreservedAnalyses(Function &, FunctionAnalysisManager &));

  MockFunctionPassHandle() { setDefaults(); }
};

struct MockModulePassHandle : MockPassHandleBase<MockModulePassHandle, Module> {
  MOCK_METHOD2(run, PreservedAnalyses(Module &, ModuleAnalysisManager &));

  MockModulePassHandle() { setDefaults(); }
};

/// Define a custom matcher for objects which support a 'getName' method
/// returning a StringRef.
///
/// LLVM often has IR objects or analysis objects which expose a StringRef name
/// and in tests it is convenient to match these by name for readability. This
/// matcher supports any type exposing a getName() method of this form.
///
/// It should be used as:
///
///   HasName("my_function")
///
/// No namespace or other qualification is required.
MATCHER_P(HasName, Name, "") {
  // The matcher's name and argument are printed in the case of failure, but we
  // also want to print out the name of the argument. This uses an implicitly
  // avaiable std::ostream, so we have to construct a std::string.
  *result_listener << "has name '" << arg.getName().str() << "'";
  return Name == arg.getName();
}

std::unique_ptr<Module> parseIR(LLVMContext &C, const char *IR) {
  SMDiagnostic Err;
  return parseAssemblyString(IR, Err, C);
}

class LoopPassManagerTest : public ::testing::Test {
protected:
  LLVMContext Context;
  std::unique_ptr<Module> M;

  LoopAnalysisManager LAM;
  FunctionAnalysisManager FAM;
  ModuleAnalysisManager MAM;

  MockLoopAnalysisHandle MLAHandle;
  MockLoopPassHandle MLPHandle;
  MockFunctionPassHandle MFPHandle;
  MockModulePassHandle MMPHandle;

  static PreservedAnalyses
  getLoopAnalysisResult(Loop &L, LoopAnalysisManager &AM,
                        LoopStandardAnalysisResults &AR, LPMUpdater &) {
    (void)AM.getResult<MockLoopAnalysisHandle::Analysis>(L, AR);
    return PreservedAnalyses::all();
  };

public:
  LoopPassManagerTest()
      : M(parseIR(Context, "define void @f() {\n"
                           "entry:\n"
                           "  br label %loop.0\n"
                           "loop.0:\n"
                           "  br i1 undef, label %loop.0.0, label %end\n"
                           "loop.0.0:\n"
                           "  br i1 undef, label %loop.0.0, label %loop.0.1\n"
                           "loop.0.1:\n"
                           "  br i1 undef, label %loop.0.1, label %loop.0\n"
                           "end:\n"
                           "  ret void\n"
                           "}\n"
                           "\n"
                           "define void @g() {\n"
                           "entry:\n"
                           "  br label %loop.g.0\n"
                           "loop.g.0:\n"
                           "  br i1 undef, label %loop.g.0, label %end\n"
                           "end:\n"
                           "  ret void\n"
                           "}\n")),
        LAM(true), FAM(true), MAM(true) {
    // Register our mock analysis.
    LAM.registerPass([&] { return MLAHandle.getAnalysis(); });

    // We need DominatorTreeAnalysis for LoopAnalysis.
    FAM.registerPass([&] { return DominatorTreeAnalysis(); });
    FAM.registerPass([&] { return LoopAnalysis(); });
    // We also allow loop passes to assume a set of other analyses and so need
    // those.
    FAM.registerPass([&] { return AAManager(); });
    FAM.registerPass([&] { return AssumptionAnalysis(); });
    FAM.registerPass([&] { return ScalarEvolutionAnalysis(); });
    FAM.registerPass([&] { return TargetLibraryAnalysis(); });
    FAM.registerPass([&] { return TargetIRAnalysis(); });

    // Cross-register proxies.
    LAM.registerPass([&] { return FunctionAnalysisManagerLoopProxy(FAM); });
    FAM.registerPass([&] { return LoopAnalysisManagerFunctionProxy(LAM); });
    FAM.registerPass([&] { return ModuleAnalysisManagerFunctionProxy(MAM); });
    MAM.registerPass([&] { return FunctionAnalysisManagerModuleProxy(FAM); });
  }
};

TEST_F(LoopPassManagerTest, Basic) {
  ModulePassManager MPM(true);
  ::testing::InSequence MakeExpectationsSequenced;

  // First we just visit all the loops in all the functions and get their
  // analysis results. This will run the analysis a total of four times,
  // once for each loop.
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.0"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.0"), _, _));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.1"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.1"), _, _));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0"), _, _));
  EXPECT_CALL(MLPHandle, run(HasName("loop.g.0"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.g.0"), _, _));
  // Wire the loop pass through pass managers into the module pipeline.
  {
    LoopPassManager LPM(true);
    LPM.addPass(MLPHandle.getPass());
    FunctionPassManager FPM(true);
    FPM.addPass(createFunctionToLoopPassAdaptor(std::move(LPM)));
    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
  }

  // Next we run two passes over the loops. The first one invalidates the
  // analyses for one loop, the second ones try to get the analysis results.
  // This should force only one analysis to re-run within the loop PM, but will
  // also invalidate everything after the loop pass manager finishes.
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.0"), _, _, _))
      .WillOnce(DoDefault())
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.1"), _, _, _))
      .WillOnce(InvokeWithoutArgs([] { return PreservedAnalyses::none(); }))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.1"), _, _));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0"), _, _, _))
      .WillOnce(DoDefault())
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLPHandle, run(HasName("loop.g.0"), _, _, _))
      .WillOnce(DoDefault())
      .WillOnce(Invoke(getLoopAnalysisResult));
  // Wire two loop pass runs into the module pipeline.
  {
    LoopPassManager LPM(true);
    LPM.addPass(MLPHandle.getPass());
    LPM.addPass(MLPHandle.getPass());
    FunctionPassManager FPM(true);
    FPM.addPass(createFunctionToLoopPassAdaptor(std::move(LPM)));
    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
  }

  // And now run the pipeline across the module.
  MPM.run(*M, MAM);
}

TEST_F(LoopPassManagerTest, FunctionPassInvalidationOfLoopAnalyses) {
  ModulePassManager MPM(true);
  FunctionPassManager FPM(true);
  // We process each function completely in sequence.
  ::testing::Sequence FSequence, GSequence;

  // First, force the analysis result to be computed for each loop.
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.0"), _, _))
      .InSequence(FSequence)
      .WillOnce(DoDefault());
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.1"), _, _))
      .InSequence(FSequence)
      .WillOnce(DoDefault());
  EXPECT_CALL(MLAHandle, run(HasName("loop.0"), _, _))
      .InSequence(FSequence)
      .WillOnce(DoDefault());
  EXPECT_CALL(MLAHandle, run(HasName("loop.g.0"), _, _))
      .InSequence(GSequence)
      .WillOnce(DoDefault());
  FPM.addPass(createFunctionToLoopPassAdaptor(
      RequireAnalysisLoopPass<MockLoopAnalysisHandle::Analysis>()));

  // No need to re-run if we require again from a fresh loop pass manager.
  FPM.addPass(createFunctionToLoopPassAdaptor(
      RequireAnalysisLoopPass<MockLoopAnalysisHandle::Analysis>()));

  // For 'f', preserve most things but not the specific loop analyses.
  EXPECT_CALL(MFPHandle, run(HasName("f"), _))
      .InSequence(FSequence)
      .WillOnce(Return(getLoopPassPreservedAnalyses()));
  EXPECT_CALL(MLAHandle, invalidate(HasName("loop.0.0"), _, _))
      .InSequence(FSequence)
      .WillOnce(DoDefault());
  // On one loop, skip the invalidation (as though we did an internal update).
  EXPECT_CALL(MLAHandle, invalidate(HasName("loop.0.1"), _, _))
      .InSequence(FSequence)
      .WillOnce(Return(false));
  EXPECT_CALL(MLAHandle, invalidate(HasName("loop.0"), _, _))
      .InSequence(FSequence)
      .WillOnce(DoDefault());
  // Now two loops still have to be recomputed.
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.0"), _, _))
      .InSequence(FSequence)
      .WillOnce(DoDefault());
  EXPECT_CALL(MLAHandle, run(HasName("loop.0"), _, _))
      .InSequence(FSequence)
      .WillOnce(DoDefault());
  // Preserve things in the second function to ensure invalidation remains
  // isolated to one function.
  EXPECT_CALL(MFPHandle, run(HasName("g"), _))
      .InSequence(GSequence)
      .WillOnce(DoDefault());
  FPM.addPass(MFPHandle.getPass());
  FPM.addPass(createFunctionToLoopPassAdaptor(
      RequireAnalysisLoopPass<MockLoopAnalysisHandle::Analysis>()));

  EXPECT_CALL(MFPHandle, run(HasName("f"), _))
      .InSequence(FSequence)
      .WillOnce(DoDefault());
  // For 'g', fail to preserve anything, causing the loops themselves to be
  // cleared. We don't get an invalidation event here as the loop is gone, but
  // we should still have to recompute the analysis.
  EXPECT_CALL(MFPHandle, run(HasName("g"), _))
      .InSequence(GSequence)
      .WillOnce(Return(PreservedAnalyses::none()));
  EXPECT_CALL(MLAHandle, run(HasName("loop.g.0"), _, _))
      .InSequence(GSequence)
      .WillOnce(DoDefault());
  FPM.addPass(MFPHandle.getPass());
  FPM.addPass(createFunctionToLoopPassAdaptor(
      RequireAnalysisLoopPass<MockLoopAnalysisHandle::Analysis>()));

  MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));

  // Verify with a separate function pass run that we didn't mess up 'f's
  // cache. No analysis runs should be necessary here.
  MPM.addPass(createModuleToFunctionPassAdaptor(createFunctionToLoopPassAdaptor(
      RequireAnalysisLoopPass<MockLoopAnalysisHandle::Analysis>())));

  MPM.run(*M, MAM);
}

TEST_F(LoopPassManagerTest, ModulePassInvalidationOfLoopAnalyses) {
  ModulePassManager MPM(true);
  ::testing::InSequence MakeExpectationsSequenced;

  // First, force the analysis result to be computed for each loop.
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.0"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.1"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.g.0"), _, _));
  MPM.addPass(createModuleToFunctionPassAdaptor(createFunctionToLoopPassAdaptor(
      RequireAnalysisLoopPass<MockLoopAnalysisHandle::Analysis>())));

  // Walking all the way out and all the way back in doesn't re-run the
  // analysis.
  MPM.addPass(createModuleToFunctionPassAdaptor(createFunctionToLoopPassAdaptor(
      RequireAnalysisLoopPass<MockLoopAnalysisHandle::Analysis>())));

  // But a module pass that doesn't preserve the actual mock loop analysis
  // invalidates all the way down and forces recomputing.
  EXPECT_CALL(MMPHandle, run(_, _)).WillOnce(InvokeWithoutArgs([] {
    auto PA = getLoopPassPreservedAnalyses();
    PA.preserve<FunctionAnalysisManagerModuleProxy>();
    return PA;
  }));
  // All the loop analyses from both functions get invalidated before we
  // recompute anything.
  EXPECT_CALL(MLAHandle, invalidate(HasName("loop.0.0"), _, _));
  // On one loop, again skip the invalidation (as though we did an internal
  // update).
  EXPECT_CALL(MLAHandle, invalidate(HasName("loop.0.1"), _, _))
      .WillOnce(Return(false));
  EXPECT_CALL(MLAHandle, invalidate(HasName("loop.0"), _, _));
  EXPECT_CALL(MLAHandle, invalidate(HasName("loop.g.0"), _, _));
  // Now all but one of the loops gets re-analyzed.
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.0"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.g.0"), _, _));
  MPM.addPass(MMPHandle.getPass());
  MPM.addPass(createModuleToFunctionPassAdaptor(createFunctionToLoopPassAdaptor(
      RequireAnalysisLoopPass<MockLoopAnalysisHandle::Analysis>())));

  // Verify that the cached values persist.
  MPM.addPass(createModuleToFunctionPassAdaptor(createFunctionToLoopPassAdaptor(
      RequireAnalysisLoopPass<MockLoopAnalysisHandle::Analysis>())));

  // Now we fail to preserve the loop analysis and observe that the loop
  // analyses are cleared (so no invalidation event) as the loops themselves
  // are no longer valid.
  EXPECT_CALL(MMPHandle, run(_, _)).WillOnce(InvokeWithoutArgs([] {
    auto PA = PreservedAnalyses::none();
    PA.preserve<FunctionAnalysisManagerModuleProxy>();
    return PA;
  }));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.0"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.1"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.g.0"), _, _));
  MPM.addPass(MMPHandle.getPass());
  MPM.addPass(createModuleToFunctionPassAdaptor(createFunctionToLoopPassAdaptor(
      RequireAnalysisLoopPass<MockLoopAnalysisHandle::Analysis>())));

  // Verify that the cached values persist.
  MPM.addPass(createModuleToFunctionPassAdaptor(createFunctionToLoopPassAdaptor(
      RequireAnalysisLoopPass<MockLoopAnalysisHandle::Analysis>())));

  // Next, check that even if we preserve everything within the function itelf,
  // if the function's module pass proxy isn't preserved and the potential set
  // of functions changes, the clear reaches the loop analyses as well. This
  // will again trigger re-runs but not invalidation events.
  EXPECT_CALL(MMPHandle, run(_, _)).WillOnce(InvokeWithoutArgs([] {
    auto PA = PreservedAnalyses::none();
    PA.preserveSet<AllAnalysesOn<Function>>();
    PA.preserveSet<AllAnalysesOn<Loop>>();
    return PA;
  }));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.0"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.1"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.g.0"), _, _));
  MPM.addPass(MMPHandle.getPass());
  MPM.addPass(createModuleToFunctionPassAdaptor(createFunctionToLoopPassAdaptor(
      RequireAnalysisLoopPass<MockLoopAnalysisHandle::Analysis>())));

  MPM.run(*M, MAM);
}

// Test that if any of the bundled analyses provided in the LPM's signature
// become invalid, the analysis proxy itself becomes invalid and we clear all
// loop analysis results.
TEST_F(LoopPassManagerTest, InvalidationOfBundledAnalyses) {
  ModulePassManager MPM(true);
  FunctionPassManager FPM(true);
  ::testing::InSequence MakeExpectationsSequenced;

  // First, force the analysis result to be computed for each loop.
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.0"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.1"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0"), _, _));
  FPM.addPass(createFunctionToLoopPassAdaptor(
      RequireAnalysisLoopPass<MockLoopAnalysisHandle::Analysis>()));

  // No need to re-run if we require again from a fresh loop pass manager.
  FPM.addPass(createFunctionToLoopPassAdaptor(
      RequireAnalysisLoopPass<MockLoopAnalysisHandle::Analysis>()));

  // Preserving everything but the loop analyses themselves results in
  // invalidation and running.
  EXPECT_CALL(MFPHandle, run(HasName("f"), _))
      .WillOnce(Return(getLoopPassPreservedAnalyses()));
  EXPECT_CALL(MLAHandle, invalidate(_, _, _)).Times(3);
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.0"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.1"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0"), _, _));
  FPM.addPass(MFPHandle.getPass());
  FPM.addPass(createFunctionToLoopPassAdaptor(
      RequireAnalysisLoopPass<MockLoopAnalysisHandle::Analysis>()));

  // The rest don't invalidate analyses, they only trigger re-runs because we
  // clear the cache completely.
  EXPECT_CALL(MFPHandle, run(HasName("f"), _)).WillOnce(InvokeWithoutArgs([] {
    auto PA = PreservedAnalyses::none();
    // Not preserving `AAManager`.
    PA.preserve<AssumptionAnalysis>();
    PA.preserve<DominatorTreeAnalysis>();
    PA.preserve<LoopAnalysis>();
    PA.preserve<LoopAnalysisManagerFunctionProxy>();
    PA.preserve<ScalarEvolutionAnalysis>();
    return PA;
  }));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.0"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.1"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0"), _, _));
  FPM.addPass(MFPHandle.getPass());
  FPM.addPass(createFunctionToLoopPassAdaptor(
      RequireAnalysisLoopPass<MockLoopAnalysisHandle::Analysis>()));

  EXPECT_CALL(MFPHandle, run(HasName("f"), _)).WillOnce(InvokeWithoutArgs([] {
    auto PA = PreservedAnalyses::none();
    PA.preserve<AAManager>();
    // Not preserving `AssumptionAnalysis`.
    PA.preserve<DominatorTreeAnalysis>();
    PA.preserve<LoopAnalysis>();
    PA.preserve<LoopAnalysisManagerFunctionProxy>();
    PA.preserve<ScalarEvolutionAnalysis>();
    return PA;
  }));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.0"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.1"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0"), _, _));
  FPM.addPass(MFPHandle.getPass());
  FPM.addPass(createFunctionToLoopPassAdaptor(
      RequireAnalysisLoopPass<MockLoopAnalysisHandle::Analysis>()));

  EXPECT_CALL(MFPHandle, run(HasName("f"), _)).WillOnce(InvokeWithoutArgs([] {
    auto PA = PreservedAnalyses::none();
    PA.preserve<AAManager>();
    PA.preserve<AssumptionAnalysis>();
    // Not preserving `DominatorTreeAnalysis`.
    PA.preserve<LoopAnalysis>();
    PA.preserve<LoopAnalysisManagerFunctionProxy>();
    PA.preserve<ScalarEvolutionAnalysis>();
    return PA;
  }));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.0"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.1"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0"), _, _));
  FPM.addPass(MFPHandle.getPass());
  FPM.addPass(createFunctionToLoopPassAdaptor(
      RequireAnalysisLoopPass<MockLoopAnalysisHandle::Analysis>()));

  EXPECT_CALL(MFPHandle, run(HasName("f"), _)).WillOnce(InvokeWithoutArgs([] {
    auto PA = PreservedAnalyses::none();
    PA.preserve<AAManager>();
    PA.preserve<AssumptionAnalysis>();
    PA.preserve<DominatorTreeAnalysis>();
    // Not preserving the `LoopAnalysis`.
    PA.preserve<LoopAnalysisManagerFunctionProxy>();
    PA.preserve<ScalarEvolutionAnalysis>();
    return PA;
  }));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.0"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.1"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0"), _, _));
  FPM.addPass(MFPHandle.getPass());
  FPM.addPass(createFunctionToLoopPassAdaptor(
      RequireAnalysisLoopPass<MockLoopAnalysisHandle::Analysis>()));

  EXPECT_CALL(MFPHandle, run(HasName("f"), _)).WillOnce(InvokeWithoutArgs([] {
    auto PA = PreservedAnalyses::none();
    PA.preserve<AAManager>();
    PA.preserve<AssumptionAnalysis>();
    PA.preserve<DominatorTreeAnalysis>();
    PA.preserve<LoopAnalysis>();
    // Not preserving the `LoopAnalysisManagerFunctionProxy`.
    PA.preserve<ScalarEvolutionAnalysis>();
    return PA;
  }));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.0"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.1"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0"), _, _));
  FPM.addPass(MFPHandle.getPass());
  FPM.addPass(createFunctionToLoopPassAdaptor(
      RequireAnalysisLoopPass<MockLoopAnalysisHandle::Analysis>()));

  EXPECT_CALL(MFPHandle, run(HasName("f"), _)).WillOnce(InvokeWithoutArgs([] {
    auto PA = PreservedAnalyses::none();
    PA.preserve<AAManager>();
    PA.preserve<AssumptionAnalysis>();
    PA.preserve<DominatorTreeAnalysis>();
    PA.preserve<LoopAnalysis>();
    PA.preserve<LoopAnalysisManagerFunctionProxy>();
    // Not preserving `ScalarEvolutionAnalysis`.
    return PA;
  }));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.0"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.1"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0"), _, _));
  FPM.addPass(MFPHandle.getPass());
  FPM.addPass(createFunctionToLoopPassAdaptor(
      RequireAnalysisLoopPass<MockLoopAnalysisHandle::Analysis>()));

  // After all the churn on 'f', we'll compute the loop analysis results for
  // 'g' once with a requires pass and then run our mock pass over g a bunch
  // but just get cached results each time.
  EXPECT_CALL(MLAHandle, run(HasName("loop.g.0"), _, _));
  EXPECT_CALL(MFPHandle, run(HasName("g"), _)).Times(7);

  MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
  MPM.run(*M, MAM);
}

TEST_F(LoopPassManagerTest, IndirectInvalidation) {
  // We need two distinct analysis types and handles.
  enum { A, B };
  MockLoopAnalysisHandleTemplate<A> MLAHandleA;
  MockLoopAnalysisHandleTemplate<B> MLAHandleB;
  LAM.registerPass([&] { return MLAHandleA.getAnalysis(); });
  LAM.registerPass([&] { return MLAHandleB.getAnalysis(); });
  typedef decltype(MLAHandleA)::Analysis AnalysisA;
  typedef decltype(MLAHandleB)::Analysis AnalysisB;

  // Set up AnalysisA to depend on our AnalysisB. For testing purposes we just
  // need to get the AnalysisB results in AnalysisA's run method and check if
  // AnalysisB gets invalidated in AnalysisA's invalidate method.
  ON_CALL(MLAHandleA, run(_, _, _))
      .WillByDefault(Invoke([&](Loop &L, LoopAnalysisManager &AM,
                                LoopStandardAnalysisResults &AR) {
        (void)AM.getResult<AnalysisB>(L, AR);
        return MLAHandleA.getResult();
      }));
  ON_CALL(MLAHandleA, invalidate(_, _, _))
      .WillByDefault(Invoke([](Loop &L, const PreservedAnalyses &PA,
                               LoopAnalysisManager::Invalidator &Inv) {
        auto PAC = PA.getChecker<AnalysisA>();
        return !(PAC.preserved() || PAC.preservedSet<AllAnalysesOn<Loop>>()) ||
               Inv.invalidate<AnalysisB>(L, PA);
      }));

  ::testing::InSequence MakeExpectationsSequenced;

  // Compute the analyses across all of 'f' first.
  EXPECT_CALL(MLAHandleA, run(HasName("loop.0.0"), _, _));
  EXPECT_CALL(MLAHandleB, run(HasName("loop.0.0"), _, _));
  EXPECT_CALL(MLAHandleA, run(HasName("loop.0.1"), _, _));
  EXPECT_CALL(MLAHandleB, run(HasName("loop.0.1"), _, _));
  EXPECT_CALL(MLAHandleA, run(HasName("loop.0"), _, _));
  EXPECT_CALL(MLAHandleB, run(HasName("loop.0"), _, _));

  // Now we invalidate AnalysisB (but not AnalysisA) for one of the loops and
  // preserve everything for the rest. This in turn triggers that one loop to
  // recompute both AnalysisB *and* AnalysisA if indirect invalidation is
  // working.
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.0"), _, _, _))
      .WillOnce(InvokeWithoutArgs([] {
        auto PA = getLoopPassPreservedAnalyses();
        // Specifically preserve AnalysisA so that it would survive if it
        // didn't depend on AnalysisB.
        PA.preserve<AnalysisA>();
        return PA;
      }));
  // It happens that AnalysisB is invalidated first. That shouldn't matter
  // though, and we should still call AnalysisA's invalidation.
  EXPECT_CALL(MLAHandleB, invalidate(HasName("loop.0.0"), _, _));
  EXPECT_CALL(MLAHandleA, invalidate(HasName("loop.0.0"), _, _));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.0"), _, _, _))
      .WillOnce(Invoke([](Loop &L, LoopAnalysisManager &AM,
                          LoopStandardAnalysisResults &AR, LPMUpdater &) {
        (void)AM.getResult<AnalysisA>(L, AR);
        return PreservedAnalyses::all();
      }));
  EXPECT_CALL(MLAHandleA, run(HasName("loop.0.0"), _, _));
  EXPECT_CALL(MLAHandleB, run(HasName("loop.0.0"), _, _));
  // The rest of the loops should run and get cached results.
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.1"), _, _, _))
      .Times(2)
      .WillRepeatedly(Invoke([](Loop &L, LoopAnalysisManager &AM,
                                LoopStandardAnalysisResults &AR, LPMUpdater &) {
        (void)AM.getResult<AnalysisA>(L, AR);
        return PreservedAnalyses::all();
      }));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0"), _, _, _))
      .Times(2)
      .WillRepeatedly(Invoke([](Loop &L, LoopAnalysisManager &AM,
                                LoopStandardAnalysisResults &AR, LPMUpdater &) {
        (void)AM.getResult<AnalysisA>(L, AR);
        return PreservedAnalyses::all();
      }));

  // The run over 'g' should be boring, with us just computing the analyses once
  // up front and then running loop passes and getting cached results.
  EXPECT_CALL(MLAHandleA, run(HasName("loop.g.0"), _, _));
  EXPECT_CALL(MLAHandleB, run(HasName("loop.g.0"), _, _));
  EXPECT_CALL(MLPHandle, run(HasName("loop.g.0"), _, _, _))
      .Times(2)
      .WillRepeatedly(Invoke([](Loop &L, LoopAnalysisManager &AM,
                                LoopStandardAnalysisResults &AR, LPMUpdater &) {
        (void)AM.getResult<AnalysisA>(L, AR);
        return PreservedAnalyses::all();
      }));

  // Build the pipeline and run it.
  ModulePassManager MPM(true);
  FunctionPassManager FPM(true);
  FPM.addPass(
      createFunctionToLoopPassAdaptor(RequireAnalysisLoopPass<AnalysisA>()));
  LoopPassManager LPM(true);
  LPM.addPass(MLPHandle.getPass());
  LPM.addPass(MLPHandle.getPass());
  FPM.addPass(createFunctionToLoopPassAdaptor(std::move(LPM)));
  MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
  MPM.run(*M, MAM);
}

TEST_F(LoopPassManagerTest, IndirectOuterPassInvalidation) {
  typedef decltype(MLAHandle)::Analysis LoopAnalysis;

  MockFunctionAnalysisHandle MFAHandle;
  FAM.registerPass([&] { return MFAHandle.getAnalysis(); });
  typedef decltype(MFAHandle)::Analysis FunctionAnalysis;

  // Set up the loop analysis to depend on both the function and module
  // analysis.
  ON_CALL(MLAHandle, run(_, _, _))
      .WillByDefault(Invoke([&](Loop &L, LoopAnalysisManager &AM,
                                LoopStandardAnalysisResults &AR) {
        auto &FAMP = AM.getResult<FunctionAnalysisManagerLoopProxy>(L, AR);
        auto &FAM = FAMP.getManager();
        Function &F = *L.getHeader()->getParent();
        if (auto *FA = FAM.getCachedResult<FunctionAnalysis>(F))
          FAMP.registerOuterAnalysisInvalidation<FunctionAnalysis,
                                                 LoopAnalysis>();
        return MLAHandle.getResult();
      }));

  ::testing::InSequence MakeExpectationsSequenced;

  // Compute the analyses across all of 'f' first.
  EXPECT_CALL(MFPHandle, run(HasName("f"), _))
      .WillOnce(Invoke([](Function &F, FunctionAnalysisManager &AM) {
        // Force the computing of the function analysis so it is available in
        // this function.
        (void)AM.getResult<FunctionAnalysis>(F);
        return PreservedAnalyses::all();
      }));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.0"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.1"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0"), _, _));

  // Now invalidate the function analysis but preserve the loop analyses.
  // This should trigger immediate invalidation of the loop analyses, despite
  // the fact that they were preserved.
  EXPECT_CALL(MFPHandle, run(HasName("f"), _)).WillOnce(InvokeWithoutArgs([] {
    auto PA = getLoopPassPreservedAnalyses();
    PA.preserveSet<AllAnalysesOn<Loop>>();
    return PA;
  }));
  EXPECT_CALL(MLAHandle, invalidate(HasName("loop.0.0"), _, _));
  EXPECT_CALL(MLAHandle, invalidate(HasName("loop.0.1"), _, _));
  EXPECT_CALL(MLAHandle, invalidate(HasName("loop.0"), _, _));

  // And re-running a requires pass recomputes them.
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.0"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.1"), _, _));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0"), _, _));

  // When we run over 'g' we don't populate the cache with the function
  // analysis.
  EXPECT_CALL(MFPHandle, run(HasName("g"), _))
      .WillOnce(Return(PreservedAnalyses::all()));
  EXPECT_CALL(MLAHandle, run(HasName("loop.g.0"), _, _));

  // Which means that no extra invalidation occurs and cached values are used.
  EXPECT_CALL(MFPHandle, run(HasName("g"), _)).WillOnce(InvokeWithoutArgs([] {
    auto PA = getLoopPassPreservedAnalyses();
    PA.preserveSet<AllAnalysesOn<Loop>>();
    return PA;
  }));

  // Build the pipeline and run it.
  ModulePassManager MPM(true);
  FunctionPassManager FPM(true);
  FPM.addPass(MFPHandle.getPass());
  FPM.addPass(
      createFunctionToLoopPassAdaptor(RequireAnalysisLoopPass<LoopAnalysis>()));
  FPM.addPass(MFPHandle.getPass());
  FPM.addPass(
      createFunctionToLoopPassAdaptor(RequireAnalysisLoopPass<LoopAnalysis>()));
  MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
  MPM.run(*M, MAM);
}

TEST_F(LoopPassManagerTest, LoopChildInsertion) {
  // Super boring module with three loops in a single loop nest.
  M = parseIR(Context, "define void @f() {\n"
                       "entry:\n"
                       "  br label %loop.0\n"
                       "loop.0:\n"
                       "  br i1 undef, label %loop.0.0, label %end\n"
                       "loop.0.0:\n"
                       "  br i1 undef, label %loop.0.0, label %loop.0.1\n"
                       "loop.0.1:\n"
                       "  br i1 undef, label %loop.0.1, label %loop.0.2\n"
                       "loop.0.2:\n"
                       "  br i1 undef, label %loop.0.2, label %loop.0\n"
                       "end:\n"
                       "  ret void\n"
                       "}\n");

  // Build up variables referring into the IR so we can rewrite it below
  // easily.
  Function &F = *M->begin();
  ASSERT_THAT(F, HasName("f"));
  auto BBI = F.begin();
  BasicBlock &EntryBB = *BBI++;
  ASSERT_THAT(EntryBB, HasName("entry"));
  BasicBlock &Loop0BB = *BBI++;
  ASSERT_THAT(Loop0BB, HasName("loop.0"));
  BasicBlock &Loop00BB = *BBI++;
  ASSERT_THAT(Loop00BB, HasName("loop.0.0"));
  BasicBlock &Loop01BB = *BBI++;
  ASSERT_THAT(Loop01BB, HasName("loop.0.1"));
  BasicBlock &Loop02BB = *BBI++;
  ASSERT_THAT(Loop02BB, HasName("loop.0.2"));
  BasicBlock &EndBB = *BBI++;
  ASSERT_THAT(EndBB, HasName("end"));
  ASSERT_THAT(BBI, F.end());

  // Build the pass managers and register our pipeline. We build a single loop
  // pass pipeline consisting of three mock pass runs over each loop. After
  // this we run both domtree and loop verification passes to make sure that
  // the IR remained valid during our mutations.
  ModulePassManager MPM(true);
  FunctionPassManager FPM(true);
  LoopPassManager LPM(true);
  LPM.addPass(MLPHandle.getPass());
  LPM.addPass(MLPHandle.getPass());
  LPM.addPass(MLPHandle.getPass());
  FPM.addPass(createFunctionToLoopPassAdaptor(std::move(LPM)));
  FPM.addPass(DominatorTreeVerifierPass());
  FPM.addPass(LoopVerifierPass());
  MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));

  // All the visit orders are deterministic, so we use simple fully order
  // expectations.
  ::testing::InSequence MakeExpectationsSequenced;

  // We run loop passes three times over each of the loops.
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.0"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.0"), _, _));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.0"), _, _, _))
      .Times(2)
      .WillRepeatedly(Invoke(getLoopAnalysisResult));

  EXPECT_CALL(MLPHandle, run(HasName("loop.0.1"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.1"), _, _));

  // When running over the middle loop, the second run inserts two new child
  // loops, inserting them and itself into the worklist.
  BasicBlock *NewLoop010BB;
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.1"), _, _, _))
      .WillOnce(Invoke([&](Loop &L, LoopAnalysisManager &AM,
                           LoopStandardAnalysisResults &AR,
                           LPMUpdater &Updater) {
        auto *NewLoop = new Loop();
        L.addChildLoop(NewLoop);
        NewLoop010BB = BasicBlock::Create(Context, "loop.0.1.0", &F, &Loop02BB);
        BranchInst::Create(&Loop01BB, NewLoop010BB,
                           UndefValue::get(Type::getInt1Ty(Context)),
                           NewLoop010BB);
        Loop01BB.getTerminator()->replaceUsesOfWith(&Loop01BB, NewLoop010BB);
        AR.DT.addNewBlock(NewLoop010BB, &Loop01BB);
        NewLoop->addBasicBlockToLoop(NewLoop010BB, AR.LI);
        Updater.addChildLoops({NewLoop});
        return PreservedAnalyses::all();
      }));

  // We should immediately drop down to fully visit the new inner loop.
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.1.0"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.1.0"), _, _));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.1.0"), _, _, _))
      .Times(2)
      .WillRepeatedly(Invoke(getLoopAnalysisResult));

  // After visiting the inner loop, we should re-visit the second loop
  // reflecting its new loop nest structure.
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.1"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));

  // In the second run over the middle loop after we've visited the new child,
  // we add another child to check that we can repeatedly add children, and add
  // children to a loop that already has children.
  BasicBlock *NewLoop011BB;
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.1"), _, _, _))
      .WillOnce(Invoke([&](Loop &L, LoopAnalysisManager &AM,
                           LoopStandardAnalysisResults &AR,
                           LPMUpdater &Updater) {
        auto *NewLoop = new Loop();
        L.addChildLoop(NewLoop);
        NewLoop011BB = BasicBlock::Create(Context, "loop.0.1.1", &F, &Loop02BB);
        BranchInst::Create(&Loop01BB, NewLoop011BB,
                           UndefValue::get(Type::getInt1Ty(Context)),
                           NewLoop011BB);
        NewLoop010BB->getTerminator()->replaceUsesOfWith(&Loop01BB,
                                                         NewLoop011BB);
        AR.DT.addNewBlock(NewLoop011BB, NewLoop010BB);
        NewLoop->addBasicBlockToLoop(NewLoop011BB, AR.LI);
        Updater.addChildLoops({NewLoop});
        return PreservedAnalyses::all();
      }));

  // Again, we should immediately drop down to visit the new, unvisited child
  // loop. We don't need to revisit the other child though.
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.1.1"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.1.1"), _, _));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.1.1"), _, _, _))
      .Times(2)
      .WillRepeatedly(Invoke(getLoopAnalysisResult));

  // And now we should pop back up to the second loop and do a full pipeline of
  // three passes on its current form.
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.1"), _, _, _))
      .Times(3)
      .WillRepeatedly(Invoke(getLoopAnalysisResult));

  EXPECT_CALL(MLPHandle, run(HasName("loop.0.2"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.2"), _, _));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.2"), _, _, _))
      .Times(2)
      .WillRepeatedly(Invoke(getLoopAnalysisResult));

  EXPECT_CALL(MLPHandle, run(HasName("loop.0"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0"), _, _));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0"), _, _, _))
      .Times(2)
      .WillRepeatedly(Invoke(getLoopAnalysisResult));

  // Now that all the expected actions are registered, run the pipeline over
  // our module. All of our expectations are verified when the test finishes.
  MPM.run(*M, MAM);
}

TEST_F(LoopPassManagerTest, LoopPeerInsertion) {
  // Super boring module with two loop nests and loop nest with two child
  // loops.
  M = parseIR(Context, "define void @f() {\n"
                       "entry:\n"
                       "  br label %loop.0\n"
                       "loop.0:\n"
                       "  br i1 undef, label %loop.0.0, label %loop.2\n"
                       "loop.0.0:\n"
                       "  br i1 undef, label %loop.0.0, label %loop.0.2\n"
                       "loop.0.2:\n"
                       "  br i1 undef, label %loop.0.2, label %loop.0\n"
                       "loop.2:\n"
                       "  br i1 undef, label %loop.2, label %end\n"
                       "end:\n"
                       "  ret void\n"
                       "}\n");

  // Build up variables referring into the IR so we can rewrite it below
  // easily.
  Function &F = *M->begin();
  ASSERT_THAT(F, HasName("f"));
  auto BBI = F.begin();
  BasicBlock &EntryBB = *BBI++;
  ASSERT_THAT(EntryBB, HasName("entry"));
  BasicBlock &Loop0BB = *BBI++;
  ASSERT_THAT(Loop0BB, HasName("loop.0"));
  BasicBlock &Loop00BB = *BBI++;
  ASSERT_THAT(Loop00BB, HasName("loop.0.0"));
  BasicBlock &Loop02BB = *BBI++;
  ASSERT_THAT(Loop02BB, HasName("loop.0.2"));
  BasicBlock &Loop2BB = *BBI++;
  ASSERT_THAT(Loop2BB, HasName("loop.2"));
  BasicBlock &EndBB = *BBI++;
  ASSERT_THAT(EndBB, HasName("end"));
  ASSERT_THAT(BBI, F.end());
  Constant *Undefi1 = UndefValue::get(Type::getInt1Ty(Context));

  // Build the pass managers and register our pipeline. We build a single loop
  // pass pipeline consisting of three mock pass runs over each loop. After
  // this we run both domtree and loop verification passes to make sure that
  // the IR remained valid during our mutations.
  ModulePassManager MPM(true);
  FunctionPassManager FPM(true);
  LoopPassManager LPM(true);
  LPM.addPass(MLPHandle.getPass());
  LPM.addPass(MLPHandle.getPass());
  LPM.addPass(MLPHandle.getPass());
  FPM.addPass(createFunctionToLoopPassAdaptor(std::move(LPM)));
  FPM.addPass(DominatorTreeVerifierPass());
  FPM.addPass(LoopVerifierPass());
  MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));

  // All the visit orders are deterministic, so we use simple fully order
  // expectations.
  ::testing::InSequence MakeExpectationsSequenced;

  // We run loop passes three times over each of the loops.
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.0"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.0"), _, _));

  // On the second run, we insert a sibling loop.
  BasicBlock *NewLoop01BB;
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.0"), _, _, _))
      .WillOnce(Invoke([&](Loop &L, LoopAnalysisManager &AM,
                           LoopStandardAnalysisResults &AR,
                           LPMUpdater &Updater) {
        auto *NewLoop = new Loop();
        L.getParentLoop()->addChildLoop(NewLoop);
        NewLoop01BB = BasicBlock::Create(Context, "loop.0.1", &F, &Loop02BB);
        BranchInst::Create(&Loop02BB, NewLoop01BB, Undefi1, NewLoop01BB);
        Loop00BB.getTerminator()->replaceUsesOfWith(&Loop02BB, NewLoop01BB);
        auto *NewDTNode = AR.DT.addNewBlock(NewLoop01BB, &Loop00BB);
        AR.DT.changeImmediateDominator(AR.DT[&Loop02BB], NewDTNode);
        NewLoop->addBasicBlockToLoop(NewLoop01BB, AR.LI);
        Updater.addSiblingLoops({NewLoop});
        return PreservedAnalyses::all();
      }));
  // We finish processing this loop as sibling loops don't perturb the
  // postorder walk.
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.0"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));

  // We visit the inserted sibling next.
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.1"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.1"), _, _));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.1"), _, _, _))
      .Times(2)
      .WillRepeatedly(Invoke(getLoopAnalysisResult));

  EXPECT_CALL(MLPHandle, run(HasName("loop.0.2"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.2"), _, _));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.2"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  // Next, on the third pass run on the last inner loop we add more new
  // siblings, more than one, and one with nested child loops. By doing this at
  // the end we make sure that edge case works well.
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.2"), _, _, _))
      .WillOnce(Invoke([&](Loop &L, LoopAnalysisManager &AM,
                           LoopStandardAnalysisResults &AR,
                           LPMUpdater &Updater) {
        Loop *NewLoops[] = {new Loop(), new Loop(), new Loop()};
        L.getParentLoop()->addChildLoop(NewLoops[0]);
        L.getParentLoop()->addChildLoop(NewLoops[1]);
        NewLoops[1]->addChildLoop(NewLoops[2]);
        auto *NewLoop03BB =
            BasicBlock::Create(Context, "loop.0.3", &F, &Loop2BB);
        auto *NewLoop04BB =
            BasicBlock::Create(Context, "loop.0.4", &F, &Loop2BB);
        auto *NewLoop040BB =
            BasicBlock::Create(Context, "loop.0.4.0", &F, &Loop2BB);
        Loop02BB.getTerminator()->replaceUsesOfWith(&Loop0BB, NewLoop03BB);
        BranchInst::Create(NewLoop04BB, NewLoop03BB, Undefi1, NewLoop03BB);
        BranchInst::Create(&Loop0BB, NewLoop040BB, Undefi1, NewLoop04BB);
        BranchInst::Create(NewLoop04BB, NewLoop040BB, Undefi1, NewLoop040BB);
        AR.DT.addNewBlock(NewLoop03BB, &Loop02BB);
        AR.DT.addNewBlock(NewLoop04BB, NewLoop03BB);
        AR.DT.addNewBlock(NewLoop040BB, NewLoop04BB);
        NewLoops[0]->addBasicBlockToLoop(NewLoop03BB, AR.LI);
        NewLoops[1]->addBasicBlockToLoop(NewLoop04BB, AR.LI);
        NewLoops[2]->addBasicBlockToLoop(NewLoop040BB, AR.LI);
        Updater.addSiblingLoops({NewLoops[0], NewLoops[1]});
        return PreservedAnalyses::all();
      }));

  EXPECT_CALL(MLPHandle, run(HasName("loop.0.3"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.3"), _, _));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.3"), _, _, _))
      .Times(2)
      .WillRepeatedly(Invoke(getLoopAnalysisResult));

  // Note that we need to visit the inner loop of this added sibling before the
  // sibling itself!
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.4.0"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.4.0"), _, _));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.4.0"), _, _, _))
      .Times(2)
      .WillRepeatedly(Invoke(getLoopAnalysisResult));

  EXPECT_CALL(MLPHandle, run(HasName("loop.0.4"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.4"), _, _));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.4"), _, _, _))
      .Times(2)
      .WillRepeatedly(Invoke(getLoopAnalysisResult));

  // And only now do we visit the outermost loop of the nest.
  EXPECT_CALL(MLPHandle, run(HasName("loop.0"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0"), _, _));
  // On the second pass, we add sibling loops which become new top-level loops.
  EXPECT_CALL(MLPHandle, run(HasName("loop.0"), _, _, _))
      .WillOnce(Invoke([&](Loop &L, LoopAnalysisManager &AM,
                           LoopStandardAnalysisResults &AR,
                           LPMUpdater &Updater) {
        auto *NewLoop = new Loop();
        AR.LI.addTopLevelLoop(NewLoop);
        auto *NewLoop1BB = BasicBlock::Create(Context, "loop.1", &F, &Loop2BB);
        BranchInst::Create(&Loop2BB, NewLoop1BB, Undefi1, NewLoop1BB);
        Loop0BB.getTerminator()->replaceUsesOfWith(&Loop2BB, NewLoop1BB);
        auto *NewDTNode = AR.DT.addNewBlock(NewLoop1BB, &Loop0BB);
        AR.DT.changeImmediateDominator(AR.DT[&Loop2BB], NewDTNode);
        NewLoop->addBasicBlockToLoop(NewLoop1BB, AR.LI);
        Updater.addSiblingLoops({NewLoop});
        return PreservedAnalyses::all();
      }));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));

  EXPECT_CALL(MLPHandle, run(HasName("loop.1"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.1"), _, _));
  EXPECT_CALL(MLPHandle, run(HasName("loop.1"), _, _, _))
      .Times(2)
      .WillRepeatedly(Invoke(getLoopAnalysisResult));

  EXPECT_CALL(MLPHandle, run(HasName("loop.2"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.2"), _, _));
  EXPECT_CALL(MLPHandle, run(HasName("loop.2"), _, _, _))
      .Times(2)
      .WillRepeatedly(Invoke(getLoopAnalysisResult));

  // Now that all the expected actions are registered, run the pipeline over
  // our module. All of our expectations are verified when the test finishes.
  MPM.run(*M, MAM);
}

TEST_F(LoopPassManagerTest, LoopDeletion) {
  // Build a module with a single loop nest that contains one outer loop with
  // three subloops, and one of those with its own subloop. We will
  // incrementally delete all of these to test different deletion scenarios.
  M = parseIR(Context, "define void @f() {\n"
                       "entry:\n"
                       "  br label %loop.0\n"
                       "loop.0:\n"
                       "  br i1 undef, label %loop.0.0, label %end\n"
                       "loop.0.0:\n"
                       "  br i1 undef, label %loop.0.0, label %loop.0.1\n"
                       "loop.0.1:\n"
                       "  br i1 undef, label %loop.0.1, label %loop.0.2\n"
                       "loop.0.2:\n"
                       "  br i1 undef, label %loop.0.2.0, label %loop.0\n"
                       "loop.0.2.0:\n"
                       "  br i1 undef, label %loop.0.2.0, label %loop.0.2\n"
                       "end:\n"
                       "  ret void\n"
                       "}\n");

  // Build up variables referring into the IR so we can rewrite it below
  // easily.
  Function &F = *M->begin();
  ASSERT_THAT(F, HasName("f"));
  auto BBI = F.begin();
  BasicBlock &EntryBB = *BBI++;
  ASSERT_THAT(EntryBB, HasName("entry"));
  BasicBlock &Loop0BB = *BBI++;
  ASSERT_THAT(Loop0BB, HasName("loop.0"));
  BasicBlock &Loop00BB = *BBI++;
  ASSERT_THAT(Loop00BB, HasName("loop.0.0"));
  BasicBlock &Loop01BB = *BBI++;
  ASSERT_THAT(Loop01BB, HasName("loop.0.1"));
  BasicBlock &Loop02BB = *BBI++;
  ASSERT_THAT(Loop02BB, HasName("loop.0.2"));
  BasicBlock &Loop020BB = *BBI++;
  ASSERT_THAT(Loop020BB, HasName("loop.0.2.0"));
  BasicBlock &EndBB = *BBI++;
  ASSERT_THAT(EndBB, HasName("end"));
  ASSERT_THAT(BBI, F.end());
  Constant *Undefi1 = UndefValue::get(Type::getInt1Ty(Context));

  // Helper to do the actual deletion of a loop. We directly encode this here
  // to isolate ourselves from the rest of LLVM and for simplicity. Here we can
  // egregiously cheat based on knowledge of the test case. For example, we
  // have no PHI nodes and there is always a single i-dom.
  auto DeleteLoopBlocks = [](Loop &L, BasicBlock &IDomBB,
                             LoopStandardAnalysisResults &AR,
                             LPMUpdater &Updater) {
    for (BasicBlock *LoopBB : L.blocks()) {
      SmallVector<DomTreeNode *, 4> ChildNodes(AR.DT[LoopBB]->begin(),
                                               AR.DT[LoopBB]->end());
      for (DomTreeNode *ChildNode : ChildNodes)
        AR.DT.changeImmediateDominator(ChildNode, AR.DT[&IDomBB]);
      AR.DT.eraseNode(LoopBB);
      LoopBB->dropAllReferences();
    }
    SmallVector<BasicBlock *, 4> LoopBBs(L.block_begin(), L.block_end());
    Updater.markLoopAsDeleted(L);
    AR.LI.markAsRemoved(&L);
    for (BasicBlock *LoopBB : LoopBBs)
      LoopBB->eraseFromParent();
  };

  // Build up the pass managers.
  ModulePassManager MPM(true);
  FunctionPassManager FPM(true);
  // We run several loop pass pipelines across the loop nest, but they all take
  // the same form of three mock pass runs in a loop pipeline followed by
  // domtree and loop verification. We use a lambda to stamp this out each
  // time.
  auto AddLoopPipelineAndVerificationPasses = [&] {
    LoopPassManager LPM(true);
    LPM.addPass(MLPHandle.getPass());
    LPM.addPass(MLPHandle.getPass());
    LPM.addPass(MLPHandle.getPass());
    FPM.addPass(createFunctionToLoopPassAdaptor(std::move(LPM)));
    FPM.addPass(DominatorTreeVerifierPass());
    FPM.addPass(LoopVerifierPass());
  };

  // All the visit orders are deterministic so we use simple fully order
  // expectations.
  ::testing::InSequence MakeExpectationsSequenced;

  // We run the loop pipeline with three passes over each of the loops. When
  // running over the middle loop, the second pass in the pipeline deletes it.
  // This should prevent the third pass from visiting it but otherwise leave
  // the process unimpacted.
  AddLoopPipelineAndVerificationPasses();
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.0"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.0"), _, _));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.0"), _, _, _))
      .Times(2)
      .WillRepeatedly(Invoke(getLoopAnalysisResult));

  EXPECT_CALL(MLPHandle, run(HasName("loop.0.1"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.1"), _, _));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.1"), _, _, _))
      .WillOnce(
          Invoke([&](Loop &L, LoopAnalysisManager &AM,
                     LoopStandardAnalysisResults &AR, LPMUpdater &Updater) {
            AR.SE.forgetLoop(&L);
            Loop00BB.getTerminator()->replaceUsesOfWith(&Loop01BB, &Loop02BB);
            DeleteLoopBlocks(L, Loop00BB, AR, Updater);
            return PreservedAnalyses::all();
          }));

  EXPECT_CALL(MLPHandle, run(HasName("loop.0.2.0"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.2.0"), _, _));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.2.0"), _, _, _))
      .Times(2)
      .WillRepeatedly(Invoke(getLoopAnalysisResult));

  EXPECT_CALL(MLPHandle, run(HasName("loop.0.2"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.2"), _, _));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.2"), _, _, _))
      .Times(2)
      .WillRepeatedly(Invoke(getLoopAnalysisResult));

  EXPECT_CALL(MLPHandle, run(HasName("loop.0"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0"), _, _));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0"), _, _, _))
      .Times(2)
      .WillRepeatedly(Invoke(getLoopAnalysisResult));

  // Run the loop pipeline again. This time we delete the last loop, which
  // contains a nested loop within it, and we reuse its inner loop object to
  // insert a new loop into the nest. This makes sure that we don't reuse
  // cached analysis results for loop objects when removed just because their
  // pointers match, and that we can handle nested loop deletion.
  AddLoopPipelineAndVerificationPasses();
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.0"), _, _, _))
      .Times(3)
      .WillRepeatedly(Invoke(getLoopAnalysisResult));

  EXPECT_CALL(MLPHandle, run(HasName("loop.0.2.0"), _, _, _))
      .Times(3)
      .WillRepeatedly(Invoke(getLoopAnalysisResult));

  EXPECT_CALL(MLPHandle, run(HasName("loop.0.2"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  BasicBlock *NewLoop03BB;
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.2"), _, _, _))
      .WillOnce(
          Invoke([&](Loop &L, LoopAnalysisManager &AM,
                     LoopStandardAnalysisResults &AR, LPMUpdater &Updater) {
            // Delete the inner loop first. we also do this manually because we
            // want to preserve the loop object and reuse it.
            AR.SE.forgetLoop(*L.begin());
            Loop02BB.getTerminator()->replaceUsesOfWith(&Loop020BB, &Loop02BB);
            assert(std::next((*L.begin())->block_begin()) ==
                       (*L.begin())->block_end() &&
                   "There should only be one block.");
            assert(AR.DT[&Loop020BB]->getNumChildren() == 0 &&
                   "Cannot have children in the domtree!");
            AR.DT.eraseNode(&Loop020BB);
            Updater.markLoopAsDeleted(**L.begin());
            AR.LI.removeBlock(&Loop020BB);
            auto *OldL = L.removeChildLoop(L.begin());
            Loop020BB.eraseFromParent();

            auto *ParentL = L.getParentLoop();
            AR.SE.forgetLoop(&L);
            Loop00BB.getTerminator()->replaceUsesOfWith(&Loop02BB, &Loop0BB);
            DeleteLoopBlocks(L, Loop00BB, AR, Updater);

            // Now insert a new sibling loop, reusing a loop pointer.
            ParentL->addChildLoop(OldL);
            NewLoop03BB = BasicBlock::Create(Context, "loop.0.3", &F, &EndBB);
            BranchInst::Create(&Loop0BB, NewLoop03BB, Undefi1, NewLoop03BB);
            Loop00BB.getTerminator()->replaceUsesOfWith(&Loop0BB, NewLoop03BB);
            AR.DT.addNewBlock(NewLoop03BB, &Loop00BB);
            OldL->addBasicBlockToLoop(NewLoop03BB, AR.LI);
            Updater.addSiblingLoops({OldL});
            return PreservedAnalyses::all();
          }));

  // To respect our inner-to-outer traversal order, we must visit the
  // newly-inserted sibling of the loop we just deleted before we visit the
  // outer loop. When we do so, this must compute a fresh analysis result, even
  // though our new loop has the same pointer value as the loop we deleted.
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.3"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLAHandle, run(HasName("loop.0.3"), _, _));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.3"), _, _, _))
      .Times(2)
      .WillRepeatedly(Invoke(getLoopAnalysisResult));

  EXPECT_CALL(MLPHandle, run(HasName("loop.0"), _, _, _))
      .Times(3)
      .WillRepeatedly(Invoke(getLoopAnalysisResult));

  // In the final loop pipeline run we delete every loop, including the last
  // loop of the nest. We do this again in the second pass in the pipeline, and
  // as a consequence we never make it to three runs on any loop. We also cover
  // deleting multiple loops in a single pipeline, deleting the first loop and
  // deleting the (last) top level loop.
  AddLoopPipelineAndVerificationPasses();
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.0"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.0"), _, _, _))
      .WillOnce(
          Invoke([&](Loop &L, LoopAnalysisManager &AM,
                     LoopStandardAnalysisResults &AR, LPMUpdater &Updater) {
            AR.SE.forgetLoop(&L);
            Loop0BB.getTerminator()->replaceUsesOfWith(&Loop00BB, NewLoop03BB);
            DeleteLoopBlocks(L, Loop0BB, AR, Updater);
            return PreservedAnalyses::all();
          }));

  EXPECT_CALL(MLPHandle, run(HasName("loop.0.3"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0.3"), _, _, _))
      .WillOnce(
          Invoke([&](Loop &L, LoopAnalysisManager &AM,
                     LoopStandardAnalysisResults &AR, LPMUpdater &Updater) {
            AR.SE.forgetLoop(&L);
            Loop0BB.getTerminator()->replaceUsesOfWith(NewLoop03BB, &Loop0BB);
            DeleteLoopBlocks(L, Loop0BB, AR, Updater);
            return PreservedAnalyses::all();
          }));

  EXPECT_CALL(MLPHandle, run(HasName("loop.0"), _, _, _))
      .WillOnce(Invoke(getLoopAnalysisResult));
  EXPECT_CALL(MLPHandle, run(HasName("loop.0"), _, _, _))
      .WillOnce(
          Invoke([&](Loop &L, LoopAnalysisManager &AM,
                     LoopStandardAnalysisResults &AR, LPMUpdater &Updater) {
            AR.SE.forgetLoop(&L);
            EntryBB.getTerminator()->replaceUsesOfWith(&Loop0BB, &EndBB);
            DeleteLoopBlocks(L, EntryBB, AR, Updater);
            return PreservedAnalyses::all();
          }));

  // Add the function pass pipeline now that it is fully built up and run it
  // over the module's one function.
  MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
  MPM.run(*M, MAM);
}
}
