To build the otb2zcfg utility you should run the following command:

 mkdir build
 cd build
 ccmake ..
 make

 
To generate the zcfgs for the OTB Applications available, you should run the following:

 mkdir zcfgs
 cd zcfgs
 export ITK_AUTOLOAD_PATH=/your/path/to/otb/applications
 ../build/otb2zcfg
 cp *zcfg /location/to/your/cgi-bin

This ITK_AUTOLOAD_PATH environment variable will be required in the [env] section of your main.cfg.
