#include "bitsflow-hl++/BitsflowHLApi.h"

#include <fstream>
#include <string>
#include <iostream>


#include "spdlog/spdlog.h"
#include "algo.h"
#include "utils.h"
#include "rapidjson/document.h"

using namespace yoyo::bitsflow::hl;
#if defined __LINUX_NEOKYLIN_5__
  #ifndef HL_FORMAT_SIZE_T
    #define HL_FORMAT_SIZE_T "%zu"
  #endif
  #ifndef HL_FORMAT_INT64
    #define HL_FORMAT_INT64       "%ld"
  #endif
#endif


int32_t ret = HL_OK;
int64_t seq = 0;
const char *session = NULL;



// HLSenderWrapper包装了HLSender、HLMessaging和客户自定义参数
typedef struct HLSenderWrapper
{
  HLMessaging *messaging;
  HLSender *sender;

  HLSenderWrapper(): messaging(NULL), sender(NULL)
                      {};

} HLSenderWrapper;


// HLListenerWrapper包装了HLListener、HLMessaging和客户自定义参数
typedef struct HLListenerWrapper
{
  HLMessaging *messaging;
  HLListener *listener;

  HLListenerWrapper(): messaging(NULL), listener(NULL)
                      {};

} HLListenerWrapper;

HLSenderWrapper *sw =new HLSenderWrapper();

// HLMessaging的回调函数
/////////////////////////////////
//  HLMessagingNotifyCallback  //
/////////////////////////////////

void 
messagingOnDiscoverySuccess(void *arg)
{
  fprintf(stderr, "HLMessagingNotifyCallback::OnDiscoverySuccess is called with arg(%p)\n", arg);
}

void 
messagingOnDiscoverFailed(DiscoveryFailReason failReason, void *arg) 
{
  std::string failReasonStr = "unknow";                        
  if (DISCOVERY_FAIL_REASON_SUBSCRIBE_FAIL == failReason) {    
    failReasonStr = "DISCOVERY_FAIL_REASON_SUBSCRIBE_FAIL";    
  } else if (DISCOVERY_FAIL_REASON_INTERNAL == failReason) {   
    failReasonStr = "DISCOVERY_FAIL_REASON_INTERNAL";          
  } else if (DISCOVERY_FAIL_REASON_UNKNOWN == failReason) {    
    failReasonStr = "DISCOVERY_FAIL_REASON_UNKNOWN";           
  }                                                            
  fprintf(stderr, "HLMessagingNotifyCallback::OnDiscoverFailed is called with failreason(%s), arg(%p)\n", failReasonStr.c_str(), arg);
}

void 
messagingOnClientOnline(const ClientId &clientId, void *arg) 
{
  fprintf(stderr, "HLMessagingNotifyCallback::OnClientOnline is called with ClientId(domainId=\'%s\', appId=\'%s\', userId=\'%s\'), arg(%p)\n", clientId.domainId.c_str(), clientId.appId.c_str(), clientId.userId.c_str(), arg); 
}

void 
messagingOnClientOffline(const ClientId &clientId, ClientOfflineReason reason, void *arg)
{
  std::string reasonStr = "unknow";                                                        
  if (CLIENTOFFLINE_REASON_LOGOUT == reason) {                                             
    reasonStr = "CLIENTOFFLINE_REASON_LOGOUT";                                             
  } else if (CLIENTOFFLINE_REASON_HEARTBEAT_TIMEOUT == reason) {                           
    reasonStr = "CLIENTOFFLINE_REASON_HEARTBEAT_TIMEOUT";                                  
  } else if (CLIENTOFFLINE_REASON_DISCOVERY_STOPPED == reason) {                           
    reasonStr = "CLIENTOFFLINE_REASON_DISCOVERY_STOPPED";                                  
  } else if (CLIENTOFFLINE_REASON_UNKNOWN == reason) {                                     
    reasonStr = "CLIENTOFFLINE_REASON_UNKNOWN";                                            
  }                                                                                        
  fprintf(stderr, "HLMessagingNotifyCallback::OnClientOffline is called with ClientId(domainId=\'%s\', appId=\'%s\', userId=\'%s\'), reason(%s), arg(%p)\n", clientId.domainId.c_str(), clientId.appId.c_str(), clientId.userId.c_str(), reasonStr.c_str(), arg);
}

