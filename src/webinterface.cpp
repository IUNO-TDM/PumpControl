#include "webinterface.h"

#include "json.hpp"
#include "easylogging++.h"
#include "websocketpp/common/thread.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <sstream>

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

using websocketpp::lib::thread;
using websocketpp::lib::mutex;
using websocketpp::lib::lock_guard;
using websocketpp::lib::unique_lock;
using websocketpp::lib::condition_variable;

using namespace std;
using namespace nlohmann;

WebInterface::WebInterface(int port, PumpControlInterface* pump_control) {
    port_ = port;
    pump_control_ = pump_control;
    pump_control_->RegisterCallbackClient(this);
    Start();
}

WebInterface::~WebInterface() {
    Stop();
    pump_control_->UnregisterCallbackClient(this);
}

void WebInterface::OnOpen(connection_hdl hdl) {
    {
        lock_guard<mutex> guard(connection_mutex_);
        LOG(DEBUG)<< "WebInterface onOpen";
        connections_.insert(hdl);
    }

    json json_message = json::object();
    json_message["mode"] = PumpControlInterface::NameForPumpControlState(pump_control_state_);
    SendMessage(json_message.dump());
}

void WebInterface::OnClose(connection_hdl hdl) {
    lock_guard<mutex> guard(connection_mutex_);
    LOG(DEBUG)<< "WebInterface onClose";
    connections_.erase(hdl);
}

void WebInterface::OnHttp(connection_hdl hdl) {
    LOG(DEBUG)<< "WebInterface onHttp";
    WebSocketServer::connection_ptr con = server_.get_con_from_hdl(hdl);

    std::string body = con->get_request_body();
    std::string uri = con->get_resource();
    std::string met = con->get_request().get_method();

    HttpResponse response;
    HandleHttpMessage(met,uri,body, response);

    con->set_body(response.response_message);
    con->set_status((websocketpp::http::status_code::value)response.response_code);
}

void WebInterface::HandleSetAmountForPump(const std::string& pump_number_string, const std::string& amount_string){
    LOG(INFO)<< "Setting amount for pump number " << pump_number_string << " to " << amount_string << "ml.";
}


