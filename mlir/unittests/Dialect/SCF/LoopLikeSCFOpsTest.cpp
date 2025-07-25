//===- LoopLikeSCFOpsTest.cpp - SCF LoopLikeOpInterface Tests -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/SCF/Utils/Utils.h"
#include "mlir/Dialect/Utils/StaticValueUtils.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/OwningOpRef.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Interfaces/LoopLikeInterface.h"
#include "gtest/gtest.h"

using namespace mlir;
using namespace mlir::scf;

//===----------------------------------------------------------------------===//
// Test Fixture
//===----------------------------------------------------------------------===//

class SCFLoopLikeTest : public ::testing::Test {
protected:
  SCFLoopLikeTest() : b(&context), loc(UnknownLoc::get(&context)) {
    context.loadDialect<affine::AffineDialect, arith::ArithDialect,
                        scf::SCFDialect>();
  }

  void checkUnidimensional(LoopLikeOpInterface loopLikeOp) {
    std::optional<OpFoldResult> maybeSingleLb =
        loopLikeOp.getSingleLowerBound();
    EXPECT_TRUE(maybeSingleLb.has_value());
    std::optional<OpFoldResult> maybeSingleUb =
        loopLikeOp.getSingleUpperBound();
    EXPECT_TRUE(maybeSingleUb.has_value());
    std::optional<OpFoldResult> maybeSingleStep = loopLikeOp.getSingleStep();
    EXPECT_TRUE(maybeSingleStep.has_value());
    std::optional<OpFoldResult> maybeSingleIndVar =
        loopLikeOp.getSingleInductionVar();
    EXPECT_TRUE(maybeSingleIndVar.has_value());

    std::optional<SmallVector<OpFoldResult>> maybeLb =
        loopLikeOp.getLoopLowerBounds();
    ASSERT_TRUE(maybeLb.has_value());
    EXPECT_EQ((*maybeLb).size(), 1u);
    std::optional<SmallVector<OpFoldResult>> maybeUb =
        loopLikeOp.getLoopUpperBounds();
    ASSERT_TRUE(maybeUb.has_value());
    EXPECT_EQ((*maybeUb).size(), 1u);
    std::optional<SmallVector<OpFoldResult>> maybeStep =
        loopLikeOp.getLoopSteps();
    ASSERT_TRUE(maybeStep.has_value());
    EXPECT_EQ((*maybeStep).size(), 1u);
    std::optional<SmallVector<Value>> maybeInductionVars =
        loopLikeOp.getLoopInductionVars();
    ASSERT_TRUE(maybeInductionVars.has_value());
    EXPECT_EQ((*maybeInductionVars).size(), 1u);
  }

  void checkMultidimensional(LoopLikeOpInterface loopLikeOp) {
    std::optional<OpFoldResult> maybeSingleLb =
        loopLikeOp.getSingleLowerBound();
    EXPECT_FALSE(maybeSingleLb.has_value());
    std::optional<OpFoldResult> maybeSingleUb =
        loopLikeOp.getSingleUpperBound();
    EXPECT_FALSE(maybeSingleUb.has_value());
    std::optional<OpFoldResult> maybeSingleStep = loopLikeOp.getSingleStep();
    EXPECT_FALSE(maybeSingleStep.has_value());
    std::optional<OpFoldResult> maybeSingleIndVar =
        loopLikeOp.getSingleInductionVar();
    EXPECT_FALSE(maybeSingleIndVar.has_value());

    std::optional<SmallVector<OpFoldResult>> maybeLb =
        loopLikeOp.getLoopLowerBounds();
    ASSERT_TRUE(maybeLb.has_value());
    EXPECT_EQ((*maybeLb).size(), 2u);
    std::optional<SmallVector<OpFoldResult>> maybeUb =
        loopLikeOp.getLoopUpperBounds();
    ASSERT_TRUE(maybeUb.has_value());
    EXPECT_EQ((*maybeUb).size(), 2u);
    std::optional<SmallVector<OpFoldResult>> maybeStep =
        loopLikeOp.getLoopSteps();
    ASSERT_TRUE(maybeStep.has_value());
    EXPECT_EQ((*maybeStep).size(), 2u);
    std::optional<SmallVector<Value>> maybeInductionVars =
        loopLikeOp.getLoopInductionVars();
    ASSERT_TRUE(maybeInductionVars.has_value());
    EXPECT_EQ((*maybeInductionVars).size(), 2u);
  }

  void checkNormalized(LoopLikeOpInterface loopLikeOp) {
    std::optional<SmallVector<OpFoldResult>> maybeLb =
        loopLikeOp.getLoopLowerBounds();
    ASSERT_TRUE(maybeLb.has_value());
    std::optional<SmallVector<OpFoldResult>> maybeStep =
        loopLikeOp.getLoopSteps();
    ASSERT_TRUE(maybeStep.has_value());

    auto allEqual = [](ArrayRef<OpFoldResult> results, int64_t val) {
      return llvm::all_of(results, [&](OpFoldResult ofr) {
        auto intValue = getConstantIntValue(ofr);
        return intValue.has_value() && intValue == val;
      });
    };
    EXPECT_TRUE(allEqual(*maybeLb, 0));
    EXPECT_TRUE(allEqual(*maybeStep, 1));
  }

