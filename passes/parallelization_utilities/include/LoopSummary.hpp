/*
 * Copyright 2016 - 2019  Angelo Matni, Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

#include "llvm/Analysis/LoopInfo.h"

using namespace llvm;

namespace llvm {

  struct LoopSummary {
    int id;
    LoopSummary *parent;
    std::set<LoopSummary *> children;
    int depth;
    BasicBlock *header;
    std::vector<BasicBlock *> orderedBBs;
    std::set<BasicBlock *> bbs;
    std::set<BasicBlock *> latchBBs;

    LoopSummary(int id, Loop *l) {
      this->id = id;
      this->depth = l->getLoopDepth();
      this->header = l->getHeader();
      for (auto bb : l->blocks()) {
        // NOTE: Unsure if this is program forward order
        orderedBBs.push_back(bb);
        this->bbs.insert(bb);
        if (l->isLoopLatch(bb)) {
          latchBBs.insert(bb);
        }
      }

      for (auto bb : this->bbs){
        for (auto& inst : *bb){
          if (l->isLoopInvariant(&inst)){
            this->invariants.insert(&inst);
          }
        }
      }
      return ;
    }

    void print (raw_ostream &stream) {
      stream << "Loop summary: " << id << ", depth: " << depth << "\n";
      header->begin()->print(stream); stream << "\n";
    }
      
    private:
      std::set<Value *> invariants;
  };

}
