/*
 * people_tracker.h
 *
 *  Created on: Apr 16, 2015
 *      Author: frm-ag
 */

#ifndef PEOPLE_LEG_DETECTOR_INCLUDE_LEG_DETECTOR_PEOPLE_TRACKER_H_
#define PEOPLE_LEG_DETECTOR_INCLUDE_LEG_DETECTOR_PEOPLE_TRACKER_H_

// ROS includes
#include <ros/ros.h>

// System includes
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/array.hpp>
#include <leg_detector/leg_feature.h>

#define DEBUG_PEOPLE_TRACKER 1

#define DEBUG_PEOPLETRACKERLIST 1

class LegFeature; //Forward declaration
typedef boost::shared_ptr<LegFeature> LegFeaturePtr;

class PeopleTracker; //Forward declaration
typedef boost::shared_ptr<PeopleTracker> PeopleTrackerPtr;

/**
 * High level People Tracker consisting of two low level leg tracks
 */
class PeopleTracker{
  public:
    // State Variables
    BFL::StatePosVel pos_vel_estimation_; /**< The currently estimated pos_vel_ of this people */

    tf::Vector3 hip_vec_; /**< Vector orthogonal to the velocity vector, representing the hip direction pos_vel_estimation.vel_.dot(hip_vec_) = 0 should hold every time*/

    tf::Vector3 hipPos0_, hipPos1_; /**< Vector of the endpoints of vector */
    tf::Vector3 hipPosLeft_, hipPosRight_; /**< Vector of the endpoints of vector */

    boost::array<int, 2> id_;

    //std::id_[2];

  private:
    bool is_static_;

    std::vector<LegFeaturePtr> legs_; /**< the legs, should be maximum 2! */

    LegFeaturePtr leftLeg_; /**< The left leg */
    LegFeaturePtr rightLeg_;/**< The right leg */

    bool addLeg(LegFeaturePtr leg);/**< Add a leg two this tracker */

    double total_probability_;/**< Overall probability in this tracker */
    double dist_probability_;/**< Probability of this tracker based on the distance of the legs */
    double leg_association_probability_; /**< The probability that the associated legs belong to this tracker */
    double leg_time_probability_;/**< Probability considering the lifetime of both leg trackers */

    ros::Time creation_time_;/**< Time that this tracker was created */

  public:
    PeopleTracker(LegFeaturePtr, LegFeaturePtr, ros::Time);/**< Construct a People tracker based on this two legs */

    ~PeopleTracker();

    bool isTheSame(LegFeaturePtr, LegFeaturePtr); /**< Check if this People Tracker is the same as one that would be constructed of the two given legs */

    bool isTheSame(PeopleTrackerPtr);

    LegFeaturePtr getLeg0() const;/**< Get Leg0 */

    LegFeaturePtr getLeg1() const;/**< Get Leg1 */

    LegFeaturePtr getLeftLeg() const; /**< Get left leg */

    LegFeaturePtr getRightLeg() const; /**< Get right leg */

    bool isValid() const;/**< Check if the people tracker is still valid */

    /**
     * Update everything of this tracker
     */
    void update(ros::Time);

    /**
     * Update the state of the Tracker
     * @param The current Time
     */
    void updateTrackerState(ros::Time);

    /**
     * Do a propagation of the two leg motion model
     * @param time (Time of the scan)
     */
    void propagate(ros::Time time);

    /**
     * Update the probabilities of this tracker
     */
    void updateProbabilities(ros::Time);

    /**
     * Get the Total Probability
     * @return  The total probability
     */
    double getTotalProbability() const{
      return this->total_probability_;
    }

    /**
     * Get the estimation of the leg with id
     * @param Id of the leg
     * @return The estimation
     */
    BFL::StatePosVel getLegEstimate(int id);

    BFL::StatePosVel getEstimate(){
      return pos_vel_estimation_;
    }

    tf::Vector3 getHipVec(){
      return this->hip_vec_;
    }

    //BFL::StatePosVel getLegEstimate(int id){
    //  return pos_vel_estimation_;
    //}

    bool isStatic(){
      return is_static_;
    }

    bool isDynamic(){
      return !is_static_;
    }

    void removeLegs(){
      legs_.clear();
    }

    /// output stream
    friend std::ostream& operator<< (std::ostream& os, const PeopleTracker& s)
    {

      os << "PeopleTracker: " << s.id_[0] << " - " << s.id_[1];

      if(s.getTotalProbability() > 0.5) // TODO make this dependend on some reliability limit
        os << BOLDMAGENTA;
      os  << " p_t: " << s.getTotalProbability();

      if(s.isValid())
        os << BOLDGREEN << " [valid]" << RESET;
      else
        os << BOLDRED << " [invalid]" << RESET;

      return os;

    };

};

bool isValidPeopleTracker(const PeopleTrackerPtr & o);

/**
 * List of all people trackers, checks that there are only unique assignments
 */
class PeopleTrackerList{
  private:
    boost::shared_ptr<std::vector<PeopleTrackerPtr> > list_; /**< the legs, should be maximum 2! */

  public:
    PeopleTrackerList();

    /**
     * Add a Tracker to the list
     * @param The Tracker to be added
     * @return  True on success
     */
    bool addPeopleTracker(PeopleTrackerPtr);

    /**
     * Check if there is already a tracker associated to these two legs
     * @param legA The one leg
     * @param legB The other leg
     * @return True if exists, false otherwise
     */
    bool exists(LegFeaturePtr legA, LegFeaturePtr legB);

    /**
     * Check if a tracker with the same two legs is already in the list
     * @param The tracker
     * @return True if the Tracker already exists
     */
    bool exists(PeopleTrackerPtr);

    /**
     * Get the list of People Trackers
     * @return Pointer to the list of people trackers
     */
    boost::shared_ptr<std::vector<PeopleTrackerPtr> > getList(){
      return list_;
    }

    /**
     * Remove Invalid trackers
     * @return Number of removed Trackers
     */
    int removeInvalidTrackers();

    /**
     * std::cout a list if the current trackers
     */
    void printTrackerList();

    /**
     * Call update on all trackers
     */
    void updateAllTrackers(ros::Time);

    /**
     * Update the probabilities of all trackers
     */
    void updateProbabilities(ros::Time);




};



#endif /* PEOPLE_LEG_DETECTOR_INCLUDE_LEG_DETECTOR_PEOPLE_TRACKER_H_ */
