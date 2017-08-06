#ifndef TRIBOOL_H
#define TRIBOOL_H

/*
 * This file is an internal header. It's not part of the public API,
 * and may change at any moment.
 */

#include <cstdint> // for uint8_t

namespace jp_private
{

//
// Simple class that implements 3-states variable logic
class TriBool
{
public:

    enum State {
        False = 0,
        True = 1,
        Indeterminate = 2
    };

    // Default to Indeterminate
    TriBool() {}
    TriBool(bool b)
    {
        if(b)
            _d = 1;
        else
            _d = 0;
    }

    TriBool(const State& state): _d(static_cast<int>(state)) {}

    bool indeterminate() const
    { return _d == 2; }

    static bool indeterminate(TriBool& b)
    { return b == TriBool::Indeterminate; }

    State state() const
    { return static_cast<State>(_d); }

    bool operator==(const TriBool& other) const
    { return state() == other.state(); }
    bool operator==(const bool &other) const
    { return (other && _d == 1) || (!other && _d == 0); }
    bool operator==(const State &other) const
    { return _d == static_cast<int>(other); }
    bool operator==(const int &other) const
    { return _d == other; }

    bool operator!=(const TriBool& other) const
    { return !(*this == other); }
    bool operator!=(const bool &other) const
    { return !(*this == other); }
    bool operator!=(const State &other) const
    { return !(*this == other); }
    bool operator!=(const int &other) const
    { return !(*this == other); }

private:
    // 0 --> false
    // 1 --> true
    // 2 --> indeterminate
    uint8_t _d = 2;
};

} // namespace jp_private

#endif // TRIBOOL_H
