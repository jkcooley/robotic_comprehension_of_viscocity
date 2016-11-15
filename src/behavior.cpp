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
//#include <segbot_arm_manipulation/moveToPoseMoveIt.h>

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

//PoseStampedConstPtr& message or just PoseStamped &message
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

//get into the correct position to accept the spoon
void get_spoon(int joint_1_pos, int joint_2_pos, int joint_3_pos, int joint_4_pos, int joint_5_pos, int joint_6_pos, int finger_1_pos, int finger_2_pos)
{

//	while(
}


//manipulate x values
/*void back_and_forth(geometry_msgs::PoseStamped goal_x, float error)
{
	geometry_msgs::PoseStamped current_pose = pose_stamped;

	//diff defined as the difference between the position of the arm and the position determined as the starting point
//	float diff = current_pose - goal_x; 
	while(1 > error)
	{
	//using ArmPose instead of ArmJointAngles as in goToLocation
        actionlib::SimpleActionClient<jaco_msgs::ArmPoseAction> ac("/mico_arm_driver/arm_pose/arm_pose", true);
        jaco_msgs::ArmPoseGoal goal;
     
	//which joint(s) do we want to move? 
        goal.pose.position.x = goal_x.pose.position.x;
     
        ac.waitForServer();
        ac.sendGoal(goal);
        ROS_INFO("Trajectory goal sent");
        ac.waitForResult();

	//get_data();

	}
}*/

//from https://github.com/utexas-bwi/segbot_arm/blob/experiments/object_exploration/object_exploration/src/interact_arm.cpp 
//completely unchanged... should probably fix that
bool goToLocation(float position[]){
//	moveit_utils::AngularVelCtrl srv;
//	srv.request.state = js;
	actionlib::SimpleActionClient<jaco_msgs::ArmJointAnglesAction> ac("/mico_arm_driver/joint_angles/arm_joint_angles", true);
	jaco_msgs::ArmJointAnglesGoal goal;
	goal.angles.joint1 = position[0];
	goal.angles.joint2 = position[1];
	goal.angles.joint3 = position[2];
	goal.angles.joint4 = position[3];
	goal.angles.joint5 = position[4];
	goal.angles.joint6 = position[5];
	//ROS_INFO("Joint6: %f", fromFile.position[5]);
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
	goal_x.pose.position.x = 0.389813;
	goal_x.pose.position.y = -0.0222033;
	goal_x.pose.position.z = 0.22019;
	goal_x.pose.orientation.x = 0.734704;
	goal_x.pose.orientation.y = 0.734704;
	goal_x.pose.orientation.z = 0.0426369;
	goal_x.pose.orientation.w = 0.0219083;
	
//	segbot_arm_manipulation::moveToPoseMoveIt(node_handle, goal_x);

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
//	segbot_arm_manipulation::moveToPoseMoveIt(node_handle, goal_x);

	
}


//what the joint state prints out
/*
string[] name
float64[] position
float64[] velocity
float64[] effort
*/

//out/tool_position


//spoon acceptance pose
/*
fri@marvin:~/viscosity_ws/src/segbot_arm/segbot_arm_tutorials$ rosrun segbot_arm_tutorials ex1_subscribing_to_topics 
[ INFO] [1477074469.354660031]: Current joint positions:
[ INFO] [1477074469.354791623]: header: 
  seq: 600
  stamp: 1477074469.341749274
  frame_id: 
name[]
  name[0]: mico_joint_1
  name[1]: mico_joint_2
  name[2]: mico_joint_3
  name[3]: mico_joint_4
  name[4]: mico_joint_5
  name[5]: mico_joint_6
  name[6]: mico_joint_finger_1
  name[7]: mico_joint_finger_2
position[]
  position[0]: 0.662198
  position[1]: -0.765186
  position[2]: -0.281049
  position[3]: -0.650929
  position[4]: 2.023
  position[5]: 2.35381
  position[6]: -0.00084
  position[7]: -0.00084
velocity[]
  velocity[0]: 0
  velocity[1]: 0
  velocity[2]: 0
  velocity[3]: 0
  velocity[4]: 0
  velocity[5]: 0
  velocity[6]: 0
  velocity[7]: 0
effort[]
  effort[0]: 6.08548e+14
  effort[1]: 1.22894e-26
  effort[2]: -2.03711e-18
  effort[3]: 4.59149e-41
  effort[4]: -2.03711e-18
  effort[5]: 4.59149e-41
  effort[6]: 0
  effort[7]: 0

[ INFO] [1477074469.354838094]: Current joint efforts:
[ INFO] [1477074469.354904775]: header: 
  seq: 599
  stamp: 0.000000000
  frame_id: 
name[]
  name[0]: jaco_joint_1
  name[1]: jaco_joint_2
  name[2]: jaco_joint_3
  name[3]: jaco_joint_4
  name[4]: jaco_joint_5
  name[5]: jaco_joint_6
position[]
velocity[]
effort[]
  effort[0]: 0.0508463
  effort[1]: -0.329351
  effort[2]: 0.388175
  effort[3]: -0.0921688
  effort[4]: -0.175491
  effort[5]: -0.149602

[ INFO] [1477074469.354927740]: Current finger positions:
[ INFO] [1477074469.354948797]: finger1: -6
finger2: -6
finger3: 0

[ INFO] [1477074469.354970198]: Current tool pose:
[ INFO] [1477074469.355011625]: header: 
  seq: 600
  stamp: 1477074469.345129396
  frame_id: mico_link_base
pose: 
  position: 
    x: 0.283979
    y: 0.476259
    z: 0.357072
  orientation: 
    x: -0.0458191
    y: 0.662054
    z: 0.726168
    w: 0.179623
*/
