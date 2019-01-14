#include <fstream>
#include <uWS/uWS.h>
#include <chrono>
#include <thread>
#include "Eigen-3.3/Eigen/Core"
#include "Eigen-3.3/Eigen/QR"
#include "json.hpp"
#include <set>
#include "util.h"

using namespace util;
// for convenience
using json = nlohmann::json;

class Car {
public:
  int id;
  double x;
  double y;
  double vx;
  double vy;
  double s;
  double d;

  Car(){};
  Car(int i, double x, double y, double vx, double vy, double s, double d){
    this->id = i;
    this->x = x;
    this->y = y;
    this->vx = vx;
    this->vy = vy;
    this->s = s;
    this->d = d;
  };
  bool operator< (const Car & other) const {
    return id < other.id;
  }
};

int main() {
  uWS::Hub h;

  // Load up map values for waypoint's x,y,s and d normalized normal vectors
  vector<double> map_waypoints_x;
  vector<double> map_waypoints_y;
  vector<double> map_waypoints_s;
  vector<double> map_waypoints_dx;
  vector<double> map_waypoints_dy;

  // Waypoint map to read from
  string map_file_ = "../data/highway_map.csv";
  // The max s value before wrapping around the track back to 0
  double max_s = 6945.554;

  ifstream in_map_(map_file_.c_str(), ifstream::in);

  string line;
  while (getline(in_map_, line)) {
  	istringstream iss(line);
  	double x;
  	double y;
  	float s;
  	float d_x;
  	float d_y;
  	iss >> x;
  	iss >> y;
  	iss >> s;
  	iss >> d_x;
  	iss >> d_y;
  	map_waypoints_x.push_back(x);
  	map_waypoints_y.push_back(y);
  	map_waypoints_s.push_back(s);
  	map_waypoints_dx.push_back(d_x);
  	map_waypoints_dy.push_back(d_y);
  }
  int lane = 1; // start with lane 1
  double ref_vel = 0.0; // mph
  vector<vector<Car>> blocking_cars(3, vector<Car>{});

  h.onMessage([&ref_vel, &map_waypoints_x,&map_waypoints_y,&map_waypoints_s,&map_waypoints_dx,&map_waypoints_dy,&lane](uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length,
                     uWS::OpCode opCode) {
    // "42" at the start of the message means there's a websocket message event.
    // The 4 signifies a websocket message
    // The 2 signifies a websocket event
    //auto sdata = string(data).substr(0, length);
    //cout << sdata << endl;
    if (length && length > 2 && data[0] == '4' && data[1] == '2') {

      auto s = hasData(data);

      if (s != "") {
        auto j = json::parse(s);

        string event = j[0].get<string>();

        if (event == "telemetry") {
          // j[1] is the data JSON object

        	// Main car's localization Data
          	double car_x = j[1]["x"];
          	double car_y = j[1]["y"];
          	double car_s = j[1]["s"];
          	double car_d = j[1]["d"];
          	double car_yaw = j[1]["yaw"];
          	double car_speed = j[1]["speed"];

          	// Previous path data given to the Planner
          	auto previous_path_x = j[1]["previous_path_x"];
          	auto previous_path_y = j[1]["previous_path_y"];
          	// Previous path's end s and d values
          	double end_path_s = j[1]["end_path_s"];
          	double end_path_d = j[1]["end_path_d"];

          	// Sensor Fusion Data, a list of all other cars on the same side of the road.
          	auto sensor_fusion = j[1]["sensor_fusion"];

          	json msgJson;

          	// TODO: define a path made up of (x,y) points that the car will visit sequentially every .02 seconds
            // build upon Aaron's solution in the video tutorial
            int prev_size = previous_path_x.size();
            if(prev_size > 0){
              car_s = end_path_s;
            }
            bool too_close = false;
            // find ref_v to use
            // The data format for each car is: [ id, x, y, vx, vy, s, d]. The id is a unique identifier for that car. The x, y values are in global map coordinates
            //vector<vector<Car>> blocking_cars(3, vector<Car>{});
            bool keep_lane = true, go_left = false, go_right = false;
            for(int i=0; i<sensor_fusion.size(); i++){
              int car_id = sensor_fusion[i][0];
              double x = sensor_fusion[i][1];
              double y = sensor_fusion[i][2];
              double vx = sensor_fusion[i][3];
              double vy = sensor_fusion[i][4];
              double s = sensor_fusion[i][5];
              double d = sensor_fusion[i][6];
              Car car(car_id, x, y, vx, vy, s, d);
              double check_speed = sqrt(vx*vx+vy*vy);
              double check_car_s = s;
              // estimate car s position given previous path
              check_car_s += ((double)prev_size* 0.02 * check_speed);
              double delta_s = check_car_s - car_s;
              int car_lane = getLane(d);
              bool is_blocker = (fabs(check_car_s-car_s) < 30) && (check_car_s > car_s);
              if (is_blocker){
                too_close = (car_lane == lane);
                blocking_cars[car_lane].push_back(car);
              }
            }
            cout<<"|-----------|"<<endl;
            // mid lane scenario
            if (lane==1){
              // check blockers
              if(blocking_cars[lane].size() ==0){
                keep_lane = true;
                cout<<"|   |( )|   |"<<endl;
              }else{
                if(blocking_cars[lane-1].size()>0 && blocking_cars[lane+1].size()>0){
                  keep_lane = true;
                  cout<<"| ^ |   | ^ |"<<endl;
                }else{
                  keep_lane = false;
                  if(blocking_cars[lane-1].size()==0){
                    cout<<"|( )|   | ^ |"<<endl;
                    go_left = true;
                    go_right = false;
                    lane = 0;
                  }else if (blocking_cars[lane+1].size()==0){
                    cout<<"| ^ |   |( )|"<<endl;
                    go_right = true;
                    go_left = false;
                    lane = 2;
                  }
                }
              }
              cout<<"|   |   |   |"<<endl;
              cout<<"|   | * |   |"<<endl;
            }
            // left lane scenario
            else if (lane ==0){
              if(blocking_cars[lane+1].size()>0){
                keep_lane = true;
                cout<<"|( )| ^ |   |"<<endl;
              }else{
                if(blocking_cars[lane].size()==0){
                  keep_lane = true;
                  cout<<"|( )|   |   |"<<endl;
                }else{
                  cout<<"|   |( )| ^ |"<<endl;
                  keep_lane = false;
                  go_right = true;
                  go_left = false;
                  lane = 1;
                }
              }
              cout<<"|   |   |   |"<<endl;
              cout<<"| * |   |   |"<<endl;
            }
            // right lane scenario
            else if(lane ==2){
              if(blocking_cars[lane-1].size()>0){
                keep_lane = true;
                cout<<"|   | ^ |( )|"<<endl;
              }else{
                if(blocking_cars[lane].size()==0){
                  keep_lane = true;
                  cout<<"|   |   |( )|"<<endl;
                }else{
                  keep_lane = false;
                  go_left = true;
                  go_right = false;
                  lane = 1;
                  cout<<"|   |( )| ^ |"<<endl;
                }
              }
              cout<<"|   |   |   |"<<endl;
              cout<<"|   |   | * |"<<endl;
            }
            cout<<"|-----------|"<<endl;
            if(keep_lane){
              if (too_close){
                ref_vel -= 0.224*2;
              }else if(ref_vel < 49.5){
                if(blocking_cars[lane].size()>0){
                  ref_vel -= 0.224*2;
                }else{
                  ref_vel += 0.224;
                }
              }
            }

            for(int i=0; i<3; i++) {
              blocking_cars[i].clear();
            }
            /* generate path by interpolation and smooth trajectory with spline */
          	vector<double> next_x_vals;
          	vector<double> next_y_vals;

            vector<vector<double>> interpolated_wps;
            // in Frenet add evenly 30m spaced points ahead of the starting reference
            for(int i=1; i<4; i++){
              vector<double> next_wps = getXY(car_s+30*i, (2+4*lane), map_waypoints_s, map_waypoints_x, map_waypoints_y);
              interpolated_wps.push_back(next_wps);
            }
            constructPath(car_x, car_y, car_yaw, ref_vel, previous_path_x, previous_path_y, interpolated_wps, next_x_vals, next_y_vals);

            msgJson["next_x"] = next_x_vals;
          	msgJson["next_y"] = next_y_vals;

          	auto msg = "42[\"control\","+ msgJson.dump()+"]";

          	//this_thread::sleep_for(chrono::milliseconds(1000));
          	ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);

        }
      } else {
        // Manual driving
        std::string msg = "42[\"manual\",{}]";
        ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
      }
    }
  });

  // We don't need this since we're not using HTTP but if it's removed the
  // program
  // doesn't compile :-(
  h.onHttpRequest([](uWS::HttpResponse *res, uWS::HttpRequest req, char *data,
                     size_t, size_t) {
    const std::string s = "<h1>Hello world!</h1>";
    if (req.getUrl().valueLength == 1) {
      res->end(s.data(), s.length());
    } else {
      // i guess this should be done more gracefully?
      res->end(nullptr, 0);
    }
  });

  h.onConnection([&h](uWS::WebSocket<uWS::SERVER> ws, uWS::HttpRequest req) {
    std::cout << "Connected!!!" << std::endl;
  });

  h.onDisconnection([&h](uWS::WebSocket<uWS::SERVER> ws, int code,
                         char *message, size_t length) {
    ws.close();
    std::cout << "Disconnected" << std::endl;
  });

  int port = 4567;
  if (h.listen(port)) {
    std::cout << "Listening to port " << port << std::endl;
  } else {
    std::cerr << "Failed to listen to port" << std::endl;
    return -1;
  }
  h.run();
}
