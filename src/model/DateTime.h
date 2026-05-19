#ifndef AIRWATCHER_DATETIME_H
#define AIRWATCHER_DATETIME_H

#include <string>
#include <ctime>

namespace airwatcher {

class DateTime {
public:
    DateTime();
    explicit DateTime(std::time_t epoch);
    DateTime(int year, int month, int day, int hour = 0, int minute = 0, int second = 0);

    static DateTime parse(const std::string& iso);
    static DateTime now();
    static DateTime min();
    static DateTime max();

    std::time_t toEpoch() const { return epoch_; }
    std::string toString() const;

    bool operator<(const DateTime& o) const  { return epoch_ <  o.epoch_; }
    bool operator<=(const DateTime& o) const { return epoch_ <= o.epoch_; }
    bool operator>(const DateTime& o) const  { return epoch_ >  o.epoch_; }
    bool operator>=(const DateTime& o) const { return epoch_ >= o.epoch_; }
    bool operator==(const DateTime& o) const { return epoch_ == o.epoch_; }
    bool operator!=(const DateTime& o) const { return epoch_ != o.epoch_; }

private:
    std::time_t epoch_;
};

} // namespace airwatcher

#endif
