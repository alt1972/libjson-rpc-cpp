/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file websocketserver.cpp
 * @date 12.12.2013
 * @author toolmmy
 * @license See attached LICENSE.txt
 ************************************************************************/

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#include "websocketserver.h"
#include "mongoose.h"


#define HTTP_GET_METHOD                         "GET"
#define HTTP_POST_METHOD                        "POST"

#define WS_HEADER_PROCOTOL                      "Sec-WebSocket-Protocol"
#define WS_HEADER_VERSION                       "Sec-WebSocket-Version"
#define WS_HEADER_VERSION_13                    "13"
#define WS_HEADER_KEY                           "Sec-WebSocket-Key"

#define WS_FRAME_LENGTH                         10
#define WS_FRAME_FIN                            0x80

#define WS_OPCODE_CONTINUATION                  0x00
#define WS_OPCODE_TEXT                          0x01
#define WS_OPCODE_BINARY                        0x02
#define WS_OPCODE_CONN_CLOSE                    0x08
#define WS_OPCODE_PING                          0x09
#define WS_OPCODE_PONG                          0x0a

#define WS_STATUS_CODE_NORMAL_CLOSURE           1000
#define WS_STATUS_CODE_GOING_AWAY               1001
#define WS_STATUS_CODE_TERM_DUE_ERROR           1002
#define WS_STATUS_CODE_TERM_DUE_CANNOT_ACCEPT   1003

#define WS_PAYLOAD_LENGTH_7BIT                  0x7e
#define WS_PAYLOAD_LENGTH_16BIT                 0x7fff
#define WS_PAYLOAD_LENGTH_64BIT                 0x7fffffffffffffff

#define WS_DATA_PING                            "PING"
#define WS_DATA_PONG                            "PONG"
#define WS_DATA_PING_PING_LENGTH                4
#define WS_SEND_PING_DELAY                      10

#define WS_WS_FRAME_PAYLOAD_LENGTH_INDEX        1

#define WS_FRAME_PAYLOAD_LENGTH_INDEX           1

namespace jsonrpc
{
    void* WebsocketServer::sendContinuousPing(void* data)
    {

        if (data == NULL)
        {
            return NULL;
        }

        WebsocketServer* source = (WebsocketServer*) data;
        while (source->_isRunning)
        {
            source->SendPingBroadcast();
            sleep(WS_SEND_PING_DELAY);
        }

        return data;
    }

    void WebsocketServer::handleWebsocketStatusCode(const int statusCode, struct mg_connection* connection)
    {
        jsonrpc::debug_log("[WebsocketServer] Handling status code <%d>", statusCode);

        switch (statusCode)
        {
            case WS_STATUS_CODE_GOING_AWAY:
            case WS_STATUS_CODE_NORMAL_CLOSURE:
            case WS_STATUS_CODE_TERM_DUE_CANNOT_ACCEPT:
            case WS_STATUS_CODE_TERM_DUE_ERROR:
            {
                const struct mg_request_info* requsteInfo = mg_get_request_info(connection);
                WebsocketServer* source = (WebsocketServer*) requsteInfo->user_data;

                //const char* sessionProtocol = mg_get_header(connection, WS_HEADER_PROCOTOL);
                //source->_wsConnections.erase(sessionProtocol);
                source->RemoveConnection(connection);
            }
                break;
            default:
                jsonrpc::debug_log("[WebsocketServer] Unsupported status code <%d> received", statusCode);
        }
    }

    int WebsocketServer::websocketConnectionRequestCallback(struct mg_connection* connection)
    {
        jsonrpc::debug_log("[WebsocketServer] Incoming Request");

        const struct mg_request_info* requestInfo = mg_get_request_info(connection);
        WebsocketServer* source = (WebsocketServer*) requestInfo->user_data;

        if (WebsocketServer::isWebsocketConnection(connection))
        {

            const char* sessionProtocol = mg_get_header(connection, WS_HEADER_PROCOTOL);

            int protocolEquals = source->_protocol.compare(sessionProtocol);
            if (sessionProtocol == NULL || protocolEquals != 0)
            {
                jsonrpc::debug_log("[WebsocketServer] Invalid Protocol <%s>", sessionProtocol);
                mg_printf(connection, "HTTP/1.1 426 Upgrade Required\r\n"
                        "Connection: Close\r\n"
                        "\r\n");
                //Mark request as processed
                return 1;
            }
            //We need to return 0 even when everything seems to be fine.
            //Returning 1 would stop the Websocket handshake
            return 0;

        }
        else if ((strcmp(requestInfo->request_method, HTTP_GET_METHOD) == 0) && source->_showSpec)
        {
            //check for plain HTTP_GET
            return mg_printf(connection, "HTTP/1.1 200 OK\r\n"
                    "Content-Type: application/json\r\n"
                    "Content-Length: %d\r\n"
                    "\r\n"
                    "%s", (int) source->GetSpecification().length(), source->GetSpecification().c_str());
        }
        else
        {
            return 0;
        }

    }

