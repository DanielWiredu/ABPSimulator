//
//  ABP.cpp
//  Project2_ABPSimulator
//
//  Created by Daniel Wiredu on 11/10/23.
//

#include <stdio.h>
#include <time.h>
#include <vector>
#include <queue>

using namespace std;

// Event Scheduler (ES) - time-ordered sequence of events
struct Event {
    enum EventType { TIME_OUT, ACK , NIL};
    EventType type;
    double time;
    //An ACK event has two extra fields
    bool error_flag;
    int sequence_number;
};

// Event comparator for priority queue - order events in queue based on time of occurence
struct EventComparator {
    bool operator()(const Event& a, const Event& b)
    {
        return a.time > b.time;
    }
};

class EventScheduler {
private:
    priority_queue<Event, vector<Event>, EventComparator> events;
    
public:
    void scheduleEvent(Event::EventType type, double time, bool error_flag = false, int sequence_number = 0)
    {
        Event event = {type, time, error_flag, sequence_number};
        events.push(event);
    }

    Event getNextEvent() {
        Event nextEvent = events.top();
        return nextEvent;
    }

    bool isEmpty() {
        return events.empty();
    }
    
    // Pop an event from the event queue
    void popEvent() {
        events.pop();
    }
    
    // schedule a timeout event in the queue
    void RegisterTimeout(double time, int SN) {
        Event e;
        e.type = Event::TIME_OUT;
        e.time = time;
        e.sequence_number = SN;
        e.error_flag = false;

        scheduleEvent(e.type, e.time);
    }
    
    // Purge Timeouts - exclude from temporary queue if event is a Timeout event
    void purgeTimeOut() {
        vector<Event> eventQueue;

        while (!isEmpty()) {
            Event e = getNextEvent();

            if (e.type != Event::TIME_OUT) {
                eventQueue.push_back(e);
            }
            popEvent();
        }

        // put back all non-timeout events
        for (const auto& event : eventQueue) {
            scheduleEvent(event.type, event.time);
        }
    }
    
    // Check for timeout events in the queue
    bool hasTimeOut() {
        vector<Event> eventQueue;
        bool result = false;

        while (!isEmpty()) {
            Event e = getNextEvent();
            if (e.type == Event::TIME_OUT) {
                result = true;
            }
            eventQueue.push_back(e);
            popEvent();
        }

        for (const auto& event : eventQueue) {
            scheduleEvent(event.type, event.time);
        }

        return result;
    }
};

// Channel
class Channel {
public:
    Channel(double propagationDelay, double lossProbability, double bitErrorRate)
        : propagationDelay(propagationDelay), lossProbability(lossProbability), bitErrorRate(bitErrorRate) {}

    Event transmitFrame(double time, int sequenceNumber, double frameLength) {
        
        // Simulate transmission with loss and errors
        double randomValue = ((double)rand() / RAND_MAX);  // Random value between 0 and 1

        // Simulate loss
        if (randomValue < lossProbability) {
            // Frame is lost
            Event event = {Event::NIL, time + propagationDelay};
            return event;
        }

        // Simulate errors
        int errorCount = 0;
        for (int i = 0; i < frameLength; ++i) {
            double errorProbability = ((double)rand() / RAND_MAX);  // Random value between 0 and 1
            if (errorProbability < bitErrorRate) {
                ++errorCount;
            }
        }

        if (errorCount >= 5) {
            // Frame is lost due to too many errors
            Event event = {Event::NIL, time + propagationDelay};
            return event;
        } else if (errorCount > 0) {
            // Frame has errors
            Event event = {Event::ACK, time + propagationDelay, true, sequenceNumber};
            return event;
        } else {
            // Frame is successfully transmitted
            Event event = {Event::ACK, time + propagationDelay, false, sequenceNumber};
            return event;
        }
    }

private:
    double propagationDelay;
    double lossProbability;
    double bitErrorRate;
};

// ABP Receiver
class ABPReceiver {
public:
    ABPReceiver() : NEXT_EXPECTED_FRAME(0), currentTime(0) {}

    Event receiveFrame(double time, bool error_flag, int sequenceNumber) {
        
        currentTime = currentTime + time;

        if (error_flag == false) {
            if (sequenceNumber == NEXT_EXPECTED_FRAME){
                // Frame is successfully received
                NEXT_EXPECTED_FRAME = (NEXT_EXPECTED_FRAME + 1) % 2;  // Update counter
            }
        }
        Event event = {Event::ACK, currentTime, error_flag, NEXT_EXPECTED_FRAME};
        return event;
    }

private:
    int NEXT_EXPECTED_FRAME;
    double currentTime;
};

