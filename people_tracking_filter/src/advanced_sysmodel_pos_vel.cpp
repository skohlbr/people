/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

/* Author: Wim Meeussen */

#include <ros/console.h>

#include "people_tracking_filter/advanced_sysmodel_pos_vel.h"
#include <dual_people_leg_tracker/benchmarking/timer.h>

using namespace std;
using namespace BFL;
using namespace tf;

static const unsigned int NUM_SYS_POS_VEL_COND_ARGS = 1;
static const unsigned int DIM_SYS_POS_VEL           = 6;

#define DEBUG_ADVANCEDSYSPDFPOSVEL 1

// Constructor
AdvancedSysPdfPosVel::AdvancedSysPdfPosVel(const StatePosVel& sigma)
  : ConditionalPdf<StatePosVel, StatePosVel>(DIM_SYS_POS_VEL, NUM_SYS_POS_VEL_COND_ARGS),
    noise_(StatePosVel(Vector3(0, 0, 0), Vector3(0, 0, 0)), sigma),
    dt_(0.0)
{}



// Destructor
AdvancedSysPdfPosVel::~AdvancedSysPdfPosVel()
{}

void
AdvancedSysPdfPosVel::CovarianceSet(const MatrixWrapper::SymmetricMatrix& cov)
{

  StatePosVel sigma;
  tf::Vector3 cov_vec_pos(sqrt(cov(1, 1)), sqrt(cov(2, 2)), sqrt(cov(3, 3)));
  tf::Vector3 cov_vec_vel(sqrt(cov(4, 4)), sqrt(cov(5, 5)), sqrt(cov(6, 6)));

  sigma.pos_ = cov_vec_pos;
  sigma.vel_ = cov_vec_vel;

  noise_.sigmaSet(sigma);

}

void
AdvancedSysPdfPosVel::MultivariateCovarianceSet(const MatrixWrapper::SymmetricMatrix& cov)
{
  ROS_DEBUG_COND(DEBUG_ADVANCEDSYSPDFPOSVEL,"AdvancedSysPdfPosVel::%s",__func__);
  noise_nl_.sigmaSet(cov);

}

Probability
AdvancedSysPdfPosVel::ProbabilityGet(const StatePosVel& state) const
{
  cerr << "SysPdfPosVel::ProbabilityGet Method not applicable" << endl;
  assert(0);
  return 0;
}

/**
 * Given a sample calculate the next state
 * @param one_sample  The given sample
 * @param method Not used ?
 * @param args Not used
 * @return
 */
bool
AdvancedSysPdfPosVel::SampleFrom(Sample<StatePosVel>& one_sample, int method, void *args) const
{
  //ROS_DEBUG_COND(DEBUG_ADVANCEDSYSPDFPOSVEL,"--------AdvancedSysPdfPosVel::%s",__func__);

  StatePosVel& res = one_sample.ValueGet();

  // get conditional argument: state
  res = this->ConditionalArgumentGet(0);

  // apply system model
  res.pos_ += (res.vel_ * dt_);

  // add noise (Gaussian)
  Sample<StatePosVel> noise_sample;
  noise_.SetDt(dt_);
  noise_.SampleFrom(noise_sample, method, args);
  res += noise_sample.ValueGet();

  // add noise (Based on nonlinear models)
  //benchmarking::Timer timer;
  //timer.start();

  Sample<StatePosVel> noise_sample_nl;
  noise_nl_.SetDt(dt_);
  noise_nl_.SampleFrom(noise_sample_nl, method, args);

  //std::cout << "Sampling took " << timer.stopAndGetTimeMs() << "ms" << std::endl;

  //assert(false);
  std::cout << noise_sample_nl.ValueGet() << std::endl;
  res += noise_sample_nl.ValueGet();

  return true;
}


StatePosVel
AdvancedSysPdfPosVel::ExpectedValueGet() const
{
  cerr << "SysPdfPosVel::ExpectedValueGet Method not applicable" << endl;
  assert(0);
  return StatePosVel();

}

SymmetricMatrix
AdvancedSysPdfPosVel::CovarianceGet() const
{
  cerr << "SysPdfPosVel::CovarianceGet Method not applicable" << endl;
  SymmetricMatrix Covar(DIM_SYS_POS_VEL);
  assert(0);
  return Covar;
}

