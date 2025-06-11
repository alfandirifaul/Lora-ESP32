#ifndef DATALOGGER_H
#define DATALOGGER_H

#include <vector>
#include <String>

class DataLogger {
public:
    DataLogger();
    void logData(const String& data);
    std::vector<String> getLogs() const;

private:
    std::vector<String> logs;
};

#endif // DATALOGGER_H