%module rte2

%{
#include "common.h"
#include "NodeBase.h"
#include "Node.h"
#include "NodeParameter.h"
#include "NodeParameterValueSerializer.h"
#include "Command.h"
#include "Context.h"
#include "Socket.h"
#include "Thread.h"
#include "TcpClient.h"
#include "TcpServer.h"
%}

%include "std_string.i"
%include "std_vector.i"
%include "stdint.i"
%include "windows.i"

%include "common.h"
%include "NodeBase.h"
%include "Node.h"
%include "NodeParameter.h"
%include "NodeParameterValueSerializer.h"
%include "Command.h"
%include "Context.h"
%include "Socket.h"
%include "Thread.h"
%include "TcpClient.h"
%include "TcpServer.h"

%include "carrays.i"
%array_class(uint8_t, buffer);

%template (StringVecotor) std::vector<std::string>;
%template (NodePtrVector) std::vector<rte::Node*>;
%template (NodeParameterPtrVector) std::vector<rte::NodeParameter*>;

%template (setValueInt32) rte::NodeParameter::setValue<int32_t>;
%template (getValueInt32) rte::NodeParameter::getValue<int32_t>;
%template (setValueFloat32) rte::NodeParameter::setValue<float>;
%template (getValueFloat32) rte::NodeParameter::getValue<float>;
%template (setValueVector3) rte::NodeParameter::setValue<rte::Vector3>;
%template (getValueVector3) rte::NodeParameter::getValue<rte::Vector3>;
%template (setValueString) rte::NodeParameter::setValue<std::string>;
%template (getValueString) rte::NodeParameter::getValue<std::string>;
%template (setValueFile) rte::NodeParameter::setValue<rte::File>;
%template (getValueFile) rte::NodeParameter::getValue<rte::File>;