void WebInterface::HandleHttpMessage(const string& method, const string& path, const string& body, HttpResponse& response) {
    LOG(DEBUG) << "HTTP Message: Method: " << method << "; Path: " << path << "; Body: '" << body << "'.";
    response.response_code = 400;
    response.response_message = "Ey yo - nix";

    {
        string combined = method + ":" + path + ":" + body;
        LOG(INFO)<< "Combined string '" << combined << "'.";
        boost::smatch what;

        if (boost::regex_search(combined, what, boost::regex("^PUT:\\/ingredients\\/([0-9]{1,2})\\/amount\\/?:([0-9]+)$"))) {
            HandleSetAmountForPump(what[1].str(), what[2].str());
        }
    }


    try {
        if (boost::starts_with(path, "/ingredients/")) {
            boost::regex expr { "^\\/ingredients\\/([0-9]{1,2}(\\/amount)?)$" };
            boost::smatch what;
            if (boost::regex_search(path, what, expr)) {
                int nr = stoi(what[1].str());
                if (what.size() == 3 && what[2].str() == "/amount") {
                    if (method == "PUT") {
                        if (body.length() > 0) {
                            try {
                                int amount = atoi(body.c_str());
                                pump_control_->SetAmountForPump(nr, amount);
                                LOG(DEBUG)<< amount << "ml has been set for pump " << nr;
                                response.response_code = 200;
                                response.response_message = "Successfully stored amount for ingredient for pump";
                            } catch (out_of_range&) {
                                LOG(DEBUG)<< "Amount for pump nr " << nr << "can't be set, because it is not configured";
                                response.response_code = 400;
                                response.response_message = "This pump is not configured";
                            } catch (exception& e) {
                                // TODO guess atoi simply returns 0 if body is invalid. must be handled differently.
                                LOG(ERROR)<< e.what();
                                response.response_code = 400;
                                response.response_message = "The body is invalid";
                            }
                        } else {
                            response.response_code = 400;
                            response.response_message = "Body is empty";
                        }
                    } else {
                        response.response_code = 400;
                        response.response_message = "Wrong method for this URL";
                    }
                } else {
                    if (method == "GET") {
                        try {
                            response.response_code = 200;
                            response.response_message = pump_control_->GetIngredientForPump(nr);
                        } catch (out_of_range&) {
                            response.response_code = 404;
                            response.response_message = "No ingredient for this pump number available!";
                        }
                    } else if (method == "PUT") {
                        if(body.length()> 0) {
                            try {
                                pump_control_->SetIngredientForPump(nr, body);
                                LOG(DEBUG) << "Pump number " << nr << "is now bound to "<< body;
                                response.response_code = 200;
                                response.response_message = "Successfully stored ingredient for pump";
                            } catch (out_of_range&) {
                                response.response_code = 404;
                                response.response_message = "Can't set ingredient for this pump number!";
                            }
                        } else {
                            response.response_code = 404;
                            response.response_message = "Can't set ingredient to an empty string!";
                        }
                    } else if (method == "DELETE") {
                        try {
                            pump_control_->DeleteIngredientForPump(nr);
                            response.response_code = 200;
                            response.response_message = "Successfully deleted ingredient for pump";
                        } catch (out_of_range&) {
                            response.response_code = 404;
                            response.response_message = "No ingredient for this pump number available!";
                        }
                     } else {
                        response.response_code = 400;
                        response.response_message = "Wrong method for this URL";
                    }
                }
            }
        } else if (boost::starts_with(path,"/pumps")) {
            if (method == "GET") {
                json responseJson = json::object();
                size_t pump_count = pump_control_->GetNumberOfPumps();
                for(size_t i = 1; i<=pump_count; i++) {
                    stringstream ss;
                    ss << i;
                    json pump = json::object();
                    PumpDriverInterface::PumpDefinition pump_definition = pump_control_->GetPumpDefinition(i);
                    pump["minFlow"] = pump_definition.min_flow;
                    pump["maxFlow"] = pump_definition.max_flow;
                    pump["flowPrecision"] = pump_definition.flow_precision;
                    responseJson[ss.str()]= pump;
                }
                response.response_code = 200;
                response.response_message = responseJson.dump();
            }
        } else if (boost::starts_with(path,"/service")) {
            if (boost::starts_with(path,"/service/pumps/")) {
                boost::regex expr {"\\/service\\/pumps\\/([0-9]{1,2})"};
                boost::smatch what;
                if(boost::regex_search(path,what,expr) &&
                        method == "PUT" && (body == "true" || body == "false")) {
                    int nr = stoi(what[1].str());
                    try {
                        float new_flow = pump_control_->SwitchPump(nr-1, (body=="true"));
                        LOG(DEBUG)<< "Pump " << nr << " should be switched to " << body;
                        response.response_code = 200;
                        response.response_message = "SUCCESS";
                        json json_message = json::object();
                        json_message["service"]["pump"] = nr;
                        json_message["service"]["flow"] = new_flow;
                        SendMessage(json_message.dump());
                    } catch(out_of_range&) {
                        response.response_code = 400;
                        response.response_message = "Requested Pump not available";
                    } catch(logic_error&) {
                        response.response_code = 400;
                        response.response_message = "PumpControl not in Service mode";
                    }
                } else {
                    response.response_code = 400;
                    response.response_message = "wrong format to control pumps in service mode";
                }
            } else if (path == "/service" ||path == "/service/" ) {
                if (method == "PUT") {
                    if (body == "true") {
                        try{
                            pump_control_->EnterServiceMode();
                            response.response_code = 200;
                            response.response_message = "Successfully changed to Service state";
                        } catch(logic_error&) {
                            response.response_code = 500;
                            response.response_message = "Could not set Service mode";
                        }
                    } else if (body == "false") {
                        try {
                            pump_control_->LeaveServiceMode();
                            response.response_code = 200;
                            response.response_message = "Successfully changed to Idle state";
                        } catch(logic_error&) {
                            response.response_code = 500;
                            response.response_message = "Could not set idle mode";
                        }
                    } else {
                        response.response_code = 400;
                        response.response_message = "incorrect parameter";
                    }

                } else {
                    response.response_code = 400;
                    response.response_message = "servicemode can only be set or unset. "
                                "No other function available. watch websocket!";
                }
            }
        } else if (path == "/program") {
            if(method == "PUT") {
                try {
                    pump_control_->StartProgram(body.c_str());
                    response.response_code = 200;
                    response.response_message = "SUCCESS";
                } catch(logic_error&) {
                    response.response_code = 500;
                    response.response_message = "Wrong state for starting a program";
                }
            } else {
                response.response_code = 400;
                response.response_message = "wrong method for program upload";
            }
        } else {
            response.response_code = 400;
            response.response_message = "unrecognized path";
        }
    } catch (boost::bad_lexical_cast&) {
        // bad parameter
    }
}

