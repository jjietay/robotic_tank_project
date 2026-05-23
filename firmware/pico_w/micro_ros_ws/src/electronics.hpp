// ---------------------------------------------------------------------------
//                       electronics.hpp  —  Base device class
// ---------------------------------------------------------------------------
//  Common interface for every peripheral on the robot.  Header-only because
//  it is tiny and used purely as a base class.
// ---------------------------------------------------------------------------

#pragma once
#include <string>
#include <utility>
#include <stdio.h>

class Electronics
{
protected:
    std::string name;
    std::string status;

public:
    Electronics(std::string device_name, std::string device_status)
        : name(std::move(device_name)), status(std::move(device_status)) {}

    void ShowStatus() const {printf("%s [%s] initialised.\n", name.c_str(), status.c_str());}
    void CheckStatus() const {printf("%s disconnected.\n", name.c_str());}
};
