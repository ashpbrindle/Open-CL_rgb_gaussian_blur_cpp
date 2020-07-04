#include <iostream>
#include <vector>
//Thread building blocks library
#include <tbb/task_scheduler_init.h>
#include "tbb/tbb.h"
#include "math.h"
#include <random>
#include <ctime>
//Free Image library
#include <FreeImagePlus.h>

using namespace std;
using namespace tbb;

void parallelGaussian(vector<double> kernel, int kernel_size) {

    // Load in input the image
    fipImage inputImage;
    inputImage.load("../Images/grey_image.jpg");
    inputImage.convertToFloat();

    // Gets the width and the height of the image to be processed
    auto width = inputImage.getWidth();
    auto height = inputImage.getHeight();

    // Sets the Input Image array buffer
    const float* const inputBuffer = (float*)inputImage.accessPixels();

    // Setup output image array buffer
    fipImage outputImage;
    outputImage = fipImage(FIT_FLOAT, width, height, 32);
    float *outputBuffer = (float*)outputImage.accessPixels();

    // running a Parallel implementation, with the width and height defined as the range
    parallel_for(blocked_range2d<int, int>(0, height, 0, width), [=](const blocked_range2d<int, int>& r) {

        // variable to hold the total calculation of all of the values multiplied by the relevent kernel positions and added up
        float total_calculation;

        // cycles through the image
        for (int y = r.rows().begin(); y != r.rows().end(); y++) {
            for (int x = r.cols().begin(); x != r.cols().end(); x++) {
                // marks the current position in the image (as the image is a 1D representation)
                int current_position = y * width + x;

                // cycles through the kernel
                for (int kernel_height = 0; kernel_height != kernel_size; kernel_height++) {
                    for (int kernel_width = 0; kernel_width != kernel_size; kernel_width++) {

                        // sets the position in the kernel
                        int kernel_pos = kernel_height * kernel_size + kernel_width;

                        // applies the calculation to all of the pixels which are accessable by the kernel in the image (depends on kernel size)
                        // applies a multiplication of the kernel position and the image pixel position , then adding this to the total
                        if (y > kernel_size && x > kernel_size && x < width - kernel_size && y < height - kernel_size) {
                            total_calculation += inputBuffer[(y + kernel_height) * width + (x + kernel_width)] * kernel[kernel_pos];
                        }
                        //}
                    }
                }
                // applies the blur to the current pixel on the image
                outputBuffer[current_position] = total_calculation;
                // and resets the calculation for the next pixel
                total_calculation = 0;
            }
        }

    });

    // saves the image
    outputImage.convertToType(FREE_IMAGE_TYPE::FIT_BITMAP);
    outputImage.convertTo24Bits();
    outputImage.save("grey_blurred.png");
    cout << "Done (Parallel Gaussian)";

}

void concurrentGaussian(vector<double> kernel, int kernel_size) {

    // Load in input the image
    fipImage inputImage;
    inputImage.load("../Images/grey_image.jpg");
    inputImage.convertToFloat();

    // Gets the width and the height of the image to be processed
    auto width = inputImage.getWidth();
    auto height = inputImage.getHeight();
    const float* const inputBuffer = (float*)inputImage.accessPixels();

    // Setup output image array buffer
    fipImage outputImage;
    outputImage = fipImage(FIT_FLOAT, width, height, 32);
    float *outputBuffer = (float*)outputImage.accessPixels();

    // variable to hold the total calculation of all of the values multiplied by the relevent kernel positions and added up
    float total_calculation;

    // cycles through the image
    for (int y = 0; y != height; y++) {
        for (int x = 0; x != width; x++) {
            // marks the current position in the image (as the image is a 1D representation)
            int current_position = y * width + x;

            // cycles through the kernel
            for (int kernel_height = 0; kernel_height != kernel_size; kernel_height++) {        // cycles through the kernel size and runs the equation to each of the differences
                for (int kernel_width = 0; kernel_width != kernel_size; kernel_width++) {
                    // sets the position in the kernel
                    int kernel_pos = kernel_height * kernel_size + kernel_width;

                    // applies the calculation to all of the pixels which are accessable by the kernel in the image (depends on kernel size)
                    if (y >= kernel_size / 2 && x >= kernel_size / 2 && y <= height - kernel_size && x <= width - kernel_size) {
                        // applies a multiplication of the kernel position and the image pixel position , then adding this to the total
                        total_calculation += inputBuffer[(y + kernel_height) * width + (x + kernel_width)] * kernel[kernel_pos];
                    }
                    // if it is not accessable it will keep the pixel the same, to avoid a solid black border
                    else {
                        total_calculation = inputBuffer[current_position];
                    }
                }
            }
            // applies the blur to the current pixel on the image
            outputBuffer[current_position] = total_calculation;
            // and resets the calculation for the next pixel
            total_calculation = 0;
        }
    }

    // saves the image
    outputImage.convertToType(FREE_IMAGE_TYPE::FIT_BITMAP);
    outputImage.convertTo24Bits();
    outputImage.save("grey_blurred.png");
    cout << "Done (Concurrent Gaussian)";


}

