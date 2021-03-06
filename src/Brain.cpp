#include <ras_group8_brain/Brain.hpp>

// STD
#include <string>

namespace ras_group8_brain {

Brain::Brain(ros::NodeHandle& node_handle)
    : node_handle_(node_handle)
{
  run_time_=ros::Time::now();
  if (!readParameters()) {
    ROS_ERROR("Could not read parameters.");
    ros::requestShutdown();
  }
  
  //subscriber_ = node_handle_.subscribe(subscriber_topic_, 1, &Brain::topicCallback, this);

  //Services
  get_path_client_=node_handle_.serviceClient<nav_msgs::GetPlan>(get_path_service_);
  move_arm_up_client_=node_handle_.serviceClient<ras_group8_arm_controller::MoveArm>(move_arm_up_service_);
  move_arm_down_client_=node_handle_.serviceClient<ras_group8_arm_controller::MoveArm>(move_arm_down_service_);

  //Publisher
  speaker_publisher_=node_handle_.advertise<std_msgs::String>(speaker_topic_,1,true);
  send_path_publisher_=node_handle_.advertise<nav_msgs::Path>(send_path_topic_,1,true);
  path_stop_publisher_=node_handle_.advertise<std_msgs::Bool>(path_stop_topic_,1,true);
  marker_publisher_ = node_handle_.advertise<visualization_msgs::Marker>("visualisation_marker", 1,true);
  add_object_publisher_= node_handle_.advertise<visualization_msgs::MarkerArray>(add_object_map_topic_, 1,true);

  //Subscriber
  path_done_subscriber_=node_handle_.subscribe(path_done_topic_,1,&Brain::pathDoneCallback, this);
  robot_position_subscriber_=node_handle_.subscribe(robot_position_topic_,1,&Brain::robotPositionCallback, this);
  vision_msg_subscriber_=node_handle_.subscribe(vision_msg_topic_,1,&Brain::visionMessageCallback, this);
  set_goal_subscriber_=node_handle_.subscribe("/move_base_simple/goal",1,&Brain::goalMessageCallback, this);
  new_map_subscriber_=node_handle_.subscribe(new_map_topic_,1,&Brain::newMapCallback, this);


  state_=0;

  ROS_INFO("Successfully launched node.");

  possible_obstacle_[0].name="no obstacle";   possible_obstacle_[0].value=0;
  possible_obstacle_[1].name="Red Cube";   possible_obstacle_[1].value=value_group1_;
  possible_obstacle_[2].name="Red Hollow Cube";   possible_obstacle_[2].value=value_group1_;
  possible_obstacle_[3].name="Red Ball";   possible_obstacle_[3].value=value_group1_;
  possible_obstacle_[4].name="Red Hollow Cylinder";   possible_obstacle_[4].value=value_group2_;
  possible_obstacle_[5].name="Green Cube";   possible_obstacle_[5].value=value_group2_;
  possible_obstacle_[6].name="Green Hollow Cube";   possible_obstacle_[6].value=value_group2_;
  possible_obstacle_[7].name="Green Hollow Cylinder";   possible_obstacle_[7].value=value_group3_;
  possible_obstacle_[8].name="Yellow Cube";   possible_obstacle_[8].value=value_group3_;
  possible_obstacle_[9].name="Yellow Ball";   possible_obstacle_[9].value=value_group3_;
  possible_obstacle_[10].name="Blue Cube";   possible_obstacle_[10].value=value_group4_;
  possible_obstacle_[11].name="Blue Hollow Triagnle";   possible_obstacle_[11].value=value_group4_;
  possible_obstacle_[12].name="Purple Cross";   possible_obstacle_[12].value=value_group4_;
  possible_obstacle_[13].name="Purple Star";   possible_obstacle_[13].value=value_group4_;
  possible_obstacle_[14].name="Orange Cross";   possible_obstacle_[14].value=value_group4_;
  possible_obstacle_[15].name="Orange Star";   possible_obstacle_[15].value=value_group4_;
  possible_obstacle_[16].name="SolideObstacle";   possible_obstacle_[16].value=0;
   possible_obstacle_[17].name="Trab";   possible_obstacle_[17].value=0;

  edges_[0].point.x=maze_size_x_-0.15; edges_[0].point.y=0.15; edges_[0].explored=false;
  edges_[1].point.x=maze_size_x_-0.15; edges_[1].point.y=maze_size_y_-0.15; edges_[1].explored=false;
  edges_[2].point.x=0.15; edges_[2].point.y=maze_size_y_-0.15; edges_[2].explored=false;
}

Brain::~Brain()
{
}

bool Brain::readParameters()
{
  if (!node_handle_.getParam("get_path_service", get_path_service_))
    return false;
  if (!node_handle_.getParam("move_arm_up_service", move_arm_up_service_))
    return false;
  if (!node_handle_.getParam("move_arm_down_service", move_arm_down_service_))
    return false;
  if (!node_handle_.getParam("speaker_topic", speaker_topic_))
    return false;
  if (!node_handle_.getParam("send_path_topic", send_path_topic_))
    return false;
  if (!node_handle_.getParam("path_stop_topic", path_stop_topic_))
    return false;
  if (!node_handle_.getParam("path_done_topic", path_done_topic_))
    return false;
  if (!node_handle_.getParam("vision_msg_topic", vision_msg_topic_))
    return false;
  if (!node_handle_.getParam("robot_position_topic", robot_position_topic_))
    return false;
  if (!node_handle_.getParam("new_map_topic", new_map_topic_))
    return false;
  if (!node_handle_.getParam("add_object_map_topic", add_object_map_topic_))
    return false;
  if (!node_handle_.getParam("arm_up/pickup_attempt", pickup_attempt_))
    return false;
  if (!node_handle_.getParam("arm_up/pickup_range", pickup_range_))
    return false;
  if (!node_handle_.getParam("obstacle/position_accurancy", obstacle_position_accurancy_))
    return false;
  if (!node_handle_.getParam("obstacle/detection_accurancy", obstacle_detection_accurancy_))
    return false;
  if (!node_handle_.getParam("obstacle/obstacle_try", obstacle_try_))
    return false;
  if (!node_handle_.getParam("obstacle/value_group1", value_group1_))
    return false;
  if (!node_handle_.getParam("obstacle/value_group2", value_group2_))
    return false;
  if (!node_handle_.getParam("obstacle/value_group3", value_group3_))
    return false;
  if (!node_handle_.getParam("obstacle/value_group4", value_group4_))
    return false;
  if (!node_handle_.getParam("home/obstacle_x", home_obstacle_pos_.x))
    return false;
  if (!node_handle_.getParam("home/obstacle_y", home_obstacle_pos_.y))
    return false;
  if (!node_handle_.getParam("home/obstacle_z", home_obstacle_pos_.z))
    return false;
  if (!node_handle_.getParam("home/position_x", robot_home_position_.position.x))
    return false;
  if (!node_handle_.getParam("home/position_y", robot_home_position_.position.y))
    return false;
  if (!node_handle_.getParam("home/orientation_z", robot_home_position_.orientation.z))
    return false;
  if (!node_handle_.getParam("home/orientation_w", robot_home_position_.orientation.w))
    return false;
  if (!node_handle_.getParam("home/home_accurancy", home_accurancy_))
    return false;
  if (!node_handle_.getParam("init/first_round", round1_))
    return false;
  if (!node_handle_.getParam("init/obstacle_file", obstacle_file_))
    return false;
  if (!node_handle_.getParam("init/round_time", round_time_))
    return false;
  if (!node_handle_.getParam("init/maze_size_x", maze_size_x_))
    return false;
  if (!node_handle_.getParam("init/maze_size_y", maze_size_y_))
    return false;

  return true;
}

void Brain::stateMachine()
{
  ros::spinOnce();
  ROS_INFO("State: %d, NumObstacles %lu, Home %d, Obstcle:%d, GoHOme:%d", state_, ObstacleList_.size(),home_,obstacle_,go_home_);
  ROS_INFO("PlanElement: %i, PickUu_ELE: %i, pathdone: %d Round1:%d", planned_element_,picked_up_element_, path_done_,round1_);
  ros::Duration act_time=ros::Time::now()-run_time_;
  ROS_INFO("Time: %f",act_time.toSec());

  switch (state_)
  {
    case 0: Brain::initState();
            break;
    case 1: Brain::explorState();
            break;
    case 2: Brain::findPathState();
            break;
    case 3: Brain::pathExectuionState();
            break;
    case 4: Brain::obstacleState();
            break;
    case 5: Brain::homeState();
            break;

  }
}



} /* namespace */