/////////////////////////////////
//       HLSenderCallback      //
/////////////////////////////////
int32_t 
senderOnAcked(int64_t seq, const ClientId &acker, int32_t ackMode, const char *data, size_t size, void *arg)
{
  if (data) {                                                                                               
    fprintf(stderr, "HLSenderCallback::OnAcked is called seq(" HL_FORMAT_INT64 "), acker(domainId=\'%s\', appId=\'%s\', userId=\'%s\'), ackMode(%d), data(%s), size(" HL_FORMAT_SIZE_T "), arg(%p)\n", seq, acker.domainId.c_str(), acker.appId.c_str(), acker.userId.c_str(), ackMode, data, size, arg);
  } else { 
    fprintf(stderr, "HLSenderCallback::OnAcked is called seq(" HL_FORMAT_INT64 "), acker(domainId=\'%s\', appId=\'%s\', userId=\'%s\'), ackMode(%d), not data, arg(%p)\n", seq, acker.domainId.c_str(), acker.appId.c_str(), acker.userId.c_str(), ackMode, arg);
  }
  return HL_OK;
}

int32_t 
senderOnReplied(int64_t seq, const ClientId &responder, int32_t status, const char *data, size_t size, void *arg)
{
  fprintf(stderr, "HLSenderCallback::OnReplied is called seq(" HL_FORMAT_INT64 "), responder(domainId=\'%s\', appId=\'%s\', userId=\'%s\'), status(%d), data(%s), size(" HL_FORMAT_SIZE_T "), arg(%p)\n", seq, responder.domainId.c_str(), responder.appId.c_str(), responder.userId.c_str(), status, data, size, arg);
  return HL_OK;
}

int32_t 
senderOnRepliedAvroData(int64_t seq, const ClientId &responder, int32_t status, AvroBFReader *bfreader, void *arg)
{
  int32_t ret = HL_OK;
  const char *physicalSchemaSubject = NULL;
  int32_t physicalSchemaVersion = 0;

  ret = bfreader->getPhysicalSchema(&physicalSchemaSubject, &physicalSchemaVersion);

  fprintf(stderr, "HLSenderCallback::OnRepliedAvroData is called seq(" HL_FORMAT_INT64 "), responder(domainId=\'%s\', appId=\'%s\', userId=\'%s\'), status(%d), physicalSchemaSubject(%s), physicalSchemaVersion(%d), arg(%p)\n", seq, responder.domainId.c_str(), responder.appId.c_str(), responder.userId.c_str(), status, physicalSchemaSubject, physicalSchemaVersion, arg);

  if (bfreader) {
    delete bfreader;
    bfreader = NULL;
  }

  return ret;
}

int32_t 
senderOnPeerReceiveError(int64_t seq, const ClientId &responder, int32_t errorCode, const char *data, size_t size, void *arg)
{
  fprintf(stderr, "HLSenderCallback::OnPeerReceiveError is called seq(" HL_FORMAT_INT64 "), peer(domainId=\'%s\', appId=\'%s\', userId=\'%s\'), errorCode(%d), data(%s), size(" HL_FORMAT_SIZE_T "), arg(%p)\n", seq, responder.domainId.c_str(), responder.appId.c_str(), responder.userId.c_str(), errorCode, data, size, arg);
  return HL_OK;
}

int32_t 
senderOnSendError(int64_t seq, int32_t errorCode, const char *data, size_t size, void *arg)
{
  fprintf(stderr, "HLSenderCallback::OnSendError is called seq(" HL_FORMAT_INT64 "), errorCode(%d), data(%s), size(" HL_FORMAT_SIZE_T "), arg(%p)\n", seq, errorCode, data, size, arg);
  return HL_OK;
}


// HLListener的回调函数
/////////////////////////////////
//      HLListenerCallback     //
/////////////////////////////////


