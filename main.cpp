#include <iostream>
#include <omp.h>
#include <exception>
#include <fstream>
#include "image_ppm.h"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cout << "Usage: <num of threads> <input_file> <output_file>\n";
        return 0;
    }
    try {
        int num = std::stoi(argv[1]);
        if (num > 0) {
            omp_set_num_threads(num);
        }
    } catch (std::invalid_argument &e) {
        std::cout << "Warning: Illegal num of threads: " << argv[1] << ". Default values will be used.\n";
    }
    std::ifstream input(argv[2], std::ios::in | std::ios::binary);
    std::ofstream output(argv[3], std::ios::out | std::ios::binary);
    try {
    	image_ppm image(input);
        double start = omp_get_wtime();
        image.improve_contrast();
        double finish = omp_get_wtime();
        printf("Threads: %s, time: %f ms\n", argv[1], (finish - start) * 1000);
        image.save(output);
    } catch (std::exception &exception) {
        std::cout << "Error: " << exception.what() << "\n";
        return -1;
    }
    input.close();
    output.close();
    return 0;
}
