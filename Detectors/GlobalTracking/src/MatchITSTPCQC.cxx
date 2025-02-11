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

#include "GlobalTracking/MatchITSTPCQC.h"
#include "ReconstructionDataFormats/TrackTPCITS.h"
#include "DataFormatsTPC/TrackTPC.h"
#include "Framework/InputSpec.h"
#include "ReconstructionDataFormats/TrackParametrization.h"
#include "DetectorsBase/Propagator.h"
#include "DetectorsBase/GeometryManager.h"

using namespace o2::globaltracking;

MatchITSTPCQC::~MatchITSTPCQC()
{

  deleteHistograms();
}

//_______________________________________________________

void MatchITSTPCQC::deleteHistograms()
{

  delete mPtTPC;
  delete mFractionITSTPCmatch;
  delete mPt;
  delete mEta;
  delete mChi2Matching;
  delete mChi2Refit;
  delete mTimeResVsPt;
}

//__________________________________________________________

void MatchITSTPCQC::reset()
{
  mPtTPC->Reset();
  mFractionITSTPCmatch->Reset();
  mPt->Reset();
  mEta->Reset();
  mChi2Matching->Reset();
  mChi2Refit->Reset();
  mTimeResVsPt->Reset();
}

//__________________________________________________________

bool MatchITSTPCQC::init()
{

  mPtTPC = new TH1F("mPtTPC", "Pt distribution of TPC tracks; Pt; dNdPt", 100, 0.f, 20.f);
  mFractionITSTPCmatch = new TH1F("mFractionITSTPCmatch", "Fraction of ITSTPC matched tracks vs Pt; Pt; Eff", 100, 0.f, 20.f);
  mPt = new TH1F("mPt", "Pt distribution of matched tracks; Pt; dNdPt", 100, 0.f, 20.f);
  mEta = new TH1F("mEta", "Eta distribution of matched tracks; Eta; dNdEta", 100, -1.2f, 1.2f);
  mChi2Matching = new TH1F("mChi2Matching", "Chi2 of matching; chi2", 200, 0, 20);
  mChi2Refit = new TH1F("mChi2Refit", "Chi2 of refit; chi2", 200, 0, 20);
  mTimeResVsPt = new TH2F("mTimeResVsPt", "Time resolution vs Pt; Pt; time res", 100, 0.f, 20.f, 100, 0.f, 2.f);

  mSrc &= mAllowedSources;

  if ((mSrc[GID::Source::ITSTPC] == 0 || mSrc[GID::Source::TPC] == 0)) {
    LOG(fatal) << "We cannot do ITSTPC QC, some sources are missing, check sources in " << mSrc;
  }

  mDataRequest = std::make_shared<o2::globaltracking::DataRequest>();
  mDataRequest->requestTracks(mSrc, mUseMC);

  o2::base::GeometryManager::loadGeometry(mGeomFileName);
  o2::base::Propagator::initFieldFromGRP(mGRPFileName);
  mBz = o2::base::Propagator::Instance()->getNominalBz();

  return true;
}

//__________________________________________________________

void MatchITSTPCQC::run(o2::framework::ProcessingContext& ctx)
{

  mRecoCont.collectData(ctx, *mDataRequest.get());
  mTPCTracks = mRecoCont.getTPCTracks();
  mITSTPCTracks = mRecoCont.getTPCITSTracks();

  mNTPCTracksInv = 1. / mTPCTracks.size();
  mNITSTPCTracksInv = 1. / mITSTPCTracks.size();

  LOG(INFO) << "****** Number of found TPC    tracks = " << mTPCTracks.size();
  LOG(INFO) << "****** Number of found ITSTPC tracks = " << mITSTPCTracks.size();

  for (auto const& trk : mTPCTracks) {
    if (selectTrack(trk)) {
      mPtTPC->Fill(trk.getPt());
    }
  }

  for (auto const& trk : mITSTPCTracks) {
    // implement selections...
    // then fill histograms
    auto const& trkTpc = mTPCTracks[trk.getRefTPC()];
    if (selectTrack(trkTpc)) {
      mPt->Fill(trkTpc.getPt());
      mEta->Fill(trkTpc.getEta());
      mChi2Matching->Fill(trk.getChi2Match());
      mChi2Refit->Fill(trk.getChi2Refit());
      mTimeResVsPt->Fill(trkTpc.getPt(), trk.getTimeMUS().getTimeStampError());
      //LOG(INFO) << "*** chi2Matching = " << trk.getChi2Match() << ", chi2refit = " << trk.getChi2Refit() << ", timeResolution = " << trk.getTimeMUS().getTimeStampError();
    }
  }
}

//__________________________________________________________

bool MatchITSTPCQC::selectTrack(o2::tpc::TrackTPC const& track)
{

  if (track.getPt() < mPtCut) {
    return false;
  }
  if (std::abs(track.getEta()) > mEtaCut) {
    return false;
  }
  if (track.getNClusters() < mNTPCClustersCut) {
    return false;
  }

  math_utils::Point3D<float> v{};
  std::array<float, 2> dca;
  if (!(const_cast<o2::tpc::TrackTPC&>(track).propagateParamToDCA(v, mBz, &dca, mDCACut)) || std::abs(dca[0]) > mDCACutY) {
    return false;
  }

  return true;
}

//__________________________________________________________

void MatchITSTPCQC::finalize()
{

  mFractionITSTPCmatch->Divide(mPt, mPtTPC, 1, 1, "b");
  mPtTPC->Scale(mNTPCTracksInv);
  mPt->Scale(mNITSTPCTracksInv);
  mEta->Scale(mNITSTPCTracksInv);
  mChi2Matching->Scale(mNITSTPCTracksInv);
  mChi2Refit->Scale(mNITSTPCTracksInv);
  mTimeResVsPt->Scale(mNITSTPCTracksInv);
}

//__________________________________________________________

void MatchITSTPCQC::getTH1FHistos(TObjArray& objarTH1F)
{

  objarTH1F.Add(mPtTPC);
  objarTH1F.Add(mFractionITSTPCmatch);
  objarTH1F.Add(mPt);
  objarTH1F.Add(mEta);
  objarTH1F.Add(mChi2Matching);
  objarTH1F.Add(mChi2Refit);
}

//__________________________________________________________

void MatchITSTPCQC::getTH2FHistos(TObjArray& objarTH2F)
{

  objarTH2F.Add(mTimeResVsPt);
}
