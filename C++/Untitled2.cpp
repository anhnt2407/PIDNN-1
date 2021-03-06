#include <stdio.h>
#include <iostream>
#include <cmath>
#include <fstream>

using namespace std;


#define WINDOW_SIZE 30
#define DZ_TOL 1e-20

#define OUT_MAX 255

    int pos = 0;

    // Starting weights
    float hw[2][3] = {{-1, -1, -1},{1, 1, 1}};
    float ow[3] = {0.03, 0.1, 0.6};
    
    // Initialize the Arrays to keep the current and past values
    float y[WINDOW_SIZE+2];
    float r[WINDOW_SIZE+2];
    float i_y[WINDOW_SIZE+2];
    float i_r[WINDOW_SIZE+2];
    float u_p[WINDOW_SIZE+2];
    float u_i[WINDOW_SIZE+2];
    float u_d[WINDOW_SIZE+2];
    float x_p[WINDOW_SIZE+2];
    float x_i[WINDOW_SIZE+2];
    float x_d[WINDOW_SIZE+2];
    float v[WINDOW_SIZE+2];

class PIDNN
{
  public:

    // To avoid division by zero
    float DivByZero( float value);

    // Neuron Acitvation Functions
    float ptransfer( float value );
    float itransfer( float value, float accum, float ts );
    float dtransfer( float cur_value, float past_value, float ts );
    
    float predict( float measured, float expected ); // Forward Propagation

    void BackProp( float dr, float dy, float d_ow[3], float d_hw[2][3], int i ); // Back Propagation
    
    void learn( void ); // PIDNN Learn and Update

    PIDNN( float l_rate, float weight_change, float tolerance, float timestep ); // Constructor
//    ~PIDNN( void ); // Destructor
 
  private:

    // Neural Net Parameters that can be tuned
    float n; // Learning Rate
    float max_change; // Maximum change in weight
    float tol; // Error Tolerance
    float ts; // Time Step
    float d_tol; // Maximum Tolerance for weight change


    // Arrays to keep the current and past value
//    float *y;
//    float *r;
//    float *i_y;
//    float *i_r;
//    float *u_p;
//    float *u_i;
//    float *u_d;
//    float *x_p;
//    float *x_i;
//    float *x_d;
//    float *v;
};

// Member functions definitions including constructor
PIDNN::PIDNN( float l_rate, float weight_change, float tolerance, float timestep )
{
    ts = timestep;
    // Function to adjust learning rate dependent on timestep
    n = l_rate*ts;
    max_change = weight_change;
    tol = tolerance;
    // Function To adjust d_tol dependent on window_size
    d_tol = max_change*log(WINDOW_SIZE+1)/n;

    // Initialize the Arrays to keep the current and past values
//  	y = new float[WINDOW_SIZE+2];
//    r = new float[WINDOW_SIZE+2];
//    i_y = new float[WINDOW_SIZE+2];
//    i_r = new float[WINDOW_SIZE+2];
//    u_p = new float[WINDOW_SIZE+2];
//    u_i = new float[WINDOW_SIZE+2];
//    u_d = new float[WINDOW_SIZE+2];
//    x_p = new float[WINDOW_SIZE+2];
//    x_i = new float[WINDOW_SIZE+2];
//    x_d = new float[WINDOW_SIZE+2];
//    v = new float[WINDOW_SIZE+2];

    x_i[0] = 0;
    u_d[0] = 0;
}

//PIDNN::~PIDNN( void )
//{
//    // Delete the arrays
//    delete[] i_y;
//    delete[] i_r;
//    delete[] u_p;
//    delete[] u_i;
//    delete[] u_d;
//    delete[] x_p;
//    delete[] x_i;
//    delete[] x_d;
//    delete[] v;
//}

float PIDNN::DivByZero( float value )
// Avoiding Division by Zero
{
  if (value == 0) {
  return DZ_TOL;
  }
  else if (abs(value) < DZ_TOL) {
    return ((value > 0) - (value < 0))*DZ_TOL;
  }
  else {
    return value;
  }
}

float PIDNN::ptransfer( float value )
{
  return value;
}

float PIDNN::itransfer ( float value, float accum, float ts )
{
  return (value*ts + accum);
}

float PIDNN::dtransfer ( float cur_value, float past_value, float ts )
{
  return (cur_value - past_value)/ts;
}

float PIDNN::predict( float measured, float expected )
{
  y[pos] = measured;
  if (pos > WINDOW_SIZE) {
    learn();
    pos = 0;
  }
  // Register values for expected
  pos++;

  r[pos] = expected;

  // Set the values for the Input Layer
  i_y[pos] = ptransfer(measured);
  i_r[pos] = ptransfer(expected);
  
  // Hidden P Neuron
  u_p[pos+1] = i_y[pos+1]*hw[0][0] + i_r[pos]*hw[1][0];
  x_p[pos+1] = ptransfer(u_p[pos+1]);

  // Hidden I Neuron
  u_i[pos] = i_y[pos]*hw[0][1] + i_r[pos]*hw[1][1];
  x_i[pos] = itransfer(u_i[pos], x_i[pos-1], ts);

  // Hidden D Neuron
  u_d[pos] = i_y[pos]*hw[0][2] + i_r[pos]*hw[1][2];
  x_d[pos] = dtransfer(u_d[pos], u_d[pos-1], ts);

  // Output Neuron
  v[pos] = x_p[pos]*ow[0] + x_i[pos]*ow[1] + x_d[pos]*ow[2];

  
  return v[pos];
}

