/*
 * particle_filter.cpp
 *
 * Created on: Dec 12, 2016
 * Author: Tiffany Huang
 */

#include "particle_filter.h"

#include <math.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "helper_functions.h"

using std::string;
using std::vector;
using std::normal_distribution;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
  /**
   * TODO: Set the number of particles. Initialize all particles to 
   *   first position (based on estimates of x, y, theta and their uncertainties
   *   from GPS) and all weights to 1. 
   * TODO: Add random Gaussian noise to each particle.
   * NOTE: Consult particle_filter.h for more information about this method 
   *   (and others in this file).
   */
  num_particles = 10;  // TODO: Set the number of particles
  std::default_random_engine gen;
  for (int i = 0; i < num_particles; i++) {
    Particle temp_particle;
    temp_particle.id = i;

    normal_distribution<double> dist_x(x, std[0]);
    temp_particle.x = dist_x(gen);

    normal_distribution<double> dist_y(y, std[1]);
    temp_particle.y = dist_y(gen);

    normal_distribution<double> dist_theta(theta, std[2]);
    temp_particle.theta = dist_theta(gen);

    temp_particle.weight = 1;
    particles.push_back(temp_particle);
  }
  is_initialized = 1;
}

void ParticleFilter::prediction(double delta_t, double std_pos[], 
                                double velocity, double yaw_rate) {
  /**
   * TODO: Add measurements to each particle and add random Gaussian noise.
   * NOTE: When adding noise you may find std::normal_distribution 
   *   and std::default_random_engine useful.
   *  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
   *  http://www.cplusplus.com/reference/random/default_random_engine/
   */
  std::default_random_engine gen;
  for (int i = 0; i < particles.size(); i++) {
    if (fabs(yaw_rate) < 1e-5) yaw_rate = 1e-5;
    double x_0 = particles[i].x;
    double y_0 = particles[i].y;
    double theta_0 = particles[i].theta;

    double theta_f = theta_0 + yaw_rate*delta_t;
    double x_f = x_0 + (velocity/yaw_rate) * (sin(theta_f) - sin(theta_0));
    double y_f = y_0 + (velocity/yaw_rate) * (cos(theta_0) - cos(theta_f));

    normal_distribution<double> dist_x(x_f, std_pos[0]);
    particles[i].x = dist_x(gen);

    normal_distribution<double> dist_y(y_f, std_pos[1]);
    particles[i].y = dist_y(gen);

    normal_distribution<double> dist_theta(theta_f, std_pos[2]);
    particles[i].theta = dist_theta(gen);
  }
}

