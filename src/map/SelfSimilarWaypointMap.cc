//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "SelfSimilarWaypointMap.h"
Define_Module(SelfSimilarWaypointMap);

//The input is the map name and the clustering radius
void SelfSimilarWaypointMap::initialize(int stage) {
  if (stage == 0) {
    simulation_canvas = getSystemModule()->getCanvas();
    hurst_parameter = par("hurstParameter").doubleValue();
    clustering_radius = par("clusteringRadius");
    map_name = par("mapName").stringValue();
    observation_area = par("observationArea");
    bool success = true;
    area_vector = new std::vector<Area>;
    weight_vector = new std::vector<unsigned>;
    bool clfExists = loadAreaVector(); //clf means cluster list file
    if (!clfExists) {
      std::cout << "Self-similar map: Load selfsimilar waypoint map from " 
        << map_name << std::endl;
      std::list<inet::Coord> waypointList;
      success = loadMap(waypointList);
      if (success) {
        computeConfinedAreas(waypointList);
        computeAreaWeights();
        saveAreaVector();
        drawMap();
        drawConvexHull();
      }
      else
        error("SelfSimilarWaypointMap: load map fails\n");
    }
    else{
      drawMap();
      drawConvexHull();
    }
  }
}

SelfSimilarWaypointMap::~SelfSimilarWaypointMap() {
  std::cout << "Selfsimilar waypoint map\n";
  if(area_vector) {
    delete area_vector;
    std::cout << "SelfSimilarWaypointMap: Delete area vector\n";
  }
  if(weight_vector){
    delete weight_vector;
    std::cout << "SelfSimilarWaypointMap: Delete weight vector\n";
  }
}

bool SelfSimilarWaypointMap::loadMap(WaypointList& waypointList) {
  std::ifstream waypointFile(map_name, std::ifstream::in);
  bool success = false;
  if (waypointFile.is_open()) {
    inet::Coord waypoint;
    while (waypointFile >> waypoint.x >> waypoint.y)
      waypointList.push_back(waypoint);
    success = testWaypointList(waypointList);
    waypointFile.close();
    if (!success)
      error("SelfSimilarWaypointMap: %s has repeated coordinates\n", map_name);
  }
  else 
    error("SelfSimilarWaypointMap: %s file is not found\n", map_name);
  return success;
}

bool SelfSimilarWaypointMap::testWaypointList(WaypointList wpl) {
  unsigned original_size = wpl.size();
  wpl.sort(
      //Lexicographical order
      [](const inet::Coord& lhs, const inet::Coord& rhs) {
        return lhs.x == rhs.x ? lhs.y < rhs.y : lhs.x < rhs.x;
      }
    );
    wpl.unique();
    return original_size == wpl.size();
}

void SelfSimilarWaypointMap::computeConfinedAreas(WaypointList& waypointList) {
std::cout << "Clustering waypoints, it may take some minutes..." 
          << std::endl;
  unsigned numOfWaypoints = 0, areaID = 0;
  Area confinedArea; //confined area
  while(!waypointList.empty()) {
    area_id_map[waypointList.front()] = areaID;
    confinedArea.push_back(waypointList.front());
    waypointList.pop_front();
    numOfWaypoints++;
    for (int i = 0; i < confinedArea.size(); i++){
      auto it = waypointList.begin();
      while(it != waypointList.end()) {
        if (confinedArea[i].distance(*it) <= clustering_radius) {
          area_id_map[*it] = areaID;
          confinedArea.push_back(*it);
          waypointList.erase(it++);
          numOfWaypoints++;
        }
        else
          it++;
      }
    }
    std::cout << confinedArea.size() << " waypoints have been clustered\n";
    area_vector->push_back(confinedArea);
    confinedArea.clear();
    areaID++;
  }
  std::cout << area_id_map.size() << "waypoints match an areaID\n";
  std::cout << numOfWaypoints << " waypoints have been clustered in "
    << area_vector->size() << " confined areas" << std::endl;
}

