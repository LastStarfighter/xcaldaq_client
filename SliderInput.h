#ifndef inputslider_h
#define inputslider_h
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Slider.H>
#include <cstdio>
#include <iostream>
#include <sstream>

// sliderinput -- simple example of tying an fltk slider and input widget together
// 1.00 erco 10/17/04

class SliderInput : public Fl_Group
{
    Fl_Int_Input *input;
    Fl_Slider    *slider;

    //Utility
    void ivalue(int val)
    {
        std::ostringstream ss;
        ss << val;
        std::cout << "Ival: "<<ss.str().c_str() << std::endl;
        input->value(ss.str().c_str());
    }

    // CALLBACK HANDLERS
    //    These 'attach' the input and slider's values together.
    //
    void Slider_CB2()
    {
        static int recurse = 0;
        if ( recurse )
            { return; }
            else
        {
            recurse = 1;
            char s[80];
            sprintf(s, "%d", (int)(slider->value() + .5));
            // fprintf(stderr, "SPRINTF(%d) -> '%s'\n", (int)slider->value(), s);
            input->value(s);                      // pass slider's value to input
            recurse = 0;
            if (m_extraCallback != NULL)
            {
                m_extraCallback(this,m_extraCBData);
            }
        }
    }

    static void Slider_CB(Fl_Widget *w, void *data)
        { ((SliderInput*)data)->Slider_CB2(); }

    void Input_CB2()
    {
        static int recurse = 0;
        if ( recurse )
            { return; }
            else
        {
            recurse = 1;
            int val = 0;
            if ( sscanf(input->value(), "%d", &val) != 1 )
                { val = 0; }

                if (val > slider->maximum())
            {
                val = (int) slider->maximum();
                ivalue(val);
            }
            if (val < slider->minimum())
            {
                val = (int) slider->minimum();
                ivalue(val);
            }

            // fprintf(stderr, "SCANF('%s') -> %d\n", input->value(), val);
            slider->value(val);                   // pass input's value to slider
            recurse = 0;
            if (m_extraCallback != NULL)
            {
                m_extraCallback(this,m_extraCBData);
            }
        }
    }
    static void Input_CB(Fl_Widget *w, void *data)
        { ((SliderInput*)data)->Input_CB2(); }

    public:
        // CTOR
        SliderInput(int x, int y, int w, int h, const char *l=0) : Fl_Group(x,y,w,h,l)
        {
            int in_w = 70;
            int in_h = 25;

            m_extraCallback = NULL;
            m_extraCBData = NULL;

            input  = new Fl_Int_Input(x, y, in_w, in_h);
            input->callback(Input_CB, (void*)this);
            input->when(FL_WHEN_ENTER_KEY|FL_WHEN_NOT_CHANGED|FL_WHEN_RELEASE);

            slider = new Fl_Slider(x+in_w, y, w - 20 - in_w, in_h);
            slider->type(FL_HOR_NICE_SLIDER);
            slider->callback(Slider_CB, (void*)this);

            bounds(1, 10);                        // some usable default
            value(5);                             // some usable default
            end();                                // close the group
        }

        // MINIMAL ACCESSORS --  Add your own as needed
        int value() { return((int)(slider->value() + 0.5)); }
        void value(int val)
        {
            if (val > slider->maximum()) val =(int) slider->maximum();
            if (val < slider->minimum()) val =(int) slider->minimum();
            slider->value(val);
            Slider_CB2();
        }

        // added by jmk 2007-04-25
        Fl_Int_Input* inputWidget (void)
        {
            return input;
        }

        Fl_Slider* sliderWidget (void)
        {
            return slider;
        }

        void minimum(int val) { slider->minimum(val); }
        int minimum() { return((int)slider->minimum()); }
        void maximum(int val) { slider->maximum(val); }
        int maximum() { return((int)slider->maximum()); }
        void step(int val) {slider->step(val);}
        void step(int val1,int val2) {slider->step(val1,val2);}
        void sltype(int type) {slider->type(type);}
        void bounds(int low, int high) { slider->bounds(low, high); }
        void callback(Fl_Callback *cb, void *data)
        {
            m_extraCallback = cb;
            m_extraCBData = data;
        }
    private:
        Fl_Callback *m_extraCallback;
        void *m_extraCBData;

};

