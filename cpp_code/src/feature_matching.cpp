//OpenCV
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>

#include <chrono>
#include <iostream>

#include "feature_matching.h"

using namespace p3dv;

bool FeatureMatching::detectFeaturesORB(frame_t &cur_frame, int max_num, bool show)
{
    cv::Ptr<cv::FeatureDetector> detector = cv::ORB::create(max_num);
    cv::Ptr<cv::DescriptorExtractor> descriptor = cv::ORB::create(max_num);

    std::chrono::steady_clock::time_point tic = std::chrono::steady_clock::now();
    detector->detect(cur_frame.rgb_image, cur_frame.keypoints);

    descriptor->compute(cur_frame.rgb_image, cur_frame.keypoints, cur_frame.descriptors);

    // std::vector<int> unique_pixel_vec(cur_frame.keypoints.size(), -1);
    // cur_frame.unique_pixel_ids.insert(cur_frame.unique_pixel_ids.begin(), unique_pixel_vec.begin(), unique_pixel_vec.end());

    std::chrono::steady_clock::time_point toc = std::chrono::steady_clock::now();
    std::chrono::duration<double> time_used = std::chrono::duration_cast<std::chrono::duration<double>>(toc - tic);
    std::cout << "extract ORB cost = " << time_used.count() << " seconds. " << std::endl;
    std::cout << "Found " << cur_frame.descriptors.size() << " features" << std::endl;
    if (show)
    {
        cv::Mat feature_image;
        cv::namedWindow("ORB features",0);
        cv::drawKeypoints(cur_frame.rgb_image, cur_frame.keypoints, feature_image, cv::Scalar::all(-1), cv::DrawMatchesFlags::DEFAULT);
        cv::imshow("ORB features", feature_image);
        cv::waitKey(0); // Wait for a keystroke in the window
    }

    return true;
}

bool FeatureMatching::detectFeaturesSURF(frame_t &cur_frame, int minHessian, bool show)
{
    cv::Ptr<cv::FeatureDetector> detector = cv::xfeatures2d::SURF::create(minHessian);
    cv::Ptr<cv::DescriptorExtractor> descriptor = cv::xfeatures2d::SURF::create();

    std::chrono::steady_clock::time_point tic = std::chrono::steady_clock::now();

    detector->detect(cur_frame.rgb_image, cur_frame.keypoints);

    descriptor->compute(cur_frame.rgb_image, cur_frame.keypoints, cur_frame.descriptors);

    std::chrono::steady_clock::time_point toc = std::chrono::steady_clock::now();
    std::chrono::duration<double> time_used = std::chrono::duration_cast<std::chrono::duration<double>>(toc - tic);
    std::cout << "extract SURF cost = " << time_used.count() << " seconds. " << std::endl;
    std::cout << "Found " << cur_frame.descriptors.size() << " features." << std::endl;

    if (show)
    {
        cv::Mat feature_image;
        cv::namedWindow("SURF features",0);
        cv::drawKeypoints(cur_frame.rgb_image, cur_frame.keypoints, feature_image, cv::Scalar::all(-1), cv::DrawMatchesFlags::DEFAULT);
        cv::imshow("SURF features", feature_image);
        cv::waitKey(0); // Wait for a keystroke in the window
    }

    return true;
}

