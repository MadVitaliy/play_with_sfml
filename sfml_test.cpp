#include "sfml_test.h"
#include <SFML/Graphics.hpp>

#include <iostream>
#include <vector>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <map>
#include <array>

#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/parallel_reduce.h>

sf::Uint32 Rgb2Lab(sf::Uint32 i_rgba)
{
	const auto* p_rgba = reinterpret_cast<const sf::Uint8*>(&i_rgba);

	double rgb[3], xyz[3], lab[3];
	// rgb to xyz
	for (size_t i = 0; i < 3; i++)
	{
		rgb[i] = p_rgba[i] / 255.0;
		if (rgb[i] > .04045)
			rgb[i] = std::pow((rgb[i] + 0.055) / 1.055, 2.4);
		else
			rgb[i] /= 12.92;
		rgb[i] *= 100.;
	}

	xyz[0] = rgb[0] * .412453 + rgb[1] * .357580 + rgb[2] * .180423;
	xyz[1] = rgb[0] * .212671 + rgb[1] * .715160 + rgb[2] * .072169;
	xyz[2] = rgb[0] * .019334 + rgb[1] * .119193 + rgb[2] * .950527;

	// xyz to lab
	xyz[0] = xyz[0] / 95.0489;
	xyz[1] = xyz[1] / 100.0;
	xyz[2] = xyz[2] / 108.8840;
	// f(t)
	constexpr double d = 6. / 29.;
	constexpr double dp2 = d * d;
	constexpr double dp3 = d * d * d;

	for (size_t i = 0; i < 3; i++)
	{
		if (xyz[i] > dp3)
			xyz[i] = std::pow(xyz[i], 1. / 3.);
		else
			xyz[i] = xyz[i] / (3 * dp2) + 4. / 29.;
	}

	lab[0] = 116. * xyz[1] - 16.;
	lab[1] = 500. * (xyz[0] - xyz[1]);
	lab[2] = 200. * (xyz[1] - xyz[2]);
	if (lab[0] < 0 || lab[0] > 100 ||
		lab[1] < -128 || lab[0] > 127 ||
		lab[2] < -128 || lab[2] > 127)
		std::cout << "overflow lab\n";

	sf::Uint32 LAB = 0;
	auto* p_lab = reinterpret_cast<char*>(&LAB);
	p_lab[0] = lab[0];
	p_lab[1] = lab[1];
	p_lab[2] = lab[2];
	return LAB;
}

sf::Uint32 Lab2Rgb(sf::Uint32 i_lab)
{
	const auto* p_lab = reinterpret_cast<const char*>(&i_lab);
	//std::cout << int(p_lab[0]) << ' ' << int(p_lab[1]) << ' ' << int(p_lab[2]) << " => ";
	double xyz[3], rgb[3];
	// lab to xyz
	xyz[1] = (p_lab[0] + 16.) / 116.;
	xyz[0] = xyz[1] + p_lab[1] / 500.;
	xyz[2] = xyz[1] - p_lab[2] / 200.;

	constexpr double d = 6. / 29.;
	constexpr double dp2 = d * d;
	for (size_t i = 0; i < 3; i++)
	{
		if (xyz[i] > d)
			xyz[i] = std::pow(xyz[i], 3.);
		else
			xyz[i] = 3 * dp2 * (xyz[i] - 4. / 29.);
	}

	xyz[0] *= 95.0489;
	xyz[1] *= 100.0;
	xyz[2] *= 108.8840;
	// xyz[0] /= 100.;
	// xyz[1] /= 100.;
	// xyz[2] /= 100.;

	// xyz to rgb
	rgb[0] = xyz[0] * 3.2406 + xyz[1] * (-1.5372) + xyz[2] * (-0.4986);
	rgb[1] = xyz[0] * (-0.9689) + xyz[1] * 1.8758 + xyz[2] * 0.0415;
	rgb[2] = xyz[0] * 0.0557 + xyz[1] * (-0.2040) + xyz[2] * 1.0570;

	for (size_t i = 0; i < 3; i++)
	{
		rgb[i] /= 100.;
		if (rgb[i] > 0.0031308)
			rgb[i] = 1.055 * std::pow(rgb[i], 1. / 2.4) - 0.055;
		else
			rgb[i] = 12.92 * rgb[i];
		rgb[i] *= 255.;
	}
	for (size_t i = 0; i < 3; i++)
		if (rgb[i] >= 256.)
			std::cout << "overflow rgb\n";


	sf::Uint32 RGB = 0;
	auto* p_rgb = reinterpret_cast<sf::Uint8*>(&RGB);
	p_rgb[0] = rgb[0];
	p_rgb[1] = rgb[1];
	p_rgb[2] = rgb[2];
	p_rgb[3] = 255;
	return RGB;
}