bool SelfSimilarWaypointMap::saveAreaVector() {
  bool result = false;
  std::string filename = map_name;
  filename += "_" + std::to_string(int(clustering_radius)) + ".clf";
  std::ofstream ofs(filename.c_str(), std::ofstream::out);
  if(ofs.is_open()) {
    for(auto& area : *area_vector) {
      for(inet::Coord& waypoint : area)
        ofs << waypoint.x << ' ' << waypoint.y << ' ';
      ofs << '\n';
    }
    ofs.close();
    result = true;
  }
  return result;
}

bool SelfSimilarWaypointMap::loadAreaVector() {
  bool result = false;
  std::string filename;
  filename = map_name;
  filename += "_" + std::to_string(int(clustering_radius)) + ".clf";
  std::ifstream ifs(filename.c_str(), std::ifstream::in);
  unsigned areaID = 0;
  if(ifs.is_open()) {
    uint16_t waypoint_counter = 0;
    inet::Coord waypoint;
    std::vector<inet::Coord> waypointCluster;
    std::string line;
    std::stringstream ss;
    while(std::getline(ifs, line)) { //Gets a line while they exist
      ss << line;
      while(ss >> waypoint.x >> waypoint.y) {
        waypoint_counter++;
        waypointCluster.push_back(waypoint);
        area_id_map[waypoint] = areaID;
      }
      area_vector->push_back(waypointCluster);
      waypointCluster.clear();
      line.clear();
      ss.clear();
      areaID++;
    }
    waypoint_number = waypoint_counter;
    computeAreaWeights();
    result = true;
    std::cout << area_vector->size()
      << " confined areas have been read from " << filename << std::endl;
  }
  return result;
}

void SelfSimilarWaypointMap::computeAreaWeights() {
  double weight = 0.0;
  weight_vector->push_back(weight);
  // for(auto& area : *areaVector) {
  //   weight += (double)area.size() / numberOfWaypoints;
  //   weightVector->push_back(weight);
  // }
  for (unsigned i = 0; i < area_vector->size(); i++)
    for (unsigned j = 0; j < area_vector->at(i).size(); j++)
      weight_vector->push_back(i);
}

const Area* SelfSimilarWaypointMap::getConfinedArea(unsigned index) {
  return static_cast<const Area*>(&area_vector->at(index));
}

const std::vector<unsigned>* SelfSimilarWaypointMap::getAreaWeights() {
  return static_cast<const std::vector<unsigned>*>(weight_vector);
}

// const std::vector<unsigned>* SelfSimilarWaypointMap::getIntAreaWeights() {
//   return static_cast<const std::vector<unsigned>*>(weightIntVector);
// }

int SelfSimilarWaypointMap::getAreaSize(unsigned index) {
  int size = -1;
  if (area_vector && (index < area_vector->size()))
    size = unsigned(area_vector->at(index).size());
  return size;
}

int SelfSimilarWaypointMap::getNumberOfAreas() {
  int size = -1;
  if (area_vector)
    size = int(area_vector->size());
  return size;
}

inet::Coord
SelfSimilarWaypointMap::getWaypoint(unsigned indexArea, unsigned index) {
  inet::Coord waypoint(-1.0, -1.0);
  if (indexArea < area_vector->size())
    if (index < area_vector->at(indexArea).size())
      waypoint = area_vector->at(indexArea).at(index);
  return waypoint;
}

void SelfSimilarWaypointMap::randomizeArea(
  omnetpp::cRNG* rng, unsigned area_index
) {
    Area* area = &(area_vector->at(area_index));
    for(size_t i = area->size() - 1; i > 0; --i) {
      unsigned rnd = omnetpp::intuniform(rng, 0, i);
      std::swap(area->at(i),area->at(rnd));
  }
}