bool FeatureMatching::matchFeaturesORB(frame_t &cur_frame_1, frame_t &cur_frame_2, std::vector<cv::DMatch> &matches,
                                       double ratio_thre, bool show)
{
    cv::Ptr<cv::DescriptorMatcher> matcher = cv::DescriptorMatcher::create("BruteForce-Hamming");
    std::vector<std::vector<cv::DMatch>> initial_matches_nn2;
    std::vector<cv::DMatch> initial_matches;

    // Initial match
    std::chrono::steady_clock::time_point tic = std::chrono::steady_clock::now();
    matcher->knnMatch(cur_frame_1.descriptors, cur_frame_2.descriptors, initial_matches_nn2, 2);
    std::cout << "Initial matching done." << std::endl;

    // Filter matches using the Lowe's ratio test
    for (int i = 0; i < initial_matches_nn2.size(); i++)
    {
        if (show)
            initial_matches.push_back(initial_matches_nn2[i][0]);
        if (initial_matches_nn2[i][0].distance < ratio_thre * initial_matches_nn2[i][1].distance)
        {
            matches.push_back(initial_matches_nn2[i][0]);
        }
    }

    std::chrono::steady_clock::time_point toc = std::chrono::steady_clock::now();
    std::chrono::duration<double> time_used = std::chrono::duration_cast<std::chrono::duration<double>>(toc - tic);
    std::cout << "match ORB cost = " << time_used.count() << " seconds. " << std::endl;
    std::cout << "# Correspondence: Initial [ " << initial_matches_nn2.size() << " ]  Filtered by Lowe ratio test [ " << matches.size() << " ]" << std::endl;

    if (show)
    {
        cv::Mat initial_match_image;
        cv::Mat match_image;
        cv::namedWindow("Initial matches",0);
        cv::namedWindow("Filtered matches",0);
        cv::drawMatches(cur_frame_1.rgb_image, cur_frame_1.keypoints, cur_frame_2.rgb_image, cur_frame_2.keypoints, initial_matches, initial_match_image);
        cv::drawMatches(cur_frame_1.rgb_image, cur_frame_1.keypoints, cur_frame_2.rgb_image, cur_frame_2.keypoints, matches, match_image);
        cv::imshow("Initial matches", initial_match_image);
        cv::imshow("Filtered matches", match_image);
        cv::waitKey(0);
    }

    return true;
}

bool FeatureMatching::matchFeaturesSURF(frame_t &cur_frame_1, frame_t &cur_frame_2, std::vector<cv::DMatch> &matches,
                                        double ratio_thre, bool show)
{

    // Since SURF is a floating-point descriptor NORM_L2 is used
    cv::FlannBasedMatcher matcher;
    std::vector<std::vector<cv::DMatch>> initial_matches_nn2;
    std::vector<cv::DMatch> initial_matches;

    std::chrono::steady_clock::time_point tic = std::chrono::steady_clock::now();
    matcher.knnMatch(cur_frame_1.descriptors, cur_frame_2.descriptors, initial_matches_nn2, 2);
    std::cout << "Initial matching done." << std::endl;

    // Filter matches using the Lowe's ratio test
    for (int i = 0; i < initial_matches_nn2.size(); i++)
    {
        if (show)
            initial_matches.push_back(initial_matches_nn2[i][0]);
        if (initial_matches_nn2[i][0].distance < ratio_thre * initial_matches_nn2[i][1].distance)
        {
            matches.push_back(initial_matches_nn2[i][0]);
        }
    }

    std::chrono::steady_clock::time_point toc = std::chrono::steady_clock::now();
    std::chrono::duration<double> time_used = std::chrono::duration_cast<std::chrono::duration<double>>(toc - tic);
    std::cout << "match SURF cost = " << time_used.count() << " seconds. " << std::endl;
    std::cout << "# Correspondence: Initial [ " << initial_matches_nn2.size() << " ]  Filtered by Lowe ratio test [ " << matches.size() << " ]" << std::endl;

    if (show)
    {
        cv::Mat initial_match_image;
        cv::Mat match_image;
        cv::namedWindow("Initial matches",0);
        cv::namedWindow("Filtered matches",0);
        cv::drawMatches(cur_frame_1.rgb_image, cur_frame_1.keypoints, cur_frame_2.rgb_image, cur_frame_2.keypoints, initial_matches, initial_match_image);
        cv::drawMatches(cur_frame_1.rgb_image, cur_frame_1.keypoints, cur_frame_2.rgb_image, cur_frame_2.keypoints, matches, match_image);
        cv::imshow("Initial matches", initial_match_image);
        cv::imshow("Filtered matches", match_image);
        cv::waitKey(0);
    }

    return true;
}

