#include <iostream>
#include <omp.h>
#include <cmath>

using namespace std;

class Color {
private:
   double R, G, B;
public:
   double Y, Cb, Cr;

   void setRGB(uint8_t r, uint8_t g, uint8_t b) {
       R = r / 255.00;
       G = g / 255.00;
       B = b / 255.00;
   }

   const double Kr = 0.299;
   const double Kg = 0.587;
   const double Kb = 0.114;

   void calcYCbCr() {
       Y = Kr * R + Kg * G + Kb * B;
       Cb = (B - Y) / (1 - Kb) / 2;
       Cr = (R - Y) / (1 - Kr) / 2;
   }

   void calcRGB() {
       R = 2 * Cr * (1 - Kr) + Y;
       B = 2 * Cb * (1 - Kb) + Y;
       G = (Y - Kb * B - Kr * R) / Kg;
   }

   uint8_t getR() {
       return round(min(255.00, max(0.00, R * 255)));
   }

   uint8_t getB() {
       return round(min(255.00, max(0.00, B * 255)));
   }

   uint8_t getG() {
       return round(min(255.00, max(0.00, G * 255)));
   }
};

class Image {
public:
   int height;
   int width;
   int max_brightness;
   Color *pixels;
   double min_y = 1.00, max_y = -1.00;

   void find_min_max() {
#pragma omp parallel
       {
           int sum = 0;
           double local_max = -1.00, local_min = 1.00;
#pragma omp for
           for (int i = 0; i < width * height; ++i) {
               pixels[i].calcYCbCr();
               sum++;
               local_max = max(local_max, pixels[i].Y);
               local_min = min(local_min, pixels[i].Y);
           }
#pragma omp critical
           {
               min_y = min(min_y, local_min);
               max_y = max(max_y, local_max);
           }
       }
   }

   void color_adjustments() {
#pragma omp parallel for
       for (int i = 0; i < width * height; i++) {
           if (max_y != min_y) {
               pixels[i].Y = (pixels[i].Y - min_y) / (max_y - min_y);
           }
           pixels[i].calcRGB();
       }
   }

   void save(const char *filename) {
       FILE *fp = fopen(filename, "wb");
       if (fp == nullptr) {
           throw "Cannot open file\n";
       }
       fwrite("P6\n", 3, 1, fp);
       string res = to_string(height) + " " + to_string(width) + "\n" + to_string(max_brightness) + "\n";
       fwrite(res.c_str(), res.length(), 1, fp);
       for (int i = 0; i < width * height; i++) {
           uint8_t *tmp = new uint8_t[3];
           tmp[0] = pixels[i].getR();
           tmp[1] = pixels[i].getG();
           tmp[2] = pixels[i].getB();
           fwrite(tmp, 3, 1, fp);
           delete[] tmp;
       }
       fclose(fp);
   }

};

Image read_image(FILE *fp);

const int REPEAT_COUNT = 1;

int main(int argc, char *argv[]) {
   if (argc != 4) {
       std::cout << "usage: <num of threads> <input_file> <output_file>\n";
       return 0;
   }
   int num_threads;
   try {
       num_threads = stoi(argv[1]);
   } catch (std::invalid_argument exception) {
       std::cout << "Error: " << argv[1] << " isn't a number\n";
   }
   if (num_threads != 0) {
       omp_set_num_threads(num_threads);
   }
   FILE *fp = fopen(argv[2], "rb");
   if (fp == nullptr) {
       std::cout << "Cannot open file\n";
       return 0;
   }
   double time = 0;
   try {
       Image image = read_image(fp);
       for (int i = 0; i < REPEAT_COUNT; i++) {
           double start = omp_get_wtime();
           image.find_min_max();
           image.color_adjustments();
           time += (omp_get_wtime() - start);
       }
       image.save(argv[3]);
       delete[] image.pixels;
   } catch (const char *exception) {
       std::cout << "Error: " << exception << "\n";
       fclose(fp);
       return 0;
   }
   time = time * 1000 / REPEAT_COUNT;
   printf("\nTime (%i thread(s)): %f ms\n", num_threads, time);
   return 0;
}


void safe_read(void *ptr, size_t size, size_t cnt, FILE *fp);

int safe_read_int(FILE *fp);

Image read_image(FILE *fp) {
   // Читаем магическое число
   uint8_t *tmp = new uint8_t[3];
   safe_read(tmp, 1, 2, fp);
   if (tmp[0] != 'P' || tmp[1] != '6') {
       throw "Bad magic number. It isn't a P6 ppm file";
   }
   safe_read(tmp, 1, 1, fp);
   int height = safe_read_int(fp);
   int width = safe_read_int(fp);
   int brightness = safe_read_int(fp);
   auto *pixels = new Color[width * height];
   for (int i = 0; i < width * height; i++) {
       safe_read(tmp, 1, 3, fp);
       pixels[i].setRGB(tmp[0], tmp[1], tmp[2]);
   }
   fread(tmp, 1, 1, fp);
   if (!feof(fp)) {
       throw "Pixels were read, but end of file not reached";
   }
   fclose(fp);
   Image image = {height, width, brightness, pixels};
   delete[] tmp;
   return image;
}

void safe_read(void *ptr, size_t size, size_t cnt, FILE *fp) {
   if (fread(ptr, size, cnt, fp) != cnt) {
       if (feof(fp)) {
           throw "Premature end of file.\n";
       } else {
           throw "File read error.\n";
       }
   }
}

int safe_read_int(FILE *fp) {
   int ans = 0;
   uint8_t *buf = new uint8_t;
   safe_read(buf, 1, 1, fp);
   while (!isspace(*buf)) {
       ans = ans * 10 + (*buf - 48);
       safe_read(buf, 1, 1, fp);
   }
   delete[] buf;
   return ans;
}
