#include "Client.h"
#include "ClientGUI.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include "misc_utils.h"
#include "string_utils.h"

#include <FL/Fl_File_Chooser.H>

const std::string ext=".trigger_config";
void cb_quit ( Fl_Widget * w, void * v)
{

    std::cout << "Quitting" << std::endl;

    ((ClientGUI *)v)->stopGUI();
}


// static callback function
void cb_trigger_state_load_dialog ( Fl_Widget * w, void * v)
{
    ((ClientGUI *)v)->trigger_state_load_dialog();
}


void cb_trigger_state_save_dialog ( Fl_Widget * w, void * v)
{
    ((ClientGUI *)v)->trigger_state_save_dialog();
}


void cb_trigger_state_load_tconf1 ( Fl_Widget * w, void * v)
{
    std::string filename="tconf1"+ext;
    ((ClientGUI *)v)->client->load_trigger_state_from_file(filename);
}


void cb_trigger_state_save_default ( Fl_Widget * w, void * v)
{
    std::string filename="default"+ext;
    ((ClientGUI *)v)->client->save_trigger_state_to_file(filename);
}


void cb_trigger_state_load_default ( Fl_Widget * w, void * v)
{
    std::string filename="default"+ext;
    ((ClientGUI *)v)->client->load_trigger_state_from_file(filename);
}


void cb_trigger_state_save_tconf1 ( Fl_Widget * w, void * v)
{
    std::string filename="tconf1"+ext;
    ((ClientGUI *)v)->client->save_trigger_state_to_file(filename);
}


void cb_trigger_state_load_tconf2 ( Fl_Widget * w, void * v)
{
    std::string filename="tconf2"+ext;
    ((ClientGUI *)v)->client->load_trigger_state_from_file(filename);
}


void cb_trigger_state_save_tconf2 ( Fl_Widget * w, void * v)
{
    std::string filename="tconf2"+ext;
    ((ClientGUI *)v)->client->save_trigger_state_to_file(filename);
}


void cb_trigger_state_load_tconf3 ( Fl_Widget * w, void * v)
{
    std::string filename="tconf3"+ext;
    ((ClientGUI *)v)->client->load_trigger_state_from_file(filename);
}


void cb_trigger_state_save_tconf3 ( Fl_Widget * w, void * v)
{
    std::string filename="tconf3"+ext;
    ((ClientGUI *)v)->client->save_trigger_state_to_file(filename);
}


void cb_trigger_state_load_tconf4 ( Fl_Widget * w, void * v)
{
    std::string filename="tconf4"+ext;
    ((ClientGUI *)v)->client->load_trigger_state_from_file(filename);
}


void cb_trigger_state_save_tconf4 ( Fl_Widget * w, void * v)
{
    std::string filename="tconf4"+ext;
    ((ClientGUI *)v)->client->save_trigger_state_to_file(filename);
}


void ClientGUI::trigger_state_load_dialog(void)
{
    char *c_filename = NULL;

    std::string filter_str="*"+ ext;

    // display the file selector

    c_filename = fl_file_chooser("Output File",filter_str.c_str(),NULL);

    if (c_filename==NULL) return;

    std::string filename(c_filename);

    std::cout << "\n We are going to open filename " << filename;
    std::cout.flush();

    if (client!=NULL)
    {
        client->load_trigger_state_from_file(filename);

        // make sure the mix window reflects the new values
        mixControlWindow_userRefresh();
        streamChannelWindow_userRefresh();
        triggerRateWindow_userRefresh();
        triggerControlWindow_userRefresh();
    }
    else
        std::cout << "\n client pointer is NULL, error in trigger_state_load_dialog, start stream?";

    std::cout.flush();
}


void ClientGUI::trigger_state_save_dialog(void)
{
    char *c_filename = NULL;

    std::string filter_str="*"+ ext;

    // display the file selector

    c_filename = fl_file_chooser("Output File",filter_str.c_str(),NULL);

    if (c_filename==NULL) return;

    std::string filename(c_filename);

    std::cout << "\n We are going to save filename " << filename;
    std::cout.flush();

    if (client!=NULL)
        client->save_trigger_state_to_file(filename);
    else
        std::cout << "\n client pointer is NULL, start stream?";

    std::cout.flush();
}


namespace xdaq
{

    std::string Client::pretty_print_trigger_state(void) const
    {
        std::stringstream a;

        for (unsigned int chan=0;chan<m_nDataStreams;chan++)
        {
            std::stringstream chan_name;
            chan_name << "CH" << chan << "_";
            a << streamData[chan]->pretty_print_trigger_state("\n",chan_name.str());
        }
        return a.str();
    }

    void Client::save_trigger_state_to_file(const std::string& basename, bool append_extension) const
    {

        std::string filename=basename;

        if (append_extension)
            filename += ext;

        std::cout << "\n Writing trigger state to file: " << filename;
        std::cout.flush();

        // make an simple ascii file
        std::ofstream ofs(filename.c_str());

        ofs << pretty_print_trigger_state();
        ofs << "\n";

    }

    void Client::load_trigger_state_from_file(const std::string& filename)
    {

        //        const std::string ext="trigger_config";
        //   std::string filename=basename + "." + ext;
        std::deque<std::string>lines;

        std::cout << "\n Reading trigger state from file: " << filename;
        std::cout.flush();

        lines=util::slurp_file(filename);

        for(unsigned int i=0; i < lines.size(); i++)
        {

            std::string line=lines[i];

            // first we are going to split the line into components
            // "CH0_autoThreshold: 1000"

            std::deque<std::string> tokens;

            // remove excess spaces
            gsd::str::search_and_replace_all(line, " ", "");

            // convert _ to :
            gsd::str::search_and_replace_all(line, "_", ":");

            if (line.size()>0)
            {
                tokens=gsd::str::split(':',line);

                if (tokens.size() == 3)
                {
                    std::string ch_name=tokens[0];
                    std::string parameter_name=tokens[1];

                    std::string value=tokens[2];

                    // remove the CH from the channel name
                    gsd::str::search_and_replace_all(ch_name, "CH", "");

                    unsigned int ch=gsd::str::string_to_val<int>(ch_name);

                    //                 std::cout << "\n line=<" << line << ">";
                    //                 std::cout << "\t ch=<" << ch << ">";
                    //                 std::cout << "\t parameter_name=<" << parameter_name << ">";
                    //                 std::cout << "\t value=<" << value << ">";
                    //                 std::cout.flush();

                    if ( (ch >=0) && (ch < m_nDataStreams))
                    {

                        streamData[ch]->set_parameter(parameter_name,value);
                    }
                    else
                    {
                        std::cout << "\n ch=" << ch;
                        std::cout << "\t <<<<WARNING: Ignoring nonexistant channel";

                    }

                }
                else
                {

                    std::cout << "\n line=<" << line << ">";
                    std::cout << "\t <<<<BAD LINE>>>";

                }
            }
        }

        std::cout << "\n Done reading trigger state\n";
        std::cout.flush();

    }
}
