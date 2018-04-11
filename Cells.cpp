// Cells.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "image.h"

void Train()
{
	// Loading images
	Image image;
	image.Append("30张细胞图/五等001.tif");
	image.Append("30张细胞图/五等002.tif");
	image.Append("30张细胞图/五等003.tif");
	image.Append("30张细胞图/五等004.tif");
	image.Append("30张细胞图/五等005.tif");
	image.Append("30张细胞图/五等006.tif");
	if (image.empty())
		return;

	// Pre-processing
	for (int i = 0; i < image.GetWidth(); i++)
	{
		for (int j = 0; j < image.GetHeight(); j++)
		{
			auto & p = image(i, j);
			auto e = (p.x + p.y + p.z) / 3;
			p -= Pixel(e, e, e);
		}
	}

	// Clustering
	std::vector <int> labels;
	std::vector <cv::Point3f> centers;
	cv::kmeans(image, 3, labels,
		cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 10, 1.0),
		3, cv::KMEANS_PP_CENTERS, centers);

	// Output
	std::cout << centers << std::endl;
	centers[0] = Pixel(255, 255, 255);
	centers[1] = Pixel(127, 127, 127);
	centers[2] = Pixel(0, 0, 0);
	for (int i = 0; i < (int)image.size(); i++)
		image[i] = centers[labels[i]];
	image.Save("test.png");
}

void Classify()
{

}

void Join()
{

}

void Split()
{

}

void Process()
{

}

int main(int argc, _TCHAR* argv[])
{
	Train();
	return 0;
}

/*
-0.69038421, 1.7464634, -1.0380501;
21.049711, -12.036834, -8.9078255;
11.166588, -25.756166, 14.589508
*/
