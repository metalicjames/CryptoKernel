/*  CryptoKernel - A library for creating blockchain based digital currency
    Copyright (C) 2016  James Lovejoy

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <ctime>
#include <chrono>
#include <sstream>
#include <iostream>

#include "log.h"
#include "version.h"

CryptoKernel::Log::Log(std::string filename, bool printToConsole) {
    fPrintToConsole = printToConsole;
    logfile.open(filename, std::ios::app);
    if(logfile.is_open()) {
        logfile << "\n\n\n\n\n";
        printf(LOG_LEVEL_INFO, "CryptoKernel version " + version + " started");
        status = true;
    } else {
        status = false;
    }
}

CryptoKernel::Log::~Log() {
    logfilemutex.lock();
    logfile.close();
    logfilemutex.unlock();
}

template <typename Duration>
std::string print_time(tm t, Duration fraction) {
    using namespace std::chrono;
    char out[30];
    std::sprintf(&out[0],"[%04u-%02u-%02u %02u:%02u:%02u.%06u]", t.tm_year + 1900,
                t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
                static_cast<unsigned>(fraction / microseconds(1)));

    // VS2013's library has a bug which may require you to replace
    // "fraction / milliseconds(1)" with
    // "duration_cast<milliseconds>(fraction).count()"
    return std::string(out);
}

bool CryptoKernel::Log::printf(int loglevel, std::string message) {
    auto now = std::chrono::system_clock::now();
    auto tp = now.time_since_epoch();
    tp -= std::chrono::duration_cast<std::chrono::seconds>(tp);
    time_t tt = std::chrono::system_clock::to_time_t(now);
    auto t = print_time(*localtime(&tt), tp);

    std::ostringstream stagingstream;
    stagingstream << t << " ";

    switch(loglevel) {
    case LOG_LEVEL_ERR:
        stagingstream << "ERROR ";
        break;

    case LOG_LEVEL_WARN:
        stagingstream << "WARNING ";
        break;

    case LOG_LEVEL_INFO:
        stagingstream << "INFO ";
        break;

    default:
        return false;
        break;
    }

    stagingstream << message << "\n";

    if(fPrintToConsole) {
        std::cout << stagingstream.str() << std::flush;
    }

    logfilemutex.lock();
    logfile << stagingstream.str();
    logfilemutex.unlock();

    if(loglevel == LOG_LEVEL_ERR) {
        throw std::runtime_error("Fatal error");
    }

    return true;
}

bool CryptoKernel::Log::getStatus() {
    return status;
}