bool FeatureMatching::findInitializeFramePair(std::vector<std::vector<bool>> &feature_track_matrix, std::vector<frame_t> &frames,
                                              std::vector<std::vector<frame_pair_t>> img_match_graph,
                                              int &initialization_frame_1, int &initialization_frame_2, double &depth_init,
                                              int min_track_num_init, double max_depth_baseline_ratio_init)
{
    std::chrono::steady_clock::time_point tic = std::chrono::steady_clock::now();

    initialization_frame_1 = 0;
    initialization_frame_2 = 0;

    int frame_number = frames.size();
    int num_unique_points = feature_track_matrix[0].size();

    //resize feature track matrix to frame_num * unique_point_num
    std::vector<int> point_track_frame_num(num_unique_points, 0);
    for (int i = 0; i < frame_number; i++)
    {
        feature_track_matrix[i].resize(num_unique_points);
        for (int j = 0; j < num_unique_points; j++)
        {
            point_track_frame_num[j] += feature_track_matrix[i][j];
        }
    }

    //Find frame pair for initialization
    int max_sum_track_frame_num = min_track_num_init; // the minimum track number for initialization frame pair
    double depth_baseline_ratio_init;  // the depth for initialization frame pair

    for (int i = 0; i < frame_number; i++)
    {
        for (int j = 0; j < i; j++)
        {
            int temp_sum_track_frame_num = 0;

            double depth_baseline_ratio = img_match_graph[i][j].appro_depth; // baseline length = 1
            if (depth_baseline_ratio > max_depth_baseline_ratio_init)        //Skip this loop since the baseline is too short.
                continue;

            for (int k = 0; k < point_track_frame_num.size(); k++)
            {
                if (feature_track_matrix[i][k] && feature_track_matrix[j][k])
                {
                    temp_sum_track_frame_num += point_track_frame_num[k];
                }
            }
            if (temp_sum_track_frame_num >= max_sum_track_frame_num)
            {
                max_sum_track_frame_num = temp_sum_track_frame_num;
                depth_baseline_ratio_init = depth_baseline_ratio;
                initialization_frame_1 = i;
                initialization_frame_2 = j;
            }
        }
    }
 
    // If there's no candidate pair found, we have to use the default one (first two frames)
    if (initialization_frame_1 == initialization_frame_2)
    {
        std::cout << "Failed to find proper frame pair for initialization. Use default frame [1] and frame [0]" << std::endl;
        initialization_frame_1 = 1;
        initialization_frame_2 = 0;
        depth_baseline_ratio_init=img_match_graph[1][0].appro_depth;
        return 0;
    }
    
    depth_init=depth_baseline_ratio_init;

    std::chrono::steady_clock::time_point toc = std::chrono::steady_clock::now();
    std::chrono::duration<double> time_used = std::chrono::duration_cast<std::chrono::duration<double>>(toc - tic);
    std::cout << "Find initialization frame pair done in " << time_used.count() << " seconds. " << std::endl;
    std::cout << "Frame " << initialization_frame_1 << " and Frame " << initialization_frame_2 << " with " << max_sum_track_frame_num << " trackings" << std::endl;
    std::cout << "The relative depth is " << depth_baseline_ratio_init << " . They are set as the frame pair for initialization." << std::endl;
    return 1;
}

bool FeatureMatching::findNextFrame(std::vector<std::vector<bool>> &feature_track_matrix, std::vector<bool> &frames_to_process,
                                    std::vector<int> &unique_3d_point_ids, int &next_frame)
{
    std::chrono::steady_clock::time_point tic = std::chrono::steady_clock::now();

    int total_frame_num = frames_to_process.size();

    //std::cout << "Total frame number: " << total_frame_num << std::endl;

    int max_common_point_num = 0;

    for (int i = 0; i < total_frame_num; i++)
    {
        if (frames_to_process[i])
        {
            int temp_common_point_num = 0;
            for (int j = 0; j < unique_3d_point_ids.size(); j++)
            {
                if (feature_track_matrix[i][unique_3d_point_ids[j]])
                    temp_common_point_num++;
            }
            if (temp_common_point_num > max_common_point_num)
            {
                max_common_point_num = temp_common_point_num;
                next_frame = i;
            }
        }
    }

    std::chrono::steady_clock::time_point toc = std::chrono::steady_clock::now();
    std::chrono::duration<double> time_used = std::chrono::duration_cast<std::chrono::duration<double>>(toc - tic);
    std::cout << "Find next frame done in " << time_used.count() << " seconds. " << std::endl;
    std::cout << "Frame " << next_frame << " is found with " << max_common_point_num << " common points" << std::endl;
}