#include "Network.h"
#include "MIDI.h"
#include <math.h>
#include <Timer.h>
#include <mutex>
#define SHADER_FILENAME(mode) (std::string(mode)+ "_" + RoomName + "_" + NickName + ".glsl")
namespace Network {

    Network::NetworkConfig config;
    Network::ShaderMessage shaderMessage;

    float timeOffset=0.f;
    struct mg_mgr mgr;
    struct mg_connection* c;
    bool done = false;
    std::thread* tNetwork = NULL;
    std::mutex network_interface_access_mutex ;
    bool IsNewShader = false;
    char szShader[65535];
    bool connected = false;
    std::string HostPort, RoomName, NickName;
    float pingTime = 0.f;
    bool isFirstShaderCompile = true;
    float grabber_last_receive = 0;
    float grabber_time = 0;
  char* GetUrl() {
      return config.Url;
  }
  void SetUrl( char* url) {

    config.Url =url;
    Network::SplitUrl(&HostPort, &RoomName, &NickName);
  }
  Network::NetworkMode GetNetworkMode() {
    return config.Mode;
  }
  void SetNetworkMode(NetworkMode mode) {
    config.Mode = mode;
  }
  bool HasNewShader() {
    if (IsNewShader) {
      IsNewShader = false;
      return true;
    }
    return false;

  }
  void RecieveShader(size_t size, char* data) {
    // TODO: very very bad, we should:
    // - use json
    // - verify size
    // - non-ascii symbols ?
    // - asynchronous update ?
    //data[size - 1] = '\0';
    std::string TextJson(data);
    jsonxx::Object NewShader;
    jsonxx::Object Data;
    bool ErrorFound = false;

    if (NewShader.parse(TextJson)) {
      if (NewShader.has<jsonxx::Object>("Data")) {
        Data = NewShader.get<jsonxx::Object>("Data");
        if (!Data.has<jsonxx::String>("Code")) ErrorFound = true;
        if (!Data.has<jsonxx::Number>("Caret")) ErrorFound = true;
        if (!Data.has<jsonxx::Number>("Anchor")) ErrorFound = true;
        if (!Data.has<jsonxx::Number>("FirstVisibleLine")) ErrorFound = true;
        if (!Data.has<jsonxx::Boolean>("Compile")) ErrorFound = true;
      }
      else {
        ErrorFound = true;
      }
    }
    else {
      ErrorFound = true;
    }
    if (ErrorFound) {
      fprintf(stderr, "Invalid json formatting\n");
      return;
    }
    if (Data.has<jsonxx::Number>("ShaderTime")) {

      shaderMessage.shaderTime = Data.get<jsonxx::Number>("ShaderTime");
      shaderMessage.Code = Data.get < jsonxx::String>("Code");
      shaderMessage.AnchorPosition = Data.get<jsonxx::Number>("Anchor");
      shaderMessage.CaretPosition = Data.get<jsonxx::Number>("Caret");
      shaderMessage.NeedRecompile = Data.get<jsonxx::Boolean>("Compile") || isFirstShaderCompile;
      IsNewShader = true;
      isFirstShaderCompile = false;

    }

  }
  static void fn(struct mg_connection* c, int ev, void* ev_data) {

    if (ev == MG_EV_OPEN) {
      c->is_hexdumping = 0;
      connected = true;
    }
    else if (ev == MG_EV_ERROR) {
      // On error, log error message
      MG_ERROR(("%p %s", c->fd, (char*)ev_data));
      connected = false;
    }
    else if (ev == MG_EV_WS_OPEN) {
      fprintf(stdout, "[Network]: Connected\n");
      connected = true;
    }
    else if (config.Mode == SENDER && ev == MG_EV_WS_MSG) {

      struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;

      if (wm->data.len == 0) { // Message 0 len = PING from Grabber
        pingTime = shaderMessage.shaderTime;
      }

    }
    else if (ev == MG_EV_WS_MSG && config.Mode == GRABBER) {

      struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;
      
      if(wm->data.len>0) {
       RecieveShader((int)wm->data.len, wm->data.buf);
      }
    } 
    else if (ev == MG_EV_ERROR || ev == MG_EV_CLOSE ) {
      connected = false;
      printf("Error\n");
    }
  }
  bool IsPinged() {
    return abs(pingTime - shaderMessage.shaderTime) < 1.f;
  }
  void Create() {
    fprintf(stdout, "[Network]: Try to connect to %s\n", config.Url);

    c = mg_ws_connect(&mgr, config.Url, fn, &done, NULL);
    if (c == NULL) {
      fprintf(stderr, "Invalid address\n");
      return;
    }
    while (true) {
      mg_mgr_poll(&mgr, 100);
    }
    mg_mgr_free(&mgr);
  }

