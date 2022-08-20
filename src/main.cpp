// Main function for running finpgerprint image enhancement algorithm
//
// This enhancement method is based on Anil Jain's paper:
// 'Fingerprint Image Enhancement: Algorithm and Performance Evaluation',
//  IEEE Transactions on Pattern Analysis and Machine Intelligence,
//  vol. 20, No. 8, August, 1998
//
// Author: Ekberjan Derman
// Contributor : Baptiste Amato, Julien Jerphanion
// Emails:
//    ekberjanderman@gmail.com
//    baptiste.amato@psycle.io
//    git@jjerphan.xyz

#include "common.h"
#include "cxxopts.hpp"
#include "fpenhancement.h"

std::string getImageType(int number) {
    // Find type
    int imgTypeInt = number % 8;
    std::string imgTypeString;

    switch (imgTypeInt) {
        case 0:
            imgTypeString = "8U";
            break;
        case 1:
            imgTypeString = "8S";
            break;
        case 2:
            imgTypeString = "16U";
            break;
        case 3:
            imgTypeString = "16S";
            break;
        case 4:
            imgTypeString = "32S";
            break;
        case 5:
            imgTypeString = "32F";
            break;
        case 6:
            imgTypeString = "64F";
            break;
        default:
            break;
    }

    // Find channel
    int channel = (number / 8) + 1;

    std::stringstream type;
    type << "CV_" << imgTypeString << "C" << channel;

    return type.str();
}

int main(int argc, char *argv[]) {

    // CLI management

    cxxopts::Options options("fingerprint", "Extract fingerprints from an image");

    options.add_options()("i,input_image", "Input image",
                          cxxopts::value<std::string>())(
            "o,output_image", "Output image",
            cxxopts::value<std::string>()->default_value("out.png"))(
            "s,show", "Show the result of the algorithm",
            cxxopts::value<bool>()->default_value("false"))(
            "d,downsize", "Downsize the image",
            cxxopts::value<bool>()->default_value("false"))(
            "b,border", "Add border to the image",
            cxxopts::value<bool>()->default_value("false"))(
            "n,no_save", "Don't save the image",
            cxxopts::value<bool>()->default_value("false"))(
            "p,no_postprocessing", "Don't perform the postprocessing",
            cxxopts::value<bool>()->default_value("false"))(
            "min_rows", "Minimum number of rows",
            cxxopts::value<int>()->default_value("1000"))(
            "min_cols", "Minimum number of columns",
            cxxopts::value<int>()->default_value("1000"))

            ("h,help", "Print usage")("v,verbose", "Verbose output",
                                      cxxopts::value<bool>()->default_value("false"));

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        exit(0);
    }

    if (result.count("input_image") + result.count("i") == 0) {
        std::cerr << "Bad usage: the input image has to be specified" << std::endl;
        std::cerr << options.help() << std::endl;
        exit(1);
    }

    // CLI parameters
    const auto &inputImage = result["input_image"].as<std::string>();
    const auto &outputImage = result["output_image"].as<std::string>();

    bool showResult = result["s"].as<bool>();
    bool downsize = result["d"].as<bool>();
    bool saveImage = !(result["n"].as<bool>());
    bool verbose = result["v"].as<bool>();
    bool performPostprocessing = !(result["p"].as<bool>());

    int minRows = result["min_rows"].as<int>();
    int minCols = result["min_cols"].as<int>();

    ///

    cv::Mat input = cv::imread(inputImage);
    cv::Scalar value(255, 255, 255);
    cv::copyMakeBorder(input, input, 20, 20, 20, 20, cv::BORDER_CONSTANT, value);

    // Make sure the input image is valid
    if (!input.data) {
        std::cerr << "The provided input image is invalid. Please check it again. "
                  << std::endl;
        exit(1);
    }

    if (downsize) {
        while (input.rows > minRows || input.cols > minCols) {
            const float fact = 0.9;
            if (verbose) {
                std::cout << "Downsizing from (" << input.rows << ", " << input.cols
                          << ") to (" << (int) (input.rows * fact) << ", "
                          << (int) (input.cols * fact) << ")" << std::endl;
            }
            cv::resize(input, input, cv::Size(), fact, fact, cv::INTER_CUBIC);
        }
    }

    // Run the enhancement algorithm
    FPEnhancement fpEnhancement(verbose=verbose);
    cv::Mat enhancedImage = fpEnhancement.extractFingerPrints(input);

    // Finally applying the filter to get the end result
    cv::Mat endResult(cv::Scalar::all(0));

    if (performPostprocessing) {
        // Apply the post processing for better results
        cv::Mat filter = fpEnhancement.postProcessingFilter(input);
        // cv::imwrite("filter.png", filter);
        // cv::imwrite("enhanced.png", enhancedImage);

        /*if (verbose) {
            std::cout << "Type of the image  : " << getImageType(enhancedImage.type())
                      << std::endl;
            std::cout << "Type of the filter : " << getImageType(filter.type())
                      << std::endl;
        }*/

        enhancedImage.copyTo(endResult, filter);
        // cv::FileStorage file2("filter.xml", cv::FileStorage::WRITE);
        // file2 << "matName" << 255-filter;
        // std::cout << endResult.rows << filter.rows << filter.size[2];
        // cv::cvtColor(filter, filter, CV_GRAY2RGB);
        // cv::FileStorage file1("endResult.xml", cv::FileStorage::WRITE);
        // file1 << "matName" << endResult;
        // std::cout << endResult.rows << filter.rows << filter.size[2];
        endResult.convertTo(endResult, CV_8U);
        cv::bitwise_or(endResult,  255-filter, endResult);
        endResult = endResult(cv::Range(20,endResult.rows-20), cv::Range(20,endResult.cols-20));
    } else {
        endResult = enhancedImage;
        endResult = endResult(cv::Range(20,endResult.rows-20), cv::Range(20,endResult.cols-20));
    }

    if (showResult) {
        cv::imshow("End result", endResult);
        std::cout << "Press any key to continue... " << std::endl;
        cv::waitKey();
    }

    if (saveImage) {
        cv::imwrite(outputImage, endResult);
    }

    return 0;
}
