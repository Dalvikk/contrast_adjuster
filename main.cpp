#include <iostream>
#include <omp.h>
#include <cmath>
#include <cerrno>
#include <cstring>
#include <exception>

class my_exception : public std::exception {
private:
    const char *msg;
public:
    explicit my_exception(const char *msg) : msg(msg) {}

    const char *what() const noexcept override {
        return msg;
    }

    ~my_exception() noexcept override {
        delete msg;
    }
};

class file_read_exception : my_exception {
public:
    explicit file_read_exception(const char *msg) : my_exception(msg) {}
};

class Color {
public:
    double R = 0, G = 0, B = 0;
    double Y = 0, Cb = 0, Cr = 0;

    void init_by_RGB(uint8_t r, uint8_t g, uint8_t b) {
        R = r / 255.00;
        G = g / 255.00;
        B = b / 255.00;
        calc_YCbCr_from_RGB();
    }

    constexpr static const double Kr = 0.299;
    constexpr static const double Kg = 0.587;
    constexpr static const double Kb = 0.114;

    void calc_YCbCr_from_RGB() {
        Y = Kr * R + Kg * G + Kb * B;
        Cb = (B - Y) / (1 - Kb) / 2;
        Cr = (R - Y) / (1 - Kr) / 2;
    }

    void calc_RGB_from_YcbCr() {
        R = 2 * Cr * (1 - Kr) + Y;
        B = 2 * Cb * (1 - Kb) + Y;
        G = (Y - Kb * B - Kr * R) / Kg;
    }

    uint8_t getR() const {
        return (uint8_t) round(std::min(255.00, std::max(0.00, R * 255)));
    }

    uint8_t getB() const {
        return (uint8_t) round(std::min(255.00, std::max(0.00, B * 255)));
    }

    uint8_t getG() const {
        return (uint8_t) round(std::min(255.00, std::max(0.00, G * 255)));
    }
};

void read(void *dest, size_t size, size_t cnt, FILE *fp) {
    if (fread(dest, size, cnt, fp) != cnt) {
        if (feof(fp)) {
            throw file_read_exception("Unexpected EOF");
        } else {
            throw file_read_exception(strerror(errno));
        }
    }
}

// read integer value and one symbol after
size_t read_int(FILE *fp) {
    size_t ans = 0;
    uint8_t buf;
    read(&buf, 1, 1, fp);
    while ('0' <= buf && buf <= '9') {
        ans = ans * 10 + (buf - 48);
        read(&buf, 1, 1, fp);
    }
    return ans;
}

class Image {
private:
    size_t height;
    size_t width;
    uint8_t max_brightness = 255;
    Color *pixels = nullptr;
    double min_y = 1.00, max_y = -1.00;

    void read_image(FILE *fp) {
        auto *tmp = (uint8_t *) malloc(sizeof(uint8_t) * 3);
        if (tmp == nullptr) {
            std::string str = "Bad allocation " + std::to_string(sizeof(uint8_t) * 3) + " bytes";
            throw my_exception(str.c_str());
        }
        try {
            read(tmp, 1, 3, fp);
            if (tmp[0] != 'P' || tmp[1] != '6' || tmp[2] != '\n') {
                throw file_read_exception("Wrong file format.");
            }
            height = read_int(fp);
            width = read_int(fp);
            if (read_int(fp) > 255) {
                throw my_exception("Max brightness can't be greater than 255");
            }
            try {
                pixels = new Color[width * height];
            } catch (const std::bad_alloc &e) {
                std::string str = "Bad allocation " + std::to_string(sizeof(Color) * width * height) + " bytes";
                throw my_exception(str.c_str());
            }
            for (size_t i = 0; i < width * height; i++) {
                read(tmp, 1, 3, fp);
                pixels[i].init_by_RGB(tmp[0], tmp[1], tmp[2]);
            }
            fread(tmp, 1, 1, fp); // newline
        } catch (std::exception &e) {
            free(tmp);
            free(pixels);
            throw e;
        }
        if (!feof(fp)) {
            std::cout << "Warning! Pixels were read, but EOF not reached\n";
        }
        free(tmp);
    }

public:
    explicit Image(FILE *fp) {
        read_image(fp);
    }

    virtual ~Image() {
        delete[] pixels;
    }


    void find_min_max() {
#pragma omp parallel default(none)
        {
            double local_max = -1.00, local_min = 1.00;
#pragma omp for nowait schedule(static)
            for (size_t i = 0; i < width * height; ++i) {
                pixels[i].calc_YCbCr_from_RGB();
                local_max = std::max(local_max, pixels[i].Y);
                local_min = std::min(local_min, pixels[i].Y);
            }
#pragma omp critical
            {
                min_y = std::min(min_y, local_min);
                max_y = std::max(max_y, local_max);
            }
        }
    }

    void color_adjustments() {
#pragma omp parallel default(none)
        {
#pragma omp for nowait schedule(static)
            for (size_t i = 0; i < width * height; i++) {
                if (max_y != min_y) {
                    pixels[i].Y = (pixels[i].Y - min_y) / (max_y - min_y);
                }
                pixels[i].calc_RGB_from_YcbCr();
            }
        }
    }

    void save(FILE *fp) {
        auto *tmp = (uint8_t *) malloc(sizeof(uint8_t) * 3);
        if (tmp == nullptr) {
            std::string str = "Bad allocation " + std::to_string(sizeof(uint8_t) * 3) + " bytes";
            throw my_exception(str.c_str());
        }
        try {
            fwrite("P6\n", 3, 1, fp);
            std::string res =
                    std::to_string(height) + " " + std::to_string(width) + "\n" + std::to_string(max_brightness) + "\n";
            fwrite(res.c_str(), res.length(), 1, fp);
            for (size_t i = 0; i < width * height; i++) {
                tmp[0] = pixels[i].getR();
                tmp[1] = pixels[i].getG();
                tmp[2] = pixels[i].getB();
                fwrite(tmp, 3, 1, fp);
            }
        } catch (std::exception &e) {
            free(tmp);
            throw e;
        }
        free(tmp);
    }

};


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
        std::cout << "Warning! Illegal num of threads: " << argv[1] << ". Default values will be used.\n";
    }
    FILE *fp = fopen(argv[2], "rb");
    FILE *fp2 = fopen(argv[3], "wb");
    if (fp == nullptr || fp2 == nullptr) {
        std::cout << "It looks like something went wrong. " << strerror(errno) << "\n";
        return -1;
    }
    Image *image;
    try {
        image = new Image(fp);
        double start = omp_get_wtime();
        image->find_min_max();
        image->color_adjustments();
        double finish = omp_get_wtime();
        printf("Threads: %s, time: %f ms\n", argv[1], (finish - start) * 1000);
        image->save(fp2);
    } catch (std::exception &exception) {
        std::cout << "Error: " << exception.what() << "\n";
        delete image;
        fclose(fp);
        fclose(fp2);
        return -1;
    }
    delete image;
    fclose(fp);
    fclose(fp2);
    return 0;
}
