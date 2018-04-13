// Cells.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "image.h"

#define SIZEMIN 1000
#define SIZEMAX 3000
#define ODDNESS 0.2

int Classify(Image & image, const std::vector <Pixel> & means)
{
	for (int j = 0; j < image.Height(); j++)
	{
		for (int i = 0; i < image.Width(); i++)
		{
			auto & p = image(i, j);
			auto a = distance(p, means[0]);
			auto b = distance(p, means[1]);
			auto c = distance(p, means[2]);
			auto best = std::min(std::min(a, b), c);
			if (best == a)
				p = Pixel(0, -1, -1);
			else if (best == b)
				p = Pixel(1, -1, -1);
			else
				p = Pixel(2, -1, -1);
		}
	}

	return 0;
}

int Join(Image & image, std::vector <Cluster> & book)
{
	Pixel background = { 0, -1, -1 };
	for (int j = 0; j < image.Height(); j++)
	{
		for (int i = 0; i < image.Width(); i++)
		{
			auto & pixel = image(i, j);
			if (pixel.x > 0)
			{
				auto & left = i > 0 ? image(i - 1, j) : background;
				auto & up = j > 0 ? image(i, j - 1) : background;
				if (left.y >= 0)
				{
					pixel.y = left.y;
					book[(int)pixel.y].push_back(Point((float)i, (float)j));
					if (up.y >= 0 && up.y != pixel.y)
					{
						auto & c = book[(int)pixel.y];
						for (auto & p : c)
						{
							image((int)p.x, (int)p.y).y = up.y;
							book[(int)up.y].push_back(p);
						}
						c.clear();
					}
				}
				else if (up.x > 0)
				{
					pixel.y = up.y;
					book[(int)pixel.y].push_back(Point((float)i, (float)j));
				}
				else
				{
					pixel.y = (float)book.size();
					book.push_back(Cluster());
					book[(int)pixel.y].push_back(Point((float)i, (float)j));
				}
			}
		}
	}
	return 0;
}

int Check(Image & image, std::vector <Cluster> & book)
{
	for (int i = 0; i < image.Width(); i++)
	{
		int k = (int)image(i, 0).y;
		if (k >= 0)
			book[k].clear();
		k = (int)image(i, image.Height() - 1).y;
		if (k >= 0)
			book[k].clear();
	}

	for (int j = 0; j < image.Height(); j++)
	{
		int k = (int)image(0, j).y;
		if (k >= 0)
			book[k].clear();
		k = (int)image(image.Width() - 1, j).y;
		if (k >= 0)
			book[k].clear();
	}

	for (auto & c : book)
	{
		if (!c.empty() && c.size() < SIZEMIN)
			c.clear();
	}

	return 0;
}

int Split(Image & image, std::vector <Cluster> & book)
{
	int n = (int)book.size();
	for (int i = 0; i < n; i++)
	{
		if (book[i].empty())
			continue;

		if (book[i].size() <= SIZEMAX && book[i].Odd() <= ODDNESS)
			continue;
	
		int k = 1;
		int ok = 0;
		std::vector <Cluster> temp;
		while (!ok)
		{
			k++;
			ok = 1;
			temp.clear();
			book[i].Split(temp, k);
			for (auto & c : temp)
			{
				if (c.size() > SIZEMAX || c.Odd() > ODDNESS)
				{
					ok = 0;
					break;
				}
			}
		}

		book[i].clear();
		for (auto & c : temp)
		{
			for (auto & p : c)
				image((int)p.x, (int)p.y).y = (float)book.size();
			book.push_back(Cluster());
			book.back().swap(c);
		}
	}

	return 0;
}

