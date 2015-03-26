# About Module
This Module is written to provide the ability of making full HTTP/S request logging in Nginx, including Status Line and Request Body. <br/>
The main reason of creation of this module, is that currently the Nginx does not provide full request logging, which will give more power in application monitoring process. <br/>

# Installation
For installing this module you will need just to make a <code>git clone https://github.com/flaxtonio/nginx-flaxton-logger-module</code>. <br/>
After you will have a code for this module, install it as a usual Nginx module, by configuring it with Nginx source
```bash
 ./configure --add-module=modules/ngx-flaxton-logger
 make
 sudo make install
```
And basically you are done!

# Configuration
Module have a few configurable steps, just to provide more flexible way of doing Nginx Request logging.
<pre>
#Making on or off the logger
flaxton_logger on;
#Setting path to log file
flaxton_log_file /home/flaxton.log;
#Setting logging level [full, headers, body], default is full (including headers and body)
flaxton_log_level full;

location / {
    proxy_pass http://127.0.0.1:8080;
    
    #Here is the tick of making full request logging! it's a way of hack :)
    set $flaxton $flaxton_logger;
}
</pre>
