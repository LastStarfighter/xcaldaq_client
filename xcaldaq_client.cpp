////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#include "xcaldaq_client.h"

////////////////////////////////////////////////////////////////////////////////
int main(int argc,char **argv)
{

    // setup the client
    xdaq::Client *client;
    client = new xdaq::Client();

    // read the config file
    client->readConfigFile();

    // init the client
    //client->init();

    // if you were going to do command line over rides, now is the place to do it!!

    // setup the GUI
    ClientGUI *GUI;
    GUI = new ClientGUI();

    // make sure GUI can see the client
    GUI->client = client;

    if (argc >= 2)
    {

        client->trigger_config_filename=std::string(argv[1]);
        std::cout << "\nSetting Triger configuration file to: ";
        std::cout << client->trigger_config_filename;
        std::cout.flush();

    }
    // initialize the gui and get it ready to display
    GUI->initGUI();

    // reveil the windows
    GUI->startGUI();

    // turn over thread control to the GUI
    Fl::run();                                    // this blocks until all windows close

    // now stop the client
    client->shutdown();

    std::cout << "That's all folks!" << std::endl;
}


////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
