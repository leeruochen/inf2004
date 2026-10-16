#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
// bug 5: missing include for usleep function

float Kp = 2.0; 
float Ki = 0.2; 
float Kd = 0.02; 

// Function to compute the control signal
float compute_pid(float setpoint, float current_value, float *integral, float *prev_error, float dt) {

    float error = setpoint - current_value;
    // bug 6: previously current_value - setpoint
    
    *integral += error * dt;
    float derivative = (error - *prev_error) / dt;
    float control_signal = Kp * error + Ki * (*integral) + Kd * derivative;
    // bug 11: wrong discreate integration and differentiation calcs
    
    *prev_error = error;
    // bug 8: previously *prev_error = current_value, prev error should track the error instead.
    
    return control_signal;
}

int main() {
    float setpoint = 100.0;  // Desired position
    float current_value = 0.0;  // Current position
    float integral = 0.0;  // Integral term
    float prev_error = 0.0;  // Previous error term
    
    float time_step = 0.1;  
    int num_iterations = 100; // bug 1: missing semicolon
    
    // Simulate the control loop
    for (int i = 0; i < num_iterations; i++) {
        float control_signal = compute_pid(setpoint, current_value, &integral, &prev_error, time_step); 
        // bug 2: prev_error should be passed by reference, compute_pid expects a pointer
        
        float motor_response = control_signal * 0.05;  // Motor response model

        current_value += motor_response;
        // bug 7: current_value was not updated when motor_response is, making current_value stay at 0
        
        printf("Iteration %d: Control Signal = %f, Current Position = %f\n", i, control_signal, current_value);
        // bug 3: Control Signal = was previously %d, %f should be used as control signal is float
        
        // prev_error == current_value;
        // bug 9: unnecesary line
        
        usleep((useconds_t)(time_step * 1000000));
        // bug 10: usleep expects microseconds, time_step is in seconds, so we multiply by 1,000,000 to convert to microseconds
    
    }
    // bug 4: missing } for the for loop.
    return 0;
}
