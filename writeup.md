# Write Up

##  Objectives
Quote the objectives from [Readme](./README.md), the goal is to design and implement a path planner that is able to create smooth, safe paths for the car to follow along a 3 lane highway with traffic.

A successful path planner will be able to:
1. keep inside its lane,
2. avoid hitting other cars,
3. pass slower moving traffic all by using localization, sensor fusion, and map data.

In general, it expects the car to a little over 5 minutes to complete 1 loop in a highway scenario. Also, the car should not accelerate or jerk too quickly.


## Problem Statement
Given the vehicle's map and localization data, as well as its observations of the road, including the other car's positions, lanes, and speeds, the path planner should compute a safe (obstacle-free) trajectory so that the car stays in the lane with the light traffic as much as possible. Meanwhile, consider the speed limits in the highway scenario, the car should not move too slowly or quickly.

In other words, this project is to **implement a local planner to drive the car in the "right" lane**. Here, the "right" lane means where the traffics are ligher than other lanes.

In short, the inputs of the local planner include:
- Map and localization data: global coordinates, lanes of cars
- Sensor fusion data: other cars' identities, positions, speeds

And the outputs should be:
- A lane with light traffics
- Velocity control

In the instruction video, the lectures already give their interpolation method to generate and smooth a trajectory made of waypoints given the start and target waypoints, as the figure shows below. Consider this benefit, the major challenge turns to **how to find a target lane** given the planner's inputs.

<img src="./image/carrace.png" width="320"/>

Take the scenario of the above picture as an example, give the sensor fusion data, the car knows
1. it is running in lane 1
2. there is one car ahead on the same lane (1)
3. there is another car on the left side (0)
4. there is no traffic on the right lane (1)

The local planner computes the "right" lane, which is lane 2 on the right and outputs a drivable path.
