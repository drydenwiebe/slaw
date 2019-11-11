#if !defined(CONNECTIVITY_OBSERVER)
#define CONNECTIVITY_OBSERVER

#include <algorithm>
#include <vector>
#include <list>
#include <fstream>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_2_algorithms.h>
#include <omnetpp.h>

#include "PositionObserver.h"
#include "../map/SelfSimilarWaypointMap.h"

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef K::Point_2 point_2;

class ConnectivityObserver : public PositionObserver {
protected:
  /** @brief A typedef to summarize an entry in the adjacency matrix*/
  typedef std::pair<int, omnetpp::simtime_t> neighbor;
  /** @brief File where the adjacency matrix will be stored */
  const char* filename;
  /** @brief The minimum link lifetime to considered a link was established */
  omnetpp::simtime_t llt_min;
  /** @brief The total number of observation to be captured */
  unsigned sample_size;
  /** @brief The current number of observations */
  unsigned observation_counter;
protected:
  /** @brief Data structures storing link lifetimes */
  std::vector< std::vector<omnetpp::simtime_t> >* adjacency_matrix;
  /** @brief Data structure storing N(x) with pairs <id, time> */
  std::vector< std::list<neighbor> >* neighborhood_list;
  /** @brief Convex hull of the observation area */
  const std::vector<point_2>* polygon;
protected:
  /** @brief Computes the one-hop neighborhood of a node x being at observation area */
  std::list<neighbor> computeOneHopNeighborhood(int);
  /** @brief Computes whether a node x is at observtion area */
  bool isInObservationArea(inet::Coord&);
public:
  /** @brief default constructor */
  ConnectivityObserver();
  /** @brief default destructor  */
  ~ConnectivityObserver();
  /** @brief Receives the quadrant of a module and updates its one-hop 
   * neighborhood */
  virtual void receiveSignal(omnetpp::cComponent*, omnetpp::simsignal_t,  
    omnetpp::cObject*, omnetpp::cObject*) override;
  /** @brief Initializes the attributes of this class */
  virtual void initialize(int stage) override;
  /** @brief Writes the adjacency matrix in a file */
  virtual void finish() override;
};

#endif