void PIDNN::learn( void )
{
  // Do Back Propagation
  float d_ow[3] = {0, 0, 0};
  float d_hw[2][3] = {{0,0,0},{0,0,0}};
  float avg_error = 0;

  for (int i = 1; i <= WINDOW_SIZE; i++) {
    float dr = r[i+1] - y[i+1];
    avg_error += abs(dr)/WINDOW_SIZE;
    float dy = y[i+1] - y[i];
    BackProp(dr, dy, d_ow, d_hw, i);
  }

  // Update weights if average error is >= tol
  if (avg_error >= tol) {
    // Update Weights
    for (int j = 0; j < 3; j++) {
      // Update Output Weights
      if (abs(d_ow[j]) > d_tol) {
        d_ow[j] = ((d_ow[j] > 0) - (d_ow[j] < 0))*d_tol;
      }
      // printf("d_ow[%d]: %f\n", j, d_ow[j]);
      ow[j] = ow[j] -n*d_ow[j]/WINDOW_SIZE;
      
      // Keep between 0 and 1
      ow[j] = (float)min(max((double)ow[j], 0.0), 1.0);
    }
  }

  // Update the Input Layer Values
  i_y[0] = i_y[WINDOW_SIZE+1];
  i_r[0] = i_r[WINDOW_SIZE+1];

  // Update the Hidden Layer Values
  u_p[0] = u_p[WINDOW_SIZE+1];
  u_i[0] = u_i[WINDOW_SIZE+1];
  u_d[0] = u_d[WINDOW_SIZE+1];

  x_p[0] = x_p[WINDOW_SIZE+1];
  x_i[0] = x_i[WINDOW_SIZE+1];
  x_d[0] = x_d[WINDOW_SIZE+1];

  // Update the Output Layer Values
  v[0] = v[WINDOW_SIZE+1];
}

void PIDNN::BackProp ( float dr, float dy, float d_ow[3], float d_hw[2][3], int i )
{

  float dv = DivByZero(v[i+1] - v[i]);
  float du_p = DivByZero(u_p[i+1] - u_p[i]);
  float du_i = DivByZero(u_i[i+1] - u_i[i]);
  float du_d = DivByZero(u_d[i+1] - u_d[i]);

  float dx_p = x_p[i+1] - x_p[i];
  float dx_i = x_i[i+1] - x_i[i];
  float dx_d = x_d[i+1] - x_d[i]; 

  // Output Weights
  d_ow[0] += -2*dr*dy*x_p[i+1]/dv;
  d_ow[1] += -2*dr*dy*x_i[i+1]/dv;
  d_ow[2] += -2*dr*dy*x_d[i+1]/dv;

  // Hidden Weights
  d_hw[0][0] += -2*dr*dy*dx_p*ow[0]*i_y[i+1]/(dv*du_p);
  d_hw[0][1] += -2*dr*dy*dx_i*ow[1]*i_y[i+1]/(dv*du_i);
  d_hw[0][2] += -2*dr*dy*dx_d*ow[2]*i_y[i+1]/(dv*du_d);
  d_hw[1][0] += -2*dr*dy*dx_p*ow[0]*i_r[i+1]/(dv*du_p);
  d_hw[1][1] += -2*dr*dy*dx_i*ow[1]*i_r[i+1]/(dv*du_i);
  d_hw[1][2] += -2*dr*dy*dx_d*ow[2]*i_r[i+1]/(dv*du_d);
}

int main(){
	
	printf("Initializing Variables and Creating File\n");
	// Create the file
	ofstream file;
	file.open("data.csv");
  file << "expected, output, measured" << std::endl;
  // Simulated System Variables
  float ts = 0.01;
  float measured = 0;
  float output = 0;

  // Dynamic System Variables
  float xddot = 0, xdot = 0, x = 0;
  float G = 10, a = 0.1, b = 1, c = 0.1;

  // Neural Net Variables
  float learning_rate = 1e-2*ts, max_change = 0.8*log(WINDOW_SIZE + 1), tolerance = 1e-20;
  
  int setpoint = 10;

  // Initializing Neural Net
  printf("Initializing Neural Net\n");
  PIDNN* NeuralNet = new PIDNN(learning_rate, max_change, tolerance, ts);

	printf("Simulating the Neural net response\n");
	for(int i = 1; i < 5000; i++){
  	// Simulating the Neural net response
    output = NeuralNet->predict(measured, setpoint);
    if (output > OUT_MAX) output = OUT_MAX;
    if (output < -OUT_MAX) output = -OUT_MAX;
    xddot = G*(output/a) - xdot*b - measured*c;
    xdot = xdot + xddot*ts;
    measured = measured + xdot*ts;
//    measured = measured + output/100;
		file << setpoint << ',' << output << ',' << measured << endl;
	}
//	for(int i = 1; i < 50; i++){
//		setpoint = 100;
//  	// Simulating the Neural net response
//    output = NeuralNet->predict(measured, setpoint);
//    xddot = G*(output/a) - xdot*b - measured*c;
//    xdot = xdot + xddot*ts;
//    measured = measured + xdot*ts;
//		file << setpoint << ',' << output << ',' << measured << endl;
//	}
//	for(int i = 1; i < 500; i++){
//		setpoint = 30;
//  	// Simulating the Neural net response
//    output = NeuralNet->predict(measured, setpoint);
//    xddot = G*(output/a) - xdot*b - measured*c;
//    xdot = xdot + xddot*ts;
//    measured = measured + xdot*ts;
//		file << setpoint << ',' << output << ',' << measured << endl;
//	}
	
file.close();
}