bool WebInterface::Start() {
    LOG(DEBUG)<< "Webinterface start on port " << port_;
    bool rv = false;
    try {
        server_.set_access_channels(websocketpp::log::alevel::all);
        server_.clear_access_channels(websocketpp::log::alevel::frame_payload);
        server_.init_asio();
        server_.listen(port_);
        server_.set_open_handler(bind(&WebInterface::OnOpen,this, ::_1));
        server_.set_close_handler(bind(&WebInterface::OnClose,this,::_1));
        server_.set_http_handler(bind(&WebInterface::OnHttp,this,::_1));
        server_.start_accept();
        thread t([this] {
                    this->server_.run();
                });
        rv = true;
        server_thread_ = move(t);
    } catch(websocketpp::exception const & e) {
        LOG(ERROR)<<e.what();
    }
    return rv;
}

void WebInterface::Stop() {
    LOG(DEBUG)<< "Webinterface stop ";
    server_.stop();
    if(server_thread_.joinable()) {
        server_thread_.join();
    }
}

void WebInterface::SendMessage(std::string message) {
    lock_guard<mutex> guard(connection_mutex_);
    LOG(DEBUG)<< "WebInterface send message: " << message;

    for (auto con : connections_) {
        server_.send(con, message, websocketpp::frame::opcode::value::text);
    }
}

void WebInterface::NewPumpControlState(PumpControlInterface::PumpControlState state){
    pump_control_state_ = state;
    LOG(DEBUG)<< "send update to websocketclients: " << PumpControlInterface::NameForPumpControlState(state);
    json json_message = json::object();
    json_message["mode"] = PumpControlInterface::NameForPumpControlState(state);
    SendMessage(json_message.dump());
}

void WebInterface::ProgramEnded(string id) {
    LOG(DEBUG)<< "ProgramEnded: " << id;
    json json_message = json::object();
    json_message["programEnded"]["orderName"] = id;
    SendMessage(json_message.dump());
}

void WebInterface::ProgressUpdate(string id, int percent) {
    LOG(DEBUG)<< "ProgressUpdate " << percent << " : " << id;
    json json_message = json::object();
    json_message["progressUpdate"]["orderName"] = id;
    json_message["progressUpdate"]["progress"] = percent;
    SendMessage(json_message.dump());
}

void WebInterface::AmountWarning(size_t pump_number, string ingredient, int warning_limit) {
    LOG(DEBUG)<< "AmountWarning: nr:" << pump_number << " ingredient: " << ingredient
            << " Amount warning level: " << warning_limit;
    json json_message = json::object();
    json_message["amountWarning"]["pumpNr"] = pump_number;
    json_message["amountWarning"]["ingredient"] = ingredient;
    json_message["amountWarning"]["amountWarningLimit"] = warning_limit;
    SendMessage(json_message.dump());
}

void WebInterface::Error(string error_type, int error_number, string details) {
    LOG(ERROR)<<"Error, type: " << error_type << "; number: " << error_number << "; details: '" << details << "'.";
    json json_message = json::object();
    json_message["error"]["type"] = error_type;
    json_message["error"]["nr"] = error_number;
    json_message["error"]["details"] = details;
    SendMessage(json_message.dump());
}