    int WebsocketServer::websocketDataCallback(struct mg_connection* connection, int bits, char *data, size_t data_len)
    {

        const struct mg_request_info *requestInfo = mg_get_request_info(connection);
        WebsocketServer* source = (WebsocketServer*) requestInfo->user_data;

        uint opCode = (0x0f & bits);
        if (opCode == WS_OPCODE_CONTINUATION)
        {
            jsonrpc::debug_log("[WebsocketServer.OpCode] Continuation");

        }
        else if (opCode == WS_OPCODE_TEXT)
        {
            jsonrpc::debug_log("[WebsocketServer.OpCode] Text data received");
            source->OnRequest(data, connection);

        }
        else if (opCode == WS_OPCODE_BINARY)
        {
            jsonrpc::debug_log("[WebsocketServer.OpCode] Binary data received");

        }
        else if (opCode == WS_OPCODE_CONN_CLOSE)
        {
            jsonrpc::debug_log("[WebsocketServer.Opcode] Connection closed");
            const unsigned int result = (data[0] << 8 | (0x00ff & data[1]));
            WebsocketServer::handleWebsocketStatusCode(result, connection);

        }
        else if (opCode == WS_OPCODE_PING)
        {
            jsonrpc::debug_log("[WebsocketServer.Opcode] Ping");
            source->SendPong(connection);

        }
        else if (opCode == WS_OPCODE_PONG)
        {
            jsonrpc::debug_log("[WebsocketServer.Opcode] Pong");
        }
        else
        {
            jsonrpc::debug_log("[WebsocketServer] Unsupported OpCode <%d> received", opCode);
        }

        return 1;
    }

    void WebsocketServer::websocketReadyCallback(struct mg_connection* connection)
    {


        const char* sessionProtocol = mg_get_header(connection, WS_HEADER_PROCOTOL);
        const struct mg_request_info* requestInfo = mg_get_request_info(connection);
        WebsocketServer* source = (WebsocketServer*) requestInfo->user_data;

        WebsocketConnection wsConnection = {sessionProtocol, connection};
        jsonrpc::debug_log("[WebsocketServer] Connection <0x%02X> established protocol <%s>",
                wsConnection.conn,
                wsConnection.protocol.c_str());

        source->_wsConnections.push_back(wsConnection);

    }

    int WebsocketServer::websocketConnectCallback(const struct mg_connection *connection)
    {
        jsonrpc::debug_log("[WebsocketServer] Incoming connection");
        return 0;
    }

    bool WebsocketServer::isWebsocketConnection(struct mg_connection *connection)
    {

        const struct mg_request_info* requestInfo = mg_get_request_info(connection);

        if (strcmp(requestInfo->request_method, HTTP_GET_METHOD) != 0)
        {
            return false;
        }

        static const char* websocketVersion = mg_get_header(connection, WS_HEADER_VERSION);
        if (websocketVersion == NULL || (strcmp(websocketVersion, WS_HEADER_VERSION_13) != 0))
        {
            return false;
        }

        return true;
    }

    WebsocketServer::WebsocketServer(unsigned int port, const std::string& websocketProtocol, bool enableSpecification, unsigned int threads)
            : AbstractServerConnector(),
              _port(port),
              _protocol(websocketProtocol),
              _showSpec(enableSpecification),
              _isRunning(false),
              _numThreads(threads),
              _connMaintainerThread(-1),
              _ctx(NULL)
    {

    }

    WebsocketServer::~WebsocketServer()
    {
        this->StopListening();
    }

    void WebsocketServer::SendPingBroadcast()
    {

        if(this->_wsConnections.empty())
        {
            return;
        }

        jsonrpc::debug_log("[WebsocketServer.SendPingBroadcast] Maintaining <%d> client connections", this->_wsConnections.size());

        websocketConnectionList::iterator it;
        std::vector<struct mg_connection*> deathList;
        for (it = this->_wsConnections.begin(); it != this->_wsConnections.end(); ++it)
        {
            if (!this->SendPing(it->conn))
            {
                jsonrpc::debug_log("[WebsocketServer] Couldn't send Ping to client. Let's assume he is gone.");
                deathList.push_back(it->conn);
            }
        }

        //check for death connections
        if(deathList.size() == 0) {
            return;
        }

        //clear the wsConnections
        uint i;
        for(i = 0 ; i < deathList.size(); i++)
        {
            this->RemoveConnection(deathList[i]);
        }
    }