int Train()
{
	// 读取图像
	Image image;
	image.Append("Samples/五等001.tif");
//	image.Append("Samples/五等002.tif");
//	image.Append("Samples/五等003.tif");
//	image.Append("Samples/五等004.tif");
//	image.Append("Samples/五等005.tif");
//	image.Append("Samples/五等006.tif");
	if (image.empty())
		return -1;

	// 预处理
	for (int j = 0; j < image.Height(); j++)
	{
		for (int i = 0; i < image.Width(); i++)
		{
			auto & p = image(i, j);
			auto e = (p.x + p.y + p.z) / 3;
			p -= Pixel(e, e, e);
		}
	}

	// 颜色分类
	std::vector <int> labels;
	std::vector <Pixel> means;
	cv::kmeans(image, 3, labels,
		cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 10, 1.0),
		3, cv::KMEANS_PP_CENTERS, means);
	Classify(image, means);

	// 细胞融合
	std::vector <Cluster> book;
	Join(image, book);
	Check(image, book);

	// 细胞分裂
	Split(image, book);
	Check(image, book);

	// 输出结果
	std::cout << means << std::endl;
	for (auto & c : book)
	{
		if(!c.empty())
			std::cout << c.size() << "," << c.Odd() << std::endl;
	}

	for (auto & c : book)
	{
		for (auto & p : c)
		{
			int x = (int)p.x;
			int y = (int)p.y;
			auto tag = image(x, y).y;
			if (image(x - 1, y).y != tag || image(x + 1, y).y != tag ||
				image(x, y - 1).y != tag || image(x, y + 1).y != tag)
				image(x, y).x = 3;
		}
	}

	for (int j = 0; j < image.Height(); j++)
	{
		for (int i = 0; i < image.Width(); i++)
		{
			auto & p = image(i, j);
			if (p.x == 0)
				p = Pixel(255, 255, 255);
			else if (p.x == 1)
				p = Pixel(127, 127, 127);
			else if (p.x == 2)
				p = Pixel(0, 0, 0);
			else
				p = Pixel(255, 0, 0);
		}
	}
	image.Save("train.png");

	return 0;
}

const std::vector <Pixel> colorrefs =
{
	{ -0.69038421f, 1.7464634f, -1.0380501f },
	{ 21.049711f, -12.036834f, -8.9078255f },
	{ 11.166588f, -25.756166f, 14.589508f }
};

#include <gmm.h>

int TestGMM()
{
	const float data[] = 
	{
		0, 0, 0, 1, 0, 2, 1, 0, 1, 1, 1, 2, 2, 0, 2, 1, 2, 2, 
		10, 0, 10, 1, 11, 0, 11, 1,
		0, 10, 0, 11, 1, 10, 1, 11 
	};

	// create a new instance of a GMM object for float data
	auto gmm = vl_gmm_new(VL_TYPE_FLOAT, 2, 3);

	// set the maximum number of EM iterations to 100
	vl_gmm_set_max_num_iterations(gmm, 100);

	// set the initialization to random selection
	vl_gmm_set_initialization(gmm, VlGMMRand);

	// cluster the data, i.e. learn the GMM
	vl_gmm_cluster(gmm, data, 17);

	// get the means, covariances, and priors of the GMM
	auto means = (float *)vl_gmm_get_means(gmm);
	auto covariances = (float *)vl_gmm_get_covariances(gmm);
	auto priors = (float *)vl_gmm_get_priors(gmm);

	// get loglikelihood of the estimated GMM
	auto loglikelihood = vl_gmm_get_loglikelihood(gmm);

	// get the soft assignments of the data points to each cluster
	auto posteriors = (float *)vl_gmm_get_posteriors(gmm);

	for (int i = 0; i < 2 * 3; i++)
		std::cout << means[i] << (i < 2 * 3 - 1 ? "," : "\n");

	for (int i = 0; i < 2 * 3; i++)
		std::cout << covariances[i] << (i < 2 * 3 - 1 ? "," : "\n");

	for (int i = 0; i < 3; i++)
		std::cout << priors[i] << (i < 3 - 1 ? "," : "\n");

	for (int i = 0; i < 17; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			std::cout << posteriors[i * 3 + j] << (j < 3 - 1 ? "," : "\n");
		}
	}

	return 0;
}

int main(int argc, _TCHAR* argv[])
{

//	TestGMM();
//	return 0;

	Train();

	if (argc < 2)
		return 0;

	Image image(argv[1]);
	if (image.empty())
	{
		std::cout << "Loading image failed." << std::endl;
		return 1;
	}

	Classify(image, colorrefs);

	return 0;
}