class FloatSliderInput : public Fl_Group
{
    Fl_Float_Input *input;
    Fl_Slider    *slider;

    //Utility
    void ivalue(double val)
    {
        std::ostringstream ss;
        ss.precision(4);
        ss << val;
        std::cout << "New Silder Val: "<<ss.str().c_str() <<std::endl;
        input->value(ss.str().c_str());
    }

    // CALLBACK HANDLERS
    //    These 'attach' the input and slider's values together.
    //
    void Slider_CB2()
    {
        static int recurse = 0;
        if ( recurse )
            { return; }
            else
        {
            recurse = 1;
            char s[80];
            sprintf(s, "%.5g", slider->value());
            // fprintf(stderr, "SPRINTF(%d) -> '%s'\n", (int)slider->value(), s);
            input->value(s);                      // pass slider's value to input
            recurse = 0;
            if (m_extraCallback != NULL)
            {
                m_extraCallback(this,m_extraCBData);
            }
        }
    }

    static void Slider_CB(Fl_Widget *w, void *data)
        { ((FloatSliderInput*)data)->Slider_CB2(); }

    void Input_CB2()
    {
        static int recurse = 0;
        if ( recurse )
            { return; }
            else
        {
            recurse = 1;
            double val = 0;
            if ( sscanf(input->value(), "%lf", &val) != 1 )
                { val = 0; }

                if (val > slider->maximum())
            {
                val = slider->maximum();
                ivalue(val);
            }

            if (val < slider->minimum())
            {
                val = slider->minimum();
                ivalue(val);
            }

            slider->value(val);                   // pass input's value to slider
            recurse = 0;
            if (m_extraCallback != NULL)
            {
                m_extraCallback(this,m_extraCBData);
            }
        }
    }
    static void Input_CB(Fl_Widget *w, void *data)
        { ((FloatSliderInput*)data)->Input_CB2(); }

    public:
        // CTOR
        FloatSliderInput(int x, int y, int w, int h, const char *l=0) : Fl_Group(x,y,w,h,l)
        {
            int in_w = 70;
            int in_h = 25;

            m_extraCallback = NULL;
            m_extraCBData = NULL;

            input  = new Fl_Float_Input(x, y, in_w, in_h);
            input->callback(Input_CB, (void*)this);
            input->when(FL_WHEN_ENTER_KEY|FL_WHEN_NOT_CHANGED|FL_WHEN_RELEASE);

            slider = new Fl_Slider(x+in_w, y, w - 20 - in_w, in_h);
            slider->type(FL_HOR_NICE_SLIDER);
            slider->callback(Slider_CB, (void*)this);

            bounds(1, 10);                        // some usable default
            value(5);                             // some usable default
            end();                                // close the group
        }

        // MINIMAL ACCESSORS --  Add your own as needed
        double value() { return(slider->value()); }
        void value(double val)
        {
            if (val > slider->maximum()) val = slider->maximum();
            if (val < slider->minimum()) val = slider->minimum();
            slider->value(val);
            Slider_CB2();
        }
        void minimum(double val) { slider->minimum(val); }
        double minimum() { return(slider->minimum()); }
        void maximum(double val) { slider->maximum(val); }
        double maximum() { return(slider->maximum()); }
        void step(double val) {slider->step(val);}
        void precision(int digits) {slider->precision(digits);}
        void sltype(int type) {slider->type(type);}
        void bounds(double low, double high) { slider->bounds(low, high); }
        void callback(Fl_Callback *cb, void *data)
        {
            m_extraCallback = cb;
            m_extraCBData = data;
        }

        // added by jmk 2007-04-25
        Fl_Float_Input* inputWidget (void)
        {
            return input;
        }

        Fl_Slider* sliderWidget (void)
        {
            return slider;
        }

    private:
        Fl_Callback *m_extraCallback;
        void *m_extraCBData;

};
#endif