  MLIRContext context;
  OpBuilder b;
  Location loc;
};

TEST_F(SCFLoopLikeTest, queryUnidimensionalLooplikes) {
  OwningOpRef<arith::ConstantIndexOp> lb =
      arith::ConstantIndexOp::create(b, loc, 0);
  OwningOpRef<arith::ConstantIndexOp> ub =
      arith::ConstantIndexOp::create(b, loc, 10);
  OwningOpRef<arith::ConstantIndexOp> step =
      arith::ConstantIndexOp::create(b, loc, 2);

  OwningOpRef<scf::ForOp> forOp =
      scf::ForOp::create(b, loc, lb.get(), ub.get(), step.get());
  checkUnidimensional(forOp.get());

  OwningOpRef<scf::ForallOp> forallOp = scf::ForallOp::create(
      b, loc, ArrayRef<OpFoldResult>(lb->getResult()),
      ArrayRef<OpFoldResult>(ub->getResult()),
      ArrayRef<OpFoldResult>(step->getResult()), ValueRange(), std::nullopt);
  checkUnidimensional(forallOp.get());

  OwningOpRef<scf::ParallelOp> parallelOp = scf::ParallelOp::create(
      b, loc, ValueRange(lb->getResult()), ValueRange(ub->getResult()),
      ValueRange(step->getResult()), ValueRange());
  checkUnidimensional(parallelOp.get());
}

TEST_F(SCFLoopLikeTest, queryMultidimensionalLooplikes) {
  OwningOpRef<arith::ConstantIndexOp> lb =
      arith::ConstantIndexOp::create(b, loc, 0);
  OwningOpRef<arith::ConstantIndexOp> ub =
      arith::ConstantIndexOp::create(b, loc, 10);
  OwningOpRef<arith::ConstantIndexOp> step =
      arith::ConstantIndexOp::create(b, loc, 2);

  OwningOpRef<scf::ForallOp> forallOp = scf::ForallOp::create(
      b, loc, ArrayRef<OpFoldResult>({lb->getResult(), lb->getResult()}),
      ArrayRef<OpFoldResult>({ub->getResult(), ub->getResult()}),
      ArrayRef<OpFoldResult>({step->getResult(), step->getResult()}),
      ValueRange(), std::nullopt);
  checkMultidimensional(forallOp.get());

  OwningOpRef<scf::ParallelOp> parallelOp = scf::ParallelOp::create(
      b, loc, ValueRange({lb->getResult(), lb->getResult()}),
      ValueRange({ub->getResult(), ub->getResult()}),
      ValueRange({step->getResult(), step->getResult()}), ValueRange());
  checkMultidimensional(parallelOp.get());
}

TEST_F(SCFLoopLikeTest, testForallNormalize) {
  OwningOpRef<arith::ConstantIndexOp> lb =
      arith::ConstantIndexOp::create(b, loc, 1);
  OwningOpRef<arith::ConstantIndexOp> ub =
      arith::ConstantIndexOp::create(b, loc, 10);
  OwningOpRef<arith::ConstantIndexOp> step =
      arith::ConstantIndexOp::create(b, loc, 3);

  scf::ForallOp forallOp = scf::ForallOp::create(
      b, loc, ArrayRef<OpFoldResult>({lb->getResult(), lb->getResult()}),
      ArrayRef<OpFoldResult>({ub->getResult(), ub->getResult()}),
      ArrayRef<OpFoldResult>({step->getResult(), step->getResult()}),
      ValueRange(), std::nullopt);
  // Create a user of the induction variable. Bitcast is chosen for simplicity
  // since it is unary.
  b.setInsertionPointToStart(forallOp.getBody());
  arith::BitcastOp::create(b, UnknownLoc::get(&context), b.getF64Type(),
                           forallOp.getInductionVar(0));
  IRRewriter rewriter(b);
  FailureOr<scf::ForallOp> maybeNormalizedForallOp =
      normalizeForallOp(rewriter, forallOp);
  EXPECT_TRUE(succeeded(maybeNormalizedForallOp));
  OwningOpRef<scf::ForallOp> normalizedForallOp(*maybeNormalizedForallOp);
  checkNormalized(normalizedForallOp.get());

  // Check that the IV user has been updated to use the denormalized variable.
  Block *body = normalizedForallOp->getBody();
  auto bitcastOps = body->getOps<arith::BitcastOp>();
  ASSERT_EQ(std::distance(bitcastOps.begin(), bitcastOps.end()), 1);
  arith::BitcastOp ivUser = *bitcastOps.begin();
  ASSERT_NE(ivUser.getIn(), normalizedForallOp->getInductionVar(0));
}
