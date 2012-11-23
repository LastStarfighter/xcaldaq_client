#include "misc_utils.h"

#include <sstream>
#include <fstream>
#include <iostream>

namespace util
{
    std::deque<std::string> slurp_file(const std::string& filename)
    {

        std::ifstream ifs ( filename.c_str() , std::ifstream::in );
        //	std::ifstream ifs(filename.c_str());

        if (!ifs)
        {

            std::string msg="Error opening file: " + filename;
            //gsd::warning("card_factory",msg.c_str());

        }

        std::deque<std::string> lines;
        while (ifs.good())
        {

            std::string line;
            //ifs.getline(inf,line)

            std::getline(ifs,line);
            lines.push_back(line);
            // cout << (char) ifs.get();
        }

        ifs.close();

        return lines;

    }

}
