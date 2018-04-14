#pragma once
#include <set>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <gmm.h>

#define MINSIZE 1200
#define MAXSIZE 3000
#define MINGAIN 500

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
	int Split(std::vector <Cluster> & book, int minsize, int maxsize)
	{
		// check
		if ((int)size() < minsize)
			return 0;

		// prepare data
		auto data = new double[2 * size()];
		for (int i = 0; i < (int)size(); i++)
		{
			data[2 * i] = (*this)[i].x;
			data[2 * i + 1] = (*this)[i].y;
		}

		// split
		double prev = -DBL_MAX;
		for(int k = 1; k < 5; k++)
		{
			std::vector <Cluster> temp;
			double likelihood = GMSplit(data, k, temp);
			if (likelihood < prev + MINGAIN)
				break;
			else
			{
				// update result and continue
				temp.swap(book);
				prev = likelihood;

				// check size
				int done = 1;
				for (auto & c : book)
				{
					if ((int)c.size() > maxsize)
					{
						done = 0;
						break;
					}
				}
				if (done)
					break;
			}
		}

		return 0;
	}

private:
	double GMSplit(const double * data, int k, std::vector <Cluster> & book)
	{
		// create a new instance of a GMM object for float data
		auto gmm = vl_gmm_new(VL_TYPE_DOUBLE, 2, k);

		// set the maximum number of EM iterations to 100
		vl_gmm_set_max_num_iterations(gmm, 100);

		// set the initialization to random selection
		vl_gmm_set_initialization(gmm, VlGMMRand);

		// cluster the data, i.e. learn the GMM
		vl_gmm_cluster(gmm, data, size());

		// get the means, covariances, and priors of the GMM
		// auto means = (double *)vl_gmm_get_means(gmm);
		// auto covariances = (double *)vl_gmm_get_covariances(gmm);
		// auto priors = (double *)vl_gmm_get_priors(gmm);

		// get loglikelihood of the estimated GMM
		auto loglikelihood = vl_gmm_get_loglikelihood(gmm);

		// get the soft assignments of the data points to each cluster
		auto posteriors = (double *)vl_gmm_get_posteriors(gmm);

		// results
		book.resize(k);
		for (int i = 0; i < (int)size(); i++)
		{
			int imax = 0;
			double vmax = 0;
			for (int j = 0; j < k; j++)
			{
				if (posteriors[i * k + j] > vmax)
				{
					imax = j;
					vmax = posteriors[i * k + j];
				}
			}
			book[imax].push_back((*this)[i]);
		}

		// delete gmm
		vl_gmm_delete(gmm);

		// return likelihood
		return loglikelihood;
	}
};
