#include "FlightLengthObserver.h"

Define_Module(FlightLengthObserver);

omnetpp::simsignal_t FlightLengthObserver::flight = registerSignal("flight");
omnetpp::simsignal_t FlightLengthObserver::flight_stat =
  registerSignal("flight_stat");
omnetpp::simsignal_t FlightLengthObserver::intraFlightLength = 
  registerSignal("intraFlightLength");
omnetpp::simsignal_t FlightLengthObserver::intraFlightLength_stat = 
  registerSignal("intraFlightLength_stat");
omnetpp::simsignal_t FlightLengthObserver::interFlightLength = 
  registerSignal("interFlightLength");
omnetpp::simsignal_t FlightLengthObserver::interFlightLength_stat = 
  registerSignal("interFlightLength_stat");
omnetpp::simsignal_t
  FlightLengthObserver::remote_random_area = registerSignal("remoteRandomArea");
omnetpp::simsignal_t
  FlightLengthObserver::remote_random_area_stat = 
    registerSignal("remoteRandomArea_stat");

FlightLengthObserver::FlightLengthObserver():
  numOfSamples(0), counter(0)
{
  getSimulation()->getSystemModule()->subscribe(flight, this);
  getSimulation()->getSystemModule()->subscribe(intraFlightLength, this);
  getSimulation()->getSystemModule()->subscribe(interFlightLength, this);
  getSimulation()->getSystemModule()->subscribe(remote_random_area, this);
}

FlightLengthObserver::~FlightLengthObserver()
{
  if (isSubscribed(flight, this))
    unsubscribe(flight, this);
  if (isSubscribed(intraFlightLength, this))
    unsubscribe(intraFlightLength, this);
  if (isSubscribed(interFlightLength, this))
    unsubscribe(interFlightLength, this);
  if (isSubscribed(remote_random_area, this))
    unsubscribe(remote_random_area, this);
}

void FlightLengthObserver::initialize(int stage)
{
  if (stage == inet::INITSTAGE_LOCAL) {
    numOfSamples = par("observations");
    classifyFlight = par("classifyFlight").boolValue();
    EV_INFO << "Number of samples: " << numOfSamples << '\n';
    WATCH(counter);
  }
}

void FlightLengthObserver::handleMessage(omnetpp::cMessage* msg)
{
  if (!msg->isSelfMessage())
    EV << "Simulation Observer: This module does not receive any messages!\n";
}

void FlightLengthObserver::receiveSignal(
  omnetpp::cComponent* src, omnetpp::simsignal_t id, double flightLength, 
  omnetpp::cObject* details
) {
  auto node_id = dynamic_cast<omnetpp::cModule*>(src)->getParentModule()->getIndex();
  if (omnetpp::simTime() >= getSimulation()->getWarmupPeriod()) {
    counter++;
    processSignal(node_id, id, flightLength);
    if (counter%10000 == 0)
      EV_INFO << "FlightLengthObserver: " << counter 
        << " samples have been produced\n";
  }
  if (counter == numOfSamples) {
    EV_INFO << "FlightLengthObserver: " << counter 
      << " samples have been gathered\n";
    endSimulation();
  }
}

void FlightLengthObserver::processSignal(
  int node_id, omnetpp::simsignal_t id, double flightLength
) {
  if (classifyFlight) {
    if (id == intraFlightLength) 
      emit(intraFlightLength_stat, flightLength);
    else if (id == interFlightLength)
      emit(interFlightLength_stat, flightLength);
  }
  else {
    EV_INFO << "New flight is received " << flightLength 
      << " from " << node_id << '\n';
    emit(flight_stat, flightLength);
  }
}

void FlightLengthObserver::receiveSignal(
  omnetpp::cComponent* src, omnetpp::simsignal_t id, bool isRemoteRandomArea, 
  omnetpp::cObject* details
) {
  if (isRemoteRandomArea)
    EV_INFO << "New flight is towards an area randomly drawn\n";
  else
    EV_INFO << "New flight is towards an area from C_k\n";
  emit(remote_random_area_stat, isRemoteRandomArea);
}