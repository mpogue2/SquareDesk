#ifndef UTILITY_H
#define UTILITY_H

class RecursionGuard
{
private:
    bool &guard_value;
public:
    RecursionGuard(bool &guard_value) :
        guard_value(guard_value)
    {
        guard_value = true;
    }
    ~RecursionGuard()
    {
        guard_value = false;
    }
};


#endif // ifndef UTILITY_H

