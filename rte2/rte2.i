%module rte2

%{
#include "common.h"
#include "NodeContent.h"
#include "Node.h"
#include "Socket.h"
#include "Thread.h"
#include "tcpCommon.h"
#include "TcpClient.h"
#include "TcpServer.h"
%}

%include "std_string.i"
%include "std_vector.i"
%include "stdint.i"
%include "windows.i"

%include "common.h"
%include "NodeContent.h"
%include "Node.h"
%include "Socket.h"
%include "Thread.h"
%include "tcpCommon.h"
%include "TcpClient.h"
%include "TcpServer.h"

%include "carrays.i"
%array_class(uint8_t, buffer);

%template (SafeArrayUInt8) rte::mem::SafeArray<uint8_t>;

%template (IntVector)std::vector<int>;
%template (StringVecotor) std::vector<std::string>;
%template (NodePtrVector) std::vector<rte::Node*>;
%template (TcpReceivedDataVector) std::vector<rte::TcpReceivedData>;
%template (TcpSentDataVector) std::vector<rte::TcpSentData>;

