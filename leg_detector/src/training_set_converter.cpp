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
#include <ros/ros.h>

#include <rosbag/bag.h>
#include <rosbag/view.h>

#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem/operations.hpp>

#include <people_msgs/PositionMeasurement.h>
#include <people_msgs/PositionMeasurementArray.h>
#include <sensor_msgs/LaserScan.h>
#include <sensor_msgs/PointCloud.h>
#include <rosgraph_msgs/Clock.h>
#include <tf2_msgs/TFMessage.h>

#include <tf/transform_listener.h>
#include <tf/message_filter.h>
#include <message_filters/subscriber.h>

#include <algorithm>

#include <iostream>
#include <fstream>
#include <string>

#define foreach BOOST_FOREACH

#define USE_BASH_COLORS
#include <leg_detector/color_definitions.h>

//#include <leg_detector/debug.h>

using namespace std;
using namespace ros;
using namespace tf;

namespace po = boost::program_options;

int g_argc;
char** g_argv;

/**
* The TrainingSetCreator Class helps creating bagfiles with annotated and visualized labels.
*/
class TrainingSetConverter {
public:
    /** Constructor */
    TrainingSetConverter(){}

    /** Deconstructor */
    ~TrainingSetConverter() {}

    /**
    * Converts the training set (given as bagfile)
    * @param file Input bagfile. Should contain the scan-data.
    */
    void convertTrainingSet(const char* file) {
        printf("Input file: %s\n", file);

        // Check if file exists
        if (!ifstream(file)) {
            std::cout << "File does not exist!" << std::endl;
            return;
        } else {
            std::cout << "File exists!" << std::endl;
        }

        // Open the bagfile
        rosbag::Bag bag(file, rosbag::bagmode::Read);

        // Read the available topics
        rosbag::View topicView(bag);
        std::vector<const rosbag::ConnectionInfo *> connection_infos = topicView.getConnections();
        std::set<std::string> topics_strings;

        cout << "Topics: " << endl;
        BOOST_FOREACH(const rosbag::ConnectionInfo *info, connection_infos) {
          if(topics_strings.find(info->topic) == topics_strings.end()) {
             topics_strings.insert(info->topic);
           cout << "\t" << info->topic << "\t" << info->datatype << endl;
         }
        }

        // TODO Check if necessary topics are available
        // TODO Check if allready a topic label topic there

        // TODO should be later all the topics
        std::vector<std::string> topics;
        topics.push_back(std::string("/scan_front"));
        topics.push_back(std::string("/label"));
        topics.push_back(std::string("/tf"));
        std::cout << "Pushed topics" << std::endl;

        // Create the view
        rosbag::View view(bag, rosbag::TopicQuery(topics));
        //rosbag::View view(bag, rosbag::TopicQuery(topics));

        rosbag::View::iterator viewIt = view.begin();
        rosbag::MessageInstance message = (*viewIt);

        // Iterate through the message -> generate the clusters
        foreach(rosbag::MessageInstance const m, view)
        {
            // If scan is a LaserScan (to avoid stopping at every tf)
            sensor_msgs::LaserScan::Ptr s = m.instantiate<sensor_msgs::LaserScan>();
            if (s != NULL) {

            }
        }
        // Close the files
        bag.close();
    }
};

int main(int argc, char **argv) {
    ros::init(argc, argv, "training_set_converter");
    g_argc = argc;
    g_argv = argv;
    TrainingSetConverter trainingSetConverter;

    po::options_description description("Training Set Converter Usage");

    description.add_options()("help,h", "Display this help message")(
            "input-file", po::value<std::string>(), "Input file")("output-file",
            po::value<std::string>(), "Output file")("version,v",
            "Display the version number");

    po::positional_options_description p;
    p.add("input-file", 1);
    p.add("output-file", 1);

    po::variables_map vm;
    po::store(
            po::command_line_parser(argc, argv).options(description).positional(
                    p).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << description;

        return 0;
    }

    if (vm.count("version")) {
        std::cout << "Training Set Creator Version 0.1" << std::endl;

        return 0;
    }

    if (vm.count("input-file")) {
        cout << "Input files are: " << vm["input-file"].as<std::string>()
                << "\n";
    }

    if (vm.count("output-file")) {
        cout << "Input files are: " << vm["input-file"].as<std::string>()
                << "\n";
    }

    const char* filename = (vm["input-file"].as<std::string>()).c_str();

    trainingSetConverter.convertTrainingSet(filename);

    return 0;
}

