#include <iostream>
#include <stdio.h>
#include <string>
#include <pugixml.hpp>
#include <curl/curl.h>

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp){
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string parse_feed(std::string channel_id, std::string author_videos[15][6],
        std::string descriptions[15], std::string invidious_instance)
{
    std::string url;
    std::string readBuffer;

    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    std::string author_name;

    if(curl) 
    {
        // Get feed (XML)
        url = invidious_instance + "/feed/channel/" + channel_id;

        readBuffer.clear();

        curl_easy_setopt(curl, CURLOPT_USERAGENT,
            "Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_perform(curl);
        curl_easy_reset(curl);
        
        // Parse XML
        pugi::xml_document doc;
        pugi::xml_parse_result parsed = doc.load_string(readBuffer.c_str());
        if (parsed) 
        {
            pugi::xml_node feed = doc.child("feed");

            pugi::xml_node link = feed.child("link");
            pugi::xml_attribute href = link.attribute("href");
            pugi::xml_node author = feed.child("title");

            author_name = author.child_value();

            int videos_count = 0;

            for ( pugi::xml_node entry = feed.child("entry"); entry; 
                    entry = entry.next_sibling())
            {
                pugi::xml_node image = entry.child("media:group")
                    .child("media:thumbnail");
                pugi::xml_attribute image_url = image.attribute("url");

                std::string yt_videoId = entry.child_value("yt:videoId");

                pugi::xml_node descr = entry.child("media:group")
                    .child("media:description");

                link = entry.child("link");
                href = link.attribute("href");

                std::string yt_link = "https://www.youtube.com/watch?v="
                    + yt_videoId;

                // Fill given array video[i] with values
                author_videos[videos_count][0] =
                    entry.child_value("title");
                author_videos[videos_count][1] =
                    yt_videoId;
                author_videos[videos_count][2] =
                    entry.child_value("published");
                author_videos[videos_count][3] =
                    image_url.value();
                author_videos[videos_count][4] =
                    href.value();
                author_videos[videos_count][5] =
                    yt_link;
                descriptions[videos_count] = descr.child_value();
                //author_videos[videos_count][6] =
                    //descr.child_value();

                videos_count++;
            }
        }
        else
        {
            std::cout << "Error while parsing XML" << std::endl;
            std::cout << parsed.description() << std::endl;
        }
    }
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return author_name;
}
