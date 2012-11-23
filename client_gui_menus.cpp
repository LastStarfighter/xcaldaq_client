#include "ClientGUI.h"
// void quit_cb(Fl_Widget*, void*) {exit(0);}

// void test_cb(Fl_Widget* w, void*)
// {
//     Fl_Menu_* mw = (Fl_Menu_*)w;
//     const Fl_Menu_Item* m = mw->mvalue();
//     if (!m)
//         printf("NULL\n");
//     else if (m->shortcut())
//         printf("%s - %s\n", m->label(), fl_shortcut_label(m->shortcut()));
//     else
//         printf("%s\n", m->label());
// }

void ClientGUI::build_menu(int menu_width,int menu_height)
{

    Fl_Menu_Bar* menubar = new Fl_Menu_Bar(0, 0, menu_width, menu_height);
    // menubar->menu(menutable);

    // userdata is always 'this
    //    menubar->add("File/Open", 0, quit_cb, (void*)this);
    menubar->add("&File/Quit", 0, cb_quit, (void*)this);

    menubar->add("&Trigger Config/Load", FL_ALT+'l', cb_trigger_state_load_dialog, (void*)this);
    menubar->add("Trigger Config/Save as", FL_ALT+'l', cb_trigger_state_save_dialog, (void*)this);

    menubar->add("Trigger Config/Load Default", FL_ALT+'d', cb_trigger_state_load_default, (void*)this);
    menubar->add("Trigger Config/Save as Default", FL_ALT+FL_SHIFT+'d', cb_trigger_state_save_default, (void*)this,FL_MENU_DIVIDER);

    menubar->add("Trigger Config/Load TCONF_1", FL_ALT+'1',  cb_trigger_state_load_tconf1, (void*)this);
    menubar->add("Trigger Config/Load TCONF_2", FL_ALT+'2',  cb_trigger_state_load_tconf2, (void*)this);
    menubar->add("Trigger Config/Load TCONF_3", FL_ALT+'3',  cb_trigger_state_load_tconf3, (void*)this);
    menubar->add("Trigger Config/Load TCONF_4", FL_ALT+'4',  cb_trigger_state_load_tconf4, (void*)this,FL_MENU_DIVIDER);

    menubar->add("Trigger Config/Save TCONF_1", FL_ALT+FL_SHIFT+'1', cb_trigger_state_save_tconf1, (void*)this);
    menubar->add("Trigger Config/Save TCONF_2", FL_ALT+FL_SHIFT+'2', cb_trigger_state_save_tconf2, (void*)this);
    menubar->add("Trigger Config/Save TCONF_3", FL_ALT+FL_SHIFT+'3', cb_trigger_state_save_tconf3, (void*)this);
    menubar->add("Trigger Config/Save TCONF_4", FL_ALT+FL_SHIFT+'4', cb_trigger_state_save_tconf4, (void*)this,FL_MENU_DIVIDER);

    ////////////////////////// Mixing state menu
    menubar->add("&Mixing Config/Load", FL_CTRL+'l', cb_mixing_state_load_dialog, (void*)this);
    menubar->add("Mixing Config/Save as", FL_CTRL+'l', cb_mixing_state_save_dialog, (void*)this);

    menubar->add("Mixing Config/Load Default", FL_CTRL+'d', cb_mixing_state_load_default, (void*)this);
    menubar->add("Mixing Config/Save as Default", FL_CTRL+FL_SHIFT+'d', cb_mixing_state_save_default, (void*)this,FL_MENU_DIVIDER);

    menubar->add("Mixing Config/Load MCONF_1", FL_CTRL+'1',  cb_mixing_state_load_mconf1, (void*)this);
    menubar->add("Mixing Config/Load MCONF_2", FL_CTRL+'2',  cb_mixing_state_load_mconf2, (void*)this);
    menubar->add("Mixing Config/Load MCONF_3", FL_CTRL+'3',  cb_mixing_state_load_mconf3, (void*)this);
    menubar->add("Mixing Config/Load MCONF_4", FL_CTRL+'4',  cb_mixing_state_load_mconf4, (void*)this,FL_MENU_DIVIDER);

    menubar->add("Mixing Config/Save MCONF_1", FL_CTRL+FL_SHIFT+'1', cb_mixing_state_save_mconf1, (void*)this);
    menubar->add("Mixing Config/Save MCONF_2", FL_CTRL+FL_SHIFT+'2', cb_mixing_state_save_mconf2, (void*)this);
    menubar->add("Mixing Config/Save MCONF_3", FL_CTRL+FL_SHIFT+'3', cb_mixing_state_save_mconf3, (void*)this);
    menubar->add("Mixing Config/Save MCONF_4", FL_CTRL+FL_SHIFT+'4', cb_mixing_state_save_mconf4, (void*)this,FL_MENU_DIVIDER);

    menubar->add("Help/About", 0, 0, (void*)this);// userdata is always 'this

}
