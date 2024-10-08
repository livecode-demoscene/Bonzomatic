#ifndef BONZOMATIC_NETWORK_H
#define BONZOMATIC_NETWORK_H
#pragma once
#include <string>

#include "mongoose.h"
#include <thread>
#include <jsonxx.h>
#include "ShaderEditor.h"
namespace Network {

  enum NetworkMode {
    OFFLINE,
    SENDER,
    GRABBER
  };


  struct NetworkConfig {
    char* Url;
    NetworkMode Mode;
    float updateInterval = 0.3f;
    bool sendMidiControls;
    bool grabMidiControls;
    bool syncTimeWithSender;
  };
  struct ShaderMessage {
    std::string Code;
    int CaretPosition;
    int AnchorPosition;
    int FirstVisibleLine;
    bool NeedRecompile;
    float shaderTime = 0.0;
  };


 
    
    bool HasNewShader();
    bool ReloadShader();
    void RecieveShader(size_t size, char* data);
    static void fn(struct mg_connection* c, int ev, void* ev_data);
    
    void Create();
    void Init();

    void ParseSettings(jsonxx::Object* options);
    void UpdateShader(ShaderEditor* mShaderEditor, float shaderTime, std::map<int, std::string> *midiRoutes);
    char* GetUrl();
    void SetUrl(char*);
    Network::NetworkMode GetNetworkMode();
    void SetNetworkMode(Network::NetworkMode mode);
    void SetNeedRecompile(bool needToRecompile);
    void UpdateShaderFileName( const char** shaderName);
    void SplitUrl(std::string *host,std::string *roomname,std::string* name);
    bool IsGrabber();
    bool IsSender();
    bool IsOffline();
    bool IsConnected();
    std::string* GetHandle();
    void GenerateWindowsTitle(char** originalTitle);
    void SyncTimeWithSender(float* time);
    float TimeOffset();
    void ResetTimeOffset(float* time);
    bool IsPinged();
    void ChecktNetwork();
}
#endif // BONZOMATIC_NETWORK_H