# Makefile for flopoco/src
SUBDIRS =
DIST_SUBDIRS =

EXTRA_DIST = VHDLLexerRule.l FPExpressions/ExpressionScanner.l FPExpressions/ExpressionParser.y $(top_srcdir)/src/FPExpressions/ExpressionParser.h \
                $(top_srcdir)/src/FPExpressions/FlopocoScanner.h
ACLOCAL_AMFLAGS = -I ../m4

BUILT_SOURCES = VHDLLexer.cpp \
                ExpressionParser.cpp \
                ExpressionScanner.cpp

CLEANFILES = $(BUILT_SOURCES) lex.yy.c *.output

noinst_LTLIBRARIES = libflopoco.la

noinst_HEADERS = \
          LexerContext.hpp \
          FloPoCo.hpp \
          Operator.hpp \
          Signal.hpp \
          utils.hpp \
          FlopocoStream.hpp \
          FPPipeline.hpp \
          Tools/ResourceEstimationHelper.hpp \
          Tools/FloorplanningHelper.hpp \
          FPExpressions/ExpressionParserData.h \
          sollya.h \
          UtilSollya.hh \
          apps/Collision.hpp \
          apps/FPFMAcc.hpp \
          apps/FPJacobi.hpp \
          ConstMult/IntIntKCM.hpp \
          ConstMult/CRFPConstMult.hpp \
          ConstMult/FixRealKCM.hpp \
          ConstMult/FPRealKCM.hpp \
          ConstMult/FPConstMult.hpp \
          ConstMult/IntConstMult.hpp \
          ConstMult/KCMTable.hpp \
          ConstMult/ShiftAddDag.hpp \
          ConstMult/ShiftAddOp.hpp \
          IntConstDiv.hpp \
          FPConstDiv.hpp \
          FPPow.hpp \
          fplogarithm/FirstInvTable.hpp \
          fplogarithm/FirstLogTable.hpp \
          fplogarithm/OtherLogTable.hpp \
          LNS/AtanPow.hpp \
          LNS/Cotran.hpp \
          LNS/CotranHybrid.hpp \
          LNS/CotranTables.hpp \
          LNS/LNSAdd.hpp \
          LNS/LNSAddSub.hpp \
          LNS/LNSDiv.hpp \
          LNS/LNSMul.hpp \
          LNS/LNSSqrt.hpp \
          LNS/LogSinCos.hpp \
          squareroot/PolynomialTableCorrectRounded.hpp \
          squareroot/PolynomialTable.hpp \
          Targets/DSP.hpp \
          Targets/CycloneII.hpp \
          Targets/CycloneIII.hpp \
          Targets/CycloneIV.hpp \
          Targets/CycloneV.hpp \
          Targets/StratixII.hpp \
          Targets/StratixIII.hpp \
          Targets/StratixIV.hpp \
          Targets/StratixV.hpp \
          Targets/Spartan3.hpp \
          Targets/Virtex4.hpp \
          Targets/Virtex5.hpp \
          Targets/Virtex6.hpp \
          DotProduct.hpp \
          Tables/DualTable.hpp \
          DualTable2.hpp \
          Fix2FP.hpp \
          FP2Fix.hpp \
          FPAddDualPath.hpp \
          FPAddSinglePath.hpp \
          FPAdd3Input.hpp \
          FPDiv.hpp \
          FPExp.hpp \
          FPLog.hpp \
          FPMult.hpp \
          FPMultKaratsuba.hpp \
          TestBenchs/FPNumber.hpp \
          FPSqrt.hpp \
          FPSqrtPoly.hpp \
          FPSquare.hpp \
          IEEENumber.hpp \
          InputIEEE.hpp \
          IntAdder.hpp \
          IntAddition/IntAdderClassical.hpp \
          IntAddition/IntAdderAlternative.hpp \
          IntAddition/IntAdderShortLatency.hpp \
          IntAddition/LongIntAdderAddAddMuxGen1.hpp \
          IntAddition/LongIntAdderCmpCmpAddGen1.hpp \
          IntAddition/LongIntAdderCmpAddIncGen1.hpp \
          IntAddition/IntAdderSpecific.hpp \
          IntAddition/IntComparatorSpecific.hpp \
          IntAddition/CarryGenerationCircuit.hpp \
          IntAddition/LongIntAdderAddAddMuxGen2.hpp \
          IntAddition/LongIntAdderCmpCmpAddGen2.hpp \
          IntAddition/LongIntAdderCmpAddIncGen2.hpp \
          IntAddition/LongIntAdderMuxNetwork.hpp \
          IntMultiAdder.hpp \
          IntAddition/IntNAdder.hpp \
          IntAddition/IntCompressorTree.hpp \
          IntAddition/PopCount.hpp \
          IntAddition/NewCompressorTree.hpp \
          IntAddition/BasicCompressor.hpp \
          BitHeap.hpp \
          WeightedBit.hpp \
          IntComparator.hpp \
          IntDualSub.hpp \
          Plotter.hpp \
          IntMultiplier.hpp \
          FixMultAdd.hpp \
          IntMultipliers/IntKaratsuba.hpp \
          IntSquarer.hpp \
          IntMultipliers/FormalBinaryProduct.hpp \
          IntMultipliers/GenericBinaryPolynomial.hpp \
          IntMultipliers/IntPower.hpp \
          IntMultipliers/MultiplierBlock.hpp \
          IntMultipliers/FixSinPoly.hpp \
          IntMultipliers/FixXPow3Div6.hpp \
          ConstMult/IntConstDiv3.hpp \
          LargeAccToFP.hpp \
          FPLargeAcc.hpp LZOC.hpp \
          OutputIEEE.hpp \
          Shifters.hpp \
          FixedPointFunctions/Function.hpp \
          FixedPointFunctions/PiecewiseFunction.hpp \
          FixedPointFunctions/FunctionTable.hpp \
          FixedPointFunctions/PolyCoeffTable.hpp \
          FixedPointFunctions/PolynomialEvaluator.hpp \
          FixedPointFunctions/FunctionEvaluator.hpp \
          FixedPointFunctions/HOTBM.hpp \
          FixedPointFunctions/HOTBM/Component.hh \
          FixedPointFunctions/HOTBM/HOTBMInstance.hh\
          FixedPointFunctions/HOTBM/Param.hh\
          FixedPointFunctions/HOTBM/Exhaustive.hh\
          FixedPointFunctions/HOTBM/Util.hh\
          FixedPointFunctions/HOTBM/PWPolynomial.hh\
          FixedPointFunctions/HOTBM/Polynomial.hh\
          FixedPointFunctions/HOTBM/MPPolynomial.hh\
          FixedPointFunctions/HOTBM/Power.hh\
          FixedPointFunctions/HOTBM/PowerAdHoc.hh\
          FixedPointFunctions/HOTBM/TermROM.hh\
          FixedPointFunctions/HOTBM/Term.hh\
          FixedPointFunctions/HOTBM/Minimax.hh\
          FixedPointFunctions/HOTBM/PowerROM.hh\
          FixedPointFunctions/HOTBM/TermPowMult.hh \
          FixedPointFunctions/GenericEvaluator.hpp \
          Complex/FixedComplexAdder.hpp \
          Complex/FixedComplexMultiplier.hpp \
          Tables/Table.hpp \
          Target.hpp \
          TestBench.hpp \
          TestCase.hpp \
          UserDefinedOperator.hpp \
          Wrapper.hpp \
          FPSumOf3Squares.hpp \
          FixSinCos.hpp \
          CORDIC/FixedPointSinOrCos.hpp \
          CORDIC/CordicSinCos.hpp \
          FixedPointFIR.hpp \
          FixedPointDCT.hpp \
          FPAddSub.hpp \
          random/urng/LutSrRng.hpp