    bool WebsocketServer::StartListening()
    {
        if (this->_isRunning)
        {
            return true;
        }

        jsonrpc::debug_log("[WebsocketServer] Starting listening on port <%d>", this->_port);

        char port[6];
        char threads[6];
        struct mg_callbacks wsCallbacks;
        memset(&wsCallbacks, 0, sizeof(wsCallbacks));
        wsCallbacks.websocket_connect = websocketConnectCallback;
        wsCallbacks.websocket_data = websocketDataCallback;
        wsCallbacks.websocket_ready = websocketReadyCallback;
        wsCallbacks.begin_request = websocketConnectionRequestCallback;

        snprintf(port, sizeof(port), "%d", this->_port);
        snprintf(threads, sizeof(threads), "%d", this->_numThreads);
        const char *options[] =
        {
        		"listening_ports", port,
        		"num_threads", threads,
        		NULL
        };

        this->_ctx = mg_start(&wsCallbacks, this, options);
        this->StartMaintenanceThread();

        this->_isRunning = (this->_ctx != NULL);
        return this->_isRunning;
    }

    bool WebsocketServer::StopListening()
    {

        if (!this->_isRunning)
        {
            return true;
        }

        mg_stop(this->_ctx);
        this->_isRunning = false;

        return this->_isRunning;
    }

    bool WebsocketServer::SendResponse(const std::string& response, void* addInfo)
    {
        struct mg_connection* conn = (struct mg_connection*) addInfo;
        return this->SendData(conn, WS_OPCODE_TEXT, response);
    }

    bool WebsocketServer::SendEvent(const std::string& event)
    {
    	bool result = false;

    	websocketConnectionList::iterator it;
    	std::vector<struct mg_connection*> deathList;
    	for (it = this->_wsConnections.begin(); it != this->_wsConnections.end(); ++it)
    	{
    		result &= this->SendData(it->conn, WS_OPCODE_TEXT, event);

    	}

    	return result;
    }

    /**
     *   0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     +-+-+-+-+-------+-+-------------+-------------------------------+
     |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
     |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
     |N|V|V|V|       |S|             |   (if payload len==126/127)   |
     | |1|2|3|       |K|             |                               |
     +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
     |     Extended payload length continued, if payload len == 127  |
     + - - - - - - - - - - - - - - - +-------------------------------+
     |                               |Masking-key, if MASK set to 1  |
     +-------------------------------+-------------------------------+
     | Masking-key (continued)       |          Payload Data         |
     +-------------------------------- - - - - - - - - - - - - - - - +
     :                     Payload Data continued ...                :
     + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
     |                     Payload Data continued ...                |
     +---------------------------------------------------------------+


     ws-frame                = frame-fin           ; 1 bit in length
     frame-rsv1          ; 1 bit in length
     frame-rsv2          ; 1 bit in length
     frame-rsv3          ; 1 bit in length
     frame-opcode        ; 4 bits in length
     frame-masked        ; 1 bit in length
     frame-payload-length   ; either 7, 7+16,
     ; or 7+64 bits in
     ; length
     [ frame-masking-key ]  ; 32 bits in length
     frame-payload-data     ; n*8 bits in
     ; length, where
     ; n >= 0

     frame-fin               = %x0 ; more frames of this message follow
     / %x1 ; final frame of this message
     ; 1 bit in length

     frame-rsv1              = %x0 / %x1
     ; 1 bit in length, MUST be 0 unless
     ; negotiated otherwise

     frame-rsv2              = %x0 / %x1
     ; 1 bit in length, MUST be 0 unless
     ; negotiated otherwise

     frame-rsv3              = %x0 / %x1
     ; 1 bit in length, MUST be 0 unless
     ; negotiated otherwise

     frame-opcode            = frame-opcode-non-control /
     frame-opcode-control /
     frame-opcode-cont

     frame-opcode-cont       = %x0 ; frame continuation

     frame-opcode-non-control= %x1 ; text frame
     / %x2 ; binary frame
     / %x3-7
     ; 4 bits in length,
     ; reserved for further non-control frames

     frame-opcode-control    = %x8 ; connection close
     / %x9 ; ping
     / %xA ; pong
     / %xB-F ; reserved for further control
     ; frames
     ; 4 bits in length
     */

