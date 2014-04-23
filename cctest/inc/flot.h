/*---------------------------------------------------------------------------------------------------------*\
  File:     flot.h                                                                      Copyright CERN 2014

  License:  This file is part of cctest.

            cctest is free software: you can redistribute it and/or modify
            it under the terms of the GNU Lesser General Public License as published by
            the Free Software Foundation, either version 3 of the License, or
            (at your option) any later version.

            This program is distributed in the hope that it will be useful,
            but WITHOUT ANY WARRANTY; without even the implied warranty of
            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
            GNU Lesser General Public License for more details.

            You should have received a copy of the GNU Lesser General Public License
            along with this program.  If not, see <http://www.gnu.org/licenses/>.

  Purpose:  Header file for Converter Control library test program to generate html file
            using flot graphing tool

  Authors:  Quentin.King@cern.ch

  Notes:    This file is automatically assembled by the makefile from two header file fragments
            and five html file fragments processed using inc/flot/htmlquote.sh to add and escape
            quotes:

                flot_header.h, flot_footer.h
                flot_part0.htm, flot_part1.htm, flot_part2.htm, flot_part3.htm and flot_part4.htm

            DO NOT EDIT flot.h - YOUR CHANGES WILL BE LOST WHEN YOU NEXT RUN MAKE!
\*---------------------------------------------------------------------------------------------------------*/

#ifndef FLOT_H
#define FLOT_H

#define MAX_FLOT_POINTS         100000           // Limit points per signal for FLOT output