// ABP Sender
class ABPSender {
public:
    ABPSender(double delta, double channelCapacity, double packetLength, double headerLength, double tau, double BER)
        : delta(delta), channelCapacity(channelCapacity),
        //The sender generates a packet of length L = H + l, where H is the fixed length of the frame header and l is the packet length
        packetLength_L((headerLength + packetLength) * 8),
        headerLength(headerLength * 8),
        BER(BER),
        tau(tau),
        SN(0), NEXT_EXPECTED_ACK(1), currentTime(0){}
    
        // Function for sending packet
        Event SEND() {
        double transmissionTime = packetLength_L / channelCapacity;
        Event e;
        e.time = currentTime + transmissionTime;
        e.sequence_number = SN;
        
        Channel channel(tau, 0.1, BER);
        ABPReceiver receiver;
        
        //forward channel
        e = channel.transmitFrame(e.time, e.sequence_number, packetLength_L);
        
        //receiver receives frame and returns ACK
        e = receiver.receiveFrame(e.time, e.error_flag, e.sequence_number);
        transmissionTime = headerLength / channelCapacity ;
        e.time = e.time + transmissionTime;
        
        //reverse channel
        e = channel.transmitFrame(e.time, e.sequence_number, headerLength);

        return e;
    }

public:
    int SN; //initialized to 0
    int NEXT_EXPECTED_ACK; //initialized to 1
    double delta; //time-out - choice of value affects the protocol performance greatly
    double currentTime; //tc is initialized to 0
    double channelCapacity;
    double packetLength_L;
    double headerLength;
    double BER;
    double tau;
};

class ABP {
public:
        ABP(double H, double l, double delta, double C, double tau, double BER, int experimentDuration)
    : H(H), l(l), delta(delta), C(C), tau(tau), BER(BER), experimentDuration(experimentDuration), packetsSent(0), packetsReceived(0) {}
    
    // simulation function
    double RunSimulation() {
        // Seed the random number generator
        srand((int)time(NULL));
        
        ABPSender sender(delta, C, l, H, tau, BER);

        // Run simulation up to expected packets to be received
        while (packetsReceived < experimentDuration) {
            
            if (!scheduler.hasTimeOut()) {
                // Purge old timeout events from the Event Scheduler ES
                scheduler.purgeTimeOut();

                // Register a new timeout event in the Event Scheduler ES
                intermediateTime = sender.currentTime + delta + (sender.packetLength_L / sender.channelCapacity);
                scheduler.RegisterTimeout(intermediateTime, sender.SN);

                // Sender sends a packet
                Event e = sender.SEND();
                packetsSent++;

                if (e.type != Event::NIL)
                    scheduler.scheduleEvent(e.type, e.time, e.error_flag, e.sequence_number);
            }
            
            // Sender reads next event from the ES and update current time
            Event e = scheduler.getNextEvent();
            sender.currentTime = e.time;
            scheduler.popEvent();

            // Successfull ACK without error and sequence number = NEXT_EXPECTED_ACK
            if (e.type == Event::ACK && e.error_flag == false && e.sequence_number == sender.NEXT_EXPECTED_ACK) {
                packetsReceived++;
                scheduler.purgeTimeOut();
                sender.SN = (sender.SN + 1) % 2;
                sender.NEXT_EXPECTED_ACK = (sender.NEXT_EXPECTED_ACK + 1) % 2;
            }
            
            // Timeouts will be purged in the next iteration, same packet will be sent again
            else if ((e.type == Event::ACK && e.error_flag == false) || (e.type == Event::ACK && e.sequence_number != sender.NEXT_EXPECTED_ACK)) {
                continue;
            }
            
            else if (e.type == Event::TIME_OUT) {
                // Do nothing, Timeout will be dequeued and packet will be sent again with same SN
            }
        }

        // Compute and return throughput
        double throughput = ((packetsReceived * l * 8) - (packetsReceived * H * 8) )  / sender.currentTime;
        return throughput;
    }
    
private:
    int packetsSent;
    int packetsReceived;
    double intermediateTime;
    double H;
    double l;
    double delta;
    double C;
    double tau;
    double BER;
    int experimentDuration;
    EventScheduler scheduler;
};



