//tutorials used: https://github.com/utexas-bwi/segbot_arm/tree/master/segbot_arm_tutorials/src

#include <ros/ros.h>
#include <std_msgs/String.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/TwistStamped.h>
#include <sensor_msgs/JointState.h>
#include <actionlib/client/simple_action_client.h>
#include <jaco_msgs/FingerPosition.h>
#include <jaco_msgs/ArmJointAnglesAction.h>
#include <jaco_msgs/ArmPoseAction.h>
#include <jaco_msgs/HomeArm.h>
#include <segbot_arm_manipulation/arm_utils.h>

#define JOINTS 8
//up-down motion

//describes the state of a set of torque-controlled joints - name, position, velocity, effort (http://docs.ros.org/kinetic/api/sensor_msgs/html/msg/JointState.html)
sensor_msgs::JointState joint_state;
bool heard_state;

//pose with reference coordinate frame and timestamp (http://docs.ros.org/api/geometry_msgs/html/msg/PoseStamped.html)
geometry_msgs::PoseStamped pose_stamped;
bool heard_pose_stamped;

//http://docs.ros.org/hydro/api/jaco_msgs/html/msg/FingerPosition.html - note that this robot does not have a finger 3
jaco_msgs::FingerPosition finger_pose;
//jaco_ros::jaco_arm_driver/out/finger_position finger_pose;
bool heard_finger_pose;

//global publisher for cartesian velocity
ros::Publisher velocity_pub;

//callback function for joint state
void joint_state_callback(const sensor_msgs::JointStateConstPtr &message)
{
	//if data from all the joints has been recieved, set joint_state and heard_state accordingly
	if (message->position.size() == JOINTS)
	{
		joint_state = *message;
		heard_state = true;
	}
}

//callback function for stamped position
//changed from sensor_msgs because PoseStamped is a geometry_msgs message
void pose_stamped_callback(const geometry_msgs::PoseStampedConstPtr &message)
{
	pose_stamped = *message;
        heard_pose_stamped = true;
}

//callback function for finger position
void finger_pose_callback(const jaco_msgs::FingerPositionConstPtr &message)
{
	finger_pose = *message;
        heard_finger_pose = true;
}

//get the states from the arm
void get_data()
{
	heard_state = false;
	heard_pose_stamped = false;
	heard_finger_pose = false;
	
	//40 Hz
	ros::Rate r(40.0);
	
	while (ros::ok())
	{
		ros::spinOnce();	
		
		//if the data has been acquired, exit; else, loop back and try again while able
		if (heard_state && heard_pose_stamped && heard_finger_pose)
			return;
		
		r.sleep();
	}
}

//from https://github.com/utexas-bwi/segbot_arm/blob/experiments/object_exploration/object_exploration/src/interact_arm.cpp 
//completely unchanged... should probably fix that
bool goToLocation(float position[]){
	
	actionlib::SimpleActionClient<jaco_msgs::ArmJointAnglesAction> ac("/mico_arm_driver/joint_angles/arm_joint_angles", true);
	jaco_msgs::ArmJointAnglesGoal goal;
	goal.angles.joint1 = position[0];
	goal.angles.joint2 = position[1];
	goal.angles.joint3 = position[2];
	goal.angles.joint4 = position[3];
	goal.angles.joint5 = position[4];
	goal.angles.joint6 = position[5];
	ac.waitForServer();
	ac.sendGoal(goal);
	ROS_INFO("Trajectory goal sent");
	ac.waitForResult();
}

//call functions to get data
int main(int argc, char **argv)
{
	//name the node
	ros::init(argc, argv, "behavior_1");

	ros::NodeHandle node_handle;

	//subscribe to topics
	ros::Subscriber joint_pose_subscriber = node_handle.subscribe("/joint_states", 1, joint_state_callback);
	ros::Subscriber pose_stamped_subscriber = node_handle.subscribe("/mico_arm_driver/out/tool_position", 1, pose_stamped_callback);	
	ros::Subscriber finger_pose_subscriber = node_handle.subscribe("/mico_arm_driver/out/finger_position", 1, finger_pose_callback);

	//publish the velocities
	ros::Publisher velocity_publisher = node_handle.advertise<geometry_msgs::TwistStamped>("/mico_arm_driver/in/cartesian_velocity", 10);

	//get the states from the arm
	get_data();

	geometry_msgs::PoseStamped goal_x;
	goal_x.header.frame_id = "mico_link_base";
	goal_x.pose.position.x = 0.283979;
	goal_x.pose.position.x = 0.476259;
	goal_x.pose.position.x = 0.357072;
	goal_x.pose.orientation.x = -0.0458191;
	goal_x.pose.orientation.y = 0.662054;
	goal_x.pose.orientation.z = 0.726168;
	goal_x.pose.orientation.w = 0.179623;
	
	segbot_arm_manipulation::moveToPoseMoveIt(node_handle, goal_x);

	geometry_msgs::TwistStamped message;

	//the back and forth motion

	int direction = 1;

	double startTime = ros::Time::now().toSec();

	double endTime = startTime + 30;

	while(endTime > ros::Time::now().toSec())
	{
		//move one direction (should be forward or backwards)
		message.twist.linear.x = .5;

		velocity_publisher.publish(message);
	
		//from http://wiki.ros.org/roscpp/Overview/Time
		ros::Duration(0.5).sleep(); // sleep for half a second

		//move the opposite direction
		direction *= -1;
	}

	message.twist.linear.x = 0;

	velocity_publisher.publish(message);

	//get back into position 
	segbot_arm_manipulation::moveToPoseMoveIt(node_handle, goal_x);
}
