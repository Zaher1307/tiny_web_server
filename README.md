Tiny Web Server<a name="TOP"></a>
===================
A simple, iterative ```HTTP/1.0``` Web server that uses the ```GET``` method to serve static
and dynamic content.

## Overview
It's a simple web server that combines many of the ideas such as process 
control, Unix I/O, the sockets interface, and HTTP. While it lacks the 
functionality, robustness, and security of a real server, it is powerful enough 
to serve both static and dynamic content to real Web browsers.

## Requirements
- ```linux```
- ```git```
- ```gcc```
- ```make```

## How to test Tiny?
1) **Compile the source code using the following commands**
    ```
    git clone https://github.com/Zaher1307/tiny_web_server.git
    ```
    ```
    make
    ```
    ```
    ./tiny <port>   
    ```

2) **Send an http requests to the server using:**

    ***telnet:***
    
    ```
    telnet localhost <port>
    ```
    ```
    GET /home.html HTTP/1.0
    ```
    or
    ```
    GET /cgi-bin/adder?4&5 HTTP/1.0
    ```
    ***
    ***browser:***
    ```
    http://localhost:<port>/home.html 
    ```
    or 
    ```
    http://localhost:<port>/cgi-bin/adder?4&5
    ```

