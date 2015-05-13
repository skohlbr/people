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

#include <people_tracking_filter/advanced_tracker_particle.h>
#include <people_tracking_filter/gaussian_pos_vel.h>
//#include <people_tracking_filter/people_particle_filter.h>


using namespace MatrixWrapper;
using namespace BFL;
using namespace tf;
using namespace std;
using namespace ros;

#define DEBUG_ADVANCEDTRACKERPARTICLE 1
#define DEBUG_ADVANCEDTRACKERPARTICLE_TIME 1


namespace estimation
{
// constructor
AdvancedTrackerParticle::AdvancedTrackerParticle(const string& name, unsigned int num_particles, const StatePosVel& sysnoise):
  Tracker(name),
  prior_(num_particles),
  filter_(NULL),
  sys_model_(sysnoise), // System noise, sysnoise is the sigma(variance) of this noise
  meas_model_(tf::Vector3(0.01, 0.01, 0.01)), // Measurement model variance
  tracker_initialized_(false),
  num_particles_(num_particles)
{};



// destructor
AdvancedTrackerParticle::~AdvancedTrackerParticle()
{
  if (filter_) delete filter_;
};


/**
 * Initialize the Filter
 * @param mu The mean of the position
 * @param sigma The variance of the initialization
 * @param time The current time
 */
void AdvancedTrackerParticle::initialize(const StatePosVel& mu, const StatePosVel& sigma, const double time)
{
  ROS_DEBUG_COND(DEBUG_ADVANCEDTRACKERPARTICLE,"--AdvancedTrackerParticle::%s",__func__);

  //cout << "Initializing tracker with " << num_particles_ << " particles, with covariance " << sigma << " around " << mu << endl;

  // This is the initialization of the tracker, particles are choosen as gaussian

  // Create the Gaussian
//  std::cout << "Mu: " << mu << "Sigma: " << sigma << std::endl;
  GaussianPosVel gauss_pos_vel(mu, sigma);

  // Prepare vector to store the particles
  vector<Sample<StatePosVel> > prior_samples(num_particles_);

  // Create the particles from the Gaussian,
  gauss_pos_vel.SampleFrom(prior_samples, num_particles_, CHOLESKY, NULL); // TODO Check if the CHOLESKY can be NULL
  prior_.ListOfSamplesSet(prior_samples);

  //Output the Samples
//  std::cout << "Prior Samples:" << std::endl;
//  for(vector<Sample<StatePosVel> >::iterator sampleIt = prior_samples.begin(); sampleIt != prior_samples.end(); sampleIt++){
//    std::cout << (*sampleIt) << std::endl;
//  }

  //filter_ = new BootstrapFilter<StatePosVel, tf::Vector3>(&prior_, &prior_, 0, num_particles_ / 4.0);

  filter_ = new PeopleParticleFilter(&prior_, &prior_, 0, num_particles_ / 4.0, 0);
  // TODO input own filter here

  // tracker initialized
  tracker_initialized_ = true;
  quality_ = 1;
  filter_time_ = time;
  init_time_ = time;

  cout << "Initialization done" << endl;

}
// Perform prediction using the motion model
bool AdvancedTrackerParticle::updatePrediction(const double time, const MatrixWrapper::SymmetricMatrix& cov){
  ROS_DEBUG_COND(DEBUG_ADVANCEDTRACKERPARTICLE,"--AdvancedTrackerParticle::%s",__func__);

  ((AdvancedSysPdfPosVel*)sys_model_.SystemPdfGet())->CovarianceSet(cov);

  return this->updatePrediction(time);
}


// Perform prediction using the motion model
bool AdvancedTrackerParticle::updatePrediction(const double time)
{
  ROS_DEBUG_COND(DEBUG_ADVANCEDTRACKERPARTICLE,"--AdvancedTrackerParticle::%s",__func__);

  bool res = true;
  if (time > filter_time_)
  {
    // set dt in sys model
    sys_model_.SetDt(time - filter_time_);
    filter_time_ = time;

    // update filter
    res = filter_->Update(&sys_model_); // TODO!! // Call Update internal of the particle Filter
    if (!res) quality_ = 0;
  }
  return res;
};

/**
 * @brief Do the prediction with the influence of the estimation of the high level filter
 * @param time
 * @return
 */
bool AdvancedTrackerParticle::updatePrediction(const double time, StatePosVel highLevelPrediction)
{
  ROS_DEBUG_COND(DEBUG_ADVANCEDTRACKERPARTICLE,"--AdvancedTrackerParticle::%s",__func__);

  // Here the particles are the Particles before the Update
  vector<WeightedSample<StatePosVel> > samples = prior_.ListOfSamplesGet();

  // Prediction using
  ROS_DEBUG_COND(DEBUG_ADVANCEDTRACKERPARTICLE,"--AdvancedTrackerParticle::%s - Doing update prediction of %u particles",__func__, prior_.NumSamplesGet());


  assert(0); // Careful! (This is not finished implementing!)

  bool res = true;
  if (time > filter_time_)
  {
    // set dt in sys model
    sys_model_.SetDt(time - filter_time_);
    filter_time_ = time;

    // update filter
    res = filter_->Update(&sys_model_); // TODO!! // Call Update internal of the particle Filter
    if (!res) quality_ = 0;
  }
  return res;
};



// update filter correction based on measurement
/**
 * Update the Particle Filter using a Measurement
 * @param meas The Measurement
 * @param cov The covariance of the Measurement
 * @return true if success, false if failed
 */
bool AdvancedTrackerParticle::updateCorrection(const tf::Vector3&  meas, const MatrixWrapper::SymmetricMatrix& cov)

{
  assert(cov.columns() == 3);

  ROS_DEBUG_COND(DEBUG_ADVANCEDTRACKERPARTICLE,"--AdvancedTrackerParticle::%s",__func__);


  // set covariance

  //meas_model_.MeasurementPdfGet();
  // Set the measurement noise
  ((AdvancedMeasPdfPos*)(meas_model_.MeasurementPdfGet()))->CovarianceSet(cov);

  // update filter
  bool res = filter_->Update(&meas_model_, meas); // TODO Fit occlusion model in here

  // If update failed for some reason
  if (!res) quality_ = 0;
  return res;
};


// get evenly spaced particle cloud
void AdvancedTrackerParticle::getParticleCloud(const tf::Vector3& step, double threshold, sensor_msgs::PointCloud& cloud) const
{
  ((MCPdfPosVel*)(filter_->PostGet()))->getParticleCloud(step, threshold, cloud);
};


// Get the Filter Posterior
/**
 * Get the current Tracker estimation
 * @param est
 */
void AdvancedTrackerParticle::getEstimate(StatePosVel& est) const
{

  ROS_DEBUG_COND(DEBUG_ADVANCEDTRACKERPARTICLE,"--AdvancedTrackerParticle::%s",__func__);

  // Here the particles are the Particles before the Update
  // vector<WeightedSample<StatePosVel> > samples = prior_.ListOfSamplesGet(); // TODO Should be the posterior maybe?

  //for(vector<WeightedSample<StatePosVel> >::iterator wSampleIt = samples.begin(); wSampleIt != samples.end(); wSampleIt++){
  //  std::cout << (*wSampleIt).WeightGet() << std::endl;
  //}

  // Calculate the Estimation getting the priors
  //est = prior_.ExpectedValueGet();


  ROS_DEBUG_COND(DEBUG_ADVANCEDTRACKERPARTICLE,"--AdvancedTrackerParticle::%s - Doing update prediction of %u particles",__func__, prior_.NumSamplesGet());

  est = ((MCPdfPosVel*)(filter_->PostGet()))->ExpectedValueGet();

#ifdef DEBUG_ADVANCEDTRACKERPARTICLE
  //std::cout << "Estimation: " << est << std::endl;
#endif
};


void AdvancedTrackerParticle::getEstimate(people_msgs::PositionMeasurement& est) const
{
  StatePosVel tmp = filter_->PostGet()->ExpectedValueGet();

  est.pos.x = tmp.pos_[0];
  est.pos.y = tmp.pos_[1];
  est.pos.z = tmp.pos_[2];

  est.header.stamp.fromSec(filter_time_);
  est.object_id = getName();
}





/// Get histogram from certain area
Matrix AdvancedTrackerParticle::getHistogramPos(const tf::Vector3& min, const tf::Vector3& max, const tf::Vector3& step) const
{
  return ((MCPdfPosVel*)(filter_->PostGet()))->getHistogramPos(min, max, step);
};


Matrix AdvancedTrackerParticle::getHistogramVel(const tf::Vector3& min, const tf::Vector3& max, const tf::Vector3& step) const
{
  return ((MCPdfPosVel*)(filter_->PostGet()))->getHistogramVel(min, max, step);
};

/**
 * Get the lifetime (time since initialization) of the filter
 * @return time in seconds since initialization
 */
double AdvancedTrackerParticle::getLifetime() const
{
  if (tracker_initialized_)
    return filter_time_ - init_time_;
  else
    return 0;
}


double AdvancedTrackerParticle::getTime() const
{
  if (tracker_initialized_)
    return filter_time_;
  else
    return 0;
}
}; // namespace



