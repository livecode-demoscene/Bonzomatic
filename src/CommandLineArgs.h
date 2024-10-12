#ifndef CMD_ARGS_H_
#define CMD_ARGS_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define assert_tuple_arg (assert(i < argc && "Expecting value"))

namespace CommandLineArgs
{

   
        bool skipDialog;
        const char* configFile;
        const char* shaderFile;

        bool optServerURL = false;
        char serverURL[512];

        bool optNetworkMode = false;
        Network::NetworkMode networkMode;
  
    void replace() {
  
      if (optServerURL) {
        Network::SetUrl(serverURL);
      }

      if (optNetworkMode) {
        Network::SetNetworkMode(networkMode);
      }
    }
    void parse_args(int argc,const char *argv[]) {
        skipDialog = false;
        configFile = "config.json";
        shaderFile = "shader.glsl";
        //Network::config.Mode = Network::NetworkMode::OFFLINE;
        //Network::config.Url = "ws://drone.alkama.com:9000/roomname/username";

        for(size_t i=0;i<argc;++i) {

            if(strcmp(argv[i],"skipdialog")==0){
                skipDialog = true;
                continue;
            }

            if(strcmp(argv[i],"configfile")==0) {
                i++;
                assert_tuple_arg;
                configFile = argv[i];
                continue;
            }

            if(strcmp(argv[i],"shader")==0) {
                i++;
                assert_tuple_arg;
                shaderFile = argv[i];
                continue;
            }

            if(strcmp(argv[i],"serverURL")==0) {
                i++;
                assert_tuple_arg;
                optServerURL = true;
                strcpy(serverURL, argv[i]);
                //Network::NetworkConfig.Url = argv[i];
                continue;
            }

            if(strcmp(argv[i],"networkMode") == 0) {
                i++;
                assert_tuple_arg;
                optNetworkMode = true;
                if(strcmp(argv[i],"grabber") == 0){
                    networkMode = Network::NetworkMode::GRABBER;
                    continue;
                }
                if(strcmp(argv[i],"sender") == 0){
                    networkMode = Network::NetworkMode::SENDER;
                    continue;
                }
                if(strcmp(argv[i],"offline") == 0){
                    networkMode = Network::NetworkMode::OFFLINE;
                    continue;
                }
            }        

        }
    }

}

#endif