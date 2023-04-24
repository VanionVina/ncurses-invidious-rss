#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <filesystem>


void save_one_channel_videos(std::string videos[15][6], int length, int author_index)
{
    std::ofstream file;
    file.open("videos");

    for (int i=0; i < length; i++)
    {
        if (author_index == i)
        {
            file << "[" << std::endl;
            for (int j=0; j < 15; j++)
            {
                for (int k=0; k < 6; k++)
                {
                    file << videos[j][k] + ";";
                }
                file << std::endl;
            }
            file << "]" << std::endl;
        }
        else 
        {
            file << "[" << std::endl;
            file << std::endl;
            file << "]" << std::endl;
        }
    }

    file.close();
}

void save_all_videos(std::string allVideos[][15][6], int length)
{
    std::ofstream file;
    file.open("videos");
    for (int i=0; i < length; i++)
    {
        file << "[" << std::endl;
        for (int j=0; j < 15; j++)
        {
            for (int k=0; k < 6; k++)
            {
                file << allVideos[i][j][k] + ";";
            }
            file << std::endl;
        }
        file << "]" << std::endl;
    }

    file.close();
}


std::vector<std::string> split(const std::string& s, char delimiter)
{
   std::vector<std::string> splits;
   std::string split;
   std::istringstream ss(s);
   while (std::getline(ss, split, delimiter))
   {
      splits.push_back(split);
   }
   return splits;
}

int read_videos_from_file(std::string videos[][15][6], int length,
        int videos_available[])
{
    std::ifstream file("videos");
    std::string line;

    int author_index = 0;

    int videos_value_index;
    int video_index;

    int res = 0;

    if (file.is_open())
    {
        while (std::getline(file, line))
        {
            if (author_index >= length)
            {
                res = -1;
                break;
            }
            if (line == "[")
            {
                video_index = 0;
                videos_available[author_index] = 1;
                continue;
            }
            else if (line == "]")
            {
                video_index = 0;
                author_index++;
                continue; 
            }

            if (line == ";;;;;;")
            {
                videos[author_index][video_index][0] = "--Doesn't exist--";
                video_index++;
                continue;
            }

            if (!line.size())
            { 
                videos_available[author_index] = 0;
                continue;
            }
            else
            {
                std::vector<std::string> splitData = split(line, ';');
                for (int value_index=0; value_index < 6; value_index++)
                {
                    videos[author_index][video_index][value_index] =
                        splitData[value_index];
                }
                video_index++;
            }
        }
    }

    file.close();
    return res;
}

void save_names_ids_unwached_videos(std::string names_and_ids[][2], int length,
        int unwached_videos[])
{
    std::ofstream file("channel_names_and_ids");

    if (file.is_open())
    {
        for (int i=0; i < length; i++)
        {
            file << names_and_ids[i][0] + '\t';
            if (unwached_videos[i] < 10)
                file << "0" + std::to_string(unwached_videos[i]) +
                    '\t';
            else
                file << std::to_string(unwached_videos[i]) +
                    '\t';
            file << names_and_ids[i][1] << std::endl;
        }
        file.close();
    }
    else std::cerr<<"File cannot be opened";
}

void read_names_ids_unwached_videos(std::string names_and_ids[][2],
        int unwached_videos_count[], bool unwached_videos_array[][15])
{
    std::ifstream file("channel_names_and_ids");
    std::string line;

    int count = 0;
    while (std::getline(file, line))
    {
        if (line.length() != 24)
        {
            names_and_ids[count][0] = line.substr(0, 24);
            unwached_videos_count[count] = stoi(line.substr(25, 2));
            names_and_ids[count][1] = line.substr(28, line.length()-28);
            count++;
        }
        else 
        {
            names_and_ids[count][0] = line.substr(0, 24);
            unwached_videos_count[count] = 15;
            for (int i=0; i<15; i++)
            {
                unwached_videos_array[count][i] = true;
            }
            names_and_ids[count][1] = "";
            count++;
        }
    }
    file.close();
}

void save_descriptions(std::string descriptions[][15], int length)
{
    std::ofstream file("descriptions");
    if (file.is_open())
    {
        file << "[" << std::endl;
        for (int i=0; i < length; i++)
        {
            for (int j=0; j < 15; j++)
            {
                file << "<" << std::endl;
                file << descriptions[i][j];
                file << "\n>" << std::endl;
            }
        }
        file << "]" << std::endl;
    }
    file.close();
}


void read_descriptions(std::string descriptions[][15])
{
    std::ifstream file("descriptions");
    std::string line;

    int descr_author_index = 0;
    int descr_video_index;
    std::string descr;

    bool first_line;


    if (file.is_open())
    {
        while (std::getline(file, line))
        {
            if (line == "[")
            {
                descr_video_index = -1;
            }
            else if (line == "]")
            {
                descr_author_index++;
            }
            else if (line == "<")
            {
                descr = "";
                first_line = true;
                descr_video_index++;
            }
            else if (line == ">")
            {
                descriptions[descr_author_index][descr_video_index] =
                    descr;
            }
            else 
            {
                if (first_line)
                {
                    if (!line.size())
                    {
                        descriptions[descr_author_index][descr_video_index] =
                            "No description available";
                    }
                    else
                        descr = descr + line + '\n';
                    first_line = false;
                }
                else
                {
                    descr = descr + line + '\n';
                }
            }
        }
    }
    file.close();
}

void save_unwached_videos_positions(bool unwachedVideos[][15], int length)
{
    std::ofstream file("unwached_videos");
    if (file.is_open())
    {
        for (int i=0; i<length; i++)
        {
            file << "[" << std::endl;
            for (int j=0; j<15; j++)
            {
                if (unwachedVideos[i][j])
                    file << "1 ";
                else
                    file << "0 ";
            }
            file << std::endl;
            file << "]" << std::endl;
        }
    }
    file.close();
}

void read_unwached_videos(bool unwachedVideos[][15])
{
    std::ifstream file("unwached_videos");
    std::string line;

    int author_index = 0;

    if (file.is_open())
    {
        while (std::getline(file, line))
        {
            if (line == "[")
                continue;
            else if (line == "]")
                author_index++;
            else
            {
                for (int i=0; i<31; i+=2)
                {
                    if (line[i] == '0')
                        unwachedVideos[author_index][i/2] = false;
                    else
                        unwachedVideos[author_index][i/2] = true;
                }
            }
        }
    }
    file.close();
}
