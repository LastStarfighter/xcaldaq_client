#include "Client.h"
#include "ClientGUI.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include "misc_utils.h"
#include "string_utils.h"

#include <FL/Fl_File_Chooser.H>

const std::string mixing_ext=".mixing_config";

// static callback function
void cb_mixing_state_load_dialog ( Fl_Widget * w, void * v)
{
    ((ClientGUI *)v)->mixing_state_load_dialog();
}


void cb_mixing_state_save_dialog ( Fl_Widget * w, void * v)
{
    ((ClientGUI *)v)->mixing_state_save_dialog();
}


void cb_mixing_state_load_mconf1 ( Fl_Widget * w, void * v)
{
    std::string filename="mconf1"+mixing_ext;
    ((ClientGUI *)v)->client->load_mixing_state_from_file(filename);
}


void cb_mixing_state_save_default ( Fl_Widget * w, void * v)
{
    std::string filename="default"+mixing_ext;
    ((ClientGUI *)v)->client->save_mixing_state_to_file(filename);
}


void cb_mixing_state_load_default ( Fl_Widget * w, void * v)
{
    std::string filename="default"+mixing_ext;
    ((ClientGUI *)v)->client->load_mixing_state_from_file(filename);
}


void cb_mixing_state_save_mconf1 ( Fl_Widget * w, void * v)
{
    std::string filename="mconf1"+mixing_ext;
    ((ClientGUI *)v)->client->save_mixing_state_to_file(filename);
}


void cb_mixing_state_load_mconf2 ( Fl_Widget * w, void * v)
{
    std::string filename="mconf2"+mixing_ext;
    ((ClientGUI *)v)->client->load_mixing_state_from_file(filename);
}


void cb_mixing_state_save_mconf2 ( Fl_Widget * w, void * v)
{
    std::string filename="mconf2"+mixing_ext;
    ((ClientGUI *)v)->client->save_mixing_state_to_file(filename);
}


void cb_mixing_state_load_mconf3 ( Fl_Widget * w, void * v)
{
    std::string filename="mconf3"+mixing_ext;
    ((ClientGUI *)v)->client->load_mixing_state_from_file(filename);
}


void cb_mixing_state_save_mconf3 ( Fl_Widget * w, void * v)
{
    std::string filename="mconf3"+mixing_ext;
    ((ClientGUI *)v)->client->save_mixing_state_to_file(filename);
}


void cb_mixing_state_load_mconf4 ( Fl_Widget * w, void * v)
{
    std::string filename="mconf4"+mixing_ext;
    ((ClientGUI *)v)->client->load_mixing_state_from_file(filename);
}


void cb_mixing_state_save_mconf4 ( Fl_Widget * w, void * v)
{
    std::string filename="mconf4"+mixing_ext;
    ((ClientGUI *)v)->client->save_mixing_state_to_file(filename);
}


void ClientGUI::mixing_state_load_dialog(void)
{
    char *c_filename = NULL;

    std::string filter_str="*"+ mixing_ext;

    // display the file selector

    c_filename = fl_file_chooser("Output File",filter_str.c_str(),NULL);

    if (c_filename==NULL) return;

    std::string filename(c_filename);

    std::cout << "\n We are going to open filename " << filename;
    std::cout.flush();

    if (client!=NULL)
    {
        client->load_mixing_state_from_file(filename);

        // make sure the mix window reflects the new values
        mixControlWindow_userRefresh();
        //	streamChannelWindow_userRefresh();
        //mixingRateWindow_userRefresh();
        //mixingControlWindow_userRefresh();
    }
    else
        std::cout << "\n client pointer is NULL, error in mixing_state_load_dialog, start stream?";

    std::cout.flush();
}


void ClientGUI::mixing_state_save_dialog(void)
{
    char *c_filename = NULL;

    std::string filter_str="*"+ mixing_ext;

    // display the file selector

    c_filename = fl_file_chooser("Output File",filter_str.c_str(),NULL);

    if (c_filename==NULL) return;

    std::string filename(c_filename);

    std::cout << "\n We are going to save filename " << filename;
    std::cout.flush();

    if (client!=NULL)
        client->save_mixing_state_to_file(filename);
    else
        std::cout << "\n client pointer is NULL, start stream?";

    std::cout.flush();
}


