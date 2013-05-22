/*
 * The MIT License (MIT)
 * Copyright (c) 2012 William Woodall <wjwwood@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <string>

#include <ros/ros.h>

#ifdef WIN32
 #ifdef DELETE
 // ach, windows.h polluting everything again,
 // clashes with autogenerated visualization_msgs/Marker.h
 #undef DELETE
 #endif
#endif
#include "nav_msgs/Odometry.h"
#include "sensor_msgs/NavSatFix.h"

#include "novatel/novatel.h"
using namespace novatel;

// Logging system message handlers
void handleInfoMessages(const std::string &msg) {ROS_INFO("%s",msg.c_str());}
void handleWarningMessages(const std::string &msg) {ROS_WARN("%s",msg.c_str());}
void handleErrorMessages(const std::string &msg) {ROS_ERROR("%s",msg.c_str());}
void handleDebugMessages(const std::string &msg) {ROS_DEBUG("%s",msg.c_str());}


// ROS Node class
class NovatelNode {
public:
  NovatelNode() : nh_("~"){


    // set up logging handlers
    gps_.setLogInfoCallback(handleInfoMessages);
    gps_.setLogWarningCallback(handleWarningMessages);
    gps_.setLogErrorCallback(handleErrorMessages);
    gps_.setLogDebugCallback(handleDebugMessages);

    gps_.set_best_utm_position_callback(boost::bind(&NovatelNode::BestUtmHandler, this, _1, _2));
    gps_.set_best_velocity_callback(boost::bind(&NovatelNode::BestVelocityHandler, this, _1, _2));
    gps_.set_ins_position_velocity_attitude_short_callback(boost::bind(&NovatelNode::InsPvaHandler, this, _1, _2));
    gps_.set_ins_covariance_short_callback(boost::bind(&NovatelNode::InsCovHandler, this, _1, _2));
    gps_.set_raw_imu_short_callback(boost::bind(&NovatelNode::RawImuHandler, this, _1, _2));
    gps_.set_receiver_hardware_status_callback(boost::bind(&NovatelNode::HardwareStatusHandler, this, _1, _2));

  }

  ~NovatelNode() {
    this->disconnect();
  }

  void BestUtmHandler(UtmPosition &pos, double &timestamp) {
    ROS_INFO("Received BestUtm");

    sensor_msgs::NavSatFix sat_fix;
    sat_fix.header.stamp = ros::Time::now();
    sat_fix.header.frame_id = "/utm";

    if (pos.position_type == NONE)
      sat_fix.status.status = sensor_msgs::NavSatStatus::STATUS_NO_FIX;
    else if ((pos.position_type == WAAS) || 
             (pos.position_type == OMNISTAR) ||   
             (pos.position_type == OMNISTAR_HP) || 
             (pos.position_type == OMNISTAR_XP) || 
             (pos.position_type == CDGPS))
      sat_fix.status.status = sensor_msgs::NavSatStatus::STATUS_SBAS_FIX;
    else if ((pos.position_type == PSRDIFF) || 
             (pos.position_type == NARROW_FLOAT) ||   
             (pos.position_type == WIDE_INT) ||     
             (pos.position_type == WIDE_INT) ||     
             (pos.position_type == NARROW_INT) ||     
             (pos.position_type == RTK_DIRECT_INS) ||     
             (pos.position_type == INS_PSRDIFF) ||    
             (pos.position_type == INS_RTKFLOAT) ||   
             (pos.position_type == INS_RTKFIXED))
      sat_fix.status.status = sensor_msgs::NavSatStatus::STATUS_GBAS_FIX;
     else 
      sat_fix.status.status = sensor_msgs::NavSatStatus::STATUS_FIX;

    if (pos.signals_used_mask & 0x30)
      sat_fix.status.service = sensor_msgs::NavSatStatus::SERVICE_GLONASS;
    else
      sat_fix.status.service = sensor_msgs::NavSatStatus::SERVICE_GPS;

    // TODO: convert positon to lat, long, alt to export

    // TODO: add covariance

    sat_fix.position_covariance_type = sensor_msgs::NavSatFix::COVARIANCE_TYPE_DIAGONAL_KNOWN;

    nav_sat_fix_publisher_.publish(sat_fix);


    cur_odom_ = new nav_msgs::Odometry();
    cur_odom_->header.stamp = sat_fix.header.stamp;
    cur_odom_->header.frame_id = "/utm";
    cur_odom_->pose.pose.position.x = pos.easting;
    cur_odom_->pose.pose.position.y = pos.northing;
    cur_odom_->pose.pose.position.z = pos.height;
    //cur_odom_->pose.covariance[0] = 


    odom_publisher_.publish(*cur_odom_);

  }

  void BestVelocityHandler(Velocity&, double&) {
    ROS_INFO("Received BestVel");

  }

  void InsPvaHandler(InsPositionVelocityAttitudeShort &ins_pva, double &timestamp) {

  }

  void RawImuHandler(RawImuShort &imu, double &timestamp) {

  }

  void InsCovHandler(InsCovarianceShort &cov, double &timestamp) {

  }

  void HardwareStatusHandler(ReceiverHardwareStatus &status, double &timestamp) {

  }

  //void HandleEmData(EM61MK2Data &data) {
    // auxos_messages::EmDataStamped msg;
    // msg.header.stamp = ros::Time(data.timestamp);
    // msg.header.frame_id = "/em";

    // msg.em_name=em_name_;
    // msg.em_data.mode=data.mode;
    // msg.em_data.channel_1=data.chan1;
    // msg.em_data.channel_2=data.chan2;
    // msg.em_data.channel_3=data.chan3;
    // msg.em_data.channel_4=data.chan4;
    // msg.em_data.current=data.current;
    // msg.em_data.voltage=data.voltage;

    // em_publisher_.publish(msg);

  //}

  void run() {

    if (!this->getParameters())
      return;

    this->odom_publisher_ = nh_.advertise<nav_msgs::Odometry>(odom_topic_,0);
    this->nav_sat_fix_publisher_ = nh_.advertise<sensor_msgs::NavSatFix>(nav_sat_fix_topic_,0);

    //em_.setDataCallback(boost::bind(&EM61Node::HandleEmData, this, _1));
    gps_.Connect(port_,baudrate_);

    // configure default log sets
    if (gps_default_logs_period_>0) {
      // request default set of gps logs at given rate
      // convert rate to string
      std::stringstream default_logs;
      default_logs.precision(2);
      default_logs << "BESTUTMB ONTIME " << std::fixed << gps_default_logs_period_ << ";";
      default_logs << "BESTVELB ONTIME " << std::fixed << gps_default_logs_period_;
      gps_.ConfigureLogs(default_logs.str());
    }

    if (span_default_logs_period_>0) {
      // request default set of gps logs at given rate
      // convert rate to string
      std::stringstream default_logs;
      default_logs.precision(2);
      default_logs << "INSPVAB ONTIME " << std::fixed << gps_default_logs_period_ << ";";
      default_logs << "INSCOVB ONTIME " << std::fixed << gps_default_logs_period_;
      gps_.ConfigureLogs(default_logs.str());
    }

    // configure additional logs
    gps_.ConfigureLogs(log_commands_);

    ros::spin();

  } // function

protected:

  void disconnect() {
    //em_.stopReading();
    //em_.disconnect();
  }

  bool getParameters() {
    // Get the serial ports

    nh_.param("odom_topic", odom_topic_, std::string("/gps_odom"));
    ROS_INFO_STREAM("Odom Topic: " << odom_topic_);

    nh_.param("nav_sat_fix_topic", nav_sat_fix_topic_, std::string("/gps_fix"));
    ROS_INFO_STREAM("NavSatFix Topic: " << nav_sat_fix_topic_);

    nh_.param("port", port_, std::string("/dev/ttyUSB0"));
    ROS_INFO_STREAM("Port: " << port_);

    nh_.param("baudrate", baudrate_, 9600);
    ROS_INFO_STREAM("Baudrate: " << baudrate_);

    nh_.param("log_commands", log_commands_, std::string("BESTUTMB ONTIME 1.0"));
    ROS_INFO_STREAM("Log Commands: " << log_commands_);

    nh_.param("gps_default_logs_period", gps_default_logs_period_, 0.05);
    ROS_INFO_STREAM("Default GPS logs period: " << gps_default_logs_period_);

    nh_.param("span_default_logs_period", gps_default_logs_period_, 0.05);
    ROS_INFO_STREAM("Default GPS logs period: " << gps_default_logs_period_);


    return true;
  }

  ////////////////////////////////////////////////////////////////
  // ROSNODE Members
  ////////////////////////////////////////////////////////////////
  ros::NodeHandle nh_;
  ros::Publisher odom_publisher_;
  ros::Publisher nav_sat_fix_publisher_;

  Novatel gps_;
  std::string odom_topic_;
  std::string nav_sat_fix_topic_;
  std::string port_;
  std::string log_commands_;
  double gps_default_logs_period_;
  double span_default_logs_period_;
  int baudrate_;
  double poll_rate_;

  nav_msgs::Odometry *cur_odom_;

};

int main(int argc, char **argv) {
  ros::init(argc, argv, "novatel_node");
  
  NovatelNode node;
  
  node.run();
  
  return 0;
}