sf::Image FlipFlopConvert(const sf::Image& i_image)
{
	const auto image_size = i_image.getSize();
	std::vector<sf::Uint32> pixel_data(image_size.x * image_size.y);
	auto* p_pixel_data = pixel_data.data();
	std::memcpy(p_pixel_data, i_image.getPixelsPtr(), pixel_data.size() * 4);
	//for (auto& pixel : raw_pixel_data)
	//{
	//	auto* p_rgba = reinterpret_cast<char*>(&pixel);
	//	std::cout << int(p_rgba[3]) << "\n";
	//}


	tbb::parallel_for(tbb::blocked_range<size_t>(0, pixel_data.size()),
		[&pixel_data](const tbb::blocked_range<size_t>& i_r) {
			for (size_t i = i_r.begin(); i < i_r.end(); ++i)
				pixel_data[i] = Rgb2Lab(pixel_data[i]);


		});


	tbb::parallel_for(tbb::blocked_range<sf::Uint32*>(p_pixel_data, p_pixel_data + pixel_data.size()),
		[&pixel_data](const tbb::blocked_range<sf::Uint32*>& i_r) {
			for (sf::Uint32* p_pixel = i_r.begin(); p_pixel != i_r.end(); ++p_pixel)
			{
				auto* p_lab = reinterpret_cast<char*>(p_pixel);
				p_lab[1] = 0;
				p_lab[2] = 0;
			}
		});

	tbb::parallel_for(tbb::blocked_range<size_t>(0, pixel_data.size()),
		[&pixel_data](const tbb::blocked_range<size_t>& i_r) {
			for (size_t i = i_r.begin(); i < i_r.end(); ++i)
				pixel_data[i] = Lab2Rgb(pixel_data[i]);
		});

	sf::Image res;
	res.create(image_size.x, image_size.y, reinterpret_cast<sf::Uint8*>(pixel_data.data()));
	return res;
}

size_t Distance(const sf::Uint32* i_color_1, const sf::Uint32* i_color_2)
{
	const auto* c1 = reinterpret_cast<const sf::Uint8*>(i_color_1);
	const auto* c2 = reinterpret_cast<const sf::Uint8*>(i_color_2);
	return static_cast<size_t>(c1[0]) * c2[0] +
		static_cast<size_t>(c1[1]) * c2[1] +
		static_cast<size_t>(c1[2]) * c2[2] +
		static_cast<size_t>(c1[3]) * c2[3];
}

sf::Uint32 Average(const sf::Uint32* ip_pixels, size_t i_number_of_pixels)
{
	std::array<size_t, 4> accum = { 0,0,0,0 };
	for (size_t i = 0; i < i_number_of_pixels; i++)
	{
		const auto* p = reinterpret_cast<const sf::Uint8*>(ip_pixels + i);
		accum[0] += p[0];
		accum[1] += p[1];
		accum[2] += p[2];
		accum[3] += p[3];
	}
	sf::Uint32 average = 0;
	sf::Uint8* p = reinterpret_cast<sf::Uint8*>(&average);
	p[0] = accum[0] / i_number_of_pixels;
	p[1] = accum[1] / i_number_of_pixels;
	p[2] = accum[2] / i_number_of_pixels;
	p[3] = accum[3] / i_number_of_pixels;
	return average;
}

