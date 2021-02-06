#ifndef PAGES_INDEX_H
#define PAGES_INDEX_H

const char page_root[] = 
"<!DOCTYPE html>"
"<html>"
    "<title>ESP32 Management AP</title>"
"<body onLoad=\"refreshAps()\">"
    "Available APs:"
    "<div id=\"aps-list\">Loading...</div>"
    "<button type=\"button\" onClick=\"refreshAps()\">Refresh</button>"
    "<script>"
    "function refreshAps() {"
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