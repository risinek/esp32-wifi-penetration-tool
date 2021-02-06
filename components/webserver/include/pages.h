#ifndef PAGES_H
#define PAGES_H

const char page_root[] = 
"<!DOCTYPE html>"
"<html>"
    "<title>ESP32 Management AP</title>"
"<body onLoad=\"refreshAps()\">"
    "<h3>Available APs:</h3>"
    "<div id=\"aps-list\">AP list should be updated automatically...</div>"
    "<button type=\"button\" onClick=\"refreshAps()\">Refresh</button>"
    "<script>"
    "function refreshAps() {"
        "document.getElementById(\"aps-list\").innerHTML = \"Loading (this make take a while)...\";"
        "var xhttp = new XMLHttpRequest();"
        "xhttp.onreadystatechange = function() {"
            "if(this.readyState == 4 && this.status == 200) {"
                "document.getElementById(\"aps-list\").innerHTML = this.responseText;"
            "}"
        "};"
        "xhttp.open(\"GET\", \"aps\", true);"
        "xhttp.send();"
    "}"
    "</script>"
"</body>"
"</html>"
;

#endif