#include <iostream>
#include <opencv2/opencv.hpp>
#include <algorithm>
#include <math.h>
#include <filesystem>

// #include <opencv2/imgcodecs.hpp>
// #include <opencv2/highgui.hpp>

using namespace cv;
using namespace std;
namespace fs = std::filesystem;

struct member{
    //记录角度相近的直线的条数
    double angle; //与零轴的夹角
    int num_lines;
};

bool cmp(member m1, member m2){
    //按照直线的条数降序排列
    return m1.num_lines > m2.num_lines;
}


int main() {
    //cout << CV_VERSION << endl;

    std::string path = "/mnt/d/Projects/XiaoWanLineRecog/images/savedImage0";

    std::cout << "Please input the absolute directory of all jpeg files (there should not be subfolders):" << endl;
    getline(cin, path);
    clock_t begin, end;
    begin = clock();

    int num_imgs = 0;

    vector<string> all_images;
    for (const auto & entry : fs::directory_iterator(path))
        all_images.push_back(entry.path()); //std::cout << entry.path() << std::endl;

    //以下直线识别算法参考自：https://blog.csdn.net/weixin_43863869/article/details/128390636
    for(string img_path : all_images){

        cv::Mat src;
        src = cv::imread(img_path);
        if (src.empty()){
            printf("could not load image..\n");
            continue;
        }
        //cv::imshow("input", src);
        
        int height = src.rows;
        int width = src.cols;

        cv::Mat canny, dst;
        cv::Canny(src, canny, 150, 200); // 配合canny算法使用
        cv::cvtColor(canny, dst, cv::COLOR_GRAY2BGR); //灰度图转换为彩色图
        //cv::imshow("edge", canny);
    
        std::vector<cv::Vec4f> plines;
        int minLineLength = 50;
        int maxLineGap = 5;
        //提取直线
        cv::HoughLinesP(canny, plines, 1, CV_PI / 180.0, 80, minLineLength, maxLineGap);
        cv::Scalar color_red = cv::Scalar(0, 0, 255);
        cv::Scalar color_blue = cv::Scalar(255, 125, 0);

        std::vector<member> groups;
        double temp_angle;

        for(size_t i = 0; i < plines.size(); i++){
            //计算每条直线与零轴的夹角
            cv::Vec4f hline = plines[i];
            temp_angle = atan2(hline[3]-hline[1], hline[2]-hline[0]); 
            //range (-pi,pi]

            //对斜率进行分组，把斜率相似的直线分成一组
            if(groups.empty()){
                member new_member = {temp_angle, 1};
                groups.push_back(new_member);
            }
            else{
                size_t j = 0;
                for(; j < groups.size(); j++){
                    double angle_diff = abs(groups[j].angle-temp_angle);
                    double diff_threshold = 2*CV_PI/180;
                    bool flag = false;

                    //防止出现角度相差pi 或2pi的情况出现
                    for(int k = 0; k < 3; k++)
                        if (abs(angle_diff - k*CV_PI) < diff_threshold){
                            if(temp_angle < groups[j].angle)
                                temp_angle+=k*CV_PI;
                            else
                                temp_angle-=k*CV_PI;
                            flag = true;
                            break;
                        }
                    
                    if(flag){
                    //取平均
                        groups[j].angle = (groups[j].angle*groups[j].num_lines+temp_angle)/(groups[j].num_lines+1);
                        groups[j].num_lines++;
                        break;
                    }
                }
                if(j == groups.size()){
                    member new_member = {temp_angle, 1};
                    groups.push_back(new_member);
                }
            }


           // cv::line(src, cv::Point(hline[0], hline[1]), cv::Point(hline[2], hline[3]), color_red, 1, cv::LINE_AA);
        }

        sort(groups.begin(), groups.end(), cmp);
        double final_angle = 0;

        if(groups.empty()){
            std::cout << "no lines found, yaw is set to 0";
        }
        else{
            final_angle = groups[0].angle;
        }


        //转到第一象限
        while(final_angle > CV_PI/2){
            final_angle -= CV_PI/2;
        }
        while(final_angle < 0){
            final_angle += CV_PI/2;
        }
        
        // cv::imshow("output", dst);
        
        // cv::waitKey(0);

        cv::line(src, cv::Point(width/2, 0), cv::Point(width/2, height), color_red, 2, cv::LINE_AA);
        cv::line(src, cv::Point(0, height/2), cv::Point(width, height/2), color_red, 2, cv::LINE_AA);
        cv::putText(src, "yaw = "+to_string(90-final_angle*180/CV_PI), cv::Point(30, 30), FONT_HERSHEY_PLAIN, 1.2, color_red);
        int arrow_len = 150;
        cv::Point end_point = cv::Point(width/2+arrow_len*sin(final_angle), height/2-cos(final_angle)*arrow_len);
        cv::arrowedLine(src, cv::Point(width/2,height/2), end_point, color_blue, 2, cv::LINE_AA);
        string filename = img_path.substr(img_path.find_last_of("/")+1, img_path.find_last_of(".")-img_path.find_last_of("/")-1);
        imwrite("../result/"+filename+"_mark.jpeg", src);
        std::cout << filename+"_mark.jpeg  " << "yaw = " + to_string(90-final_angle*180/CV_PI) << endl;
        num_imgs++;
    }
    
    end = clock();
    double process_time = double(end - begin) / CLOCKS_PER_SEC * 1000 ;
    std::cout << "处理" + to_string(num_imgs) + "张照片所用时间：" +to_string(process_time)<< "ms" << endl;
    std::cout << "平均时间： "+to_string(process_time/num_imgs) << "ms" << endl;
    
    return 0;
}