void ParticleFilter::dataAssociation(vector<LandmarkObs> predicted, 
                                     vector<LandmarkObs>& observations) {
  /**
   * TODO: Find the predicted measurement that is closest to each 
   *   observed measurement and assign the observed measurement to this 
   *   particular landmark.
   * NOTE: this method will NOT be called by the grading code. But you will 
   *   probably find it useful to implement this method and use it as a helper 
   *   during the updateWeights phase.
   */
  vector<LandmarkObs> new_observations;
  for (int i = 0; i < predicted.size(); i++) {
    double nearest_distance = 999999999;
    int idx_nearest = 0;
    int id = 0;
    for (int j = 0; j < observations.size(); j++) {
      double distance = dist(predicted[i].x, predicted[i].y, observations[j].x, observations[j].y);
      if (distance < nearest_distance) {
        nearest_distance = distance;
        idx_nearest = j;
        id = observations[j].id;
      }
    }
    LandmarkObs closest_pt;
    closest_pt.id = id;
    closest_pt.x = observations[idx_nearest].x;
    closest_pt.y = observations[idx_nearest].y;

    new_observations.push_back(closest_pt);
  }
  observations = new_observations;
}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[], 
                                   const vector<LandmarkObs> &observations, 
                                   const Map &map_landmarks) {
  /**
   * TODO: Update the weights of each particle using a mult-variate Gaussian 
   *   distribution. You can read more about this distribution here: 
   *   https://en.wikipedia.org/wiki/Multivariate_normal_distribution
   * NOTE: The observations are given in the VEHICLE'S coordinate system. 
   *   Your particles are located according to the MAP'S coordinate system. 
   *   You will need to transform between the two systems. Keep in mind that
   *   this transformation requires both rotation AND translation (but no scaling).
   *   The following is a good resource for the theory:
   *   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
   *   and the following is a good resource for the actual equation to implement
   *   (look at equation 3.33) http://planning.cs.uiuc.edu/node99.html
   */
  double gauss_norm = 1 / (2 * M_PI  * std_landmark[0] * std_landmark[1]);
  for (int i = 0; i < particles.size(); i++) {
    // Identify landmarks in range of the particle based on sensor_range
    vector<LandmarkObs> inrange_landmarks;
    for (int j = 0; j < map_landmarks.landmark_list.size(); j++) {
      double range_pt2land = dist(map_landmarks.landmark_list[j].x_f, map_landmarks.landmark_list[j].y_f,
                             particles[i].x,  particles[i].y);
      if (range_pt2land < sensor_range) {
        LandmarkObs temp;
        temp.x = map_landmarks.landmark_list[j].x_f;
        temp.y = map_landmarks.landmark_list[j].y_f;
        inrange_landmarks.push_back(temp);
      }
    } 

    // Convert the observation coordinates (relative coordinate) to map coordinate (global)
    vector<LandmarkObs> observations_map_coord;

    for (int j = 0; j < observations.size(); j++) {
      double x_part = particles[i].x;
      double y_part = particles[i].y;
      double x_obs = observations[j].x;
      double y_obs = observations[j].y;
      double theta = particles[i].theta;

      double x_map = x_part + (cos(theta) * x_obs) - (sin(theta) * y_obs);
      double y_map = y_part + (sin(theta) * x_obs) + (cos(theta) * y_obs);

      LandmarkObs temp;
      temp.id = observations[j].id;
      temp.x = x_map;
      temp.y = y_map;
      observations_map_coord.push_back(temp);
    }
    dataAssociation(inrange_landmarks, observations_map_coord);

    double weight = 1;
    for (int j = 0; j < observations_map_coord.size(); j++) {
      double exponent = (pow(observations_map_coord[j].x - inrange_landmarks[j].x,2) /
                        (2 * pow(std_landmark[0],2))) +
                        (pow(observations_map_coord[j].y - inrange_landmarks[j].y,2) /
                        (2 * pow(std_landmark[1],2)));
      weight *= gauss_norm * exp(-exponent);
    }
    particles[i].weight = weight;
  }
}

void ParticleFilter::resample() {
  /**
   * TODO: Resample particles with replacement with probability proportional 
   *   to their weight. 
   * NOTE: You may find std::discrete_distribution helpful here.
   *   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
   */
  vector<Particle> temp_particles;
  vector<double> weights;

  // Creating discrete_distrubution with weight vector
  for (int i = 0; i < particles.size(); i++) {
    weights.push_back(particles[i].weight);
  }
  std::discrete_distribution<int> weight_distribution(weights.begin(), weights.end());

  std::default_random_engine gen;
  for (int i = 0; i < particles.size(); i++) {
    int idx = weight_distribution(gen);
    temp_particles.push_back(particles.at(idx));
  }
  particles = temp_particles;
}

void ParticleFilter::SetAssociations(Particle& particle, 
                                     const vector<int>& associations, 
                                     const vector<double>& sense_x, 
                                     const vector<double>& sense_y) {
  // particle: the particle to which assign each listed association, 
  //   and association's (x,y) world coordinates mapping
  // associations: The landmark id that goes along with each listed association
  // sense_x: the associations x mapping already converted to world coordinates
  // sense_y: the associations y mapping already converted to world coordinates
  particle.associations= associations;
  particle.sense_x = sense_x;
  particle.sense_y = sense_y;
}

string ParticleFilter::getAssociations(Particle best) {
  vector<int> v = best.associations;
  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<int>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}

string ParticleFilter::getSenseCoord(Particle best, string coord) {
  vector<double> v;

  if (coord == "X") {
    v = best.sense_x;
  } else {
    v = best.sense_y;
  }

  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<float>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}
