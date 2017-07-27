#include "webinterface.h"

#include "json.hpp"
#include "easylogging++.h"

#include <boost/regex.hpp>
#include <sstream>

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using namespace std;
using namespace nlohmann;

WebInterface::WebInterface(int port, PumpControlInterface* pump_control) {
    LOG(DEBUG) << "Webinterface constructor, starting to listen on port " << port_;

    port_ = port;
    pump_control_ = pump_control;
    pump_control_->RegisterCallbackClient(this);

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
    server_thread_ = move(t);

    LOG(DEBUG) << "Webinterface constructor done successfully";
}

WebInterface::~WebInterface() {
    LOG(DEBUG)<< "Webinterface destructor";

    server_.stop();
    if(server_thread_.joinable()) {
        server_thread_.join();
    }

    pump_control_->UnregisterCallbackClient(this);

    LOG(DEBUG)<< "Webinterface destructor done successfully";
}

void WebInterface::NewPumpControlState(PumpControlInterface::PumpControlState state){
    pump_control_state_ = state;
    LOG(DEBUG)<< "send update to websocketclients: " << PumpControlInterface::NameForPumpControlState(state);
    json json_message = json::object();
    json_message["mode"] = PumpControlInterface::NameForPumpControlState(state);
    SendMessage(json_message.dump());
}

void WebInterface::ProgressUpdate(string id, int percent) {
    LOG(DEBUG)<< "ProgressUpdate " << percent << " : " << id;
    json json_message = json::object();
    json_message["progressUpdate"]["orderName"] = id;
    json_message["progressUpdate"]["progress"] = percent;
    SendMessage(json_message.dump());
}

void WebInterface::ProgramEnded(string id) {
    LOG(DEBUG)<< "ProgramEnded: " << id;
    json json_message = json::object();
    json_message["programEnded"]["orderName"] = id;
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
    websocketpp::server<websocketpp::config::asio>::connection_ptr con = server_.get_con_from_hdl(hdl);

    string body = con->get_request_body();
    string uri = con->get_resource();
    string met = con->get_request().get_method();

    HttpResponse response;
    HandleHttpMessage(met,uri,body, response);

    con->set_body(response.response_message);
    con->set_status((websocketpp::http::status_code::value)response.response_code);
}

