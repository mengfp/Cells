#pragma once
#include <set>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

typedef cv::Point3f Pixel;
typedef cv::Point2f Point;

template <typename T> double distance(const T & a, const T & b)
{
	auto x = a - b;
	return sqrt(x.ddot(x));
}

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

	int Width()
	{
		return width;
	}

	int Height()
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

class Cluster : public std::vector <Point>
{
public:
	Point Center()
	{
		if (empty())
			return Point(0, 0);

		Point c(0, 0);
		for (auto & p : *this)
			c += p;
		return (1.0 / (int)size()) * c;
	}

	double Radius()
	{
		double r = 0;
		Point c = Center();
		for (auto & p : *this)
			r = std::max(r, distance(p, c));
		return r;
	}

	int Split(std::vector <Cluster> & book, int k)
	{
		return 0;
	}
};
