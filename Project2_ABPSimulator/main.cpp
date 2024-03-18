//
//  main.cpp
//  Project2_ABPSimulator
//
//  Created by Daniel Wiredu on 11/10/23.
//
#include "ABP.cpp"
#include <iostream>
#include <iostream>

int main(int argc, const char * argv[]) {
    // Set simulation parameters
    int H = 54; //header length in bytes
    int l = 1500; //frame length in bytes
    double C = 5000000.0;  //Channel capacity 5 Mb/s
    int experimentDuration = 5000;  // Number of successfully delivered packets to simulate
    double delta_tau_values[] = {2.5, 5.0, 7.5, 10.0, 12.5}; //values of Δ to be taken from this given set
    double BER_values[] = { 0.0, 0.00001, 0.0001 }; //given set of Bit Error Rates
    double tau_values[] = { 10, 500 }; //τ - propagation delays in milliseconds
    double delta = 0;
    double throughput1 = 0.0;
    double throughput2 = 0.0;

    // iterate through given set delta/tau values
    for (double delta_tau : delta_tau_values) {

        // iterate through tau - propagation delays
        for (double tau : tau_values) {
            
            tau = double(tau / 1000); // convert propagation delay in milliseconds into seconds
            delta = delta_tau * tau; // Calculate delta (timeout)
            
            // iterate through bit error rates - BER
            for (double BER : BER_values) {
                
                // Create first simulator instance
                ABP simulator(H, l, delta, C, tau, BER, experimentDuration);
                throughput1 = simulator.RunSimulation();

                // Create second simulator instance
                ABP simulator2(H, l, delta, C, tau, BER, experimentDuration);
                throughput2 = simulator2.RunSimulation();

                cout << "Delta/Tau: " << delta_tau << ", Propagation Delay: " << (tau * 1000) << " ms" << ", Bit Error Rate: " << BER << endl ;
                cout << "....................................................................." << endl;
                cout << "Throughput 1 (" << experimentDuration << ") packets = " << throughput1 << endl;
                cout << "Throughput 2 (" << experimentDuration << ") packets = " << throughput2 << endl;

                if (throughput1 == throughput2) {
                    cout << "Throughput for both simulations are same... COMPARABLE" << endl;
                }
                else if (throughput1 > throughput2){
                    cout << "Throughput for 1st " << experimentDuration << " packets is GREATER" << endl;
                }
                else {
                    cout << "Throughput for 2nd " << experimentDuration << " packets is GREATER" << endl;
                }
                cout << "..............................................................." << endl << endl;
            }
        }
    }

    return 0;
}
