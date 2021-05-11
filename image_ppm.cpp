#include "image_ppm.h"
#include <iostream>
#include <fstream>

// Read integer value and one symbol after
uint32_t read_int(std::ifstream &in) {
    uint32_t ans = 0;
    char buf;
    in.read(&buf, 1);
    while ('0' <= buf && buf <= '9') {
        ans = ans * 10 + (buf - '0');
        in.read(&buf, 1);
    }
    return ans;
}

image_ppm::image_ppm(std::ifstream &in) {
    if (!in.is_open()) {
        throw std::invalid_argument("File doesn't exists.");
    }
    char buff[3];
    in.read(buff, 3);
    if (buff[0] != 'P' || buff[1] != '6' || buff[2] != '\n') {
        throw std::invalid_argument("Wrong file format");
    }
    height_ = read_int(in);
    width_ = read_int(in);
    max_brightness_ = read_int(in);
    if (max_brightness_ > 255) {
        throw std::invalid_argument("Max brightness > 255 not supported");
    }
    pixels_.resize(width_ * height_);
    for (auto &pixel : pixels_) {
        in.read(buff, 3);
        const auto r = static_cast<uint8_t>(buff[0]);
        const auto g = static_cast<uint8_t>(buff[1]);
        const auto b = static_cast<uint8_t>(buff[2]);
        pixel.init_by_RGB(r, g, b);
    }
    in.read(buff, 1); // newline
    if (!in.eof()) {
        std::cout << "Warning! Pixels were read, but EOF not reached\n";
    }
}

void image_ppm::swap(image_ppm &other) noexcept {
    using std::swap;
    swap(height_, other.height_);
    swap(width_, other.width_);
    swap(max_brightness_, other.max_brightness_);
    swap(min_y_, other.min_y_);
    swap(max_y_, other.max_y_);
    swap(pixels_, other.pixels_);
}

image_ppm &image_ppm::operator=(const image_ppm &other) {
    image_ppm tmp(*this);
    swap(tmp);
    return *this;
}

void image_ppm::improve_contrast() noexcept {
    update_min_max_y();
    increase_linearly_y();
}

void image_ppm::update_min_max_y() noexcept {
#pragma omp parallel default(none)
    {
        double local_max = -1.00, local_min = 1.00;
#pragma omp for nowait schedule(static)
        for (auto &pixel : pixels_) {
            pixel.to_YCbCr_from_RGB();
            local_max = std::max(local_max, pixel.Y);
            local_min = std::min(local_min, pixel.Y);
        }
#pragma omp critical
        {
            min_y_ = std::min(min_y_, local_min);
            max_y_ = std::max(max_y_, local_max);
        }
    }
}

void image_ppm::increase_linearly_y() noexcept {
#pragma omp parallel default(none)
    {
#pragma omp for nowait schedule(static)
        for (auto &pixel : pixels_) {
            if (max_y_ != min_y_) {
                pixel.Y = (pixel.Y - min_y_) / (max_y_ - min_y_);
            }
            pixel.to_RGB_from_YcbCr();
        }
    }
}

void image_ppm::save(std::ofstream &out) const {
    char buff[3];
    out.write("P6\n", 3);
    std::string res =
            std::to_string(height_) + " " + std::to_string(width_) + "\n" + std::to_string(max_brightness_) + "\n";
    out.write(res.c_str(), res.length());
    for (const auto &pixel : pixels_) {
        buff[0] = pixel.get_r();
        buff[1] = pixel.get_g();
        buff[2] = pixel.get_b();
        out.write(buff, 3);
    }
}

void image_ppm::pixel::init_by_RGB(uint8_t r, uint8_t g, uint8_t b) noexcept {
    R = r / 255.00;
    G = g / 255.00;
    B = b / 255.00;
    to_YCbCr_from_RGB();
}

void image_ppm::pixel::to_YCbCr_from_RGB() noexcept {
    Y = kr * R + kg * G + kb * B;
    Cb = (B - Y) / (1 - kb) / 2;
    Cr = (R - Y) / (1 - kr) / 2;
}

void image_ppm::pixel::to_RGB_from_YcbCr() noexcept {
    R = 2 * Cr * (1 - kr) + Y;
    B = 2 * Cb * (1 - kb) + Y;
    G = (Y - kb * B - kr * R) / kg;
}

uint8_t image_ppm::pixel::get_r() const noexcept {
    return std::min(static_cast<uint8_t>(255), std::max(static_cast<uint8_t>(0), static_cast<uint8_t>(R * 255)));
}

uint8_t image_ppm::pixel::get_b() const noexcept {
    return std::min(static_cast<uint8_t>(255), std::max(static_cast<uint8_t>(0), static_cast<uint8_t>(B * 255)));
}

uint8_t image_ppm::pixel::get_g() const noexcept {
    return std::min(static_cast<uint8_t>(255), std::max(static_cast<uint8_t>(0), static_cast<uint8_t>(G * 255)));
}