void concurrentRGB() {

    // the threshold is defined
    int threshold = 1;

    // Setup Input image array
    fipImage inputImage1;
    inputImage1.load("../Images/render_1.png");
    fipImage inputImage2;
    inputImage2.load("../Images/render_2.png");

    unsigned int width = inputImage1.getWidth();
    unsigned int height = inputImage1.getHeight();

    // Setup Output image array
    fipImage outputImage;
    outputImage = fipImage(FIT_BITMAP, width, height, 24);

    //2D Vector to hold the RGB colour data of an image
    vector<vector<RGBQUAD>> rgbValues1;
    rgbValues1.resize(height, vector<RGBQUAD>(width));
    // a 2D vector is made for each of the rgb values i will be saving (1 for each image to compare)
    vector<vector<RGBQUAD>> rgbValues2;
    rgbValues2.resize(height, vector<RGBQUAD>(width));
    // a vector to hold the RGB values of the output image
    vector<vector<RGBQUAD>> outputrgb;
    outputrgb.resize(height, vector<RGBQUAD>(width));

    //Extract colour data from image and store it as individual RGBQUAD elements for every pixel
    for(int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {

            RGBQUAD rgb1;  //FreeImage structure to hold RGB values of a single pixel
            RGBQUAD rgb2;  //representing both images to compare

            inputImage1.getPixelColor(x, y, &rgb1); //Extract pixel(x,y) colour data and place it in rgb
            inputImage2.getPixelColor(x, y, &rgb2); //for each of the iamge

            rgbValues1[y][x].rgbRed = rgb1.rgbRed;  //fill the vector with the rgb values extracted from the first image
            rgbValues1[y][x].rgbGreen = rgb1.rgbGreen;
            rgbValues1[y][x].rgbBlue = rgb1.rgbBlue;

            rgbValues2[y][x].rgbRed = rgb2.rgbRed;  //fill the vector with the rgb values extracted from the second image
            rgbValues2[y][x].rgbGreen = rgb2.rgbGreen;
            rgbValues2[y][x].rgbBlue = rgb2.rgbBlue;

            //get absolute differences between the 2 images
            outputrgb[y][x].rgbRed = abs(rgbValues1[y][x].rgbRed - rgbValues2[y][x].rgbRed);
            outputrgb[y][x].rgbGreen = abs(rgbValues1[y][x].rgbGreen - rgbValues2[y][x].rgbGreen);
            outputrgb[y][x].rgbBlue = abs(rgbValues1[y][x].rgbBlue - rgbValues2[y][x].rgbBlue);

            //binary threshold applied to the image (above variable), to set all the values which are not black (threshold) to white (255,255,255)
            if (outputrgb[y][x].rgbRed > threshold && outputrgb[y][x].rgbGreen  > threshold&& outputrgb[y][x].rgbBlue > threshold) {
                outputrgb[y][x].rgbRed = 255;
                outputrgb[y][x].rgbGreen = 255;
                outputrgb[y][x].rgbBlue = 255;
            }
        }
    }

    //Saving the RGB values into the output image
    for(int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // set values through each pixel
            outputImage.setPixelColor(x, y, &outputrgb[y][x]);
        }
    }

    cout << "<---    Image Differences Calculated   --->" << endl;

    // 2 variables to hold the values for how many black and white pixels are in the output image
    double black_pixels = 0;
    double white_pixels = 0;

    // loop to cycle through the image and find the number of black and white pixels
    for(int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            RGBQUAD rgb;
            // for each pixel gets the colour from the image
            outputImage.getPixelColor(x, y, &rgb);
            // if the pixel is black (threshold) it will increment the black_pixel variable
            if (rgb.rgbRed <= threshold && rgb.rgbGreen <= threshold && rgb.rgbBlue <= threshold) {
                black_pixels += 1;
            }
            //else will increment the white_pixels
            else {
                white_pixels += 1;
            }
        }
    }

    // defines the total size of the image to calculate the total percentage of the black to white pixels
    double total_size = height * width;

    double percentage_white = (white_pixels / total_size) * 100;
    double percentage_black = (black_pixels / total_size) * 100;
    cout << "<--- White and Black Pixels Calculated ---> " << endl;

    cout << "Total White: " << percentage_white << "% " << endl;
    cout << "Total Black: " << percentage_black << "% " << endl;

    // next i will assign a random red pixel to the image
    // by getting a random x and random y value to assign it to
    srand(time(NULL));
    int random_y = rand() % height;
    int random_x = rand() % width;

    // assigns the red pixel to the random location
    outputrgb[random_y][random_x].rgbRed = 255;
    outputrgb[random_y][random_x].rgbGreen = 0;
    outputrgb[random_y][random_x].rgbBlue = 0;

    for(int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // re-assigns the new pixel values to the image
            outputImage.setPixelColor(x, y, &outputrgb[y][x]);
        }
    }

    cout << "<---    Red Pixel Randomly Assigned    --->" << endl;

    for(int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            RGBQUAD rgb;
            outputImage.getPixelColor(x, y, &rgb);
            // searches through the iamge to find the red pixel and prints the (x, y) location
            if (rgb.rgbRed == 255 && rgb.rgbGreen <= threshold && rgb.rgbBlue <= threshold) {
                cout << "Red Pixel Located at: [x: " << x << ", y: " << y << "]" << endl;
            };
        }
    }

    //Save the processed image
    outputImage.save("RGB_processed.png");


}