    bool WebsocketServer::SendData(struct mg_connection* connection, const unsigned int opCode, const std::string& data)
    {
        int bytesSent = -1;

        unsigned char* buff;
        size_t buffLength = data.length();
        int extendedHeaderLength = 0;

        buff = (unsigned char*) malloc(buffLength + WS_FRAME_LENGTH);

        //add FIN, OPCODE etc.
        buff[0] = WS_FRAME_FIN + (opCode & 0x0f);

        //we have to check ifbuffLength the payload fits in the 7 bit payload field
        if (buffLength < WS_PAYLOAD_LENGTH_7BIT)
        {

            buff[1] = buffLength;
            extendedHeaderLength = 2;

        }
        else if (buffLength <= WS_PAYLOAD_LENGTH_16BIT)
        {

            buff[1] = WS_PAYLOAD_LENGTH_7BIT; //0x7E indicates a 16 bit payload length field
            buff[2] = (buffLength >> 8) & 0xFF;
            buff[3] = buffLength & 0xFF;

            extendedHeaderLength = 4;
        }
        else
        {

            //Oh loard, we need a 64 bit payload length field;
            buff[1] = WS_PAYLOAD_LENGTH_7BIT + 1; //0x7F indicates a 64 bit payload length field
            buff[2] = (buffLength >> 56) & 0xFF;
            buff[3] = (buffLength >> 48) & 0xFF;
            buff[4] = (buffLength >> 40) & 0xFF;
            buff[5] = (buffLength >> 32) & 0xFF;
            buff[6] = (buffLength >> 24) & 0xFF;
            buff[7] = (buffLength >> 16) & 0xFF;
            buff[8] = (buffLength >> 8) & 0xFF;
            buff[9] = (buffLength & 0xFF);

            extendedHeaderLength = 10;
        }

        memcpy(buff + extendedHeaderLength, data.c_str(), buffLength);
        bytesSent = mg_write(connection, buff, buffLength + extendedHeaderLength);

        if(jsonrpc::debug_enabled() && bytesSent > 0)
        {
            std::string payload;
            char tmp[32+16+1]; //hex-value + whitespace + /0
            int i;
            for(i = 0; i < bytesSent; i++)
            {
                snprintf(tmp, sizeof(tmp), "0x%02x ", buff[i]);
                payload.append(tmp);
            }
            payload.erase(payload.size() -1);
            jsonrpc::debug_log("[WebsocketServer.SendData] <%d> byte send; payload <%s>", bytesSent, payload.c_str());
        }

        free(buff);

        return (bytesSent > 0);

    }

    bool WebsocketServer::SendPing(struct mg_connection* connection)
    {
        jsonrpc::debug_log("[WebsocketServer] Sending Ping");
        return this->SendData(connection, WS_OPCODE_PING, WS_DATA_PING);
    }

    bool WebsocketServer::SendPong(struct mg_connection* connection)
    {
        jsonrpc::debug_log("[WebsocketServer SendPong");
        return this->SendData(connection, WS_OPCODE_PONG, WS_DATA_PONG);
    }

    bool WebsocketServer::RemoveConnection(struct mg_connection* connection)
    {
        bool result = false;

        websocketConnectionList::iterator it;
        for(it = this->_wsConnections.begin(); it != this->_wsConnections.end(); ++it)
        {
            if(it->conn == connection)
            {
                break;
            }
        }

        if(it != this->_wsConnections.end())
        {
            if(jsonrpc::debug_enabled())
            {
                const char* sessionProtocol = mg_get_header(connection, WS_HEADER_PROCOTOL);
                jsonrpc::debug_log("[WebsocketServer] Removing client connection <0x%02X> protocol <%s>", connection, sessionProtocol);
            }

            this->_wsConnections.erase(it);
            mg_close_connection(it->conn);
            result = true;
        }

        return result;
    }

    bool WebsocketServer::IsClientMaintenanceThreadRunning()
    {
        return (this->_connMaintainerThread != -1);
    }

    void WebsocketServer::StartMaintenanceThread()
    {
        jsonrpc::debug_log("[WebsocketServer] Starting client connection maintenance thread");
        //We need to start a continuous Thread that keeps the connection alive while sending a ping to all clients
        this->_connMaintainerThread = mg_start_thread(&WebsocketServer::sendContinuousPing, this);
    }

    void WebsocketServer::StopMaintenanceThread()
    {
        if(!this->IsClientMaintenanceThreadRunning())
        {
            return;
        }
        jsonrpc::debug_log("[WebsocketServer] Stopping client connection maintenance thread");
        //TODO
        //there's no mg_stop_thread method. It is attached to mongoose itself
        //- Option 1: create own thread with pthread
        //- Option 2: just keep the worker thread even when we have no clients.
    }


} /* namespace jsonrpc */
