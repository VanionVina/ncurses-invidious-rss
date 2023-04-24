menu: menu.cpp read_write.h rss_parse.h
	g++ -lcurses -ljsoncpp -lpugixml -lcurl menu.cpp -o menu