void WebInterface::HandleHttpMessage(const string& method, const string& path, const string& body, HttpResponse& response) {
    LOG(DEBUG) << "HTTP Message: Method: " << method << "; Path: " << path << "; Body: '" << body << "'.";

    string combined = method + ":" + path + ":" + body;
    boost::smatch what;

    if (boost::regex_search(combined, what, boost::regex("^PUT:\\/program\\/?:.*$"))) {
        HandleStartProgram(body, response);
    } else if (boost::regex_search(combined, what, boost::regex("^PUT:\\/ingredients\\/([0-9]{1,2})\\/amount\\/?:([0-9]+)$"))) {
        HandleSetAmountForPump(what[1].str(), body, response);
    } else if (boost::regex_search(combined, what, boost::regex("^PUT:\\/ingredients\\/([0-9]{1,2})\\/?:(.+)$"))) {
        HandleSetIngredientForPump(what[1].str(), body, response);
    } else if (boost::regex_search(combined, what, boost::regex("^GET:\\/ingredients\\/([0-9]{1,2})\\/?:.*$"))) {
        HandleGetIngredientForPump(what[1].str(), response);
    } else if (boost::regex_search(combined, what, boost::regex("^DELETE:\\/ingredients\\/([0-9]{1,2})\\/?:.*$"))) {
        HandleDeleteIngredientForPump(what[1].str(), response);
    } else if (boost::regex_search(combined, what, boost::regex("^GET:\\/pumps\\/?:.*$"))) {
        HandleGetPumps(response);
    } else if (boost::regex_search(combined, what, boost::regex("^PUT:\\/service\\/pumps\\/([0-9]{1,2})\\/?:(true|false)$"))) {
        HandleSwitchPump(what[1].str(), what[2].str(), response);
    } else if (boost::regex_search(combined, what, boost::regex("^PUT:\\/service\\/?:true$"))) {
        HandleEnterServiceMode(response);
    } else if (boost::regex_search(combined, what, boost::regex("^PUT:\\/service\\/?:false$"))) {
        HandleLeaveServiceMode(response);
    } else if(boost::regex_search(combined, what, boost::regex("^PUT:\\/ingredients\\/([0-9]{1,2})\\/amount\\/?:.*$"))) {
        response.Set(404, "Amount (body) is invalid or empty");
    } else if(boost::regex_search(combined, what, boost::regex("^.*:\\/ingredients\\/([0-9]{1,2})\\/amount\\/?:.*$"))) {
        response.Set(400, "Wrong method for this URL");
    } else if (boost::regex_search(combined, what, boost::regex("^PUT:\\/ingredients\\/([0-9]{1,2})\\/?:$"))) {
        response.Set(404, "Can't set ingredient to an empty string");
    } else if (boost::regex_search(combined, what, boost::regex("^.*:\\/ingredients\\/([0-9]{1,2})\\/?:$"))) {
        response.Set(400, "Wrong method for this URL");
    } else if (boost::regex_search(combined, what, boost::regex("^.*:\\/pumps\\/?:.*$"))) {
        response.Set(400, "Wrong method for this URL");
    } else if (boost::regex_search(combined, what, boost::regex("^PUT:\\/service\\/pumps\\/([0-9]{1,2})\\/?:.*$"))) {
        response.Set(404, "On/off (body) is invalid or empty");
    } else if (boost::regex_search(combined, what, boost::regex("^.*:\\/service\\/pumps\\/([0-9]{1,2})\\/?:.*$"))) {
        response.Set(400, "Wrong method for this URL");
    } else if (boost::regex_search(combined, what, boost::regex("^PUT:\\/service\\/?:.*$"))) {
        response.Set(404, "On/off (body) is invalid or empty");
    } else if (boost::regex_search(combined, what, boost::regex("^.*:\\/service\\/?:.*$"))) {
        response.Set(400, "Wrong method for this URL");
    } else if (boost::regex_search(combined, what, boost::regex("^.*:\\/program\\/?:.*$"))) {
        response.Set(400, "Wrong method for this URL");
    } else {
        response.Set(400, "Path could not be recognized");
    }

    if(response.response_code == 200){
        LOG(DEBUG) << "HTTP Message successfully handled.";
    } else {
        LOG(DEBUG) << "HTTP Message failed, code: " << response.response_code << "; message: '" << response.response_message << "'.";
    }
}

void WebInterface::HandleStartProgram(const string& program_string, HttpResponse& response){
    try {
        pump_control_->StartProgram(program_string);
        response.Set(200, "SUCCESS");
    } catch(logic_error&) {
        response.Set(500, "Wrong state for starting a program");
    }
}

void WebInterface::HandleGetPumps(HttpResponse& response){
    LOG(DEBUG)<< "Getting pump definitions.";
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
    response.Set(200 ,responseJson.dump());
    LOG(DEBUG)<< "Pump definitions successfully got.";
}

void WebInterface::HandleSetAmountForPump(const string& pump_number_string, const string& amount_string, HttpResponse& response){
    LOG(DEBUG)<< "Setting amount for pump number " << pump_number_string << " to " << amount_string << "ml.";
    int pump_number = atoi(pump_number_string.c_str());
    int amount = atoi(amount_string.c_str());
    try {
        pump_control_->SetAmountForPump(pump_number, amount);
        LOG(DEBUG) << "Successfully set amount for pump number " << pump_number_string << " to " << amount_string << "ml.";
        response.Set(200, "Successfully set amount for pump");
    } catch (out_of_range&) {
        LOG(DEBUG)<< "Amount for pump number " << pump_number << "can't be set, because it is not configured";
        response.Set(400, "This pump is not configured");
    }
}

