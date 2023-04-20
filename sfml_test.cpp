#include "sfml_test.h"
#include <SFML/Graphics.hpp>

#include <iostream>
#include <vector>
#include <cstring>
#include <chrono>
#include <algorithm>

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
	size_t accum[4] = { 0,0,0,0 };
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

	CreateColorPallet(p_pixel_data, number_of_pixels,3);

	sf::Image res;
	res.create(image_size.x, image_size.y, reinterpret_cast<sf::Uint8*>(raw_pixel_data.data()));
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

	const auto pallet = CreateColorPallet(image);
	timestamps.emplace_back(std::chrono::system_clock::now());

	const auto reduced_image = ApplyColorPallet(image, pallet);
	timestamps.emplace_back(std::chrono::system_clock::now());

	const auto sorted_image = CreateSortedImage(image);
	timestamps.emplace_back(std::chrono::system_clock::now());

	const auto reduced_sorted_image = ApplyColorPallet(sorted_image, pallet);
	timestamps.emplace_back(std::chrono::system_clock::now());

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