unsigned SelfSimilarWaypointMap::getAreaID(inet::Coord& c) {
  return area_id_map[c];
}

bool SelfSimilarWaypointMap::isSameArea(inet::Coord& c1, inet::Coord& c2) {
  return area_id_map[c1] == area_id_map[c2];
}

void SelfSimilarWaypointMap::drawMap() {
  std::cout << "Drawing self-similar map\n";
  for (auto& area : *area_vector) {
    for (auto& coordinate : area) {
      omnetpp::cRectangleFigure* point = new omnetpp::cRectangleFigure;
      point->setBounds(omnetpp::cFigure::Rectangle(10, 10, 10, 10));
      point->setFilled(true);
      point->setFillColor(omnetpp::cFigure::BLUE);
      point->setPosition(omnetpp::cFigure::Point(coordinate.x, coordinate.y), omnetpp::cFigure::ANCHOR_CENTER);
      point->setZIndex(-1.0);
      simulation_canvas->addFigure(point);
    }
  }
}

void SelfSimilarWaypointMap::drawConvexHull() {
  std::vector<point_2> observered_area;
  omnetpp::cPolygonFigure *cv = new omnetpp::cPolygonFigure("convexhull");
  std::vector<omnetpp::cFigure::Point> cv_point;
  if (observation_area == -1) {
    size_t size_max = 0;
    for (int i = 0; i < area_vector->size(); i++)
      if (area_vector->at(i).size() > size_max) {
        observation_area = i;
        size_max = area_vector->at(i).size();
      }
  }
  for (auto& point : area_vector->at(observation_area)) 
    observered_area.push_back(point_2(point.x, point.y));
  CGAL::ch_eddy(observered_area.begin(), observered_area.end(), std::back_inserter(convexhull));
  std::cout << "Convex hull:" << '\n';
  for (auto& point : convexhull) {
    cv_point.push_back(omnetpp::cFigure::Point(point.x(), point.y()));
    std::cout << point.x() << ' ' << point.y() << '\n';
  }
  cv->setPoints(cv_point);
  cv->setLineColor(omnetpp::cFigure::BLUE);
  cv->setLineWidth(1);
  simulation_canvas->addFigure(cv);
}

// DEPRECATED
// void SelfSimilarWaypointMap::computeConfinedAreas(std::list<inet::Coord>& 
// waypointList) {
//   std::cout << "Computing confined areas... it may take some minutes" 
//             << std::endl;
//   unsigned numOfWaypoints = 0;
//   Area confinedArea; //confined area
//   while(!waypointList.empty()) {
//     inet::Coord popularWaypoint = getPopularWaypoint(waypointList);
//     auto it = waypointList.begin();
//     while(it != waypointList.end()) {
//       if(popularWaypoint.distance(*it) <= clusteringRadius) {
//         confinedArea.push_back(*it);
//         numOfWaypoints++;
//         waypointList.erase(it++);
//       }
//       else it++;
//     }
//     areaVector->push_back(confinedArea);
//     std::cout << confinedArea.size() << " waypoints have been clustered\n";
//     confinedArea.clear();
//   }
//   std::cout << numOfWaypoints << " waypoints have been clustered in "
//     << areaVector->size() << " confined areas" << std::endl;
// }
// inet::Coord SelfSimilarWaypointMap::
//   getPopularWaypoint(std::list<inet::Coord>& waypointList) {
//   inet::Coord popularWaypoint(0.0, 0.0);
//   uint16_t maximum = 0;
//   for(inet::Coord& w : waypointList) {
//     uint16_t surroundingWaypoints = 0;
//     for(inet::Coord& v : waypointList)
//       if((w.distance(v) < clusteringRadius) && (w != v))
//         surroundingWaypoints++;
//     if(surroundingWaypoints >= maximum) {
//       popularWaypoint = w;
//       maximum = surroundingWaypoints;
//     }
//   }
//   return popularWaypoint;
// }