sf::Uint32 PAverage(const sf::Uint32* ip_pixels, size_t i_number_of_pixels)
{
	std::array<size_t, 4> accum = { 0,0,0,0 };
	for (size_t i = 0; i < i_number_of_pixels; i++)
	{
		const auto* p = reinterpret_cast<const sf::Uint8*>(ip_pixels + i);
		accum[0] += p[0];
		accum[1] += p[1];
		accum[2] += p[2];
		accum[3] += p[3];
	}
	sf::Uint32 average = 0;
	sf::Uint8* p = reinterpret_cast<sf::Uint8*>(&average);
	p[0] = accum[0] / i_number_of_pixels;
	p[1] = accum[1] / i_number_of_pixels;
	p[2] = accum[2] / i_number_of_pixels;
	p[3] = accum[3] / i_number_of_pixels;
	return average;
}

void Sort(sf::Uint32* iop_pixels, size_t i_number_of_pixels, size_t i_chanel)
{

	for (size_t i = 0; i < i_number_of_pixels - 1; i++)
		for (size_t j = 0; j < i_number_of_pixels - i - 1; j++)
		{
			if (reinterpret_cast<sf::Uint8*>(iop_pixels + j)[i_chanel] > reinterpret_cast<sf::Uint8*>(iop_pixels + j + 1)[i_chanel])
			{
				sf::Uint32 temp = iop_pixels[j];
				iop_pixels[j] = iop_pixels[j + 1];
				iop_pixels[j + 1] = temp;
				//std::swap(indicies[j], indicies[j + 1]);
			}
		}
}

size_t WidestChannel(const sf::Uint32* ip_pixels, size_t i_number_of_pixels)
{
	sf::Uint8 min_r = 0, min_g = 0, min_b = 0, max_r = 0, max_g = 0, max_b = 0;
	for (size_t i = 0; i < i_number_of_pixels; i++)
	{
		const auto* p = reinterpret_cast<const sf::Uint8*>(ip_pixels + i);
		min_r = std::min(min_r, p[0]);
		min_g = std::min(min_g, p[1]);
		min_b = std::min(min_b, p[2]);
		max_r = std::max(max_r, p[0]);
		max_g = std::max(max_g, p[1]);
		max_b = std::max(max_b, p[2]);
	}
	sf::Uint8 d_r = max_r - min_r, d_g = max_g - min_g, d_b = max_b - min_b;
	std::cout << "Ranges: " << int(d_r) << " " << int(d_g) << " " << int(d_b) << " " << std::endl;
	if (d_r >= d_b && d_r >= d_g)
		return 0;
	if (d_g >= d_r && d_g >= d_b)
		return 1;
	return 2;
}


void CreateHistogram(sf::Uint32* ip_pixels, size_t i_number_of_pixels, size_t i_devisions = 4)
{
	std::map<sf::Uint8, size_t> hist;
	size_t chanel = WidestChannel(ip_pixels, i_number_of_pixels);
	for (size_t i = 0; i < i_number_of_pixels; i++)
	{
		const auto* p = reinterpret_cast<const sf::Uint8*>(ip_pixels + i);
		auto& colummn = hist[p[chanel]];
		++colummn;
	}
	for (auto const& [chanel_value, count] : hist)
		std::cout << int(chanel_value) << '\t' << count << '\n';

}

void CreateHistogram(const sf::Image& i_image, size_t i_devisions = 4)
{
	const auto image_size = i_image.getSize();
	std::vector<sf::Uint32> raw_pixel_data(image_size.x * image_size.y);
	auto* p_pixel_data = raw_pixel_data.data();
	std::memcpy(p_pixel_data, i_image.getPixelsPtr(), raw_pixel_data.size() * 4);
	CreateHistogram(p_pixel_data, raw_pixel_data.size());
}

