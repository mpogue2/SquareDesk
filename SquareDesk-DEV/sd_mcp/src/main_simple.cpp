#include <iostream>
#include <string>
#include <sstream>
#include <csignal>
#include <atomic>

// Global flag for shutdown
std::atomic<bool> g_shutdown{false};

// Signal handler for clean shutdown
void signal_handler(int signal) {
    g_shutdown.store(true);
}

// Simple JSON string escaping
std::string escape_json_string(const std::string& input) {
    std::string output;
    output.reserve(input.length());
    
    for (char c : input) {
        switch (c) {
            case '"': output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            default: output += c; break;
        }
    }
    return output;
}

// Send JSON-RPC response
void send_response(const std::string& id, const std::string& result) {
    std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id << ",\"result\":" << result << "}" << std::endl;
}

// Send JSON-RPC error
void send_error(const std::string& id, int code, const std::string& message) {
    std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id << ",\"error\":{\"code\":" << code 
              << ",\"message\":\"" << escape_json_string(message) << "\"}}" << std::endl;
}

// Handle initialize request
std::string handle_initialize() {
    return "{\"protocolVersion\":\"2024-11-05\",\"capabilities\":{\"tools\":{}},\"serverInfo\":{\"name\":\"SquareDesk MCP Server\",\"version\":\"1.0.0\"}}";
}

// Handle tools/list request
std::string handle_tools_list() {
    return "{\"tools\":[{\"name\":\"echo\",\"description\":\"Echo the provided text with a prefix\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"text\":{\"type\":\"string\",\"description\":\"Text to echo\"}},\"required\":[\"text\"]}}]}";
}

// Handle tools/call request
std::string handle_tools_call(const std::string& text) {
    std::string escaped_text = escape_json_string("Echo: " + text);
    return "{\"content\":[{\"type\":\"text\",\"text\":\"" + escaped_text + "\"}]}";
}

// Simple JSON value extraction (very basic)
std::string extract_json_string_value(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":\"";
    size_t start = json.find(search);
    if (start == std::string::npos) return "";
    
    start += search.length();
    size_t end = json.find("\"", start);
    if (end == std::string::npos) return "";
    
    return json.substr(start, end - start);
}

// Extract method from JSON request
std::string extract_method(const std::string& json) {
    return extract_json_string_value(json, "method");
}

// Extract id from JSON request
std::string extract_id(const std::string& json) {
    std::string search = "\"id\":";
    size_t start = json.find(search);
    if (start == std::string::npos) return "null";
    
    start += search.length();
    size_t end = json.find_first_of(",}", start);
    if (end == std::string::npos) return "null";
    
    return json.substr(start, end - start);
}

// Extract text argument from tools/call
std::string extract_text_argument(const std::string& json) {
    // Look for "arguments":{"text":"value"}
    std::string search = "\"arguments\":{\"text\":\"";
    size_t start = json.find(search);
    if (start == std::string::npos) return "No text provided";
    
    start += search.length();
    size_t end = json.find("\"", start);
    if (end == std::string::npos) return "No text provided";
    
    return json.substr(start, end - start);
}

int main(int argc, char *argv[]) {
    // Set up signal handler
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    std::string line;
    while (!g_shutdown.load() && std::getline(std::cin, line)) {
        if (line.empty()) continue;
        
        // Extract method and id
        std::string method = extract_method(line);
        std::string id = extract_id(line);
        
        if (method.empty()) continue;
        
        // Check if this is a notification (no id or id is null)
        bool is_notification = (id == "null" || id.empty());
        
        try {
            if (method == "initialize") {
                if (!is_notification) {
                    send_response(id, handle_initialize());
                }
            } else if (method == "tools/list") {
                if (!is_notification) {
                    send_response(id, handle_tools_list());
                }
            } else if (method == "tools/call") {
                if (!is_notification) {
                    std::string text = extract_text_argument(line);
                    send_response(id, handle_tools_call(text));
                }
            } else if (method.find("notifications/") == 0) {
                // Silently ignore all notifications/* methods
                continue;
            } else {
                // Only send error for requests (not notifications)
                if (!is_notification) {
                    send_error(id, -32601, "Method not found: " + method);
                }
            }
        } catch (const std::exception& e) {
            // Only send error for requests (not notifications)
            if (!is_notification) {
                send_error(id, -32603, e.what());
            }
        }
    }
    
    return 0;
}