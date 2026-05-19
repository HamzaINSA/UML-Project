#include "DateTime.h"

#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace airwatcher {

DateTime::DateTime() : epoch_(0) {}

DateTime::DateTime(std::time_t epoch) : epoch_(epoch) {}

DateTime::DateTime(int year, int month, int day, int hour, int minute, int second) {
    std::tm t{};
    t.tm_year = year - 1900;
    t.tm_mon  = month - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min  = minute;
    t.tm_sec  = second;
    t.tm_isdst = -1;
#if defined(_WIN32)
    epoch_ = _mkgmtime(&t);
#else
    epoch_ = timegm(&t);
#endif
}

DateTime DateTime::parse(const std::string& iso) {
    if (iso.empty()) return DateTime();
    int y=0, mo=0, d=0, h=0, mi=0, s=0;
    int n = std::sscanf(iso.c_str(), "%d-%d-%d %d:%d:%d", &y, &mo, &d, &h, &mi, &s);
    if (n < 3) {
        n = std::sscanf(iso.c_str(), "%d-%d-%dT%d:%d:%d", &y, &mo, &d, &h, &mi, &s);
    }
    if (n < 3) {
        throw std::runtime_error("DateTime::parse - format invalide: " + iso);
    }
    return DateTime(y, mo, d, h, mi, s);
}

DateTime DateTime::now() {
    return DateTime(std::time(nullptr));
}

DateTime DateTime::min() { return DateTime(static_cast<std::time_t>(0)); }
DateTime DateTime::max() { return DateTime(std::numeric_limits<std::time_t>::max()); }

std::string DateTime::toString() const {
    std::tm t{};
#if defined(_WIN32)
    gmtime_s(&t, &epoch_);
#else
    gmtime_r(&epoch_, &t);
#endif
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                  t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                  t.tm_hour, t.tm_min, t.tm_sec);
    return buf;
}

} // namespace airwatcher