std::vector<sf::Uint32> CreateColorPallet(sf::Uint32* ip_pixels, size_t i_number_of_pixels, size_t i_devisions = 4)
{
	if (i_devisions == 0) //calculate averege
	{
		const auto average = Average(ip_pixels, i_number_of_pixels);
		for (size_t i = 0; i < i_number_of_pixels; i++)
			ip_pixels[i] = average;

		return { average };
	}

	const auto channel = WidestChannel(ip_pixels, i_number_of_pixels);
	std::cout << "Div " << i_devisions << " channel to sort" << channel << "\n";
	Sort(ip_pixels, i_number_of_pixels, channel);

	--i_devisions;
	const auto first_half_of_pixels = i_number_of_pixels / 2;
	auto pallet_1 = CreateColorPallet(ip_pixels, first_half_of_pixels, i_devisions);
	auto pallet_2 = CreateColorPallet(ip_pixels + first_half_of_pixels, i_number_of_pixels - first_half_of_pixels, i_devisions);
	pallet_1.insert(pallet_1.cend(), pallet_2.cbegin(), pallet_2.cend());
	return pallet_1;
}

std::vector<sf::Uint32> CreateColorPallet(const sf::Image& i_image)
{
	const auto image_size = i_image.getSize();
	std::vector<sf::Uint32> raw_pixel_data(image_size.x * image_size.y);
	auto* p_pixel_data = raw_pixel_data.data();
	std::memcpy(p_pixel_data, i_image.getPixelsPtr(), raw_pixel_data.size() * 4);


	return CreateColorPallet(p_pixel_data, raw_pixel_data.size());
}

sf::Image ApplyColorPallet(const sf::Image& i_image, const std::vector<sf::Uint32>& i_pallet)
{
	const auto image_size = i_image.getSize();
	std::vector<sf::Uint32> raw_pixel_data(image_size.x * image_size.y);
	auto* p_pixel_data = raw_pixel_data.data();
	std::memcpy(p_pixel_data, i_image.getPixelsPtr(), raw_pixel_data.size() * 4);

	for (size_t i = 0; i < raw_pixel_data.size(); i++)
	{
		sf::Uint32 closest_color = raw_pixel_data[i];
		size_t dist = 0;
		for (const auto color : i_pallet)
		{
			const auto temp_dist = Distance(p_pixel_data + i, &color);
			if (temp_dist > dist) {
				dist = temp_dist;
				closest_color = color;
			}
		}
		raw_pixel_data[i] = closest_color;
	}
	auto raw_pixel_data_copy = raw_pixel_data;
	std::sort(raw_pixel_data_copy.begin(), raw_pixel_data_copy.end());
	auto it = std::unique(raw_pixel_data_copy.begin(), raw_pixel_data_copy.end());
	std::cout << "Image contains " << std::distance(raw_pixel_data_copy.begin(), it) << std::endl;
	bool out_of_pallet = std::any_of(raw_pixel_data.cbegin(), raw_pixel_data.cend(), [&i_pallet](sf::Uint32 i_pixel)
		{
			return	std::find(i_pallet.cbegin(), i_pallet.cend(), i_pixel) == i_pallet.cend();
		});

	if (out_of_pallet)
		std::cout << "something wrong" << std::endl;

	sf::Image res;
	res.create(image_size.x, image_size.y, reinterpret_cast<sf::Uint8*>(raw_pixel_data.data()));
	return res;
}

sf::Image CreateSortedImage(const sf::Image& i_image)
{
	const auto image_size = i_image.getSize();
	const auto number_of_pixels = image_size.x * image_size.y;
	std::vector<sf::Uint32> raw_pixel_data(number_of_pixels);
	const auto raw_data_size = number_of_pixels * 4;
	auto* p_pixel_data = raw_pixel_data.data();
	std::memcpy(p_pixel_data, i_image.getPixelsPtr(), raw_data_size);

	CreateColorPallet(p_pixel_data, number_of_pixels);

	sf::Image res;
	res.create(image_size.x, image_size.y, reinterpret_cast<sf::Uint8*>(raw_pixel_data.data()));
	return res;

}

