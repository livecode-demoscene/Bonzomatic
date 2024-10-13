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

        bool optBorderless = false;
        bool borderless = false;
  
    void replace(Renderer::Settings* settings) {
  
      if (optServerURL) {
        Network::SetUrl(serverURL);
      }

      if (optNetworkMode) {
        Network::SetNetworkMode(networkMode);
      }
      if (optBorderless) {
        settings->borderless = borderless;
      }
    }
    void parse_args(int argc,const char *argv[]) {
        skipDialog = false;
        configFile = "config.json";
        shaderFile = "shader.glsl";
        //Network::config.Mode = Network::NetworkMode::OFFLINE;
        //Network::config.Url = "ws://drone.alkama.com:9000/roomname/username";

        for(size_t i=0;i<argc;i++) {
            if (strlen(argv[i]) <= 0) continue;
            char * token = strtok((char *)argv[i], "=");
            
            if(strcmp(token,"skipdialog")==0){
                skipDialog = true;
                continue;
            }

            if(strcmp(token,"configfile")==0) {
                token = strtok(NULL, "=");
                assert_tuple_arg;
                strcpy((char *)configFile,token);
                continue;
            }

            if(strcmp(token,"shader")==0) {
                token = strtok(NULL, "=");
                assert_tuple_arg;
                strcpy((char*)configFile, token);
                continue;
            }

            if (strcmp(token, "borderless") == 0) {
              borderless = true;
              optBorderless = true;
              continue;
            }

            if(strcmp(token,"serverURL")==0) {
                token = strtok(NULL, "=");
                assert_tuple_arg;
                optServerURL = true;
                strcpy(serverURL, token);
                
                continue;
            }

            if(strcmp(token,"networkMode") == 0) {
                token = strtok(NULL, "=");
                assert_tuple_arg;
                optNetworkMode = true;
                if(strcmp(token,"grabber") == 0){
                    networkMode = Network::NetworkMode::GRABBER;
                    continue;
                }
                if(strcmp(token,"sender") == 0){
                    networkMode = Network::NetworkMode::SENDER;
                    continue;
                }
                if(strcmp(token,"offline") == 0){
                    networkMode = Network::NetworkMode::OFFLINE;
                    continue;
                }
            }        

        }
    }

}

#endif