  void CheckNetwork() {
    if (!IsConnected() && !IsOffline()) {
        connected = true;
        fprintf(stdout, "[Network]: Starting Thread\n");
        std::thread network(Create);
        tNetwork = &network;
        tNetwork->detach();
  
    }
  }
  void timer_fn(void* arg) {

    if (c == NULL) {
      c = mg_ws_connect(&mgr, config.Url, fn, &done, NULL);
    }
  }
  void Init() {
    mg_mgr_init(&mgr);
    mg_timer_add(&mgr, 3000, MG_TIMER_REPEAT | MG_TIMER_RUN_NOW, timer_fn, &mgr);
    if(config.Mode != OFFLINE){
      CheckNetwork();
    }
    else {
      fprintf(stdout, "[Network]: OFFLINE Mode, not starting Network loop\n");
    }
  }
  bool ReloadShader() {
    if (config.Mode == GRABBER && shaderMessage.NeedRecompile) {
      shaderMessage.NeedRecompile = false;
      return true;
    }
    return false;
  }
  void SetNeedRecompile(bool needToRecompile) {
    shaderMessage.NeedRecompile = needToRecompile;
  }
  void SplitUrl(std::string* host, std::string* roomname, std::string* name) {
    std::string FullUrl((const char*)Network::GetUrl());
    std::size_t PathPartPtr = FullUrl.find('/', 6);
    std::size_t HandlePtr = FullUrl.find('/', PathPartPtr + 1);
    *host = FullUrl.substr(0, PathPartPtr);
    *roomname = FullUrl.substr(PathPartPtr + 1, HandlePtr - PathPartPtr - 1);
    *name = FullUrl.substr(HandlePtr + 1, FullUrl.size() - HandlePtr);
  }
  void UpdateShaderFileName(const char** shaderName) {
    if (IsOffline()) return;
    std::string filename;
    Network::SplitUrl(&HostPort, &RoomName, &NickName);
    if (IsSender()) {
      filename = SHADER_FILENAME("sender");
    }
    else if(IsGrabber()) {
      filename = SHADER_FILENAME("grabber");
    }
    *shaderName = strdup(filename.c_str());
  }
  void UpdateShader(ShaderEditor* mShaderEditor, float shaderTime, std::map<int, std::string> *midiRoutes) {
    if (!IsOffline()) { // If we arn't offline mode
      if (IsGrabber()  && HasNewShader()) { // Grabber mode
        grabber_last_receive = grabber_time;
        int PreviousTopLine = mShaderEditor->WndProc(SCI_GETFIRSTVISIBLELINE, 0, 0);
        int PreviousTopDocLine = mShaderEditor->WndProc(SCI_DOCLINEFROMVISIBLE, PreviousTopLine, 0);
        int PreviousTopLineTotal = PreviousTopDocLine;

        mShaderEditor->SetText(shaderMessage.Code.c_str());
        mShaderEditor->WndProc(SCI_SETCURRENTPOS, shaderMessage.CaretPosition, 0);
        mShaderEditor->WndProc(SCI_SETANCHOR, shaderMessage.AnchorPosition, 0);
        mShaderEditor->WndProc(SCI_SETFIRSTVISIBLELINE, PreviousTopLineTotal, 0);

        //if (bGrabberFollowCaret) {
        //mShaderEditor.WndProc(SCI_SETFIRSTVISIBLELINE, NewMessage.FirstVisibleLine, 0);
        mShaderEditor->WndProc(SCI_SCROLLCARET, 0, 0);
        //}

        /*if (config.grabMidiControls && Data.has<jsonxx::Object>("Parameters")) {
          const std::map<std::string, jsonxx::Value*>& shadParams = Data.get<jsonxx::Object>("Parameters").kv_map();
          for (auto it = shadParams.begin(); it != shadParams.end(); it++)
          {
            float goalValue = it->second->number_value_;
            auto cache = networkParamCache.find(it->first);
            if (cache == networkParamCache.end()) {
              ShaderParamCache newCache;
              newCache.lastValue = goalValue;
              newCache.currentValue = goalValue;
              newCache.goalValue = goalValue;
              newCache.duration = duration;
              networkParamCache[it->first] = newCache;
              cache = networkParamCache.find(it->first);
            }
            ShaderParamCache& cur = cache->second;
            cur.lastValue = cur.currentValue;
            cur.goalValue = goalValue;
            cur.duration = duration;
          }
        }*/
        mg_ws_send(c, 0, 0, WEBSOCKET_OP_BINARY); // Send Ping to Sender to notify received

      }
      else if (IsSender() && shaderTime - shaderMessage.shaderTime > 0.1) {
        //std::cout << shaderTime<<"-"<<ShaderMessage.shaderTime << "="<< shaderTime - ShaderMessage.shaderTime << std::endl;

        mShaderEditor->GetText(szShader, 65535);
        shaderMessage.Code = std::string(szShader);

        shaderMessage.CaretPosition = mShaderEditor->WndProc(SCI_GETCURRENTPOS, 0, 0);
        shaderMessage.AnchorPosition = mShaderEditor->WndProc(SCI_GETANCHOR, 0, 0);
        int TopLine = mShaderEditor->WndProc(SCI_GETFIRSTVISIBLELINE, 0, 0);
        shaderMessage.FirstVisibleLine = mShaderEditor->WndProc(SCI_DOCLINEFROMVISIBLE, TopLine, 0);
        shaderMessage.shaderTime = shaderTime;
        jsonxx::Object Data;
        Data << "Code" << shaderMessage.Code;
        Data << "Compile" << shaderMessage.NeedRecompile;
        Data << "Caret" << shaderMessage.CaretPosition;
        Data << "Anchor" << shaderMessage.AnchorPosition;
        Data << "FirstVisibleLine" << shaderMessage.FirstVisibleLine;
        Data << "RoomName" << RoomName;
        Data << "NickName" << NickName;
        Data << "ShaderTime" << shaderMessage.shaderTime;

        if(config.sendMidiControls) { // Sending Midi Controls
          jsonxx::Object networkShaderParameters;
          for (std::map<int, std::string>::iterator it = midiRoutes->begin(); it != midiRoutes->end(); it++)
          {
            networkShaderParameters << it->second << MIDI::GetCCValue(it->first);
          }
          Data << "Parameters" << networkShaderParameters;
        }
        jsonxx::Object Message = jsonxx::Object("Data", Data);
        std::string TextJson = Message.json();

        if (connected) {
          mg_ws_send(c, TextJson.c_str(), TextJson.length(), WEBSOCKET_OP_TEXT);
          shaderMessage.NeedRecompile = false;
        }

      }
    }
  }
  bool IsGrabber() {
    return config.Mode == GRABBER;
  }
  bool IsSender() {
    return config.Mode == SENDER;
  }
  bool IsOffline() {
    return config.Mode == OFFLINE;
  }
  bool IsConnected() {
    return connected;
  }
  bool IsGrabberReceiving() {
    return abs(grabber_time-grabber_last_receive)<3.;
  }
  void GenerateWindowsTitle(char** originalTitle) {
    if (IsOffline()) {
      return;
    }
    std::string host, roomname, user, title(*originalTitle), newName;
    Network::SplitUrl(&host, &roomname, &user);
    NickName = user;
    if (IsGrabber()) {
      newName = title + " grabber " + user;
    } 
    if (IsSender()) {
      newName = title + " sender " + user;
    }
    *originalTitle = strdup(newName.c_str());
  }

