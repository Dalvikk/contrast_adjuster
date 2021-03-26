# Images contrast adjuster
Parallel contrast adjuster using OpenMP library.

## What's happening?
The task to familiarize with parallel programming and the openMP library possibilities

Work with YCbCr.601 color space (previously translate from RGB).
Let change the pixel values to obtain maximum contrast (range of values is [0..255]) without changing the color (hue).
That can be achieved by linear adjusting the contrast in the luminance channel Y color space YCbCr (601 in the PC range: [0; 255]).

The input file contains data in PPM (P6) format.

The output file will be a new image in the format PPM (P6).

## Run 
Clone this repo: 
> `git clone https://github.com/Dalvikk/contrast_adjuster`  

Compile using `g++` or `clang++`:  
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
2. Following steps can easily parallelize   
   Translate from RGB into YCbCr using these formulas from [wikipedia](https://en.wikipedia.org/wiki/YCbCr):
   
   ![Formula](./.github/images/formula1.svg)   
   ![Constants](./.github/images/formula2.svg)
3. Find min and max Y. Be careful to avoid [race condition](https://en.wikipedia.org/wiki/Race_condition)
4. Linear adjusting the Y
5. Translate from YCbCr to RGB
