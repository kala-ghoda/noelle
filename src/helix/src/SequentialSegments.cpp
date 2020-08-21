/*
 * Copyright 2016 - 2019  Angelo Matni, Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "HELIX.hpp"
#include "HELIXTask.hpp"

using namespace llvm ;

std::vector<SequentialSegment *> HELIX::identifySequentialSegments (
  LoopDependenceInfo *originalLDI,
  LoopDependenceInfo *LDI
){

  auto helixTask = static_cast<HELIXTask *>(this->tasks[0]);

  /*
   * Map from old to new SCCs (for use in determining what SCC can be left out of sequential segments)
   */
  std::unordered_map<SCC *, SCC*> taskToOriginalFunctionSCCMap;
  auto originalSCCDAG = originalLDI->sccdagAttrs.getSCCDAG();
  auto taskSCCDAG = LDI->sccdagAttrs.getSCCDAG();
  DGPrinter::writeGraph<SCCDAG, SCC>("sccdag-original-" + std::to_string(LDI->getID()) + ".dot", originalSCCDAG);
  DGPrinter::writeGraph<SCCDAG, SCC>("sccdag-task-" + std::to_string(LDI->getID()) + ".dot", taskSCCDAG);
  for (auto originalNode : originalSCCDAG->getNodes()) {

    /*
     * Get clones of original instructions for the given SCC
     * HACK: Since we spill loop carried PHIs, we must gloss over mappings to the initial loaded value
     * of that loop carried PHI
     */
    auto originalSCC = originalNode->getT();
    Instruction *anyClonedInstInLoop = nullptr;
    auto clonedLoop = LDI->getLoopStructure();
    for (auto nodePair : originalSCC->internalNodePairs()) {
      auto originalInst = cast<Instruction>(nodePair.first);
      auto clonedInst = helixTask->getCloneOfOriginalInstruction(originalInst);
      if (!clonedLoop->isIncluded(clonedInst)) continue;
      anyClonedInstInLoop = clonedInst;
      break;
    }
    assert(anyClonedInstInLoop != nullptr);

    SCC *singleMappingSCC = nullptr;
    for (auto taskNode : taskSCCDAG->getNodes()) {
      auto taskSCC = taskNode->getT();
      bool hasOverlappingInstruction = taskSCC->isInternal(anyClonedInstInLoop);
      if (!hasOverlappingInstruction) continue;

      assert(singleMappingSCC == nullptr);
      singleMappingSCC = taskSCC;
    }

    // if (singleMappingSCC == nullptr) {
    //   originalSCC->print(errs() << "Original SCC:\n");
    //   anyClonedInstInLoop->print(errs() << "Any cloned INST: "); errs() << "\n";
    // }

    assert(singleMappingSCC != nullptr);
    taskToOriginalFunctionSCCMap.insert(std::make_pair(singleMappingSCC, originalSCC));
  }

  std::vector<SequentialSegment *> sss;

  /*
   * Prepare the initial partition.
   */
  ParallelizationTechniqueForLoopsWithLoopCarriedDataDependences::partitionSCCDAG(LDI);

  /*
   * Compute reachability analysis
   */
  auto reachabilityDFR = this->computeReachabilityFromInstructions(LDI);

  /*
   * Identify the loop's preamble, and whether the original loop was IV governed
   */
  auto loopSCCDAG = LDI->sccdagAttrs.getSCCDAG();
  auto preambleSCCNodes = loopSCCDAG->getTopLevelNodes();
  assert(preambleSCCNodes.size() == 1 && "The loop internal SCCDAG should only have one preamble");
  auto preambleSCC = (*preambleSCCNodes.begin())->getT();
  bool wasOriginalLoopIVGoverned = originalLDI->getLoopGoverningIVAttribution() != nullptr;

  /*
   * Fetch the subsets.
   */
  auto sets = this->partitioner->getDepthOrderedSets();

  /*
   * Fetch the set of SCCs that have loop-carried data dependences.
   */
  auto depsSCCs = LDI->sccdagAttrs.getSCCsWithLoopCarriedDataDependencies();

  /*
   * Allocate the sequential segments, one per partition.
   */
  int32_t ssID = 0;
  for (auto set : sets){

    /*
     * Check if the current set of SCCs require a sequential segments.
     */
    auto requireSS = false;
    for (auto scc : set->sccs){

      /*
       * Fetch the SCC metadata.
       */
      auto originalSCC = taskToOriginalFunctionSCCMap.at(scc);
      auto originalSCCInfo = originalLDI->sccdagAttrs.getSCCAttrs(originalSCC);
      originalSCCInfo = LDI->sccdagAttrs.getSCCAttrs(scc);

      /*
       * Do not synchronize induction variables
       */
      if (originalSCCInfo->isInductionVariableSCC()) {
        continue;
      }

      /*
       * If the SCC is due to a control dependence, but the number of iterations can be computed just before executing the loop, then we can skip it.
       */
      if (  true
            && wasOriginalLoopIVGoverned
            && (depsSCCs.find(scc) == depsSCCs.end())
         ){
        continue ;
      }

      /*
       * Fetch the type of the SCC.
       */
      auto sccType = originalSCCInfo->getType();

      /*
       * Only sequential SCC can generate a sequential segment.
       * FIXME: A reducible SCC should not be sequential in nature
       */
      if (sccType == SCCAttrs::SEQUENTIAL) {
        requireSS = true;
        break ;
      }
    }
    if (!requireSS){
      continue ;
    }

    /*
     * Allocate a sequential segment.
     */
    auto ss = new SequentialSegment(LDI, reachabilityDFR, set, ssID, this->verbose);

    /*
     * Insert the new sequential segment to the list.
     */
    ssID++;
    sss.push_back(ss);
  }

  delete reachabilityDFR;

  return sss;
}