int32_t 
listenerOnReceived(const ClientId &peer, const EndpointId &endpointId, const MessageMetaData &origMessageMetaData, const char *data, size_t size, bool needAck, bool needReply, void *arg)
{
  fprintf(stderr, "!!!!!!!!HLListenerCallback::OnReceived is called with peer(domainId=\'%s\', appId=\'%s\', userId=\'%s\'), \
  EndpointId(id=\'%s\'), origMessageMetaData(seq=" HL_FORMAT_INT64 ", messageType=%d, session=%s, replyAddress=%s, \
  codecAlgorithm=%s, schemaType=%d, schemaSubject=%s, schemaVersion=%d), data(%s), size(" HL_FORMAT_SIZE_T "), \
  needAck(%s), needReply(%s), arg(%p)\n", peer.domainId.c_str(), peer.appId.c_str(), peer.userId.c_str(), endpointId.id.c_str(),\
   origMessageMetaData.seq, origMessageMetaData.messageType, origMessageMetaData.session.c_str(), origMessageMetaData.replyAddress.c_str(),\
    origMessageMetaData.codecAlgorithm.c_str(), origMessageMetaData.schemaType, origMessageMetaData.schemaSubject.c_str(), \
    origMessageMetaData.schemaVersion, data, size, (needAck?"true":"false"), (needReply?"true":"false"), arg);

  std::string s(data);
  // s.back() = '\0';
  // printf("last:%c\nendddd", s.back());



  // printf("rcv:\n%s\nendddddd", s.c_str());

  printf("TsService start \n");

  EnTaskCls enTaskCls = EN_TS_NONE;
  ParseJsonInput(s.c_str(), enTaskCls);

  std::string retstr;
  if(EN_TS_FUS == enTaskCls)
  {
      spdlog::debug("Task is EN_TS_FUS");
      StTsFusInput tsfusinput;
      StTsFusResultOutput tsfusoutput;
      Json2TsFusInput(data, tsfusinput);
      Ts_Fus(tsfusinput, tsfusoutput);
      TsFusResult2Js(tsfusoutput, retstr);
  }
  else if(EN_TS_ANA == enTaskCls)
  {
      spdlog::debug("Task is EN_TS_ANA");
      StTsAnaInput tsanainput;
      StTsAnaResultOutput tsanaoutput;
      Json2TsAnaInput(data, tsanainput);
      Ts_Anals(tsanainput, tsanaoutput);
      TsAnaResult2Js(tsanaoutput, retstr);
  }

  printf("retstr:\n%s\nendddddd", retstr.c_str());




  do
    {
      ret = sw->sender->publish(retstr.c_str(), retstr.length()+1, session, &seq);
      if (HL_TRY_AGAIN_LATER == ret) {
        usleep(5);
        fprintf(stderr, "too fast sleep  ... ...\n");
      }
    } while (HL_TRY_AGAIN_LATER == ret);

  //   fprintf(stderr, "publish-raw-seq(" HL_FORMAT_INT64 ") to session(%s) with dataLen(" HL_FORMAT_SIZE_T ") ret:%d(%s)\n", seq, session,  retstr.length()+1, retstr.c_str(), hl_status_get_str(ret));






  
  return HL_OK;
}

int32_t 
listenerOnReceivedAvroData(const ClientId &peer, const EndpointId &endpointId, const MessageMetaData &origMessageMetaData, AvroBFReader *bfreader, bool needAck, bool needReply, void *arg)
{
  int32_t ret = HL_OK;
  const char *physicalSchemaSubject = NULL;
  int32_t physicalSchemaVersion = 0;

  ret = bfreader->getPhysicalSchema(&physicalSchemaSubject, &physicalSchemaVersion);
  fprintf(stderr, "HLListenerCallback::OnReceivedAvroData is called with peer(domainId=\'%s\', appId=\'%s\', userId=\'%s\'), EndpointId(id=\'%s\'), origMessageMetaData(seq=" HL_FORMAT_INT64 ", messageType=%d, session=%s, replyAddress=%s, codecAlgorithm=%s, schemaType=%d, schemaSubject=%s, schemaVersion=%d), physicalSchemaSubject(%s), physicalSchemaVersion(%d), needAck(%s), needReply(%s), arg(%p)\n", peer.domainId.c_str(), peer.appId.c_str(), peer.userId.c_str(), endpointId.id.c_str(), origMessageMetaData.seq, origMessageMetaData.messageType, origMessageMetaData.session.c_str(), origMessageMetaData.replyAddress.c_str(), origMessageMetaData.codecAlgorithm.c_str(), origMessageMetaData.schemaType, origMessageMetaData.schemaSubject.c_str(), origMessageMetaData.schemaVersion, physicalSchemaSubject, physicalSchemaVersion, (needAck?"true":"false"), (needReply?"true":"false"), arg);
  if (bfreader) {
    delete bfreader;
    bfreader = NULL;
  }
  return ret;
}


std::string readFileToString(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Failed to open file: " << filename << std::endl;
        return "";
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        (std::istreambuf_iterator<char>()));

    file.close();
    return content;
}


