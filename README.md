# Images contrast adjuster
Parallel contrast adjuster using OpenMP library.

## What's happening?
The task to familiarize with parallel programming and the OpenMP library possibilities

Translate the values from the YCBCr color space to the RGB     
Let change the pixel values to obtain maximum contrast without changing the color (hue).
That can be achieved by linear adjusting the contrast in the luminance channel Y.

The **input** file contains data in PPM (P6) format.   
**The file doesn't contain comments. Maximum brightness value: 255**

The **output** file will be a new image in the format PPM (P6).

## Benchmark

Test using [test.cmd](./test.cmd)

As we can see, the min time is reached when the number of threads is nearly equal to the number of cores.
Hyper-threading is not beneficial because each thread uses the same pipelining instructions in a superscalar.

Test system: [Intel Core i7-9750H](https://ark.intel.com/content/www/us/en/ark/products/191045/intel-core-i7-9750h-processor-12m-cache-up-to-4-50-ghz.html), 6 cores, 12 threads

> Image’s size: 5184 * 3456 pixels, 52 489 KB  
> Path to input image: image.ppm  
> Path to output image: result.ppm  
> Enter max threads value: 12  
> Path to .exe: contrast_adjuster.exe  
> Threads: 0, time: 182.999849 ms  
> Threads: 1, time: 505.999804 ms  
> Threads: 2, time: 239.000082 ms  
> Threads: 3, time: 164.999962 ms  
> Threads: 4, time: 149.000168 ms  
> Threads: 5, time: 130.000114 ms  
> Threads: 6, time: 135.000229 ms  
> Threads: 7, time: 137.000084 ms  
> Threads: 8, time: 137.000084 ms  
> Threads: 9, time: 145.999908 ms  
> Threads: 10, time: 139.000177 ms  
> Threads: 11, time: 149.999857 ms  
> Threads: 12, time: 141.999960 ms    

## Run
Clone this repo:
> `git clone https://github.com/Dalvikk/contrast_adjuster`  
> `cd contrast_adjuster`

Compile using `g++/clang++`:  
> `g++/clang++ main.cpp -std=c++14 -fopenmp -o contrast_adjuster`  

or using `cmake`:
> `mkdir build`  
> `cd build`  
> `cmake ..`  
> `make (or mingw32-make if you use windows and mingw)`


Run:
> `./contrast_adjuster (or contrast_adjuster.exe) <num of threads> <input_file> <output_file>`

The number of threads can be 0, in that case, OpenMP self decide how many will use

## Example

![example1](.github/images/example1.jpg)
![example2](.github/images/example2.jpg)

## Algorithm

1. Read the image in RGB color space  
2. Translate from RGB into YCbCr using these formulas from [wikipedia](https://en.wikipedia.org/wiki/YCbCr):
3.
   ![Formula](./.github/images/formula1.svg)   
   ![Constants](./.github/images/formula2.svg)
3. Find min and max Y. Be careful to avoid [race condition](https://en.wikipedia.org/wiki/Race_condition)
4. Linear adjusting Y values from [min_Y; max_Y] to [0, 255]
5. Translate from YCbCr to RGB

Steps 2-5 can easily parallelize   