namespace xdaq
{

    std::string Client::pretty_print_mixing_state(void) const
    {
        std::stringstream a;

        for (unsigned int chan=0;chan<m_nDataStreams;chan++)
        {
            std::stringstream chan_name;
            chan_name << "CH" << chan << "_";
            a << streamData[chan]->pretty_print_mixing_state("\n",chan_name.str());
        }
        return a.str();
    }

    void Client::save_mixing_state_to_file(const std::string& basename, bool append_extension) const
    {

        std::string filename=basename;

        if (append_extension)
            filename += mixing_ext;

        std::cout << "\n Writing mixing state to file: " << filename;
        std::cout.flush();

        // make an simple ascii file
        std::ofstream ofs(filename.c_str());

        ofs << pretty_print_mixing_state();
        ofs << "\n";

    }

    void Client::load_generic_config_from_file_into_stream_data(const std::string& filename)
    {

        //        const std::string ext="mixing_config";
        //   std::string filename=basename + "." + ext;
        std::deque<std::string>lines;

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

    }

    void Client::load_mixing_state_from_file(const std::string& filename)
    {

        //        const std::string ext="mixing_config";
        //   std::string filename=basename + "." + ext;
        std::deque<std::string>lines;

        std::cout << "\n Reading mixing state from file: " << filename;
        std::cout.flush();

        load_generic_config_from_file_into_stream_data(filename);

        // jonathan has the mixing flags and inverse mixing flags in
        // both the client object & the stream data objects
        //
        // this is the workaround for this
        //
        // the client mix functions set the client state & the server

        // next we want to sync this up with the server
        push_stream_data_mixing_state_to_server_and_client();

        //read back state from server
        //	updateClientToServer();
        std::cout << "\n Done reading mixing state\n";
        std::cout.flush();

    }

    void Client::push_stream_data_mixing_state_to_server_and_client(void)
    {
        //         printf ("\n");

        //         // get the decimation levels
        //         for (unsigned int i=0; i<m_nDataStreams; i++)
        //         {

        // 	  server->set(XCD_MIXFLAG,i,m_mixFlag[i]);
        // 	  server->set(XCD_MIXINVERSIONFLAG,i,m_mixInversionFlag[i]);
        // 	  server->set(XCD_MIXLEVEL,i,streamData[i]->mixLevel());
        // 	  //	  mixFlag(i,mixFlag(i));
        // 	  //	  mixLevel(i,mixLevel(i));
        // 	  //mixInversionFlag(i,mixInversionFlag(i));

        // 	  printf ("\nCH%02d: Setting server mix flag to %d",i,mixFlag(i));
        // 	  printf ("\nCH%02d: Setting server mix iflag to %d",i,mixInversionFlag(i));
        // 	  printf ("\nCH%02d: Setting server mix level to %1.4g",i,mixLevel(i));

        //         }
        //     }

        //    void Client::set_client_mixing_state_from_stream_data(void)
        //     {

        for (unsigned int i=0; i<m_nDataStreams; i++)
        {

            //  server->set(XCD_MIXFLAG,i,m_mixFlag[i]);
            //server->set(XCD_MIXINVERSIONFLAG,i,m_mixInversionFlag[i]);
            //server->set(XCD_MIXLEVEL,i,streamData[i]->mixLevel());

            mixFlag(i,streamData[i]->mixFlag());
            mixInversionFlag(i,streamData[i]->mixInversionFlag());
            mixLevel(i,streamData[i]->mixLevel());

            printf ("\nCH%02d: Setting server mix flag to %d",i,mixFlag(i));
            printf ("\nCH%02d: Setting server mix iflag to %d",i,mixInversionFlag(i));
            printf ("\nCH%02d: Setting server mix level to %1.4g",i,mixLevel(i));

        }
    }

}