static char *flot[] =
{
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n"
"<html>\n"
"  <head>\n"
"    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n"
"    <title>CCTEST</title>\n"
"    <style type=\"text/css\">\n"
"        html, body\n"
"        {\n"
"            height: 95%%;\n"
"            font-family: sans-serif;\n"
"            font-size: 15px;\n"
"            margin: 0px 0px 0px 0px;\n"
"        }\n"
"        a\n"
"        {\n"
"            text-decoration:    none;\n"
"        }\n"
"        a:link\n"
"        {\n"
"            color:              #0000CC;\n"
"            text-decoration:    none;\n"
"        }\n"
"        a:visited\n"
"        {\n"
"            color:              #0000CC;\n"
"            text-decoration:    none;\n"
"        }\n"
"        a:hover\n"
"        {\n"
"            color:              #CC0000;\n"
"            text-decoration:    none;\n"
"        }\n"
"    </style>\n"
"\n"
"    <link rel=\"stylesheet\" href=\"../../colorbox/example4/colorbox.css\" />\n"
"\n"
"    <script language=\"javascript\" type=\"text/javascript\" src=\"%s/colorbox/jquery.min.js\"></script>\n"
"    <script language=\"javascript\" type=\"text/javascript\" src=\"%s/colorbox/jquery.colorbox-min.js\"></script>\n"
"    <script language=\"javascript\" type=\"text/javascript\" src=\"%s/flot/jquery.flot.js\"></script>\n"
"    <script language=\"javascript\" type=\"text/javascript\" src=\"%s/flot/jquery.flot.selection.js\"></script>\n"
"    <script language=\"javascript\" type=\"text/javascript\" src=\"%s/modernizr/modernizr.touch.js\"></script>\n"
"    <script language=\"javascript\" type=\"text/javascript\">    \n"
"        \n"
"    // ----- Only load flot.resize plug-in on non-touch devices because it uses CPU and generates garbage -----\n"
"    \n"
"    if(!Modernizr.touch)\n"
"    {\n"
"       var selection_script = document.createElement('script');\n"
"       selection_script.setAttribute('src', \"%s/flot/jquery.flot.resize.js\");\n"
"       document.getElementsByTagName('head')[0].appendChild(selection_script);\n"
"    }\n"
"    \n"
"    // ----- Global variables -----\n"
"\n"
"    var i;\n"
"    var analog_chart_div;\n"
"    var analog_legend_div;\n"
"    var digital_chart_div;\n"
"    var analog_plot;\n"
"    var analog_xaxis;\n"
"    var escale_flag = 0;\n"
"    var derivative_flag = 0;\n"
"    var visible_analog_signals = [];\n"
"    var visible_digital_signals = [];\n"
"    var options_stack = [];\n"
"    var stack_depth = 0;\n"
"    var analog_options;\n"
"    var analog_options_zoom;\n"
"    var digital_options;\n"
"    var digital_options_zoom;\n"
"\n"
"    // ------ Analog signals -----\n"
"\n"
"    var analog_signals = {\n"
,
"};\n"
"\n"
"    // ----- Digital signals -----\n"
"\n"
"    var digital_signals = {\n"
,
"};\n"
"\n"
"//------------------------------------------------------------------------------------------------\n"
"    function OnLoad()\n"
"    {\n"
"\n"
"        // ----- Get container variables -----\n"
"\n"
"        analog_chart_div   = $(\"#analog_chart_div\");\n"
"        analog_legend_div  = $(\"#analog_legend_div\");\n"
"        digital_chart_div  = $(\"#digital_chart_div\");\n"
"\n"
"        // ----- Initialise colour, index and scaling for each signal -----\n"
"\n"
"        var i = 0;\n"
"\n"
"        $.each(analog_signals, function(sig_name, signal)\n"
"        {\n"
"            signal.color     = i + 2 + (i > 8);         // Skip color 11 as it is too pale\n"
"            signal.index     = i++;\n"
"            signal.visible   = 1;\n"
"            signal.exp       = 0;\n"
"            signal.exp_deriv = 0;\n"
"        });\n"
"\n"
"        i = 0;\n"
"        var dig_legend = [];\n"
"\n"
"        $.each(digital_signals, function(sig_name, signal)\n"
"        {\n"
"            signal.color = i + 2 + (i > 8);             // Skip color 11 as it is too pale\n"
"            dig_legend.push([-(++i),sig_name]);\n"
"            signal.label = sig_name;\n"
"            visible_digital_signals.push(digital_signals[sig_name]);\n"
"        });\n"
"\n"
"        // ----- Set up plot options for analog and digital signals -----\n"
"\n"
"        analog_options =\n"
"        {\n"
"            series:         { shadowSize: 1, lines: { show: true, lineWidth: 1}, points: { radius: 2} },\n"
"            grid:           { color: \"#444\", borderWidth: 1, backgroundColor: \"#FFF\" },\n"
"            yaxis:          { labelWidth:220 },\n"
"            legend:         { container: analog_legend_div, position: \"nw\", labelFormatter: function (label, series) { return '<a href=\"javascript:ToggleVisibility(' + series.index + ');\">' + label + '</a>'; }},\n"
"            selection:      { mode: \"xy\", touch: Modernizr.touch, tapCallback: ZoomOut }\n"
"        };\n"
"\n"
"        analog_options_zoom = analog_options;\n"
"\n"
"        digital_options =\n"
"        {\n"
"            series:    { shadowSize: 1, lines: { show: true, lineWidth: 1} },\n"
"            grid:      { color: \"#444\", borderWidth: 1, backgroundColor: \"#FFF\", axisMargin: 50 },\n"
"            yaxis:     { labelWidth:220, max: -0.25, min: -i-0.75, ticks: dig_legend },\n"
"            legend:    { show: false }\n"
"        };\n"
"\n"
"        digital_options_zoom = digital_options;\n"
"\n"
"        // ----- Display analog and digital plots (if digital signals are included) -----\n"
"\n"
"        PlotSignals();\n"
"\n"
"        // ----- Save x-axis setting for analog plot for when all signals are invisible -----\n"
"\n"
"        analog_xaxis = analog_plot.getAxes().xaxis;\n"
"\n"
"        // ----- Link colorbox pop-ups to inline links -----\n"
"\n"
"        $(document).ready(function(){$(\".inline\").colorbox({inline:true, width:\"80%\", height:\"90%\"});});\n"
"\n"
"        // ----- Calculate derivative signal and the exponent scaling factor for each signal & its derivative -----\n"
"\n"
"        $.each(analog_signals, function(sig_name, signal)\n"
"        {\n"
"            var derivative;\n"
"            var abs_value;\n"
"            var last_time           = signal.data[0][0]\n"
"            var last_value          = signal.data[0][1];\n"
"            var max_abs_value       = 0.0;\n"
"            var max_abs_deriv_value = 0.0;\n"
"            var exp_scaling         = 1.0;\n"
"            var exp_scaling_deriv   = 1.0;\n"
"            var exponent;\n"
"\n"
"            // ----- Initialise three new signals -----\n"
"\n"
"            signal.derivative   = [];\n"
"            signal.deriv_escale = [];\n"
"            signal.data_escale  = [];\n"
"\n"
"            signal.derivative.push( [ last_time, 0.0 ] );       // Add a first point\n"
"\n"
"            // ----- Scan signal to calculate derivative and the max abs value of the signal and its derivative -----\n"
"\n"
"            for(i=0 ; i < signal.data.length ; i++)\n"
"            {\n"
"                if((abs_value = Math.abs(signal.data[i][1])) > max_abs_value) max_abs_value = abs_value;\n"
"\n"
"                if(signal.data[i][1] != last_value)\n"
"                {\n"
"                    derivative = (signal.data[i][1] - last_value) / (signal.data[i][0] - last_time);\n"
"                    last_time  = signal.data[i][0];\n"
"                    last_value = signal.data[i][1];\n"
"                    signal.derivative.push( [ last_time, derivative ] );\n"
"\n"
"                    if((abs_value = Math.abs(derivative)) > max_abs_deriv_value) max_abs_deriv_value = abs_value;\n"
"                }\n"
"            }\n"
"\n"
"            if(signal.data[i-1][0] != last_time)\n"
"            {\n"
"                signal.derivative.push( [ signal.data[i-1][0], 0.0 ] );       // Add a last point\n"
"            }\n"
"\n"
"            // ----- Calculate exponential scaling factor for the signal -----\n"
"\n"
"            exponent = 0;\n"
"\n"
"            if(max_abs_value > 0.0)\n"
"            {\n"
"                exponent  = Math.floor(Math.log(max_abs_value) / Math.LN10 + 999.999) - 1000;\n"
"                exp_scaling = Math.pow(10.0, -exponent);\n"
"            }\n"
"\n"
"            if(exponent >= 0)\n"
"            {\n"
"                signal.exp = '.E+' + exponent;\n"
"            }\n"
"            else\n"
"            {\n"
"                signal.exp = '.E' + exponent;\n"
"            }\n"
"\n"
"            // ----- Calculate exponential scaling factor for the signal derivative -----\n"
"\n"
"            exponent = 0;\n"
"\n"
"            if(max_abs_deriv_value > 0.0)\n"
"            {\n"
"                exponent = Math.floor(Math.log(max_abs_deriv_value) / Math.LN10 + 999.999) - 1000;\n"
"                exp_scaling_deriv = Math.pow(10.0, -exponent);\n"
"            }\n"
"\n"
"            if(exponent >= 0)\n"
"            {\n"
"                signal.exp_deriv = '.E+' + exponent;\n"
"            }\n"
"            else\n"
"            {\n"
"                signal.exp_deriv = '.E' + exponent;\n"
"            }\n"
"\n"
"            // ----- Calculate e-scaled signal -----\n"
"\n"
"            for(i=0 ; i < signal.data.length ; i++)\n"
"            {\n"
"                signal.data_escale.push( [ signal.data[i][0], signal.data[i][1] * exp_scaling ] );\n"
"            }\n"
"\n"
"            // ----- Calculate e-scaled derivative ------\n"
"\n"
"            for(i=0 ; i < signal.derivative.length ; i++)\n"
"            {\n"
"                signal.deriv_escale.push( [ signal.derivative[i][0], signal.derivative[i][1] * exp_scaling_deriv ] );\n"
"            }\n"
"        });\n"
"\n"
"        // ----- Bind functions to zoom in/out to pointer button events -----\n"
"\n"
"        analog_chart_div.bind('plotselected', ZoomIn);\n"
"        analog_chart_div.bind('contextmenu',  ZoomOut);\n"
"    }\n"
"\n"
"//------------------------------------------------------------------------------------------------\n"
"    function PlotSignals()\n"
"    {\n"
"        visible_analog_signals = [];\n"
"\n"
"        // ----- If zoomed out then rescale plot automatically -----\n"
"\n"
"        if(stack_depth == 0)\n"
"        {\n"
"            analog_options_zoom = analog_options;\n"
"        }\n"
"\n"
"        // ----- Create list of visible signals -----\n"
"\n"
"        $.each(analog_signals, function(sig_name, signal)\n"
"        {\n"
"            if(signal.visible == 1)\n"
"            {\n"
"                if(derivative_flag == 1)\n"
"                {\n"
"                    if(escale_flag == 1)\n"
"                    {\n"
"\n"
"                        visible_analog_signals.push({data: signal.deriv_escale, label: sig_name + \" '\" + signal.exp_deriv, color: signal.color, lines: signal.lines, points: signal.points, index: signal.index});\n"
"                    }\n"
"                    else\n"
"                    {\n"
"                        visible_analog_signals.push({data: signal.derivative, label: sig_name + \" '\", color: signal.color, lines: signal.lines, points: signal.points, index: signal.index});\n"
"                    }\n"
"                }\n"
"                else\n"
"                {\n"
"                    if(escale_flag == 1)\n"
"                    {\n"
"                        visible_analog_signals.push({data: signal.data_escale, label: sig_name + signal.exp, color: signal.color, lines: signal.lines, points: signal.points, index: signal.index});\n"
"                    }\n"
"                    else\n"
"                    {\n"
"                        visible_analog_signals.push({data: signal.data, label: sig_name, color: signal.color, lines: signal.lines, points: signal.points, index: signal.index});\n"
"                    }\n"
"                }\n"
"            }\n"
"            else\n"
"            {\n"
"                visible_analog_signals.push({ data: [[analog_xaxis.min,0], [analog_xaxis.max,0]], label: sig_name, color: \"#CCC\", index: signal.index});\n"
"            }\n"
"        });\n"
"\n"
"        // ----- Plot analog signals -----\n"
"\n"
"        analog_plot = $.plot(analog_chart_div, visible_analog_signals, analog_options_zoom);\n"
"\n"
"        // ----- Plot digital signals if they are defined -----\n"
"\n"
"        if (visible_digital_signals.length > 0)\n"
"        {\n"
"            $.plot(digital_chart_div, visible_digital_signals, digital_options_zoom);\n"
"        }\n"
"    }\n"
"\n"
"//------------------------------------------------------------------------------------------------\n"
"    function FullScale()\n"
"    {\n"
"        // ----- Zoom out to full scale -----\n"
"\n"
"        stack_depth   = 0;\n"
"        options_stack = [];\n"
"        analog_options_zoom  = analog_options;\n"
"        digital_options_zoom = digital_options;\n"
"\n"
"        // ----- Replot charts -----\n"
"\n"
"        PlotSignals();\n"
"    }\n"
"\n"
"//------------------------------------------------------------------------------------------------\n"
"    function ToggleMode(mode)\n"
"    {\n"
"        if(mode == 0)\n"
"        {\n"
"            // ----- mode 0 : Toggle E-Scale flag -----\n"
"\n"
"            escale_flag = 1 - escale_flag;\n"
"        }\n"
"        else\n"
"        {\n"
"            // ----- mode 1 : Toggle Derivative flag -----\n"
"\n"
"            derivative_flag = 1 - derivative_flag;\n"
"        }\n"
"\n"
"        // ----- Zoom out to full scale -----\n"
"\n"
"        FullScale();\n"
"    }\n"
"\n"
"//------------------------------------------------------------------------------------------------\n"
"    function ToggleVisibility(series_index)\n"
"    {\n"
"        $.each(analog_signals, function(sig_name, signal)\n"
"        {\n"
"            if(signal.index == series_index) signal.visible = 1 - signal.visible;\n"
"        });\n"
"\n"
"        PlotSignals();\n"
"    }\n"
"\n"
"//------------------------------------------------------------------------------------------------\n"
"    function AllNoneVisible(visibility)\n"
"    {\n"
"        $.each(analog_signals, function(sig_name, signal)\n"
"        {\n"
"            signal.visible = visibility;\n"
"        });\n"
"\n"
"        PlotSignals();\n"
"    }\n"
"\n"
"//------------------------------------------------------------------------------------------------\n"
"    function ZoomIn(event, ranges)\n"
"    {\n"
"        if (visible_analog_signals.length > 0)\n"
"        {\n"
"            // ----- Clip selection range to 10 micro units in both X and Y -----\n"
"\n"
"            if (ranges.xaxis.to - ranges.xaxis.from < 0.00001)\n"
"            {\n"
"                ranges.xaxis.to = ranges.xaxis.from + 0.00001;\n"
"            }\n"
"\n"
"            if (ranges.yaxis.to - ranges.yaxis.from < 0.00001)\n"
"            {\n"
"                ranges.yaxis.to = ranges.yaxis.from + 0.00001;\n"
"            }\n"
"\n"
"            // ------ Push current zoom options onto the options stack -----\n"
"\n"
"            stack_depth++;\n"
"            options_stack.push(analog_options_zoom);\n"
"            options_stack.push(digital_options_zoom);\n"
"\n"
"            // ------ Zoom to new selection range -----\n"
"\n"
"            analog_options_zoom = $.extend(true, {}, analog_options, {\n"
"                                           xaxis: { min: ranges.xaxis.from, max: ranges.xaxis.to },\n"
"                                           yaxis: { min: ranges.yaxis.from, max: ranges.yaxis.to } });\n"
"\n"
"            analog_plot = $.plot(analog_chart_div, visible_analog_signals, analog_options_zoom);\n"
"\n"
"            if (visible_digital_signals.length > 0)\n"
"            {\n"
"                digital_options_zoom = $.extend(true, {}, digital_options, {\n"
"                                                xaxis: { min: ranges.xaxis.from, max: ranges.xaxis.to } });\n"
"\n"
"                $.plot(digital_chart_div, visible_digital_signals, digital_options_zoom);\n"
"            }\n"
"        }\n"
"    }\n"
"\n"
"//------------------------------------------------------------------------------------------------\n"
"    function ZoomOut()\n"
"    {\n"
"        if (visible_analog_signals.length > 0 && stack_depth > 0)\n"
"        {\n"
"            // ----- Pop options from options stack to restore previous zoom level -----\n"
"\n"
"            stack_depth--;\n"
"            digital_options_zoom = options_stack.pop();\n"
"            analog_options_zoom  = options_stack.pop();\n"
"\n"
"            analog_plot = $.plot(analog_chart_div, visible_analog_signals, analog_options_zoom);\n"
"\n"
"            if (visible_digital_signals.length > 0)\n"
"            {\n"
"                $.plot($(\"#digital_chart_div\"), visible_digital_signals, digital_options_zoom);\n"
"            }\n"
"        }\n"
"\n"
"        // ----- return false to suppress the right-mouse context menu from appearing -----\n"
"\n"
"        return false\n"
"    }\n"
"\n"
"    </script>\n"
"</head>\n"
"\n"
"<!-- ========================================================================================== -->\n"
"\n"
"<body bgcolor='#DDDDDD'>\n"
"\n"
"  <!--  Control links above analog plot -->\n"
"\n"
"  <p id='links_line' style='font-size:14px'>\n"
"    <a href='javascript:AllNoneVisible(1);'>Show all signals</a> |\n"
"    <a href='javascript:AllNoneVisible(0);'>Hide all signals</a> |\n"
"    <a href='javascript:ToggleMode(0);'>Toggle E-Scaling</a> |\n"
"    <a href='javascript:ToggleMode(1);'>Toggle Derivative</a> |\n"
"    <a href='javascript:FullScale();'>Full Scale</a> |\n"
"    <a class='inline' href='#inline_pars'>Show pars</a> |\n"
"    <a class='inline' href='#inline_debug'>Show debug</a> |\n"
"    <a class='inline' href='#inline_about'>About</a> |\n"
"    <a class='inline' href='#inline_help'>Help</a>\n"
"  </p>\n"
"\n"
"  <!-- Divs for analog and digital plots -->\n"
"\n"
"  <div id='analog_chart_div'  style='width:95%;height:65%;'></div>\n"
"  <div id='analog_legend_div' style='position:absolute;left:5px;top:50px;width:200px;height:65%;'></div>\n"
"  <div id='digital_chart_div' style='width:95%;height:35%;'></div>\n"
"\n"
"  <!-- Pop-up texts -->\n"
"\n"
"  <div style='display:none'>\n"
"\n"
"    <!-- About pop-up -->\n"
"\n"
"    <div id='inline_about' style='padding:20px;background:#fff;font-size:17px;text-align:justify;'>\n"
"        <p style='font-size:22px;font-weight:bold;'>About this chart:</p>\n"
"        <p style=>cctest is program that simulates the control of current or magnetic field in a magnet circuit\n"
"        driven by a power converter.  It is written in standard C and uses the function generation library (libfg) and regulation\n"
"        library (libreg).  These libraries are also written in C and are part of the\n"
"        Converter Control libraries (cclibs) provided under the Lesser General Public License\n"
"        by the CERN power converter group. The cctest program is driven by a set of text files that\n"
"        contain all the simulation, regulation and function generation parameters.</p>\n"
"        <p>For more information or to download the cclibs software, go to the\n"
"        <a href='http://cern.ch/project-cclibs/' target='_blank'>website for cclibs</a>.\n"
"        For information on all the parameters, consult the\n"
"        <a href='https://github.com/qking/cclibs/raw/master/cctest/doc/cctest_user_guide.pdf' target='_blank'>user guide</a>.\n"
"        </p>\n"
"        <p>This webpage uses the\n"
"        <a href='http://www.flotcharts.org/' target='_blank'>FLOT library</a> for the graphs and\n"
"        <a href='http://colorpowered.com/colorbox/' target='_blank'>Colorbox</a> for the pop-ups.\n"
"        </p>\n"
"    </div>\n"
"\n"
"    <!-- Help pop-up -->\n"
"\n"
"    <div id='inline_help' style='padding:20px; background:#fff;font-size:17px;text-align:justify;'>\n"
"        <p style='font-size:22px;font-weight:bold;'>Controlling this chart:</p>\n"
"        <p>\n"
"        <ul>\n"
"            <li><strong>Signal Visibility</strong><br/><br/>To toggle the visibility of an analog signal, click\n"
"                on the signal's name in the legend.  To control the visiblity of all the analog signals, click\n"
"                on the \"Show All Signals\" or \"Hide All Signals\" links.\n"
"                The visibility of digital signals cannot be controlled.</li><br/>\n"
"            <li><strong>Zooming in</strong><br/><br/>Select the zoom area using the left pointer button or your \n"
"                finger on a touch device.</li><br/>\n"
"            <li><strong>Zooming out</strong><br/><br/>Press the right pointer button or tap anywhere on the chart\n"
"                on a touch device to zoom out to the previous zoom level. Select \"Full Scale\" to zoom\n"
"                out to see all the signals.</li><br/>\n"
"            <li><strong>E-Scaling</strong><br/><br/>Click on the \"Toggle E-Scaling\" link to toggle between normal\n"
"                scaling mode and exponential scaling mode.  In this mode, every signal is scaled by a\n"
"                power of 10 to be within the range &plusmn;10. The scale factors are shown in the legend\n"
"                with \"E&plusmn;N\" notation.</li><br/>\n"
"            <li><strong>Derivative</strong><br/><br/>Click on the \"Toggle Derivative\" link to toggle between normal\n"
"                mode and derivative mode.  In Derivative mode, the first derivatives of all the signals are displayed.</li><br/>\n"
"            <li><strong>Digital signals</strong><br/><br/>If the data includes digital signals they will be\n"
"                displayed in the lower chart.  Zooming the analog signals will zoom the time-axis\n"
"                for the digital signals automatically.</li>\n"
"        </ul>\n"
"        </p>\n"
"    </div>\n"
"    \n"
"    <!-- Simulation parameters pop-up -->\n"
"\n"
"    <div id='inline_pars' style='padding:10px; background:#fff;font-size:14px;'>\n"
"\n"
"        <p style='font-size:22px;font-weight:bold;'>cctest Simulation Parameters:</p>\n"
"        <p><pre>\n",
"        </pre></p>\n"
"    </div>\n"
"\n"
"    <!-- Debug parameters pop-up -->\n"
"\n"
"    <div id='inline_debug' style='padding:10px; background:#fff;font-size:14px;'>\n"
"\n"
"        <p style='font-size:22px;font-weight:bold;'>cctest Debug Information:</p>\n"
"         <p><pre>\n"
,
"        </pre></p>\n"
"    </div>\n"
"</div>\n"
"  \n"
"<script>\n"
"//  Delay to allow browsers to catch up - this is a crude hack while I work out what to wait for\n"
"    setTimeout(OnLoad,20);\n"
"</script>\n"
"\n"
"</body>\n"
"</html>\n"
,
};

#endif
// EOF