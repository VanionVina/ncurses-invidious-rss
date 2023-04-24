#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <chrono>
#include <filesystem>
#include <pugixml.hpp>
#include <curl/curl.h>
#include <json/json.h>
#include <ncurses.h>

#include "rss_parse.h"
#include "read_write.h"


std::string generate_menu_line(int menu_index, int newVideosNum, std::string author_name)
{
    std::string menu_line = std::to_string(menu_index) + "\t" +
        "( " + std::to_string(newVideosNum) +
        " / 15 ) " +
    author_name;

    return menu_line;
}

std::tm make_tm(int year, int month, int day)
{
    std::tm tm = {0};
    tm.tm_year = year - 1900; // years count from 1900
    tm.tm_mon = month - 1;    // months count from January=0
    tm.tm_mday = day;         // days count from 1
    return tm;
}

int calculate_date_diff(std::string date)
{
    using namespace std::chrono;

    // Create video date
    int year = std::stoi(date.substr(0,4));
    int month = std::stoi(date.substr(5,2));
    int day = std::stoi(date.substr(8,2));

    std::tm publishedDate = make_tm(year, month, day);
    //

    // Get current date
    system_clock::time_point now = system_clock::now();

    time_t tt = system_clock::to_time_t(now);
    tm utc_tm = *gmtime(&tt);
    tm local_tm = *localtime(&tt);

    std::tm todayDate = make_tm(local_tm.tm_year+1900, local_tm.tm_mon + 1,
            local_tm.tm_mday);
    //

    std::time_t time1 = std::mktime(&publishedDate);
    std::time_t time2 = std::mktime(&todayDate);

    const int seconds_per_day = 60*60*24;
    std::time_t difference = (time2 - time1) / seconds_per_day;    
    return difference;
}

void sort_2d_array(std::string names_ids[][2], int length,
        std::string videos[][15][6])
{
    std::string tempValue;
    for (int i=0; i<length; i++)
        for (int j=i; j<length; j++)
            if (names_ids[j][1] < names_ids[i][1])
            {
                // Swap channelNamesAndIds values
                for (int k=0; k<2; k++)
                {
                    tempValue = names_ids[j][k];
                    names_ids[j][k] = names_ids[i][k];
                    names_ids[i][k] = tempValue;
                }
                // Swap allVideos values
                for (int k=0; k<15; k++)
                {
                    for (int z=0; z<6; z++)
                    {
                        tempValue = videos[j][k][z];
                        videos[j][k][z] = videos[i][k][z];
                        videos[i][k][z] = tempValue;
                    }
                }
            }
}