  std::string* GetHandle() {
    return &NickName;
  }
  void SyncTime(float* time) {
    grabber_time = *time;
    if (!IsConnected() || !IsGrabber() || !config.syncTimeWithSender) return;

    if (IsNewShader && abs(*time + timeOffset - shaderMessage.shaderTime) > 1.f) {
      timeOffset = shaderMessage.shaderTime - *time;
      // printf("<diff>%f\n", (timeOffset));
    }

  }
  float TimeOffset() {
    return timeOffset;
  }
  void ResetTimeOffset(float *time) {
    timeOffset = -*time;
  }
  /* From here are methods for parsing json */
  void ParseSyncTimeWithSender(jsonxx::Object* network) {
    if (!network->has<jsonxx::Boolean>("syncTimeWithSender")) {
      fprintf(stderr,"[JSON Network Configuration Parsing] " "Can't find 'syncTimeWithSender', set to true\n");
      config.syncTimeWithSender = true;
      return;
    }
    fprintf(stderr, "[JSON Network Configuration Parsing] " "ParseSyncTimeWithSender");
    config.syncTimeWithSender = network->get<jsonxx::Boolean>("syncTimeWithSender");
    printf("%i\n", config.syncTimeWithSender);
  }
  void ParseNetworkGrabMidiControls(jsonxx::Object * network) {
    if (!network->has<jsonxx::Boolean>("grabMidiControls")) {
      fprintf(stderr,"[JSON Network Configuration Parsing] " "Can't find 'grabMidiControls', set to false\n");
      config.grabMidiControls = false;
      return;
    }
    config.grabMidiControls = network->get<jsonxx::Boolean>("grabMidiControls");
  }
  void ParseNetworkSendMidiControls(jsonxx::Object* network) {
    if (!network->has<jsonxx::Boolean>("sendMidiControls")) {
      fprintf(stderr, "[JSON Network Configuration Parsing] " "Can't find 'sendMidiControls', set to false\n");
      config.sendMidiControls = false;
      return;
    }
    config.sendMidiControls = network->get<jsonxx::Boolean>("sendMidiControls");
  }
  void ParseNetworkUpdateInterval(jsonxx::Object* network) {
    if (!network->has<jsonxx::Boolean>("updateInterval")) {
      fprintf(stderr, "[JSON Network Configuration Parsing] " "Can't find 'updateInterval', set to 0.3\n");
      config.updateInterval = 0.3f;
      return;
    }
    
    config.updateInterval = network->get<jsonxx::Number>("updateInterval");
  }
  void ParseNetworkMode(jsonxx::Object* network) {
    if (!network->has<jsonxx::String>("networkMode")) {
      fprintf(stderr, "[JSON Network Configuration Parsing] " "Can't find 'networkMode' Set to OFFLINE\n");
      config.Mode = OFFLINE;
      return;
    }

    const char* mode = network->get<jsonxx::String>("networkMode").c_str();
    bool isSenderMode = strcmp(mode, "sender") == 0;
    bool isGrabberMode = strcmp(mode, "grabber") == 0;
    if (!isSenderMode && !isGrabberMode) {
      fprintf(stderr, "[JSON Network Configuration Parsing] " "networkMode is neither SENDER or GRABBER, fallback config to OFFLINE\n");
      config.Mode = OFFLINE;
      return;
    }
    if(isSenderMode){
      fprintf(stderr, "[JSON Network Configuration Parsing] " "networkMode is set to SENDER\n");
      config.Mode = SENDER;
    }
    if(isGrabberMode){
      fprintf(stderr, "[JSON Network Configuration Parsing] " "networkMode is set to GRABBER\n");
      config.Mode = GRABBER;
    }

    // From now on, we have a minimal config working we can try to parse extra option
    ParseNetworkGrabMidiControls(network);
    ParseNetworkSendMidiControls(network);
    ParseNetworkUpdateInterval(network);
    ParseSyncTimeWithSender(network);
  }
  void ParseNetworkUrl(jsonxx::Object* network) {
    if (!network->has<jsonxx::String>("serverURL")) {
      fprintf(stderr, "[JSON Network Configuration Parsing] " "Can't find 'serverURL', set to 'OFFLINE'\n");
      config.Mode = OFFLINE;
      config.Url = "";
      return;
    }
   
    config.Url = strdup(network->get<jsonxx::String>("serverURL").c_str());

    ParseNetworkMode(network);

  }
  void ParseNetworkEnabled(jsonxx::Object* network) {
 
    if (!network->has<jsonxx::Boolean>("enabled")) {
      fprintf(stderr,"[JSON Network Configuration Parsing] " "Can't find 'enabled', set to 'OFFLINE'\n");
      config.Mode = OFFLINE;
      config.Url = "";
      return;
    }

    if (!network->get<jsonxx::Boolean>("enabled")) {
      fprintf(stderr, "[JSON Network Configuration Parsing] " "Set to 'OFFLINE'\n");
      config.Mode = OFFLINE;
      config.Url = "";
      // As we can activate this on setup dialog, let's try to get serverURL
      if (network->has<jsonxx::String>("serverURL")) {
        config.Url = strdup(network->get<jsonxx::String>("serverURL").c_str());
      }
      return;
    }
    ParseNetworkUrl(network);


  }
  
  /*
    Parse the json settings. Cascading calls, not perfect but keep clear code
    - Check that Network block exists on json
    - Check that 'enabled' exists and is true
    - Check that 'serverUrl' exists
    - Check that 'networkMode' exists

    If something doesn't match above path, will fallback to OFFLINE Mode
    */
  void ParseSettings(jsonxx::Object* options) {
    
    fprintf(stderr, "[JSON Network Configuration Parsing] " "Parsing network configuration data from json\n");
    if (!options->has<jsonxx::Object>("network")) {
      fprintf(stderr, "[JSON Network Configuration Parsing] " "Can't find 'network' block, set to 'OFFLINE'\n");
      config.Mode = OFFLINE;
      config.Url = "";
      return;
    }
    jsonxx::Object network = options->get<jsonxx::Object>("network");
    ParseNetworkEnabled(&network);


  }
}