
#include <string>
#include <map>
#include <sstream>
#include <iostream>
#include "../include/event.h"
#include "../include/SingletonCounter.h"
#include "../include/StompFrame.h"

    // Constructor
    StompFrame::StompFrame(const std::string& command, const std::map<std::string, std::string>& headers, const std::string& body)
        : command(command), headers(headers), body(body) {}

    // Getters
    std::string StompFrame::getCommand() const {
        return command;
    }

    std::string StompFrame::getHeader(const std::string& key) const {
        auto it = headers.find(key);
        return it != headers.end() ? it->second : "";
    }

    std::string StompFrame::getBody() const {
        return body;
    }

    StompFrame StompFrame::parseFromServer(const std::string& frame) {
        // Check if the frame ends with the null character
        // if (frame.empty() || frame.back() != '\0') {
        //     return StompFrame("ERROR", {{"message", "Frame does not terminate with null character"}}, "");
        // }

        // Remove the null character from the end
        // std::string trimmedFrame = frame.substr(0, frame.size() - 1);

        std::istringstream stream(frame);
        std::string line;

        // Extract the command (first line)
        std::getline(stream,line,'\n');

        // Append the command
        std::string command = line;

        // Parse the headers
        std::map<std::string, std::string> headers;
        while (std::getline(stream, line) && !line.empty()) {
            size_t pos = line.find(':');
            if (pos == std::string::npos) {
                return StompFrame("ERROR", {{"message", "Malformed header: " + line}}, "");
            }
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            headers[key] = value;
        }

        // Parse the body
        std::string body;
        while (std::getline(stream, line)) {
            body += line + "\n";
        }

        // Remove the trailing newline from the body, if it exists
        if (!body.empty() && body.back() == '\n') {
            body.pop_back();
        }

        return StompFrame(command, headers, body);
    }


    // Convert the StompFrame object to a raw frame string
    std::string StompFrame::toRawFrame() const {
        std::ostringstream rawFrame;

        // Append the command
        rawFrame << command << "\n";

        // Append headers
        for (const auto& header : headers) {
            rawFrame << header.first << ":" << header.second << "\n";
        }

        // Add an empty line to separate headers from the body
        rawFrame << "\n";

        // Append the body if it exists
        if (!body.empty()) {
            rawFrame << body;
        }

        // Append the null character to terminate the frame
        rawFrame.put('\0');

        return rawFrame.str();
    }

    StompFrame StompFrame::parseEvent(Event& event)
    {   
        std::map<std::string, std::string> general_information = event.get_general_information();  
        std::string active = general_information["active"];
        std::string forces_arrival = general_information["forces_arrival_at_scene"];
        StompFrame frame = StompFrame(
            "SEND",
            {
                {"destination:/", event.get_channel_name()},
                {"user", event.getEventOwnerUser()},
                {"city", event.get_city()},
                {"event name", event.get_name()},
                {"general information", ""},
                {"    active",active },
                {"    forces_arrival_at_scene", forces_arrival},
                {"description", event.get_description()}
            },"");
        return frame;
    }

    