int main(int argc, const char **argv)
{
  printf("main start!!!!!!!!!!!!!!\n");
  spdlog::set_level(spdlog::level::debug);

  // std::string jstxt = argv[1];
  // std::string jdata = readFileToString(jstxt);

  // EnTaskCls enTaskCls = EN_TS_NONE;
  // ParseJsonInput(jdata.c_str(), enTaskCls);


  // rapidjson::Document dom;
  // printf("input data:%s\n", jdata.c_str());
  // if(dom.Parse(jdata.c_str()).HasParseError())
  // {
  //     printf("json parse failed\n");
  //     return 0;
  // }

  // if (dom.HasMember("userid") && dom["userid"].IsString())
  // {
  //     std::string userid = dom["userid"].GetString();
  //     if(userid.compare("/AIService/TS/Send") == 0)
  //     {
  //     }
  //     else if(userid.compare("/AIService/analysis/Send") == 0)
  //     {
  //     }
  // }


  // fflush(stdout);
  (void) argc;
  (void) argv;
  
  const char *data = "hello";
  size_t len = strlen(data) + 1;
  
  

  // 指定HLMessaging的配置文件路径
  // std::string messagingConfigFile = "/home/xdl/sample/c++case/case/PublishReveiceDataNoneCluster/bitsflow-hl-messaging-for-sender.xml";
  std::string messagingConfigFile = "/app/prog/930/c++simple/pubsub/config/aiservice/bitsflow_hl_messaging_for_sender.xml";
  
  // 指定HLSender的配置文件路径
  // std::string SenderConfigFile = "/home/xdl/sample/c++case/case/PublishReveiceDataNoneCluster/bitsflow-hl-sender.xml";
  std::string SenderConfigFile = "/app/prog/930/c++simple/pubsub/config/aiservice/bitsflow_hl_sender.xml";


  std::string ListenderMessagingConfigFile = "/app/prog/930/c++simple/pubsub/config/aiservice/bitsflow_hl_messaging_for_listener.xml";
  // 指定HLListener的配置文件路径
  // std::string listenerConfigFile = "/home/xdl/sample/c++case/case/PublishReveiceDataNoneCluster/bitsflow-hl-listener-none-XTESTSession.xml";
  std::string listenerConfigFile = "/app/prog/930/c++simple/pubsub/config/aiservice/bitsflow_hl_listener.xml";



  HLListenerWrapper *lw =new HLListenerWrapper();

  do {

    // 设置HLMessaging、HLSender Callback
    HLMessagingNotifyCallback mCallBacks;
    HLSenderCallback sCallBacks;
    HLListenerCallback lCallBacks;

    mCallBacks.OnDiscoverySuccess = messagingOnDiscoverySuccess;
    mCallBacks.OnDiscoverFailed = messagingOnDiscoverFailed;
    mCallBacks.OnClientOffline = messagingOnClientOffline;
    mCallBacks.OnClientOnline = messagingOnClientOnline;

    sCallBacks.OnAcked = senderOnAcked;
    sCallBacks.OnReplied = senderOnReplied;
    sCallBacks.OnRepliedAvroData = senderOnRepliedAvroData;
    sCallBacks.OnPeerReceiveError = senderOnPeerReceiveError;
    sCallBacks.OnSendError = senderOnSendError;

    lCallBacks.OnReceived = listenerOnReceived;
    lCallBacks.OnReceivedAvroData = listenerOnReceivedAvroData;

    // 创建HLMessage
    if (HL_OK != (ret = HLMessaging::createHLMessaging(messagingConfigFile.c_str(), mCallBacks, (void*)sw, &sw->messaging))) {
      fprintf(stderr, "%s:%d -- create HLMessaging fail with %d(%s)\n", __FILE__, __LINE__, ret, hl_status_get_str(ret));
      break;
    }

    // 创建HLSender
    if (HL_OK != (ret = sw->messaging->createHLSender(SenderConfigFile.c_str(), sCallBacks, (void*)sw, &sw->sender))) {
      fprintf(stderr, "%s:%d -- create HLSender fail with %d(%s)\n", __FILE__, __LINE__, ret, hl_status_get_str(ret));
      break;
    }

    // 创建HLMessage
    if (HL_OK != (ret = HLMessaging::createHLMessaging(ListenderMessagingConfigFile.c_str(), mCallBacks, (void*)lw, &lw->messaging))) {
      fprintf(stderr, "%s:%d -- create HLMessaging fail with %d(%s)\n", __FILE__, __LINE__, ret, hl_status_get_str(ret));
      break;
    }

    // 创建HLListener
    if (HL_OK != (ret = lw->messaging->createHLListener(listenerConfigFile.c_str(), lCallBacks, (void*)lw, &lw->listener))) {
      fprintf(stderr, "%s:%d -- create HLListener fail with %d(%s)\n", __FILE__, __LINE__, ret, hl_status_get_str(ret));
      break;
    }

  } while (0);

  usleep(1 * 1000 * 1000);
  // for (int i = 0; i < 1; i++) {

    do
    {
      ret = sw->sender->publish(data, len, session, &seq);
      if (HL_TRY_AGAIN_LATER == ret) {
        usleep(5);
        fprintf(stderr, "too fast sleep  ... ...\n");
      }
    } while (HL_TRY_AGAIN_LATER == ret);

    // fprintf(stderr, "publish-raw-seq(" HL_FORMAT_INT64 ") to session(%s) with dataLen(" HL_FORMAT_SIZE_T ") ret:%d(%s)\n", seq, session, len, ret, hl_status_get_str(ret));
  // }

  getchar();

  if (sw) {
    
    // 结束析构HLSender
    if (sw->sender) {
      delete sw->sender;
      sw->sender = NULL;
    }

    // 结束析构HLMessaging
    if (sw->messaging) {
      delete sw->messaging;
      sw->messaging = NULL;
    }

    delete sw;
    sw = NULL;
  }
  return ret;
}
