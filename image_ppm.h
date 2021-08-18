#pragma once
#include <cstdint>
#include <iosfwd>
#include <vector>

class image_ppm {
public:
	explicit image_ppm(std::ifstream& in);
	~image_ppm() noexcept = default;
	image_ppm(const image_ppm& other) = default;

	void swap(image_ppm& other) noexcept;
	image_ppm& operator=(const image_ppm& other);

	void improve_contrast() noexcept;
	void save(std::ofstream& out) const;

private:
	class pixel {
	public:
		double R = 0, G = 0, B = 0;
		double Y = 0, Cb = 0, Cr = 0;
		void init_by_RGB(uint8_t r, uint8_t g, uint8_t b) noexcept;
		void to_YCbCr_from_RGB() noexcept;
		void to_RGB_from_YcbCr() noexcept;
		uint8_t get_r() const noexcept;
		uint8_t get_g() const noexcept;
		uint8_t get_b() const noexcept;
	private:
		constexpr static double kr = 0.299;
		constexpr static double kg = 0.587;
		constexpr static double kb = 0.114;
	};

	void update_min_max_y() noexcept;
	void increase_linearly_y() noexcept;

	uint32_t height_;
	uint32_t width_;
	uint32_t max_brightness_;
	std::vector<pixel> pixels_;
	double min_y_ = 1.00, max_y_ = -1.00;
};