int main() {
    int nt = task_scheduler_init::default_num_threads();
    task_scheduler_init T(nt);

    //Part 1 (Greyscale Gaussian blur): -----------DO NOT REMOVE THIS COMMENT----------------------------//

    //variables used in gaussian kernel calculation
    const double sigma = 8.0;
    const int kernel_size = 9;
    const double pi = 3.14159265359;

    // these are used to define the differences from the centre of the kernel
    int xkernel = kernel_size / 2;
    int ykernel = kernel_size / 2;

    // used to determine if the middle column or row has been met for x and y
    // stating the the differences should now increment rather than decrement
    bool xzero = false;
    bool yzero = false;

    vector<double> temp;
    vector<double> kernel;
    double total;
    int index = 0;

    // cycles through the kernel size and runs the equation to each of the differences
    for (int width_pos = 0; width_pos != kernel_size; width_pos++) {
        for (int height_pos = 0; height_pos != kernel_size; height_pos++) {

            // applies the gaussian blur equation
            temp.push_back((1 / (2 * pi * (pow(sigma, 2)))) * exp(-(((pow(xkernel, 2)) + (pow(ykernel, 2))) / (2 * pow(sigma, 2)))));
            index += 1;

            // this will run through the first side of the kernel, once it reaches 0, it will run through the other side
            if (xkernel == 0) { xzero = true; }
            if (xzero == false) { xkernel -= 1; }
            else { xkernel += 1; }
        }

        // x resets back to the far left of the kernel again
        xzero = false;
        xkernel = kernel_size / 2;

        //changes the row for y
        if (ykernel == 0) { yzero = true; }
        if (yzero == false) { ykernel -= 1; }
        else { ykernel += 1; }
    }

    // adds up all of the kernel values
    for (int index = 0; index != temp.size(); index++) { total += temp[index]; }

    // then divides the total by 1
    total = 1 / total;

    //adds the value * total to the kernel for the final value
    for (int index = 0; index != temp.size(); index++) { kernel.push_back(temp[index] * total); }

    //parallelGaussian(kernel, kernel_size);
    //concurrentGaussian(kernel, kernel_size);

    //Part 2 (Colour image processing): -----------DO NOT REMOVE THIS COMMENT----------------------------//

    //concurrentRGB();

	return 0;
}