int main(int argc, char ** argv)
{
    // Because
    int ascii_size = 6;
    std::string ascii_art[6][8] =
    {
        {
            {"░█▐▄▒▒▒▌▌▒▒▌░▌▒▐▐▐▒▒▐▒▒▌▒▀▄▀"},
            {"█▐▒▒▀▀▌░▀▀▀░░▀▀▀░░▀▀▄▌▌▐▒▒▒▌"},
            {"▒▒▀▀▄▐░▀▀▄▄░░░░░░░░░░░▐▒▌▒▒▐"},
            {"▒▌▒▒▒▌░▄▄▄▄█▄░░░░░░░▄▄▄▐▐▄▄▀"},
            {"▐▒▒▒▐░░░░░░░░░░░░░▀█▄░░░░▌▌░"},
            {"▒▌▒▒▐░░░░░░░▄░░▄░░░░░▀▀░░▌▌░"},
            {"▒▐▒▒▐░░░░░░░▐▀▀▀▄▄▀░░░░░░▌▌░"},
            {"░█▌▒▒▌░░░░░▐▒▒▒▒▒▌░░░░░░▐▐▒▀"},
        },
        {
            {"░▄█████░█████▌░█░▀██████▌█▄▄▀"},
            {"░▌███▌█░▐███▌▌░░▄▄░▌█▌███▐███"},
            {"▐░▐██░░▄▄▐▀█░░░▐▄█▀▌█▐███▐█░░"},
            {"░░███░▌▄█▌░░▀░░▀██░░▀██████▌░"},
            {"░░░▀█▌▀██▀░▄░░░░░░░░░███▐███▌"},
            {"░░░░██▌░░░░░░░░░░░░░▐███████▌"},
            {"░░░░███░░░░░▀█▀░░░░░▐██▐███▀▌"},
            {"░░░░▌█▌█▄░░░░░░░░░▄▄████▀░▀ ░"},
        },
        {
            {"⣟⣟⢨⣿⠀⣼⣏⣾⡟⣰⣟⣿⢣⣿⡟⠀⠈⢀⢀⣄⣾⣿⡟⣇⣿⣀⡐⣈"},
            {"⣹⡇⣯⣿⠀⣿⣿⣿⣿⣿⣼⣧⡟⣽⡃⠀⢈⠈⣭⣿⡿⠃⠉⣿⣿⡍⢉⡏"},
            {"⣸⡗⣯⣿⢘⣿⢼⣿⣿⣿⣿⣿⢠⣿⠀⠀⣠⡾⣻⣟⣡⣄⡀⣽⡏⣧⢸⡇"},
            {"⡿⣷⣹⣿⡞⡿⣸⣿⣿⣿⣿⡏⢸⡿⠀⣰⡿⢣⡿⢻⣿⣿⣷⣿⣥⣿⣾⢡"},
            {"⣷⣿⣿⣿⢿⣇⠹⣟⡹⠿⡟⠀⠀⣿⣼⠏⢠⠋⠀⣿⣿⣿⣿⣿⣿⠿⣿⡇"},
            {"⣻⣿⣯⣿⣌⣷⠀⠉⠛⠁⠀⠀⢨⡿⠁⠀⠀⠀⠀⣿⠟⣿⢿⣿⠟⣰⡟⠀"},
            {"⠉⠉⠙⠛⠻⠾⢧⣤⣤⣤⣤⣀⣈⠀⠀⠀⠀⠀⠀⠙⠿⠶⠞⠁⣰⡿⠁⢀"},
            {"⢈⣿⠃⠀⠀⠀⢠⣦⡀⢠⣌⡉⠙⠛⠛⠷⠶⠶⠴⣶⣤⣦⣰⣾⡟⠁⢠⡾"}
        },
        {
            {"⢀⡾⣻⠁⢠⢀⣿⣽⠿⠖⠋⠟⠀⣺⣾⣁⣶⠿⠉⠀⠸⡆⠀⣿⠀⣷⠀⢻"},
            {"⣾⣿⡇⠀⣼⢸⣧⣿⠀⠲⣄⠀⠒⠛⠋⠁⠀⠀⠀⢀⣠⣧⢠⣿⠀⣿⡄⢸"},
            {"⣯⣼⡖⢠⡇⣸⠁⠈⢠⣤⣌⠙⠀⠀⠀⠀⠀⠛⣛⣉⢀⣧⠞⣿⠀⢹⣿⣨"},
            {"⣿⣽⢣⣼⣷⣇⣶⣿⣿⣿⣿⠇⠀⠀⠀⠀⠀⣶⣿⣿⣾⣧⣸⡇⠀⣿⣿⢸"},
            {"⣿⣿⣾⣿⣿⡿⣿⣿⣿⣿⣏⠀⠀⠀⠀⠀⠀⢋⣽⣿⣿⣿⣿⡇⠀⢸⣷⢼"},
            {"⣿⠛⠃⣿⣏⢣⡽⣿⣻⡿⠃⠀⠀⠀⠀⠀⠀⠘⣿⡿⣿⡿⢻⠇⠀⠸⣿⡆"},
            {"⡏⠀⢸⣿⣿⡋⡠⣢⡆⠀⠀⠀⠀⠀⠀⠀⠀⣶⣟⣿⣿⠀⢸⣦⠀⠰⣿⣧"},
            {"⡇⠀⠸⣿⣿⣻⠖⠋⠀⠀⠀⠠⠖⠒⠲⣄⠀⠁⠘⡟⠏⠚⢸⣿⡀⠀⢻⣿"},
        },
        {
            {"⢸⠇⠀⢀⣀⣀⣠⠿⠿⠾⠷⢾⡇⢸⡟⠋⠛⠋⠉⠉⠙⠓⠒⠲⠶⠦⣼⢽⡄"},
            {"⣿⠛⠋⠁⠀⠀⠀⠀⠀⠀⠀⠀⠛⠛⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢿⠠⢿"},
            {"⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⠀⠈"},
            {"⢼⡇⠀⠀⠀⠀⠀⢀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡀⠀⠀⠀⠀⠀⠀⠀⣇⠀"},
            {"⣼⡇⠀⠀⠀⠀⠀⣿⣿⠆⠀⠀⠐⠒⠒⠒⠀⠀⢼⣿⡷⠀⠀⠀⠀⠀⠀⡇⠀"},
            {"⣿⡇⠀⠀⠀⠀⠀⠈⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠁⠀⠀⠀⠀⠀⠀⡇⠀"},
            {"⠀⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣧⡇"},
            {"⣾⡟⡷⢤⣤⣤⠴⣶⣶⣶⠶⣶⣶⣖⣲⠒⢒⣒⣶⣶⣷⣾⠒⣶⢲⠒⠲⣿⠇"},
        },
        {
            {"⠀⠀⠀                       ⠀⠀⠀"},
            {"⠀⠀⠀⣀⠤⠶⠒⠚⠛⠛⠳⠆⠀⠀⠀⠀⠀⠰⠞⠛⠛⠓⠒⠶⠤⣀⠀⠀⠀"},
            {"⠀⠀⠈⢀⢠⣶⣶⣿⣄⡢⡀⠀⠀⠀⠀⠀⠀⠀⠀⢔⣬⣿⣶⣶⡄⡀⠁⠀⠀"},
            {"⠀⠐⢶⣿⣿⣿⡟⠋⠙⢿⣾⠄⠀⠀⠀⠀⠀⠠⣷⡿⠋⠙⢻⣿⣿⣿⡶⠂⠀"},
            {"⠐⠒⢻⡏⢸⣿⡇⠀⠀⢸⡿⠀⠀⠀⠀⠀⠀⠘⢿⡇⠀⠀⢸⣿⡇⢹⡟⠒⠂"},
            {"⠀⠀⠀⠱⠈⠏⣹⣶⣶⣿⠇⠀⠀⠀⠀⠀⠀⠀⠸⠛⣶⣶⣿⡿⠁⠎⠀⠀⠀"},
            {"⠀⠀⠀⠀⠀⠨⣍⣛⣛⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⣛⣛⣩⠅⠀⠀⠀⠀⠀"},
            {"⠀⠀⠀                       ⠀⠀⠀"},
        },
    };

    std::string invidious_instance = "https://invidious.snopyta.org";

    setlocale(LC_CTYPE, "");
    initscr();
    noecho();
    curs_set(0);
    start_color();

    // For new videos
    init_color(COLOR_RED, 1000, 0, 0);
	init_pair(1, COLOR_RED, COLOR_BLACK);

    // Count channels
    std::ifstream namesAndIds_file("channel_names_and_ids");
    int length = 0;
    std::string line;
    while (std::getline(namesAndIds_file, line))
        length++;

    namesAndIds_file.close();
    //

    // Load unwached videos bool array
    bool unwachedVideos[length][15];
    for (int i=0; i < length; i++)
        for (int j=0; j<15; j++)
            unwachedVideos[i][j] = false;
    
    read_unwached_videos(unwachedVideos);
    //

    // Load names, ids and unwached videos count from file
    std::string channelNamesAndIds[length][2];

    int newVideosCount[length];
    read_names_ids_unwached_videos(channelNamesAndIds, newVideosCount,
            unwachedVideos);
    //

    // Load videos from file
    std::string allVideos[length][15][6];
    int videos_available[length];
    int res;

    res = read_videos_from_file(allVideos, length, videos_available); 
    if (res == -1)
    {
        printw("Channel IDS decreased, please reload all feeds to avoid bugs");
        mvprintw(1, 0, "Press ENTER to continue");
        refresh();
        getchar();
    }
    //

    // Load descriptions from file
    std::string allDescriptions[length][15];

    read_descriptions(allDescriptions);
    //


    // Fill menu
    WINDOW *pad = newpad(length+50, COLS);
    int menu_lines = length;

    std::string menu[menu_lines];
    std::string menu_line;

    for (int i=0; i < length; i++)
    {
        if (channelNamesAndIds[i][1] != "")
        {
            menu_line = generate_menu_line(i, newVideosCount[i],
                    channelNamesAndIds[i][1]);
            menu[i] = menu_line;
            if (newVideosCount[i] > 0)
                wattron(pad, COLOR_PAIR(1));
            mvwaddstr(pad, i, 0, menu[i].c_str());
            wattroff(pad, COLOR_PAIR(1));
        }
        // If name doesn't exist, replace it with channel ID
        else
        {
            menu_line = generate_menu_line(i, newVideosCount[i],
                    channelNamesAndIds[i][0]);
            menu_line = menu_line + "\tUpdate required"; 
            menu[i] = menu_line;
            mvwaddstr(pad, i, 0, menu[i].c_str());
        }
    }

    prefresh(pad, 0, 0, 0, 0, LINES-1, COLS);
    //

    int choice;
    int highlight = 0;
    int highlight_saved;
    int minrow = 0;
    int minrow_saved;

    int menu_index;
    int menu_page = 0;

    std::string videos[15][6];
    std::string descriptions[15];
    std::string author_name;

    std::string command;
    std:: string empty_line = " ";

    while ( (choice=wgetch(pad)) != 'q' )
    {
        switch (choice)
        {
            // Scroll down
            case 'j':
            highlight++;
            if (highlight == menu_lines)
                highlight = menu_lines-1;
            break;

            //Scroll up
            case 'k':
                highlight--;
                if (highlight == -1)
                    highlight = 0;
                break;

            // Go to end
            case 'G':
                mvwchgat(pad, highlight, 0, -1, WA_NORMAL, 0, NULL);
                highlight = menu_lines-1;
                minrow = menu_lines - LINES + 1;
                break;

            // Go to begining
            case 'g':
                choice = wgetch(pad);
                if (choice == 'g') 
                {
                    mvwchgat(pad, highlight, 0, -1, WA_NORMAL, 0, NULL);
                    highlight = 0;
                    minrow = 0;
                }
                break;

            // Open channel videos
            case 'l':
            {
                if (menu_page == 1)
                    break;
                memset(videos, 0, 15*6*sizeof videos[0][0]);

                // Fill menu with video titles
                if (allVideos[highlight][0][0] == "")
                    break;
                else{
                    wclear(pad);
                    mvwaddstr(pad, 0, COLS/3,
                            channelNamesAndIds[highlight][1].c_str());
                    for (int i=0; i < 15; i++)
                    {
                        menu_line = std::to_string(i) + "\t";
                        menu_line = menu_line + allVideos[highlight][i][0];
                        // Mark unwached videos
                        if ( unwachedVideos[highlight][i] == true )
                        {
                            wattron(pad, COLOR_PAIR(1));
                        }
                        mvwaddstr(pad, i+1, 0, menu_line.c_str());
                        wattroff(pad, COLOR_PAIR(1));
                    }
                }

                menu_lines = 16;
                menu_page = 1;
                highlight_saved = highlight;
                highlight = 1;
                minrow_saved = minrow;
                minrow = 0;
                break;
            }

            // Go back from videos
            case 'h':
            {
                if (menu_page != 1)
                    break;

                wclear(pad);
                for (int i=0; i < length; i++)
                {
                    // Update new videos count
                    if (i == highlight_saved)
                    {
                        menu_line = generate_menu_line(highlight_saved,
                                newVideosCount[highlight_saved],
                                channelNamesAndIds[highlight_saved][1]);

                            
                        menu[highlight_saved] = menu_line;
                    }
                    if (newVideosCount[i] > 0)
                        wattron(pad, COLOR_PAIR(1));
                    mvwaddstr(pad, i, 0, menu[i].c_str());
                    wattroff(pad, COLOR_PAIR(1));
                }

                menu_lines = length;
                menu_page = 0;

                highlight = highlight_saved;
                minrow = minrow_saved;
                
                break;
            }

            // Load single channel
            case 'r':
            {
                if (menu_page == 1)
                    break;
                //memset(videos, 0, 15*6*sizeof videos[0][0]);
                author_name = parse_feed(channelNamesAndIds[highlight][0],
                        videos, descriptions, invidious_instance);

                channelNamesAndIds[highlight][1] = author_name;

                //  Check for new videos
                if (allVideos[highlight][0][0] == "")
                    newVideosCount[highlight] = 15;
                else 
                {
                    for (int i=0; i < 15; i++)
                    {
                        if (allVideos[highlight][0][1] == videos[i][1])
                            break;

                        // If there a new video, move unwachedVideos array
                        // to right
                        for (int j=14; j>0; j--)
                            unwachedVideos[highlight][j] =
                                unwachedVideos[highlight][j-1];

                        newVideosCount[highlight]++;
                        unwachedVideos[highlight][i] = true;
                    }
                }
                //

                // Add videos to allVideos
                // Add descriptions to allDescriptions
                for (int i=0; i < 15; i++)
                {
                    allDescriptions[highlight][i] = descriptions[i];
                    for (int j=0; j < 6; j++)
                    {
                        allVideos[highlight][i][j] = videos[i][j];
                    }
                }
                //

                // Save new values
                save_names_ids_unwached_videos(channelNamesAndIds, length,
                        newVideosCount);
                save_all_videos(allVideos, length);
                save_descriptions(allDescriptions, length);
                save_unwached_videos_positions(unwachedVideos, length);
                //

                menu_line = generate_menu_line(highlight,
                        newVideosCount[highlight], author_name);

                menu[highlight] = menu_line;

                empty_line.resize(COLS, ' ');
                mvwaddstr(pad, highlight, 0, empty_line.c_str());

                mvwaddstr(pad, highlight, 0, menu[highlight].c_str());
                mvwaddstr(pad, highlight, 5, "V");
                prefresh(pad, minrow, 0, 0, 0, LINES-1, COLS);

                break;
            }

            // Load all entries
            case 'R':
            {
                if (menu_page == 1)
                    break;
                memset(videos, 0, 15*6*sizeof videos[0][0]);
                minrow = 0;
                menu_index = 0;
                for (int author_index=0; author_index < length; author_index++)
                {
                    author_name =
                        parse_feed(channelNamesAndIds[author_index][0],
                                videos, descriptions, invidious_instance);

                    channelNamesAndIds[author_index][1] = author_name;

                    // Check for new videos
                    if (allVideos[author_index][0][0] == "")
                        newVideosCount[author_index] = 15;
                    else 
                    {
                        for (int i=0; i<15; i++)
                        {
                            if (allVideos[author_index][0][1] == videos[i][1])
                                break;

                            // If there a new video, move unwachedVideos array
                            // to right
                            for (int j=14; j>0; j--)
                                unwachedVideos[author_index][j] =
                                    unwachedVideos[author_index][j-1];

                            newVideosCount[author_index]++;
                            unwachedVideos[author_index][i] = true;
                        }
                    }
                    //

                    // Fill allVideos array
                    // Fill allDescriptions
                    for (int video_index=0; video_index < 15; video_index++)
                    {
                        allDescriptions[author_index][video_index] =
                            descriptions[video_index];
                        for (int value_index=0; value_index < 6;
                                value_index++)
                        {
                            allVideos[author_index][video_index][value_index]=
                                videos[video_index][value_index];
                        }
                    }
                    //

                    menu_line = generate_menu_line(menu_index,
                            newVideosCount[author_index], author_name);

                    // Show total progress in top-right corner
                    std::string progress_count =
                        "( " + std::to_string(menu_index+1) + " / " +
                        std::to_string(length) + " ) ";

                    menu[menu_index] = menu_line;

                    wmove(pad, menu_index, 0);
                    wclrtoeol(pad);

                    if (newVideosCount[menu_index] > 0)
                        wattron(pad, COLOR_PAIR(1));

                    mvwaddstr(pad, menu_index, 0, menu[menu_index].c_str());

                    wattroff(pad, COLOR_PAIR(1));

                    mvwaddstr(pad, 0, COLS-12, progress_count.c_str());
                    //

                    // Highlight updating lines
                    mvwchgat(pad, author_index, 0, -1,
                            COLOR_PAIR(1), 1, NULL);
                    // Color authors with new videos
                    if (newVideosCount[author_index-1] > 0)
                        mvwchgat(pad, author_index-1, 0, -1,
                                COLOR_PAIR(1), 1, NULL);
                    else
                        mvwchgat(pad, author_index-1, 0, -1,
                                WA_NORMAL, 0, NULL);
                    //

                    if (menu_index == LINES)
                        minrow = LINES;

                    prefresh(pad, minrow, 0, 0, 0, LINES-1, COLS);

                    menu_index++;
                }

                // De-highlight last line
                mvwchgat(pad, length-1, 0, -1, WA_NORMAL, 0, NULL);

                // Jump back to highlighted item
                minrow = highlight - 15;

                // Save new values
                save_names_ids_unwached_videos(channelNamesAndIds, length,
                        newVideosCount);
                save_all_videos(allVideos, length);
                save_descriptions(allDescriptions, length);
                save_unwached_videos_positions(unwachedVideos, length);
                //
                break;
            }

            // Set channel/video as watched
            case 'w':
            {
                // If channel
                if (menu_page == 0)
                {
                    // Set new videos to 0
                    newVideosCount[highlight] = 0;
                    // Set all unwachedVideos (positions) to 0
                    for (int i=0; i<15; i++)
                        unwachedVideos[highlight][i] = false;

                    menu[highlight] = generate_menu_line(highlight,
                            newVideosCount[highlight], 
                            channelNamesAndIds[highlight][1]);
                    wattron(pad, A_REVERSE);
                    mvwaddstr(pad, highlight, 0, menu[highlight].c_str());
                    wattroff(pad, A_REVERSE);

                    // Move selection down
                    highlight++;

                    // Save new values
                    save_names_ids_unwached_videos(channelNamesAndIds, length,
                            newVideosCount);
                    save_unwached_videos_positions(unwachedVideos, length);
                    prefresh(pad, minrow, 0, 0, 0, LINES-1, COLS);
                    break;
                }
                // If video
                else 
                {
                    if (unwachedVideos[highlight_saved][highlight-1] == true)
                        newVideosCount[highlight_saved]--;        
                    unwachedVideos[highlight_saved][highlight-1] = false;
                    if (newVideosCount[highlight_saved] == -1)
                        newVideosCount[highlight_saved] = 0;

                    // Save new values
                    save_unwached_videos_positions(unwachedVideos, length);
                    save_names_ids_unwached_videos(channelNamesAndIds, length,
                            newVideosCount);
                }
                break;
            }

            // Show video / channel info
            case 'i':
            {
                if (menu_page == 0)
                    break;

                // Size for info window
                int y1 = 0;
                int x1 = COLS/3;
                int y2 = LINES;
                int x2 = COLS;
                int width = COLS-COLS/3;
                int height = LINES;

                WINDOW *infoPad = newpad(200, 300); 
                WINDOW *infoBox = newwin(height, width, y1, x1); 
                WINDOW *imageBox = newwin(10, 31, 0, 0); 

                std::string description =
                    allDescriptions[highlight_saved][highlight-1];

                // Date values
                std::string date = allVideos[highlight_saved][highlight-1][2];
                std::string stringDateDifference = "Published: " +
                    std::to_string(calculate_date_diff(date)) + " days ago";

                box(infoBox, 0, 0);
                box(imageBox, 0, 0);
                // Title
                mvwaddstr(infoPad, 0, 0,
                        allVideos[highlight_saved][highlight-1][0].c_str());
                // Published date
                mvwaddstr(infoPad, 1, 0, date.c_str());
                // Show how many days ago was published
                mvwaddstr(infoPad, 2, 0, stringDateDifference.c_str());
                // Invidious link
                mvwaddstr(infoPad, 3, 0,
                        allVideos[highlight_saved][highlight-1][4].c_str());
                // Youtube link
                mvwaddstr(infoPad, 4, 0,
                        allVideos[highlight_saved][highlight-1][5].c_str());
                // Description
                mvwaddstr(infoPad, 5, 0, "------------------------------");
                mvwaddstr(infoPad, 6, 0, description.c_str());

                // Show image
                bool imageExist = true;
                command = "/usr/bin/kitty +kitten icat --place 30x20@0x1 " +
                        allVideos[highlight_saved][highlight-1][3];
                std::system(command.c_str());
                
                // Fill image box with ASCII art
                int rand_ascii = rand() % ascii_size;

                for (int i=1; i<9; i++)
                {
                    mvwaddstr(imageBox, i, 1,
                            ascii_art[rand_ascii][i-1].c_str());
                }
                //

                // Refresh
                wrefresh(infoBox);
                prefresh(infoPad, 0, 0, y1+1, x1+1, y2-2, x2-2);
                wrefresh(imageBox);
                //

                char infoChoice;
                int infoMinrow = 0;
                int infoMinline = 0;
                while (true)
                {
                    infoChoice = wgetch(infoPad);

                    // Clear image ( it's bugged when scrolling )
                    if (imageExist)
                    {
                        command = "/usr/bin/kitty +kitten icat --clear";
                        std::system(command.c_str());
                        box(infoBox, 0, 0);
                        wrefresh(infoBox);
                        imageExist = false;
                    }

                    switch (infoChoice)
                    {
                        case 'j':
                        {
                            infoMinrow++;
                            break;
                        }
                        case 'k':
                        {
                            infoMinrow--;
                            if (infoMinrow == -1)
                                infoMinrow = 0;
                            break;
                        }
                        case 'h':
                        {
                            infoMinline -= 3;
                            if (infoMinline < 0)
                                infoMinline = 0;
                            break;
                        }
                        case 'l':
                        {
                            infoMinline += 3;
                            break;
                        }
                    }

                    if (infoChoice == 'q')
                        break;
                    prefresh(infoPad, infoMinrow, infoMinline,
                            y1+1, x1+1, y2-2, x2-2);
                }

                delwin(infoPad);
                delwin(infoBox);
                delwin(imageBox);
                break;
            }

            // Open video in mpv
            case 'o':
            {
                if (menu_page == 0)
                    break;
                command = "mpv --ytdl-format='bestvideo[height<=?1080]"
                    "+bestaudio/best' " +
                    allVideos[highlight_saved][highlight-1][5];
                std::system(command.c_str());
                break;
            }
            // Open in browser
            case 'O':
            {
                // Channel
                if (menu_page == 0)
                {
                    command = "xdg-open " + invidious_instance +
                        "/channel/" + channelNamesAndIds[highlight][0] + " &";
                    std::system(command.c_str());
                    break;
                }

                // Video
                command = "xdg-open " + 
                    allVideos[highlight_saved][highlight-1][4];
                std::system(command.c_str());
                break;
            }

            // Copy youtube link
            case 'c':
            {
                if (menu_page == 0)
                    break;
                command = "echo " + allVideos[highlight_saved][highlight-1][5]
                    + " | xclip -selection c";
                std::system(command.c_str());
                break;
            }

            // Sort channels by name
            // Unwached videos and descriptions will break
            // as they are binded to channel indexes
            // So full update on feeds is required
            case 'S':
            {
                if (menu_page == 1)
                    break;

                sort_2d_array(channelNamesAndIds, length, allVideos);

                // Fill menu with new indexes
                for (int i=0; i < length; i++)
                {
                    if (channelNamesAndIds[i][1] != "")
                    {
                        menu_line = generate_menu_line(i, newVideosCount[i],
                                channelNamesAndIds[i][1]);
                        menu[i] = menu_line;
                        if (newVideosCount[i] > 0)
                            wattron(pad, COLOR_PAIR(1));
                        mvwaddstr(pad, i, 0, menu[i].c_str());
                        wattroff(pad, COLOR_PAIR(1));
                    }
                    // If name doesn't exist, replace it with channel ID
                    else
                    {
                        menu_line = generate_menu_line(i, newVideosCount[i],
                                channelNamesAndIds[i][0]);
                        menu_line = menu_line + "\tUpdate required"; 
                        menu[i] = menu_line;
                        mvwaddstr(pad, i, 0, menu[i].c_str());
                    }
                }

                // Save sorted channels and videos
                save_names_ids_unwached_videos(channelNamesAndIds, length,
                        newVideosCount) ;
                save_all_videos(allVideos, length);

                break;
            }
        // Switch case ends here 
        }
        
            
        // Keep highilght in screen
        if (highlight > (minrow + LINES - 3))
            minrow++;

        if (highlight < (minrow + 3))
            minrow--;

        if (minrow == menu_lines+1)
            minrow = menu_lines;
        if (minrow == -1)
            minrow = 0;
        //

        // Highlight selected line
        if (menu_page == 0)
        {
            // If channel highlighted because of new videos
            if (newVideosCount[highlight] == 0)
                mvwchgat(pad, highlight, 0, -1, A_REVERSE, 0, NULL);
            else
                mvwchgat(pad, highlight, 0, -1, A_REVERSE, 1, NULL);
        }
        else
        {
            // If video highlighted because it's unwached
            if (unwachedVideos[highlight_saved][highlight-1] == false)
                mvwchgat(pad, highlight, 0, -1, A_REVERSE, 0, NULL);
            else
                mvwchgat(pad, highlight, 0, -1, A_REVERSE, 1, NULL);
        }

        // De - highlight previous lines
        // Color unwached videos
        if (menu_page == 1)
        {
            //if (highlight-2 < newVideosCount[highlight_saved])
            if (unwachedVideos[highlight_saved][highlight-2])
            {
                mvwchgat(pad, highlight-1, 0, -1,
                        COLOR_PAIR(1), 1, NULL);
            }
            // Color channels with unwached videos
            else
            {
                mvwchgat(pad, highlight-1, 0, -1, WA_NORMAL, 0, NULL);
            }
        }
        // Color channels with unwached videos
        else 
        {
            if (newVideosCount[highlight-1] > 0)
                mvwchgat(pad, highlight-1, 0, -1, COLOR_PAIR(1), 1, NULL);
            else
                mvwchgat(pad, highlight-1, 0, -1, WA_NORMAL, 0, NULL);

        }

        // Color unwached videos
        if (menu_page == 1)
        {
            //if (highlight < newVideosCount[highlight_saved])
            if (unwachedVideos[highlight_saved][highlight])
            {
                mvwchgat(pad, highlight+1, 0, -1,
                        COLOR_PAIR(1), 1, NULL);
            }
            else
                mvwchgat(pad, highlight+1, 0, -1, WA_NORMAL, 0, NULL);
        }
        // Color channels with unwached videos
        else 
        {
            if (newVideosCount[highlight+1] > 0)
                mvwchgat(pad, highlight+1, 0, -1, COLOR_PAIR(1), 1, NULL);
            else
                mvwchgat(pad, highlight+1, 0, -1, WA_NORMAL, 0, NULL);
        }

        prefresh(pad, minrow, 0, 0, 0, LINES-1, COLS);

    // While ends here
    }

    delwin(pad);
    endwin();
    return 0;
}
