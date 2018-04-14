// Cells.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "image.h"

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

int Polish(Image & image, std::vector <Cluster> & book)
{
	// 忽略处于图像边缘的细胞
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

	// 打磨处理
	for (auto & c : book)
	{
		if (c.size() < MINSIZE)
		{
			c.clear();
			continue;
		}

		int remain = (int)c.size();
		for (auto & p : c)
			image((int)p.x, (int)p.y).z = 0;

		while (remain > c.size() * 0.5)
		{
			for (auto & p : c)
			{
				int x = (int)p.x;
				int y = (int)p.y;
				if (image(x, y).z < 0)
					continue;
				if (image(x - 1, y).z < 0 || image(x + 1, y).z < 0 ||
					image(x, y - 1).z < 0 || image(x, y + 1).z < 0)
					image(x, y).z = 1;
			}

			for (auto & p : c)
			{
				auto & pixel = image((int)p.x, (int)p.y);
				if (pixel.z == 1)
				{
					pixel.z = -1;
					remain--;
				}
			}
		}

		int n = (int)c.size();
		for (int i = 0; i < n; i++)
		{
			if (image((int)c[i].x, (int)c[i].y).z == 0)
				c.push_back(c[i]);
		}
	}

	return 0;
}

int Split(Image & image, std::vector <Cluster> & book)
{
	int n = (int)book.size();
	for (int i = 0; i < n; i++)
	{
		if (book[i].size() < MINSIZE)
			continue;

		std::vector <Cluster> temp;
		book[i].Split(temp, (int)(MINSIZE * 1.5), (int)(MAXSIZE * 1.5));
		if (temp.size() == 1)
			continue;

		book[i].clear();
		for (auto & c : temp)
		{
			if (c.size() < MINSIZE)
				continue;
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
//	image.Append("Samples/五等001.tif");
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

	// 融合
	std::vector <Cluster> book;
	Join(image, book);

	// 打磨
	Polish(image, book);

	// 分裂
	Split(image, book);

	// 计数
	int cells = 0;
	int infect = 0;
	for (auto & c : book)
	{
		if (c.size() < MINSIZE)
			continue;

		cells++;
		for (auto & p : c)
		{
			if (image((int)p.x, (int)p.y).x == 2)
			{
				infect++;
				break;
			}
		}
	}

	// 输出结果
	std::cout << "颜色值:" << std::endl;
	std::cout << means << std::endl;
	std::cout << "计数:" << std::endl;
	std::cout << cells << "," << infect << "," << (double)infect / (double)cells << std::endl;

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

int main(int argc, _TCHAR* argv[])
{
	if (Train() == 0)
		return 0;

	if (argc < 2)
	{
		std::cout << "用法:" << std::endl;
		std::cout << "Cells <input_file>" << std::endl;
		std::cout << "或" << std::endl;
		std::cout << "Cells <input_file> <output_file>" << std::endl;
		return 0;
	}

	Image image(argv[1]);
	if (image.empty())
	{
		std::cout << "Loading image failed." << std::endl;
		return -1;
	}

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
	Classify(image, colorrefs);

	// 融合
	std::vector <Cluster> book;
	Join(image, book);

	// 打磨
	Polish(image, book);

	// 分裂
	Split(image, book);

	// 计数
	int cells = 0;
	int infect = 0;
	for (auto & c : book)
	{
		if (c.size() < MINSIZE)
			continue;

		cells++;
		for (auto & p : c)
		{
			if (image((int)p.x, (int)p.y).x == 2)
			{
				infect++;
				break;
			}
		}
	}

	// 输出结果
	std::cout << argv[1] << "," << cells << "," << infect << "," << (double)infect / (double)cells << std::endl;

	// 输出图像
	if (argc < 3)
		return 0;

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

	image.Save(argv[2]);

	return 0;
}
