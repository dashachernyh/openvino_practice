#include "classifier.hpp"

#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/filesystem.hpp>
#include <inference_engine.hpp>

using namespace InferenceEngine;
using namespace cv;
using namespace cv::utils::fs;



void topK(const std::vector<float>& src, unsigned k,
          std::vector<float>& dst,
          std::vector<unsigned>& indices) {
	
	dst.resize(k);

	std::vector<float> tmp(src);    

	std::sort(tmp.begin(), tmp.end(), std::greater<float>());
	for (int i = 0; i < k; i++)
	{
		dst.at(i) = tmp.at(i);
		std::cout << dst[i]<< std::endl;
		
		auto it = std::find(src.begin(), src.end(), tmp.at(i));
		indices.push_back(std::distance(src.begin(), it));
	}
	
}

void softmax(std::vector<float>& values) {

	std::vector<float> tmp(values);
	float sum_exp=0;
	float max = *std::max_element(values.begin(), values.end());
	for (int i = 0; i < values.size(); i++)
	{
		sum_exp += exp(tmp.at(i)-max);

	}
	for (int i = 0; i < values.size(); i++)
	{
		values.at(i) = exp(tmp.at(i)-max) / sum_exp;
	}
    
}

Blob::Ptr wrapMatToBlob(const Mat& m) {
    CV_Assert(m.depth() == CV_8U);
    std::vector<size_t> dims = {1, (size_t)m.channels(), (size_t)m.rows, (size_t)m.cols};
    return make_shared_blob<uint8_t>(TensorDesc(Precision::U8, dims, Layout::NHWC),
                                     m.data);
}

Classifier::Classifier() {
    Core ie;

    // Load deep learning network into memory
    CNNNetwork net = ie.ReadNetwork(join(DATA_FOLDER, "DenseNet_121.xml"),
                                    join(DATA_FOLDER, "DenseNet_121.bin"));

    // Specify preprocessing procedures
    // (NOTE: this part is different for different models!)
    InputInfo::Ptr inputInfo = net.getInputsInfo()["data"];
    inputInfo->getPreProcess().setResizeAlgorithm(ResizeAlgorithm::RESIZE_BILINEAR);
    inputInfo->setLayout(Layout::NHWC);
    inputInfo->setPrecision(Precision::U8);
    outputName = net.getOutputsInfo().begin()->first;

    // Initialize runnable object on CPU device
    ExecutableNetwork execNet = ie.LoadNetwork(net, "CPU");

    // Create a single processing thread
    req = execNet.CreateInferRequest();
}

void Classifier::classify(const cv::Mat& image, int k, std::vector<float>& probabilities,
                          std::vector<unsigned>& indices) {
    // Create 4D blob from BGR image
    Blob::Ptr input = wrapMatToBlob(image);

    // Pass blob as network's input. "data" is a name of input from .xml file
    req.SetBlob("data", input);

    // Launch network
    req.Infer();

    // Copy output. "prob" is a name of output from .xml file
    float* output = req.GetBlob(outputName)->buffer(); 
	std::vector<float> tmp;
	for (int i = 0; i < req.GetBlob(outputName)->size(); i++)
	{
		tmp.push_back(output[i]);
	}
	
	topK(tmp,k,probabilities, indices);
	softmax(probabilities);
}
