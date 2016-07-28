#include "../include/WiSARD.hpp"
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include "kernelCanvas.hpp"

#define RESAMPLING_SIZE 32
#define PRECISION 8
#define RETINA_LENGTH (RESAMPLING_SIZE * PRECISION)
#define NUM_BITS_ADDR 2

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

double* readInputFromFile(std::string path, int* inputSize) {
  double *inputValues;
  double value;
  std::ifstream file (path);

  file >> *inputSize;

  inputValues = (double *) malloc(*inputSize * sizeof(double));
  for (int i = 0 ; i < *inputSize ; i++) {
    file >> inputValues[i];
  }

  file.close();
  return inputValues;
}

std::vector<int> buildRetinaFromFile(std::string path, int resamplingSize, int precision) {
  int inputSize;
  double* inputValues;
  double* resampledValues;
  int* discreteValues;
  int** retina;
  int zeroIndex = precision/2;

  std::vector<int> retinaVector;

  inputValues = readInputFromFile(path, &inputSize);

  // allocate memory for the intermediate arrays
	resampledValues = (double *) malloc(resamplingSize * sizeof(double));
	discreteValues = (int *) malloc(resamplingSize * sizeof(int));
	retina = (int**) malloc(precision * sizeof(int*));

  for(int i = 0; i < precision; i++){
		retina[i] = (int*) malloc(resamplingSize * sizeof(int));
	}

  //Do intermediate steps to build retina matrix
  temporalResampling(inputSize, inputValues, resamplingSize, resampledValues);
  zeroIndex = thermometerDiscretization(resamplingSize, resampledValues, discreteValues, precision, 0, 0);
  buildRetina(resamplingSize, discreteValues, precision, retina, zeroIndex);

  //Converts retina matrix into a vector
  for (int i = 0 ; i < precision ; i++) {
    for (int j = 0 ; j < resamplingSize ; j++) {
      retinaVector.push_back(retina[i][j]);
    }
  }

  // Free all arrays used in the process
  free(inputValues);
  free(resampledValues);
  free(discreteValues);
  for (int i = 0 ; i < precision ; i++) {
    free(retina[i]);
  }
  free(retina);

  return retinaVector;
}

// Initializes vectors with training data.
void getTrainingData(std::string trainingDataPath, std::vector<std::vector<int>> &trainingSamples, std::vector<std::string> &trainingClasses) {
    std::ifstream file (trainingDataPath);
    std::string line;

    while (std::getline(file, line)) {
      std::vector<std::string> tokens = split(line, ' ');

      trainingSamples.push_back(buildRetinaFromFile(tokens[0], RESAMPLING_SIZE, PRECISION));
      trainingClasses.push_back(tokens[1]);
    }

    file.close();
}

// Initializes the samples vector with the samples for prediction
void getSamples(std::vector<std::vector<int>> &samples) {
    samples.push_back(buildRetinaFromFile("genwavs/a_3.gw", RESAMPLING_SIZE, PRECISION)); // a
    samples.push_back(buildRetinaFromFile("genwavs/b_3.gw", RESAMPLING_SIZE, PRECISION)); // b
}

void printResults(std::vector<std::string> results) {
    std::cout << "Results:";

    for (std::vector<std::string>::iterator it = results.begin(); it != results.end() ; ++it) {
        std::cout << " " << *it;
    }

    std::cout << std::endl;
}

int main(int argc, char **argv) {
    // Training data
    std::string trainingDataPath;
    std::vector<std::vector<int>> trainingSamples;
    std::vector<std::string> trainingClasses;

    // get training data path from the argument
    if (argc > 1) {
      trainingDataPath = argv[1];
    }

    // Actual samples for preditcion
    std::vector<std::vector<int>> samples;

    wann::WiSARD *wisard = new wann::WiSARD(RETINA_LENGTH, NUM_BITS_ADDR);

    // Train
    getTrainingData(trainingDataPath, trainingSamples, trainingClasses);
    wisard->fit(trainingSamples, trainingClasses);

    // Predict
    getSamples(samples);
    std::vector<std::string> results = wisard->predict(samples);

    // Print results
    printResults(results);

    return 0;
}
