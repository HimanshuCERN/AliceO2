// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// @file   CookedTrackerSpec.h

#ifndef O2_ITS_COOKEDTRACKERDPL
#define O2_ITS_COOKEDTRACKERDPL

#include "Framework/DataProcessorSpec.h"
#include "ITSReconstruction/CookedTracker.h"
#include "DataFormatsParameters/GRPObject.h"
#include "DataFormatsITSMFT/TopologyDictionary.h"
#include "Framework/Task.h"
#include "TStopwatch.h"

using namespace o2::framework;

namespace o2
{
namespace its
{

class CookedTrackerDPL : public Task
{
 public:
  CookedTrackerDPL(bool useMC, const std::string& trMode);
  ~CookedTrackerDPL() override = default;
  void init(InitContext& ic) final;
  void run(ProcessingContext& pc) final;
  void endOfStream(framework::EndOfStreamContext& ec) final;

 private:
  int mState = 0;
  bool mUseMC = true;
  bool mRunVertexer = true;
  o2::itsmft::TopologyDictionary mDict;
  std::unique_ptr<o2::parameters::GRPObject> mGRP = nullptr;
  o2::its::CookedTracker mTracker;
  TStopwatch mTimer;
};

/// create a processor spec
/// run ITS CookedMatrix tracker
framework::DataProcessorSpec getCookedTrackerSpec(bool useMC, const std::string& trMode);

} // namespace its
} // namespace o2

#endif /* O2_ITS_COOKEDTRACKERDPL */