sf::Image UniformQuantizedImage(const sf::Image& i_image, size_t i_n = 3)
{
	const auto image_size = i_image.getSize();
	std::vector<sf::Uint32> pixel_data(image_size.x * image_size.y);
	auto* p_pixel_data = pixel_data.data();
	std::memcpy(p_pixel_data, i_image.getPixelsPtr(), pixel_data.size() * 4);

	tbb::parallel_for(tbb::blocked_range<size_t>(0, pixel_data.size()),
		[p_pixel_data, i_n](const tbb::blocked_range<size_t>& i_r) {
			for (size_t i = i_r.begin(); i < i_r.end(); ++i)
			{
				auto* p_raw_pixel_data = reinterpret_cast<sf::Uint8*>(p_pixel_data + i);
				for (size_t j = 0; j < 4; j++)
					p_raw_pixel_data[j] = std::round(p_raw_pixel_data[j] * (i_n / 255.)) * 255. / i_n;
			}
		});

	sf::Image res;
	res.create(image_size.x, image_size.y, reinterpret_cast<sf::Uint8*>(pixel_data.data()));
	return res;
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		std::cout << "Pass image path as a command line argument" << std::endl;
		return 1;
	}

	std::string image_path(argv[1]);

	sf::Image image;
	if (image.loadFromFile(image_path))
		std::cout << "Background image was loaded" << std::endl;
	const auto image_size = image.getSize();
	std::vector<std::chrono::time_point<std::chrono::system_clock>> timestamps;
	timestamps.emplace_back(std::chrono::system_clock::now());

	CreateHistogram(image);
	const auto reduced_image = FlipFlopConvert(image);
	timestamps.emplace_back(std::chrono::system_clock::now());
	const auto sorted_image = UniformQuantizedImage(image, 6);
	timestamps.emplace_back(std::chrono::system_clock::now());
	const auto reduced_sorted_image = image;
	timestamps.emplace_back(std::chrono::system_clock::now());

	/*
	const auto pallet = CreateColorPallet(image);
	timestamps.emplace_back(std::chrono::system_clock::now());
	//return 3;

	const auto reduced_image = ApplyColorPallet(image, pallet);
	timestamps.emplace_back(std::chrono::system_clock::now());

	const auto sorted_image = CreateSortedImage(image);
	timestamps.emplace_back(std::chrono::system_clock::now());

	const auto reduced_sorted_image = ApplyColorPallet(sorted_image, pallet);
	timestamps.emplace_back(std::chrono::system_clock::now());

	*/


	for (size_t i = 1; i < timestamps.size(); ++i)
	{
		std::chrono::duration<int64_t, std::nano> time = timestamps[i] - timestamps[i - 1];
		std::cout << "Period " << i << ": " << time.count() / 1000000.0 << std::endl;
	}


	sf::Texture background_texture;
	background_texture.loadFromImage(image);
	sf::Sprite background(background_texture);

	sf::Texture sorted_image_texture;
	sorted_image_texture.loadFromImage(sorted_image);
	sf::Sprite sorted_image_sprite(sorted_image_texture);
	sorted_image_sprite.setPosition(sf::Vector2f(0, image_size.y));

	sf::Texture reduced_sorted_image_texture;
	reduced_sorted_image_texture.loadFromImage(reduced_sorted_image);
	sf::Sprite reduced_sorted_image_sprite(reduced_sorted_image_texture);
	reduced_sorted_image_sprite.setPosition(sf::Vector2f(image_size.x, image_size.y));

	sf::Texture reduced_image_texture;
	reduced_image_texture.loadFromImage(reduced_image);
	sf::Sprite reduced_image_sprite(reduced_image_texture);
	reduced_image_sprite.setPosition(sf::Vector2f(image_size.x, 0));

	auto window = sf::RenderWindow{ { image_size.x * 2, image_size.y * 2}, "sfml_test" };
	window.setFramerateLimit(60);
	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}

		window.clear();

		window.draw(background);
		window.draw(reduced_image_sprite);
		window.draw(sorted_image_sprite);
		window.draw(reduced_sorted_image_sprite);

		window.display();
	}
}