void WebInterface::HandleGetIngredientForPump(const string& pump_number_string, HttpResponse& response){
    LOG(DEBUG)<< "Getting ingredient for pump number " << pump_number_string << ".";
    int pump_number = atoi(pump_number_string.c_str());
    try {
        string ingredient = pump_control_->GetIngredientForPump(pump_number);
        LOG(DEBUG) << "Successfully got ingredient for pump number " << pump_number_string << " as " << ingredient << ".";
        response.Set(200, ingredient);
    } catch (out_of_range&) {
        LOG(DEBUG)<< "Ingredient for pump number " << pump_number << "can't be got because it is not configured";
        response.Set(400, "This pump is not configured");
    }
}

void WebInterface::HandleSetIngredientForPump(const string& pump_number_string, const string& ingredient, HttpResponse& response){
    LOG(DEBUG)<< "Setting ingredient for pump number " << pump_number_string << " to " << ingredient << ".";
    int pump_number = atoi(pump_number_string.c_str());
    try {
        pump_control_->SetIngredientForPump(pump_number, ingredient);
        LOG(DEBUG) << "Successfully set ingredient for pump number " << pump_number_string << " to " << ingredient << ".";
        response.Set(200, "Successfully set ingredient for pump");
    } catch (out_of_range&) {
        LOG(DEBUG)<< "Amount for pump number " << pump_number << "can't be set because pump is out of range";
        response.Set(400, "This pump is not configured");
    }
}

void WebInterface::HandleDeleteIngredientForPump(const string& pump_number_string, HttpResponse& response){
    LOG(DEBUG)<< "Getting ingredient for pump number " << pump_number_string << ".";
    int pump_number = atoi(pump_number_string.c_str());
    try {
        pump_control_->DeleteIngredientForPump(pump_number);
        LOG(DEBUG) << "Successfully deleted ingredient for pump number " << pump_number_string << ".";
        response.Set(200, "Successfully deleted ingredient for pump");
    } catch (out_of_range&) {
        LOG(DEBUG)<< "Ingredient for pump number " << pump_number << "can't be deleted because it is not configured";
        response.Set(400, "This pump is not configured");
    }
}

void WebInterface::HandleEnterServiceMode(HttpResponse& response){
    LOG(DEBUG)<< "Entering service mode";
    try{
        pump_control_->EnterServiceMode();
        LOG(DEBUG)<< "Successfully entered service mode";
        response.Set(200, "Successfully entered service mode");
    } catch(logic_error&) {
        LOG(DEBUG)<< "Could not enter service mode";
        response.Set(500, "Could not enter service mode");
    }
}

void WebInterface::HandleLeaveServiceMode(HttpResponse& response){
    LOG(DEBUG)<< "Leaving service mode";
    try{
        pump_control_->LeaveServiceMode();
        LOG(DEBUG)<< "Successfully left service mode";
        response.Set(200, "Successfully left service mode");
    } catch(logic_error&) {
        LOG(DEBUG)<< "Could not leave service mode";
        response.Set(500, "Could not leave service mode");
    }
}

void WebInterface::HandleSwitchPump(const string& pump_number_string, const string& on_off, HttpResponse& response){
    LOG(DEBUG)<< "Switching pump " << pump_number_string << " to " << (on_off=="true"?"on":"off");
    int pump_number = stoi(pump_number_string);
    try {
        float new_flow = pump_control_->SwitchPump(pump_number, (on_off=="true"));
        response.response_code = 200;
        response.response_message = "SUCCESS";
        json json_message = json::object();
        json_message["service"]["pump"] = pump_number;
        json_message["service"]["flow"] = new_flow;
        SendMessage(json_message.dump());
        LOG(DEBUG)<< "Switched pump " << pump_number_string << " successfully to " << (on_off=="true"?"on":"off");
    } catch(out_of_range&) {
        LOG(DEBUG)<< "Pump number " << pump_number << " can't be switched because it is out of range";
        response.Set(400, "Requested Pump not available");
    } catch(logic_error&) {
        LOG(DEBUG)<< "Pump control is not in service mode";
        response.Set(400, response.response_message = "PumpControl not in Service mode");
    }
}

void WebInterface::SendMessage(const string& message) {
    lock_guard<mutex> guard(connection_mutex_);
    LOG(DEBUG)<< "WebInterface send message: " << message;

    for (auto con : connections_) {
        server_.send(con, message, websocketpp::frame::opcode::value::text);
    }
}