libflopoco_la_SOURCES = \
                        Operator.cpp \
                        Signal.cpp \
                        utils.cpp \
                        FlopocoStream.cpp \
                        VHDLLexer.cpp \
                        Tools/ResourceEstimationHelper.cpp \
                        Tools/FloorplanningHelper.cpp \
                        Targets/DSP.cpp \
                        Target.cpp \
                        Targets/CycloneII.cpp \
                        Targets/CycloneIII.cpp \
                        Targets/CycloneIV.cpp \
                        Targets/CycloneV.cpp \
                        Targets/StratixII.cpp \
                        Targets/StratixIII.cpp \
                        Targets/StratixIV.cpp \
                        Targets/StratixV.cpp \
                        Targets/Spartan3.cpp \
                        Targets/Virtex4.cpp \
                        Targets/Virtex5.cpp \
                        Targets/Virtex6.cpp \
                        Table.cpp \
                        DualTable.cpp \
                        DualTable2.cpp \
                        FPExpressions/ExpressionParserData.cpp \
                        ExpressionParser.cpp \
                        ExpressionScanner.cpp \
                        FPPipeline.cpp \
                        Wrapper.cpp \
                        TestBench.cpp \
                        UserDefinedOperator.cpp\
                        IntAdder.cpp\
                        IntAddition/IntAdderClassical.cpp\
                        IntAddition/IntAdderAlternative.cpp\
                        IntAddition/IntAdderShortLatency.cpp\
                        IntAddition/LongIntAdderAddAddMuxGen1.cpp \
                        IntAddition/LongIntAdderCmpCmpAddGen1.cpp \
                        IntAddition/LongIntAdderCmpAddIncGen1.cpp \
                        IntAddition/IntAdderSpecific.cpp \
                        IntAddition/IntComparatorSpecific.cpp \
                        IntAddition/CarryGenerationCircuit.cpp \
                        IntAddition/LongIntAdderAddAddMuxGen2.cpp \
                        IntAddition/LongIntAdderCmpCmpAddGen2.cpp \
                        IntAddition/LongIntAdderCmpAddIncGen2.cpp \
                        IntAddition/LongIntAdderMuxNetwork.cpp \
                        IntMultiAdder.cpp \
                        IntAddition/IntNAdder.cpp \
                        IntAddition/IntCompressorTree.cpp \
                        IntAddition/PopCount.cpp \
                        IntAddition/NewCompressorTree.cpp \
                        IntAddition/BasicCompressor.cpp \
                        BitHeap.cpp \
                        WeightedBit.cpp \
                        IntComparator.cpp \
                        IntDualSub.cpp \
                        Plotter.cpp \
                        IntMultiplier.cpp \
			FixMultAdd.cpp \
                        IntMultipliers/IntKaratsuba.cpp \
                        IntSquarer.cpp \
                        IntMultipliers/FormalBinaryProduct.cpp \
                        IntMultipliers/GenericBinaryPolynomial.cpp \
                        IntMultipliers/IntPower.cpp \
                        IntMultipliers/MultiplierBlock.cpp \
                        IntMultipliers/FixSinPoly.cpp \
                        IntMultipliers/FixXPow3Div6.cpp \
                        ConstMult/IntConstDiv3.cpp \
                        LZOC.cpp \
                        LZOCShifterSticky.cpp \
                        Shifters.cpp \
                        FixedPointFunctions/Function.cpp \
                        FixedPointFunctions/PiecewiseFunction.cpp \
                        FixedPointFunctions/FunctionTable.cpp \
                        FixedPointFunctions/PolyCoeffTable.cpp \
                        FixedPointFunctions/PolynomialEvaluator.cpp \
                        FixedPointFunctions/FunctionEvaluator.cpp \
                        FixedPointFunctions/HOTBM.cpp \
                        FixedPointFunctions/HOTBM/HOTBMInstance.cc\
                        FixedPointFunctions/HOTBM/Param.cc\
                        FixedPointFunctions/HOTBM/Exhaustive.cc\
                        FixedPointFunctions/HOTBM/Util.cc\
                        FixedPointFunctions/HOTBM/PWPolynomial.cc\
                        FixedPointFunctions/HOTBM/Polynomial.cc\
                        FixedPointFunctions/HOTBM/MPPolynomial.cc\
                        FixedPointFunctions/HOTBM/Power.cc\
                        FixedPointFunctions/HOTBM/PowerAdHoc.cc\
                        FixedPointFunctions/HOTBM/TermROM.cc\
                        FixedPointFunctions/HOTBM/Term.cc\
                        FixedPointFunctions/HOTBM/Minimax.cc\
                        FixedPointFunctions/HOTBM/PowerROM.cc\
                        FixedPointFunctions/HOTBM/TermPowMult.cc \
                        FixSinCos.cpp \
                        CORDIC/FixedPointSinOrCos.cpp \
                        CORDIC/CordicSinCos.cpp \
                        FixedPointFIR.cpp \
                        FixedPointDCT.cpp \
                        FPAddDualPath.cpp \
                        FPAddSinglePath.cpp \
                        FPAdd3Input.cpp \
                        FPAddSub.cpp \
                        FPMult.cpp \
                        FPMultKaratsuba.cpp \
                        FPSquare.cpp \
                        FPDiv.cpp \
                        FPSqrt.cpp \
                        FPSqrtPoly.cpp \
                        squareroot/PolynomialTable.cpp \
                        squareroot/PolynomialTableCorrectRounded.cpp \
                        DotProduct.cpp \
                        ConstMult/IntConstMult.cpp \
                        ConstMult/ShiftAddOp.cpp \
                        ConstMult/ShiftAddDag.cpp \
                        ConstMult/IntIntKCM.cpp \
                        ConstMult/KCMTable.cpp \
                        ConstMult/FPConstMult.cpp \
                        ConstMult/CRFPConstMult.cpp \
                        ConstMult/FixRealKCM.cpp \
                        ConstMult/FPRealKCM.cpp \
                        IntConstDiv.cpp\
                        FPConstDiv.cpp\
                        fplogarithm/FirstInvTable.cpp \
                        fplogarithm/FirstLogTable.cpp \
                        fplogarithm/OtherLogTable.cpp \
                        FPLog.cpp \
                        FPExp.cpp \
                        FPPow.cpp \
                        FPLargeAcc.cpp \
                        LargeAccToFP.cpp \
                        FPSumOf3Squares.cpp \
                        Fix2FP.cpp \
                        InputIEEE.cpp \
                        OutputIEEE.cpp \
                        LNS/LNSAdd.cpp \
                        LNS/Cotran.cpp \
                        LNS/CotranHybrid.cpp \
                        LNS/CotranTables.cpp \
                        LNS/LNSAddSub.cpp \
                        LNS/LNSMul.cpp \
                        LNS/LNSDiv.cpp \
                        LNS/LNSSqrt.cpp \
                        LNS/AtanPow.cpp \
                        FixedPointFunctions/GenericEvaluator.cpp \
                        Complex/FixedComplexAdder.cpp \
                        Complex/FixedComplexMultiplier.cpp \
                        LNS/LogSinCos.cpp \
                        apps/Collision.cpp \
                        apps/FPFMAcc.cpp \
                        apps/FPJacobi.cpp \
                        TestCase.cpp \
                        TestBenchs/FPNumber.cpp \
                        IEEENumber.cpp \
                        UtilSollya.cc \
                        FP2Fix.cpp \
                        random/random_main.cpp \
                        random/urng/LutSrRng.cpp

libflopoco_la_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/src/FPExpressions

VHDLLexer.cpp:$(top_srcdir)/src/VHDLLexerRule.l
	${LEX} -o VHDLLexer.cpp $(top_srcdir)/src/VHDLLexerRule.l

VHDLLexer.hpp:VHDLLexer.cpp

ExpressionParser.cpp: $(top_srcdir)/src/FPExpressions/ExpressionParser.y ExpressionScanner.cpp
	${YACC} -y -d --defines=$(top_srcdir)/src/FPExpressions/ExpressionParser.h --output=ExpressionParser.cpp --name-prefix=FlopocoExpression $(top_srcdir)/src/FPExpressions/ExpressionParser.y

ExpressionScanner.cpp:$(top_srcdir)/src/FPExpressions/ExpressionScanner.l
	${LEX} --prefix=FlopocoExpression --header-file=$(top_srcdir)/src/FPExpressions/FlopocoScanner.h -o ExpressionScanner.cpp  $(top_srcdir)/src/FPExpressions/ExpressionScanner.l

$(top_srcdir)/src/FPExpressions/FlopocoScanner.h:ExpressionScanner.cpp ExpressionParser.cpp

$(top_srcdir)/src/FPExpressions/ExpressionParser.h:ExpressionParser.cpp
