#pragma once
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

typedef cv::Point3f Pixel;

class Image : public std::vector <Pixel>
{
private:
	int width;
	int height;

public:
	Image(int width = 0, int height = 0) :
		width(width), height(height)
	{
		resize(width * height);
	}

	Image(const char * filename)
	{
		Load(filename);
	}

	int GetWidth()
	{
		return width;
	}

	int GetHeight()
	{
		return height;
	}

	Pixel & operator () (int x, int y)
	{
		return (*this)[x + y * width];
	}

	int Load(const char * filename)
	{
		cv::Mat im = cv::imread(filename);
		if (im.empty())
			return -1;
		width = im.cols;
		height = im.rows;
		resize(width * height);
		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				auto & p = *im.ptr<cv::Vec3b>(y, x);
				(*this)(x, y) = Pixel(p[2], p[1], p[0]);
			}
		}
		return 0;
	}

	int Append(const char * filename)
	{
		cv::Mat im = cv::imread(filename);
		if (im.empty())
			return 0;
		if (width == 0)
			width = im.cols;
		else if(im.cols != width)
			return -1;
		resize(width * (height + im.rows));
		for (int y = 0; y < im.rows; y++)
		{
			for (int x = 0; x < width; x++)
			{
				auto & p = *im.ptr<cv::Vec3b>(y, x);
				(*this)(x, height + y) = Pixel(p[2], p[1], p[0]);
			}
		}
		height += im.rows;
		return 0;
	}

	int Save(const char * filename)
	{
		cv::Mat im(height, width, CV_8UC3);
		for (int x = 0; x < width; x++)
		{
			for (int y = 0; y < height; y++)
			{
				auto & pixel = (*this)(x, y);
				auto & p = *im.ptr<cv::Vec3b>(y, x);
				p[0] = (uchar)std::max(0.0f, std::min(255.0f, pixel.z));
				p[1] = (uchar)std::max(0.0f, std::min(255.0f, pixel.y));
				p[2] = (uchar)std::max(0.0f, std::min(255.0f, pixel.x));
			}
		}
		return cv::imwrite(filename, im) ? 0 : -1;
	}
};
